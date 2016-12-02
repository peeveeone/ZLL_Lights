/*****************************************************************************
 *
query image status
 * MODULE:             JN-AN-1189
 *
 * COMPONENT:          app_ota_client.c
 *
 * DESCRIPTION:        DK4 (DR1175/DR1199) OTA Client App (Implementation)
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5164,
 * JN5161, JN5148, JN5142, JN5139].
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright NXP B.V. 2013. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>

#include "dbg.h"
#include "pdm.h"
#include "PDM_IDs.h"

#include "os_gen.h"
#include "pdum_gen.h"


#include "ota.h"
//#include "ha.h"
#include "zll.h"
#include "zcl_options.h"

#include "app_timer_driver.h"
#include "app_ota_client.h"
#include "zpr_light_node.h"
#include "app_common.h"
#include "Utilities.h"
#include "rnd_pub.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
//#define DEBUG_APP_OTA
#define OTA_LNT  FALSE

#ifndef DEBUG_APP_OTA
    #define TRACE_APP_OTA           	FALSE
#else
    #define TRACE_APP_OTA           	TRUE
#endif
//#define OTA_CLIENT_EP 1 //2
#define OTA_STARTUP_DELAY_IN_SEC 5

typedef enum {
    OTA_FIND_SERVER,
    OTA_FIND_SERVER_WAIT,
    OTA_IEEE_LOOK_UP,
    OTA_IEEE_WAIT,
    OTA_QUERYIMAGE,
    OTA_QUERYIMAGE_WAIT,
    OTA_DL_PROGRESS
} teOtA_State;


#define APP_IEEE_ADDR_RESPONSE 0x8001
#define APP_MATCH_DESCRIPTOR_RESPONSE 0x8006

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PRIVATE void vInitAndDisplayKeys(void);
PRIVATE bool_t bMatchRecieved(void);

PRIVATE teZCL_Status eClientQueryNextImageRequest(
					uint8 u8SourceEndpoint,
                    uint8 u8DestinationEndpoint,
					uint16 u16DestinationAddress,
                    uint32 u32FileVersion,
                    uint16 u16HardwareVersion,
                    uint16 u16ImageType,
                    uint16 u16ManufacturerCode,
                    uint8 u8FieldControl);

PRIVATE ZPS_teStatus eSendOTAMatchDescriptor(uint16 u16ProfileId);
PRIVATE void vManagaeOTAState(void);
PRIVATE void vGetIEEEAddress( void);
PRIVATE void vOTAPersist(void);
PRIVATE void vRestetOTADiscovery(void);
PRIVATE void vManageDLProgressState(void);
PRIVATE uint8 u8VerifyLinkKey(tsOTA_CallBackMessage *psCallBackMessage);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
PDM_tsRecordDescriptor OTA_PersistedDataPDDesc;
tsOTA_PersistedData        sOTA_PersistedData;

/* mac address */
PUBLIC	uint8 au8MacAddress[]  __attribute__ ((section (".ro_mac_address"))) = {
              	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/* Pre-configured Link Key */
PUBLIC	uint8 s_au8LnkKeyArray[16] __attribute__ ((section (".ro_se_lnkKey"))) = {0x5a, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41, 0x6c,
                                                                  0x6c, 0x69, 0x61, 0x6e, 0x63, 0x65, 0x30, 0x39};



/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE teOtA_State eOTA_State = OTA_FIND_SERVER;
PRIVATE uint32 u32TimeOut=0;
PRIVATE uint32 u32OTARetry;
PRIVATE bool_t bWaitUgradeTime;

PRIVATE uint32 u32OTAQueryTimeinSec = OTA_SERVER_QUERY_TIME_IN_SEC-30;

PRIVATE uint8 au8MacAddressVolatile[8];

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vAppInitOTA
 *
 * DESCRIPTION:
 * Initialises application OTA client related data structures and calls
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/

PUBLIC void vAppInitOTA(void)
{

    tsNvmDefs sNvmDefs;
    teZCL_Status eZCL_Status;
    uint8 au8CAPublicKey[22];

    vInitAndDisplayKeys();

#ifdef JENNIC_CHIP_FAMILY_JN516x
  #if JENNIC_CHIP==JN5168
		    uint8 u8StartSector[1] = {0};
  #elif JENNIC_CHIP == JN5169
		    uint8 u8StartSector[1] = {8};     /* TODO  */
  #endif
#else
	error define chip family
#endif

    eZCL_Status = eOTA_UpdateClientAttributes(OTA_CLIENT_EP);
    if (eZCL_Status != E_ZCL_SUCCESS)
    {
	    DBG_vPrintf(TRACE_APP_OTA, "eOTA_UpdateClientAttributes returned error 0x%x\n", eZCL_Status);
    }

#if JENNIC_CHIP==JN5168
    sNvmDefs.u32SectorSize = 64*1024; /* Sector Size = 64K*/
    sNvmDefs.u8FlashDeviceType = E_FL_CHIP_AUTO;
#elif JENNIC_CHIP==JN5169
    sNvmDefs.u32SectorSize = 32*1024; /* Sector Size = 32K*/
    sNvmDefs.u8FlashDeviceType = E_FL_CHIP_INTERNAL;
#else
    error define the chip
#endif


    vOTA_FlashInit(NULL,&sNvmDefs);


#ifdef JENNIC_CHIP_FAMILY_JN516x
  #if JENNIC_CHIP==JN5168
	    eZCL_Status = eOTA_AllocateEndpointOTASpace(
	    		OTA_CLIENT_EP,
	    		u8StartSector,
	    		OTA_MAX_IMAGES_PER_ENDPOINT,
	    		4,
	    		FALSE,
	    		au8CAPublicKey);
  #elif JENNIC_CHIP==JN5169
	    eZCL_Status = eOTA_AllocateEndpointOTASpace(
	                    OTA_CLIENT_EP,
	                    u8StartSector,
	                    OTA_MAX_IMAGES_PER_ENDPOINT,
	                    8,                                 // max sectors per image
	                    FALSE,
	                    au8CAPublicKey);

  #endif
#endif

	if (eZCL_Status != E_ZCL_SUCCESS)
    {
	    DBG_vPrintf(TRACE_APP_OTA, "eAllocateEndpointOTASpace returned error 0x%x", eZCL_Status);
    }
    else
    {
        #if TRACE_APP_OTA
            tsOTA_ImageHeader          sOTAHeader;
    	    eOTA_GetCurrentOtaHeader(OTA_CLIENT_EP,FALSE,&sOTAHeader);
    	    DBG_vPrintf(TRACE_APP_OTA,"\n\nCurrent Image Details \n");
    	    DBG_vPrintf(TRACE_APP_OTA,"File ID = 0x%08x\n",sOTAHeader.u32FileIdentifier);
    	    DBG_vPrintf(TRACE_APP_OTA,"Header Ver ID = 0x%04x\n",sOTAHeader.u16HeaderVersion);
    	    DBG_vPrintf(TRACE_APP_OTA,"Header Length ID = 0x%04x\n",sOTAHeader.u16HeaderLength);
    	    DBG_vPrintf(TRACE_APP_OTA,"Header Control Filed = 0x%04x\n",sOTAHeader.u16HeaderControlField);
    	    DBG_vPrintf(TRACE_APP_OTA,"Manufac Code = 0x%04x\n",sOTAHeader.u16ManufacturerCode);
    	    DBG_vPrintf(TRACE_APP_OTA,"Image Type = 0x%04x\n",sOTAHeader.u16ImageType);
    	    DBG_vPrintf(TRACE_APP_OTA,"File Ver = 0x%08x\n",sOTAHeader.u32FileVersion);
    	    DBG_vPrintf(TRACE_APP_OTA,"Stack Ver = 0x%04x\n",sOTAHeader.u16StackVersion);
    	    DBG_vPrintf(TRACE_APP_OTA,"Image Len = 0x%08x\n\n\n",sOTAHeader.u32TotalImage);
		#endif

		eZCL_Status = eOTA_RestoreClientData( OTA_CLIENT_EP, &sOTA_PersistedData, 1);
		DBG_vPrintf(TRACE_APP_OTA,"OTA PDM Status = %d \n",eZCL_Status);


		DBG_vPrintf(TRACE_APP_OTA, "Valid %d Nwk Addr %04x Ieee %016llx\n",
		        sZllState.bValid, sZllState.u16NwkAddrOfServer, sZllState.u64IeeeAddrOfServer);
		if (sZllState.bValid == TRUE)
		{
			eOTA_State = OTA_QUERYIMAGE;
			u32OTAQueryTimeinSec = OTA_INITIAL_TIME_IN_SEC;
			DBG_vPrintf(TRACE_APP_OTA, "Start With Server\n");
		} else {
			eOTA_State = OTA_FIND_SERVER;
			u32OTAQueryTimeinSec = OTA_INITIAL_TIME_IN_SEC;
			DBG_vPrintf(TRACE_APP_OTA,  "Start No Server\n");
		}
		u32OTARetry = 0;
    }
	bWaitUgradeTime = FALSE;
	DBG_vPrintf(OTA_LNT, "Init OTA Tick to %d\n", u32OTAQueryTimeinSec);

}
/****************************************************************************
 *
 * NAME: vLoadOTAPersistedData
 *
 * DESCRIPTION:
 * Loads back OTA persisted data from PDM in application at start up.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vLoadOTAPersistedData(void)
{
    /*Restore OTA Persistent Data*/
    PDM_teStatus eStatusOTAReload;
    uint16 u16ByteRead;

    eStatusOTAReload = PDM_eReadDataFromRecord(PDM_ID_OTA_DATA,
                &sOTA_PersistedData,
                sizeof(tsOTA_PersistedData), &u16ByteRead);

    DBG_vPrintf(TRACE_APP_OTA,"eStatusOTAReload=%d size %d\n",eStatusOTAReload, sizeof(tsOTA_PersistedData) );



    if(sOTA_PersistedData.u32RequestBlockRequestTime != 0)
    {
        sOTA_PersistedData.u32RequestBlockRequestTime = 10;  // 
        DBG_vPrintf(TRACE_APP_OTA, "Set block time to 10\n");
    }
	/*Make retries 0*/
    sOTA_PersistedData.u8Retry = 0;
}

/****************************************************************************
 *
 * NAME: vCheckforOTAOTAUpgrade
 *
 * DESCRIPTION:
 * Timely checks for the OTA upgrade when the device state is running.
 * This is called from a timer ticking at rate of 1sec
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vRunAppOTAStateMachine(void)
{

	if( E_RUNNING ==  sZllState.eNodeState)
	{
	    /*Increment Second timer */
	    u32OTAQueryTimeinSec++;
	    vManagaeOTAState();
	}

}

/****************************************************************************
 *
 * NAME: vHandleAppOtaClient
 *
 * DESCRIPTION:
 * Handles the OTA Cluster Client events.
 * This is called from the EndPoint call back in the application
 * when an OTA event occurs.
 *
 * INPUT:
 * tsOTA_CallBackMessage *psCallBackMessage Pointer to cluster callback message
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vHandleAppOtaClient(tsOTA_CallBackMessage *psCallBackMessage)
{
    tsZCL_Address sZCL_Address;
   //DBG_vPrintf(TRACE_APP_OTA, "OTA Event Id = %d\n",psCallBackMessage->eEventId);

    if(psCallBackMessage->eEventId == E_CLD_OTA_COMMAND_QUERY_NEXT_IMAGE_RESPONSE )
    {
        DBG_vPrintf(TRACE_APP_OTA,"\n\n\nQuery Next Image Response \n");
        u32OTARetry = 0;
        if (sZllState.bValid)
        {
            eOTA_State = OTA_QUERYIMAGE;
            u32OTAQueryTimeinSec = 0;
            if (psCallBackMessage->uMessage.sQueryImageResponsePayload.u8Status != E_ZCL_SUCCESS) {
                DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,"Query Status 0x%02x\n", psCallBackMessage->uMessage.sQueryImageResponsePayload.u8Status);
                /* Got Response, nothing there, wait for query period */
            } else {
                /* there is an image, image check to follow */
                DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,"Image Size 0x%08x\n",psCallBackMessage->uMessage.sQueryImageResponsePayload.u32ImageSize);
                DBG_vPrintf(TRACE_APP_OTA,"File Ver  0x%08x \n",psCallBackMessage->uMessage.sQueryImageResponsePayload.u32FileVersion);
                DBG_vPrintf(TRACE_APP_OTA,"Manufacture Code 0x%04x  \n",psCallBackMessage->uMessage.sQueryImageResponsePayload.u16ManufacturerCode);
                DBG_vPrintf(TRACE_APP_OTA,"Image Type 0x%04x\n\n\n",psCallBackMessage->uMessage.sQueryImageResponsePayload.u16ImageType);
            }
        } else {
            eOTA_State = OTA_FIND_SERVER;
            u32OTAQueryTimeinSec = OTA_SERVER_QUERY_TIME_IN_SEC-1;
            DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,  "Image Response while lost server\n");
        }
    }

    if(psCallBackMessage->eEventId == E_CLD_OTA_INTERNAL_COMMAND_VERIFY_IMAGE_VERSION )
    {
    	DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,"\n\nVerify the version \n");
    	DBG_vPrintf(TRACE_APP_OTA,"Current Ver = 0x%08x\n",psCallBackMessage->uMessage.sImageVersionVerify.u32CurrentImageVersion );
    	DBG_vPrintf(TRACE_APP_OTA,"Notified Ver = 0x%08x\n",psCallBackMessage->uMessage.sImageVersionVerify.u32NotifiedImageVersion);

    	if (psCallBackMessage->uMessage.sImageVersionVerify.u32CurrentImageVersion ==
    		psCallBackMessage->uMessage.sImageVersionVerify.u32NotifiedImageVersion	)
    	{
				psCallBackMessage->uMessage.sImageVersionVerify.eImageVersionVerifyStatus =E_ZCL_FAIL;
				/* go back to waiting */
				DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,  "UNWANTED IMAGE\n");
				eOTA_State = OTA_QUERYIMAGE;
				u32OTAQueryTimeinSec = 0;
    	} else {
    	    /* down load about to start */
    	    vStartEffect(E_CLD_IDENTIFY_EFFECT_BREATHE);
    	    eOTA_State = OTA_DL_PROGRESS;
    	    u32OTAQueryTimeinSec = 0;
    	    u32TimeOut = 0;
    	    DBG_vPrintf(OTA_LNT, "Accept Image\n");
    	}
    }

#ifdef OTA_STRING_COMPARE
    if(psCallBackMessage->eEventId == E_CLD_OTA_INTERNAL_COMMAND_VERIFY_STRING )
    {
        int i=0;
        DBG_vPrintf(TRACE_APP_OTA, "Status = %d\n",psCallBackMessage->uMessage.sImageIdentification.eStatus);

        DBG_vPrintf(TRACE_APP_OTA, "Current Header String:\n");
        for(i=0;i<32;i++)
            DBG_vPrintf(TRACE_APP_OTA, "%02x ",*(psCallBackMessage->uMessage.sImageIdentification.pu8Current +i));

        DBG_vPrintf(TRACE_APP_OTA, "\nNotified Header String: \n");
        for(i=0;i<32;i++)
            DBG_vPrintf(TRACE_APP_OTA, "%02x ",*(psCallBackMessage->uMessage.sImageIdentification.puNotified +i));

        if(psCallBackMessage->uMessage.sImageIdentification.eStatus == E_ZCL_FAIL)
        {
                DBG_vPrintf(TRACE_APP_OTA,  "Mismatch\n\n\n");
                eOTA_State = OTA_QUERYIMAGE;
                u32OTAQueryTimeinSec = OTA_IMAGE_QUERY_TIME_IN_SEC;
        }

    }
#endif

    if(psCallBackMessage->eEventId == E_CLD_OTA_COMMAND_UPGRADE_END_RESPONSE )
    {
        DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,"\n\n\nUpgrade End Response \n");
        DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,"Upgrade Time : 0x%08x\n",psCallBackMessage->uMessage.sUpgradeResponsePayload.u32UpgradeTime);
        DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,"Current Time : 0x%08x\n",psCallBackMessage->uMessage.sUpgradeResponsePayload.u32CurrentTime);
        DBG_vPrintf(TRACE_APP_OTA,"File Version : 0x%08x\n",psCallBackMessage->uMessage.sUpgradeResponsePayload.u32FileVersion);
        DBG_vPrintf(TRACE_APP_OTA,"Image Type   : 0x%04x\n",psCallBackMessage->uMessage.sUpgradeResponsePayload.u16ImageType);
        DBG_vPrintf(TRACE_APP_OTA,"Manufacturer : 0x%04x\n",psCallBackMessage->uMessage.sUpgradeResponsePayload.u16ManufacturerCode);
        if(0xFFFFFFFF != psCallBackMessage->uMessage.sUpgradeResponsePayload.u32UpgradeTime )
        {
            if(psCallBackMessage->uMessage.sUpgradeResponsePayload.u32UpgradeTime > psCallBackMessage->uMessage.sUpgradeResponsePayload.u32CurrentTime)
            {
                psCallBackMessage->uMessage.sUpgradeResponsePayload.u32UpgradeTime -= psCallBackMessage->uMessage.sUpgradeResponsePayload.u32CurrentTime;
                DBG_vPrintf(OTA_LNT, "FUTURE UPGRADE %d\n", psCallBackMessage->uMessage.sUpgradeResponsePayload.u32UpgradeTime);
            }
            else
            {
                /* If upgrade time is in past , upgrade in one second*/
                psCallBackMessage->uMessage.sUpgradeResponsePayload.u32UpgradeTime = 5;
        }
                psCallBackMessage->uMessage.sUpgradeResponsePayload.u32CurrentTime = 0;
                DBG_vPrintf(OTA_LNT, "DO IT IN 5 SECOND\n");
        }
        bWaitUgradeTime = TRUE;
        vStartEffect(E_CLD_IDENTIFY_EFFECT_OKAY);
    }

    if(psCallBackMessage->eEventId == E_CLD_OTA_INTERNAL_COMMAND_SAVE_CONTEXT )
    {
        DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,"\nSave Context\n");
        vOTAPersist();
    }
    if(psCallBackMessage->eEventId == E_CLD_OTA_INTERNAL_COMMAND_SWITCH_TO_UPGRADE_DOWNGRADE )
    {
        DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,"\nSwitching to New Image\n");
    }

    if(psCallBackMessage->eEventId ==  E_CLD_OTA_INTERNAL_COMMAND_OTA_DL_ABORTED)
    {
        DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,"DL complete INvalid Image\n\n");
        eOTA_State = OTA_QUERYIMAGE;
        u32OTAQueryTimeinSec = 0;
    }
    if(psCallBackMessage->eEventId ==  E_CLD_OTA_INTERNAL_COMMAND_RESET_TO_UPGRADE)
    {
        DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,"E_CLD_OTA_INTERNAL_COMMAND_RESET_TO_UPGRADE\n\n");
        //vDumpFlashData(0,200000);
    }
    if(psCallBackMessage->eEventId ==  E_CLD_OTA_COMMAND_UPGRADE_END_RESPONSE)
    {
        DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,"E_CLD_OTA_COMMAND_UPGRADE_END_RESPONSE\n\n");
    }
    if(psCallBackMessage->eEventId == E_CLD_OTA_COMMAND_BLOCK_RESPONSE)
    {
        u32TimeOut = 0;
        eOTA_State = OTA_DL_PROGRESS;
        DBG_vPrintf(TRACE_APP_OTA|OTA_LNT, "Block Resp Status = %02x file offset %d\n",
                psCallBackMessage->uMessage.sImageBlockResponsePayload.u8Status,
                psCallBackMessage->uMessage.sImageBlockResponsePayload.uMessage.sBlockPayloadSuccess.u32FileOffset);
        if(psCallBackMessage->uMessage.sImageBlockResponsePayload.u8Status == OTA_STATUS_SUCCESS)
        {
            psCallBackMessage->uMessage.sImageBlockResponsePayload.u8Status = u8VerifyLinkKey(psCallBackMessage);
            if(OTA_STATUS_ABORT == psCallBackMessage->uMessage.sImageBlockResponsePayload.u8Status)
            {
                /* Send an Abort from Application, the ZCL is not doing it.*/
                tsOTA_UpgradeEndRequestPayload sUpgradeEndRequestPayload;

                sUpgradeEndRequestPayload.u32FileVersion = psCallBackMessage->uMessage.sImageBlockResponsePayload.uMessage.sBlockPayloadSuccess.u32FileVersion;
                sUpgradeEndRequestPayload.u16ImageType = psCallBackMessage->uMessage.sImageBlockResponsePayload.uMessage.sBlockPayloadSuccess.u16ImageType;
                sUpgradeEndRequestPayload.u16ManufacturerCode = psCallBackMessage->uMessage.sImageBlockResponsePayload.uMessage.sBlockPayloadSuccess.u16ManufacturerCode;
                sUpgradeEndRequestPayload.u8Status = psCallBackMessage->uMessage.sImageBlockResponsePayload.u8Status;
#if OTA_ACKS_ON == TRUE
                sZCL_Address.eAddressMode = E_ZCL_AM_SHORT;
#else
                sZCL_Address.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
#endif
                sZCL_Address.uAddress.u16DestinationAddress = psCallBackMessage->sPersistedData.u16ServerShortAddress;

                eOTA_ClientUpgradeEndRequest(
                         OTA_CLIENT_EP,                                         // 0uint8 u8SourceEndpoint,
                         psCallBackMessage->sPersistedData.u8DstEndpoint,       // uint8  u8DestinationEndpoint,
                         &sZCL_Address,                                         // tsZCL_Address
                         &sUpgradeEndRequestPayload);
            }
        }

    }
    if(psCallBackMessage->eEventId ==  E_CLD_OTA_INTERNAL_COMMAND_OTA_DL_ABORTED)
    {
        DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,"E_CLD_OTA_INTERNAL_COMMAND_OTA_DL_ABORTED\n\n");
    }
    u32TimeOut = 0;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: u8VerifyLinkKey
 *
 * DESCRIPTION:
 * Verifies link key once first 1K data is downloaded and saved in flash
 *
 * RETURNS:
 * staus of the verification
 *
 ****************************************************************************/
PRIVATE uint8 u8VerifyLinkKey(tsOTA_CallBackMessage *psCallBackMessage)
{

    static bool_t bKeyVerified = FALSE;

    //DBG_vPrintf(TRACE_APP_OTA, "Block Resp Offset = %d \n",psCallBackMessage->uMessage.sImageBlockResponsePayload.uMessage.sBlockPayloadSuccess.u32FileOffset);
    //DBG_vPrintf(TRACE_APP_OTA, "Flash_start = 0x%08x\n",&_flash_start);
    //DBG_vPrintf(TRACE_APP_OTA, "LinkKey Location = 0x%08x\n",(uint32)&FlsLinkKey);

    /*Assumption : First 1 K downloaded and saved to external flash*/
    if(
            (bKeyVerified == FALSE) &&
            (psCallBackMessage->uMessage.sImageBlockResponsePayload.uMessage.sBlockPayloadSuccess.u32FileOffset > APP_OTA_OFFSET_WRITEN_BEFORE_LINKKEY_VERIFICATION )
       )
    {
        bKeyVerified = TRUE;

        volatile int i;
        uint8 au8DownloadedLnkKey[0x10];
        uint8 au8Key[0x10];

        uint32 u32LnkKeyLocation = (uint32)(&(_FlsLinkKey)) - (uint32)(&(_flash_start));
#ifdef OTA_INTERNAL_STORAGE
        u32LnkKeyLocation += psCallBackMessage->u8ImageStartSector[0] * sNvmDefsStruct.u32SectorSize;
#endif
        DBG_vPrintf(TRACE_APP_OTA,"Link Key Offset in External Flash = 0x%08x\n",u32LnkKeyLocation);
        bAHI_FullFlashRead(u32LnkKeyLocation,0x10,au8DownloadedLnkKey);



        /*Get a copy of the Lnk Key*/
        for(i=0;i<16;i++)
        {
            au8Key[i] = s_au8LnkKeyArray[i];
        }

#ifndef OTA_INTERNAL_STORAGE
        extern uint32 _enc_start;
        /* for internal storage of the image the key will never be encrypted
         * as the downloaded image is decrypted before placing it in the internal flash
         */
        uint32 u32CustomerSettings = *((uint32*)FL_INDEX_SECTOR_CUSTOMER_SETTINGS);
        bool_t bEncExternalFlash = (u32CustomerSettings & FL_INDEX_SECTOR_ENC_EXT_FLASH)?FALSE:TRUE;

        if(bEncExternalFlash)
        {
            uint8 au8Iv[0x10];
            uint8 au8DataOut[0x10];

            uint32 u32IVLocation = 0x10;
            tsReg128 sKey;

            /*Get the downloaded IV from Ext Flash */
            bAHI_FullFlashRead(u32IVLocation,0x10,au8Iv);
            DBG_vPrintf(TRACE_APP_OTA,"The Plain IV :\n");

            for (i=0;i<0x10;i++)
                DBG_vPrintf(TRACE_APP_OTA,"au8Iv[%d]: 0x%02x\n",i,au8Iv[i]);

            DBG_vPrintf(TRACE_APP_OTA,"The Enc Offset Address = 0x%08x\n",((uint32)(&(_enc_start))));
            uint32 u32LnkKeyFromEncStart =  ((uint32)(&(_FlsLinkKey)) - ((uint32)(&(_enc_start))));
            DBG_vPrintf(TRACE_APP_OTA,"The The Total Bytes between Enc Offset and LnkKey = 0x%08x\n", u32LnkKeyFromEncStart );
            uint8 u8IVIncrement = (uint8)(u32LnkKeyFromEncStart>>4);
            DBG_vPrintf(TRACE_APP_OTA,"The IV should be increased by = 0x%08x\n", u8IVIncrement);

            /*Increase IV*/
            au8Iv[15] = au8Iv[15] + u8IVIncrement;

            /*Get the EFUSE keys*/
            uint32 *pu32KeyPtr = (uint32*)FL_INDEX_SECTOR_ENC_KEY_ADDRESS;

            sKey.u32register0 = *pu32KeyPtr;
            sKey.u32register1 = *(pu32KeyPtr+1);
            sKey.u32register2 = *(pu32KeyPtr+2);
            sKey.u32register3 = *(pu32KeyPtr+3);

            DBG_vPrintf(TRACE_APP_OTA,"The Key is :\n");
            DBG_vPrintf(TRACE_APP_OTA,"sKey.u32register0: 0x%08x\n",sKey.u32register0);
            DBG_vPrintf(TRACE_APP_OTA,"sKey.u32register1: 0x%08x\n",sKey.u32register1);
            DBG_vPrintf(TRACE_APP_OTA,"sKey.u32register2: 0x%08x\n",sKey.u32register2);
            DBG_vPrintf(TRACE_APP_OTA,"sKey.u32register3: 0x%08x\n",sKey.u32register3);

            /*Encrypt the IV*/
            vOTA_EncodeString(&sKey,au8Iv,au8DataOut);

            /* Encrypt the Internal Key */
            for(i=0;i<16;i++)
            {
                au8Key[i] = au8Key[i] ^ au8DataOut[i];
            }
        }
#endif

        /*Comparing the Keys*/
        for(i=0;i<0x10;i++)
        {
            DBG_vPrintf(TRACE_APP_OTA,"Internal Key[%d] = %02x Downloaded Key[%d] = 0x%02x \n",i,au8Key[i],i,au8DownloadedLnkKey[i]);

            /*Compare to see if they match else its an invalid image*/
            if(au8Key[i]!=au8DownloadedLnkKey[i])
            {
                DBG_vPrintf(TRACE_APP_OTA,"Key Mismatch, Abort DownLoad\n");
                bKeyVerified=FALSE;
                return OTA_STATUS_ABORT;
            }
        }


        #ifdef OTA_CLIENT_ABORT_TH
            /*First Link key check will fail  if the TH is defined to test Client initiated Aborts.*/
            volatile static bool_t bClientAbort = TRUE;

            if(TRUE == bClientAbort)
            {
                bClientAbort = FALSE;
                return OTA_STATUS_ABORT;
            }
        #endif
    }



    return OTA_STATUS_SUCCESS;
}




/****************************************************************************
 *
 * NAME: vInitAndDisplayKeys
 *
 * DESCRIPTION:
 * Initialize Keys and displays the content
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vInitAndDisplayKeys(void)
{
	#ifndef JENNIC_CHIP_FAMILY_JN514x
        /*ZPS_vSetOverrideLocalMacAddress((uint64 *)&au8MacAddress);*/
        volatile uint8 u8Index;
        for(u8Index=0;u8Index<8;u8Index++)
        	au8MacAddressVolatile[u8Index] = au8MacAddress[u8Index];
	#endif



	#if TRACE_APP_OTA==TRUE
        uint8 i;
        DBG_vPrintf(TRACE_APP_OTA, "MAC Address at address = %08x\n\n\n",au8MacAddress);
        for (i =0;i<8;i++)
        {
        	DBG_vPrintf(TRACE_APP_OTA, "%02x ",au8MacAddress[i] );
        }

        DBG_vPrintf(TRACE_APP_OTA, "\n\n\n Link Key ");
        for (i =0;i<16;i++)
        {
        	DBG_vPrintf(TRACE_APP_OTA, "%02x ",s_au8LnkKeyArray[i] );
        }
        DBG_vPrintf(TRACE_APP_OTA, "\n\n\n");
	#endif
}

/****************************************************************************
 *
 * NAME: vHandleMatchDescriptor
 *
 * DESCRIPTION:
 * Checks for the OTA cluster match during OTA server discovery, if a match
 * found it will make an entry in the local discovery table.This table will be
 * used to query image requests by the client.
 *
 *
 * INPUT:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vHandleMatchDescriptor( ZPS_tsAfEvent  * psStackEvent)
{
    ZPS_tsAplZdpMatchDescRsp sMatchDescRsp;
    uint32 u32Location;
    PDUM_thAPduInstance hAPduInst = psStackEvent->uEvent.sApsDataIndEvent.hAPduInst;
    uint16 u16ShortAddress;

    u32Location = 1;   // first loc has seq no
    sMatchDescRsp.u8Status = ((pdum_tsAPduInstance*)hAPduInst)->au8Storage[u32Location++];
    if (sMatchDescRsp.u8Status == 0)
    {
        APDU_BUF_READ16_INC(sMatchDescRsp.u16NwkAddrOfInterest,hAPduInst, u32Location);
        sMatchDescRsp.u8MatchLength = ((pdum_tsAPduInstance*)hAPduInst)->au8Storage[u32Location++];
        if(psStackEvent->uEvent.sApsDataIndEvent.u8SrcAddrMode == ZPS_E_ADDR_MODE_SHORT)
        {
            u16ShortAddress = psStackEvent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr;
        }
        else
        {
            u16ShortAddress = ZPS_u16NwkNibFindNwkAddr(ZPS_pvAplZdoGetNwkHandle(),psStackEvent->uEvent.sApsDataIndEvent.uSrcAddress.u64Addr);
        }
        if(sMatchDescRsp.u16NwkAddrOfInterest == ZPS_E_BROADCAST_RX_ON)
        {

            sMatchDescRsp.u16NwkAddrOfInterest = u16ShortAddress;
        }

        DBG_vPrintf(TRACE_APP_OTA,  "Match Ep Count %d\n", sMatchDescRsp.u8MatchLength);
        if(sMatchDescRsp.u8MatchLength != 0)
        {

            sZllState.u8OTAserverEP = ((pdum_tsAPduInstance*)hAPduInst)->au8Storage[u32Location];
            sZllState.u16NwkAddrOfServer = sMatchDescRsp.u16NwkAddrOfInterest;
            DBG_vPrintf(TRACE_APP_OTA,"\n\nNwk Address oF server = %04x\n", sZllState.u16NwkAddrOfServer);

            DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,"OTA Server EP# = %d\n", sZllState.u8OTAserverEP);

            //vGetIEEEAddress();
            eOTA_State = OTA_IEEE_LOOK_UP;
            u32OTAQueryTimeinSec = 0;
        }

    }

}
/****************************************************************************
 *
 * NAME: vHandleIeeeAddressRsp
 *
 * DESCRIPTION:
 * Handles IEEE address look up query query
 * Makes an entry in the application OTA discovery table. Later this is used
 * for by the OTA requests.
 *
 * This function is called from the application OTA handler with stack event
 * as input.
 *
 *
 * INPUT:
 * ZPS_tsAfEvent  * psStackEvent   Pointer to the stack event
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vHandleIeeeAddressRsp( ZPS_tsAfEvent  * psStackEvent)
{
    ZPS_tsAplZdpIeeeAddrRsp sIeeeAddrRsp;
    uint32 u32Location = 1;                     // first location has seq no, so skip
    PDUM_thAPduInstance hAPduInst = psStackEvent->uEvent.sApsDataIndEvent.hAPduInst;

    sIeeeAddrRsp.u8Status = ((pdum_tsAPduInstance*)hAPduInst)->au8Storage[u32Location++];
    DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,  "IEEE Rsp Status %02x ", sIeeeAddrRsp.u8Status);

    if (sIeeeAddrRsp.u8Status == 0)
    {
        u32Location += PDUM_u16APduInstanceReadNBO(hAPduInst, u32Location, "l",
                                                   &sIeeeAddrRsp.u64IeeeAddrRemoteDev);
        APDU_BUF_READ16_INC(sIeeeAddrRsp.u16NwkAddrRemoteDev,hAPduInst, u32Location);

    DBG_vPrintf(TRACE_APP_OTA,  "Addr of sever %04x Addr Rem Dev %04x\n ",
            sZllState.u16NwkAddrOfServer,
            sIeeeAddrRsp.u16NwkAddrRemoteDev);
    if( sZllState.u16NwkAddrOfServer == sIeeeAddrRsp.u16NwkAddrRemoteDev)
    {
        /*Make an entry in the OTA server tables*/
        sZllState.u64IeeeAddrOfServer = sIeeeAddrRsp.u64IeeeAddrRemoteDev;
        //DBG_vPrintf(TRACE_APP_OTA,  "ieee long %016llx\n", psStackEvent->uEvent.sApsZdpEvent.uZdpData.sIeeeAddrRsp.u64IeeeAddrRemoteDev);
        DBG_vPrintf(TRACE_APP_OTA,"Entry Added NWK Addr 0x%04x IEEE Addr 0x%016llx",
                sZllState.u16NwkAddrOfServer, sZllState.u64IeeeAddrOfServer);
        ZPS_eAplZdoAddAddrMapEntry( sZllState.u16NwkAddrOfServer,
                sZllState.u64IeeeAddrOfServer,
                                    FALSE);
        sZllState.bValid = TRUE;
        PDM_eSaveRecordData(PDM_ID_APP_ZLL_ROUTER,&sZllState,sizeof(tsZllState));
        eOTA_State = OTA_QUERYIMAGE;
        u32OTAQueryTimeinSec = OTA_IMAGE_QUERY_TIME_IN_SEC-15;
        u32OTARetry = 0;
        DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,  "SERVER DISCOVERED\n");
    } else { DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,  "NOT VALID\n" ); }


} else { DBG_vPrintf(TRACE_APP_OTA,  "IEEE req failed\n"); }
}
/****************************************************************************
 *
 * NAME: vGetIEEEAddress
 *
 * DESCRIPTION:
 * Finds an IEEE address on the local node by calling Stack API, if no entries
 * found it request the IEEE look up on air.
 *
 *
 * INPUT:
 * uint8 u8Index   Index to the discovery table point to the NWK address of
 *                 the discovered server
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vGetIEEEAddress( void)
{
	/* Always query for address Over the air - Uncomment if required for local look up first*/
#ifdef LOCAL_ADDRESS_LOOK_UP
	/* See if there is a local address map exists  */
	uint64 u64IeeeAdress = ZPS_u64AplZdoLookupIeeeAddr(sDiscovedOTAServers[u8Index].u16NwkAddrOfServer);
	if( u64IeeeAdress != 0x0000000000000000 || u64IeeeAdress != 0xFFFFFFFFFFFFFFFF )
	{
		/*Valid address found, setting up the OTA server address */
		sDiscovedOTAServers[u8Index].u64IeeeAddrOfServer = u64IeeeAdress;
	}
	else
#endif
	{
		/* If there is no address map existing, then do a look up */
		PDUM_thAPduInstance hAPduInst;
		hAPduInst = PDUM_hAPduAllocateAPduInstance(apduZDP);

		if (hAPduInst == PDUM_INVALID_HANDLE)
		{
			DBG_vPrintf(TRACE_APP_OTA|OTA_LNT, "IEEE Address Request - PDUM_INVALID_HANDLE\n");
		}
		else
		{
			ZPS_tuAddress uDstAddr;
			bool bExtAddr;
			uint8 u8SeqNumber;
			ZPS_teStatus eStatus;
			ZPS_tsAplZdpIeeeAddrReq sZdpIeeeAddrReq;

			uDstAddr.u16Addr = sZllState.u16NwkAddrOfServer;
			bExtAddr = FALSE;
			sZdpIeeeAddrReq.u16NwkAddrOfInterest = sZllState.u16NwkAddrOfServer;
			sZdpIeeeAddrReq.u8RequestType = 0;
			sZdpIeeeAddrReq.u8StartIndex = 0;

			eStatus= ZPS_eAplZdpIeeeAddrRequest(
    	                                    hAPduInst,
    	                                    uDstAddr,
    	                                    bExtAddr,
    	                                    &u8SeqNumber,
    	                                    &sZdpIeeeAddrReq);
			if (eStatus)
			{
				PDUM_eAPduFreeAPduInstance(hAPduInst);
				DBG_vPrintf(TRACE_APP_OTA|OTA_LNT, "Address Request failed: 0x%02x\n", eStatus);
			} else { DBG_vPrintf(OTA_LNT, "IEEE SENT OK\n"); }
		}
	}
}
/****************************************************************************
 *
 * NAME: bMatchRecieved
 *
 * DESCRIPTION:
 * Validation function for the match and sets the valid field true in the
 * discovery table if there is valid OTA Server found in the network
 *
 *
 * INPUT:
 *
 * RETURNS:
 * TRUE/FALSE
 *
 ****************************************************************************/
PRIVATE bool_t bMatchRecieved(void)
{

	return (sZllState.bValid == TRUE)? TRUE: FALSE;
}
/****************************************************************************
 *
 * NAME: eClientQueryNextImageRequest
 *
 * DESCRIPTION:
 * Query nest image request application wrapper.
 *
 *
 * INPUT:
 *	uint8 u8SourceEndpoint
 *  uint8 u8DestinationEndpoint,
 *	uint16 u16DestinationAddress,
 *  uint32 u32FileVersion,
 *  uint16 u16HardwareVersion,
 *  uint16 u16ImageType,
 *  uint16 u16ManufacturerCode,
 *  uint8 u8FieldControl
 *
 * RETURNS:
 * ZCL status of the call
 *
 ****************************************************************************/
PRIVATE teZCL_Status eClientQueryNextImageRequest(
					uint8 u8SourceEndpoint,
                    uint8 u8DestinationEndpoint,
					uint16 u16DestinationAddress,
                    uint32 u32FileVersion,
                    uint16 u16HardwareVersion,
                    uint16 u16ImageType,
                    uint16 u16ManufacturerCode,
                    uint8 u8FieldControl)
{
    teZCL_Status eStatus;
    tsZCL_Address sAddress;
	sAddress.eAddressMode = E_ZCL_AM_SHORT;
	sAddress.uAddress.u16DestinationAddress = u16DestinationAddress;

	tsOTA_QueryImageRequest sRequest;
	sRequest.u32CurrentFileVersion = u32FileVersion;
	sRequest.u16HardwareVersion = u16HardwareVersion;
	sRequest.u16ImageType = u16ImageType;
	sRequest.u16ManufacturerCode = u16ManufacturerCode;
	sRequest.u8FieldControl = u8FieldControl;


	eStatus = eOTA_ClientQueryNextImageRequest(
			u8SourceEndpoint,
            u8DestinationEndpoint,
			&sAddress,
            &sRequest);

	DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,  "query Next ret %02x\n", eStatus);
	return eStatus;
}
/****************************************************************************
 *
 * NAME: eSendOTAMatchDescriptor
 *
 * DESCRIPTION:
 * Sends the OTA match descriptor for OTA server discovery as a broadcast.
 *
 *
 * INPUT:
 *  uint16 u16ProfileId Profile Identifier
 *
 * RETURNS:
 * ZPS status of the call
 *
 ****************************************************************************/
PRIVATE ZPS_teStatus eSendOTAMatchDescriptor(uint16 u16ProfileId)
{
    uint16 au16InClusters[]={OTA_CLUSTER_ID};
    uint8 u8TransactionSequenceNumber;
    ZPS_tuAddress uDestinationAddress;
    ZPS_tsAplZdpMatchDescReq sMatch;

    sMatch.u16ProfileId = u16ProfileId;
    sMatch.u8NumInClusters=sizeof(au16InClusters)/sizeof(uint16);
    sMatch.pu16InClusterList=au16InClusters;
    sMatch.pu16OutClusterList=NULL;
    sMatch.u8NumOutClusters=0;
    sMatch.u16NwkAddrOfInterest=0xFFFD;

    uDestinationAddress.u16Addr = 0xFFFD;

    PDUM_thAPduInstance hAPduInst = PDUM_hAPduAllocateAPduInstance(apduZDP);

    if (hAPduInst == PDUM_INVALID_HANDLE)
    {
        DBG_vPrintf(TRACE_APP_OTA, "Allocate PDU ERR:\n");
        return (ZPS_teStatus)PDUM_E_INVALID_HANDLE;
    }

    ZPS_teStatus eStatus = ZPS_eAplZdpMatchDescRequest(
    		                hAPduInst,
    		                uDestinationAddress,
    		                FALSE,
    		                &u8TransactionSequenceNumber,
    		                &sMatch);

    if (eStatus)
    {
    	PDUM_eAPduFreeAPduInstance(hAPduInst);
        DBG_vPrintf(TRACE_APP_OTA|OTA_LNT, "Match ERR: 0x%x", eStatus);
    } else { DBG_vPrintf(OTA_LNT, "MATCH D Sent OK\n"); }

    return eStatus;
}
/****************************************************************************
 *
 * NAME: vManagaeOTAState
 *
 * DESCRIPTION:
 * Simple State Machine to move the OTA state from Dicovery to Download.
 * It alos implements a simple mechanism of time out.
 *
 * INPUT:
 * uint32 u32OTAQueryTime
 *
 * RETURNS:
 *
 *
 ****************************************************************************/
PRIVATE void vManagaeOTAState(void)
{
    ZPS_teStatus eStatus;
    if ( (u32OTAQueryTimeinSec % 10) == 0) {
        DBG_vPrintf(TRACE_APP_OTA|OTA_LNT, "OTA State %d Time %d\n", eOTA_State, u32OTAQueryTimeinSec);
    }
    switch(eOTA_State)
    {
        case OTA_FIND_SERVER:
        {
            if(u32OTAQueryTimeinSec > OTA_SERVER_QUERY_TIME_IN_SEC )
            {
                u32OTAQueryTimeinSec = 0;
                ZPS_teStatus eStatus;
                eStatus = eSendOTAMatchDescriptor(HA_PROFILE_ID);
                if ( eStatus)
                {
                    u32TimeOut = 0;
                    vRestetOTADiscovery();
                    DBG_vPrintf(TRACE_APP_OTA|OTA_LNT, "Send Match Error 0x%02x\n",eStatus);
                }
                else
                {
                    /*Wait for the discovery to complete */
                    u32TimeOut = 0;
                    eOTA_State = OTA_FIND_SERVER_WAIT;
                    u32OTAQueryTimeinSec = 0;
                    DBG_vPrintf(OTA_LNT, "Wait Server Rsp\n");

                }
            }
        }
        break;
        case OTA_FIND_SERVER_WAIT:
        {
            if( bMatchRecieved() )
            {
                u32OTAQueryTimeinSec = OTA_IMAGE_QUERY_TIME_IN_SEC-10;
                eOTA_State = OTA_QUERYIMAGE;
                u32OTARetry = 0;
            }
            else if(u32OTAQueryTimeinSec > OTA_DISCOVERY_TIMEOUT_IN_SEC )
            {

                u32OTAQueryTimeinSec = 0;
            	u32OTARetry = 0;
            	            	eOTA_State = OTA_FIND_SERVER;
                DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,  "MATCH TIME OUT\n");
            }
        }
        break;
        case OTA_IEEE_LOOK_UP:
        {
            if (u32OTAQueryTimeinSec > OTA_IEEE_LOOKUP_TIME_IN_SEC)
            {
                vGetIEEEAddress();
                eOTA_State = OTA_IEEE_WAIT;
                u32OTAQueryTimeinSec = 0;
            }
        }
        break;
        case OTA_IEEE_WAIT:
        {
            if( bMatchRecieved() )
            {
                u32OTAQueryTimeinSec = OTA_IMAGE_QUERY_TIME_IN_SEC-10;
                eOTA_State = OTA_QUERYIMAGE;
                u32OTARetry = 0;
            }
            if( u32OTAQueryTimeinSec > OTA_DISCOVERY_TIMEOUT_IN_SEC )
            {
                u32OTAQueryTimeinSec = OTA_SERVER_QUERY_TIME_IN_SEC-1;
                eOTA_State = OTA_FIND_SERVER;
                DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,  "IEEE TIME OUT\n");
            }

        }
        break;
        case OTA_QUERYIMAGE:
        {

            if(u32OTAQueryTimeinSec > OTA_IMAGE_QUERY_TIME_IN_SEC )
            {
                if(sZllState.bValid)
                {

                    tsOTA_ImageHeader          sOTAHeader;
                    eOTA_GetCurrentOtaHeader(OTA_CLIENT_EP,FALSE,&sOTAHeader);
                    DBG_vPrintf(TRACE_APP_OTA,"\n\nFile ID = 0x%08x\n",sOTAHeader.u32FileIdentifier);
                    DBG_vPrintf(TRACE_APP_OTA,"Header Ver ID = 0x%04x\n",sOTAHeader.u16HeaderVersion);
                    DBG_vPrintf(TRACE_APP_OTA,"Header Length ID = 0x%04x\n",sOTAHeader.u16HeaderLength);
                    DBG_vPrintf(TRACE_APP_OTA,"Header Control Filed = 0x%04x\n",sOTAHeader.u16HeaderControlField);
                    DBG_vPrintf(TRACE_APP_OTA,"Manufac Code = 0x%04x\n",sOTAHeader.u16ManufacturerCode);
                    DBG_vPrintf(TRACE_APP_OTA,"Image Type = 0x%04x\n",sOTAHeader.u16ImageType);
                    DBG_vPrintf(TRACE_APP_OTA,"File Ver = 0x%08x\n",sOTAHeader.u32FileVersion);
                    DBG_vPrintf(TRACE_APP_OTA,"Stack Ver = 0x%04x\n",sOTAHeader.u16StackVersion);
                    DBG_vPrintf(TRACE_APP_OTA,"Image Len = 0x%08x\n\n",sOTAHeader.u32TotalImage);

                    /*Set server address */
                    eOTA_SetServerAddress(
                            OTA_CLIENT_EP,
                            sZllState.u64IeeeAddrOfServer,
                            sZllState.u16NwkAddrOfServer
                    );


                    eStatus = eClientQueryNextImageRequest(
                            OTA_CLIENT_EP,
                            sZllState.u8OTAserverEP,
                            sZllState.u16NwkAddrOfServer,
                            sOTAHeader.u32FileVersion + 1,
                            0,
                            sOTAHeader.u16ImageType,
                            sOTAHeader.u16ManufacturerCode,
                            0
                    );

                    DBG_vPrintf(OTA_LNT, "Query Image Status Send %02x\n", eStatus);
                    eOTA_State = OTA_QUERYIMAGE_WAIT;
                    u32OTAQueryTimeinSec = 0;
                } else {
                    DBG_vPrintf(TRACE_APP_OTA,  "OTA Server not valid %d\n", sZllState.bValid);
                    eOTA_State = OTA_FIND_SERVER;
                    u32OTAQueryTimeinSec = OTA_SERVER_QUERY_TIME_IN_SEC-1;
                    u32OTARetry = 0;
                }
            }

        }
        break;
        case OTA_QUERYIMAGE_WAIT:
        {
            if(u32OTAQueryTimeinSec > OTA_IMAGE_QUERY_TIMEOUT_IN_SEC )
            {
                DBG_vPrintf(TRACE_APP_OTA,  "IMAGE QUERY WAIT TIMEOUT -> ");
                u32OTARetry++;
                DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,  "Retry %d ", u32OTARetry);
                if (u32OTARetry > 2) {
                	eOTA_State = OTA_FIND_SERVER;
                	u32OTAQueryTimeinSec = OTA_SERVER_QUERY_TIME_IN_SEC - 10;
                	u32OTARetry = 0;
                	vRestetOTADiscovery();
                	DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,  "CLEAR OUT\n");

                } else {
                	eOTA_State = OTA_QUERYIMAGE;
                	u32OTAQueryTimeinSec = OTA_IMAGE_QUERY_TIME_IN_SEC-1;
                	DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,  "TRY AGAIN\n");
                }

            }
        }
        break;
        case OTA_DL_PROGRESS:
        {
            vManageDLProgressState();
        }
        break;
        default:
            break;
    }
}

/****************************************************************************
 *
 * NAME: vManageDLProgressState
 *
 * DESCRIPTION:
 * Manages the DL progress state, mainly to get the state machine out of it
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vManageDLProgressState(void)
{

	u32TimeOut++;
	DBG_vPrintf(TRACE_APP_OTA,  "OTA in progress\n");
	if( ((u32TimeOut > OTA_DL_IN_PROGRESS_TIME_IN_SEC ) ||
	     ( sOTA_PersistedData.sAttributes.u8ImageUpgradeStatus == E_CLD_OTA_STATUS_NORMAL))
	        && (bWaitUgradeTime == FALSE)
	  )
	{
		u32TimeOut = 0;
		eOTA_State = OTA_QUERYIMAGE;
		u32OTAQueryTimeinSec = 0;
		DBG_vPrintf(TRACE_APP_OTA|OTA_LNT,  "OTA In Progress CLEAR OUT\n");
	}
}

/****************************************************************************
 *
 * NAME: vRestetOTADiscovery
 *
 * DESCRIPTION:
 * Resets OTA discovery so that a fresh discovery is possible
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vRestetOTADiscovery(void)
{
	sZllState.u64IeeeAddrOfServer = 0;
	sZllState.u16NwkAddrOfServer = 0xffff;
	sZllState.bValid = FALSE;
	PDM_eSaveRecordData(PDM_ID_APP_ZLL_ROUTER,&sZllState,sizeof(tsZllState));
}

/****************************************************************************
 *
 * NAME: vOTAPersist
 *
 * DESCRIPTION:
 * Persists OTA data when called by the OTA client ebvent, this is required to
 * restore the down load in case of power failure.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/

PRIVATE void vOTAPersist(void)
{
    sOTA_PersistedData = sGetOTACallBackPersistdata();
    PDM_eSaveRecordData(PDM_ID_OTA_DATA, &sOTA_PersistedData, sizeof(tsOTA_PersistedData));
}

/****************************************************************************
 *
 * NAME: vOTAResetPersist
 *
 * DESCRIPTION:
 * clear and save persistant data related to the OTA record
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vOTAResetPersist(void)
{
    sOTA_PersistedData = sGetOTACallBackPersistdata();
    memset(&sOTA_PersistedData, 0, sizeof(tsOTA_PersistedData));
    PDM_eSaveRecordData(PDM_ID_OTA_DATA, &sOTA_PersistedData, sizeof(tsOTA_PersistedData));
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
