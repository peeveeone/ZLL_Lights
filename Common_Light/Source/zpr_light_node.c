/*****************************************************************************
 *
 *
 * COMPONENT:          zpr_light_node.c
 *
 * DESCRIPTION:        ZLL Demo: Light Node NWK Startup - Implementation
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
 * Copyright NXP B.V. 2012. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <string.h>
#include "dbg.h"
#include "os.h"
#include "os_gen.h"
#include "pdum_apl.h"
#include "pdum_nwk.h"
#include "pdum_gen.h"
#include "pwrm.h"
#include "PDM.h"
#include "zps_gen.h"
#include "zps_apl.h"
#include "zps_apl_af.h"
#include "zps_apl_zdo.h"
#include "zps_apl_zdp.h"
#include "zps_apl_aib.h"
#include "zps_apl_aps.h"

#include "app_common.h"
#include "app_timer_driver.h"
#include "app_events.h"
#include <rnd_pub.h>

#include "app_zcl_light_task.h"
#include "zpr_light_node.h"
#include "zll.h"
#include "zps_nwk_nib.h"
#include "appZpsBeaconHandler.h"


#include "PDM_IDs.h"
#include "zcl_options.h"
#include "app_scenes.h"



#include "DriverBulb.h"

#include "zcl.h"


#ifndef DEBUG_APP
#define TRACE_APP   FALSE
#else
#define TRACE_APP   TRUE
#endif

#ifndef DEBUG_LIGHT_NODE
#define TRACE_LIGHT_NODE   FALSE
#else
#define TRACE_LIGHT_NODE   TRUE
#endif


#ifndef DEBUG_CLASSIC_JOIN
#define TRACE_CLASSIC FALSE
#else
#define TRACE_CLASSIC TRUE
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DBG_EVENT  FALSE

#define NO_CLASSIC_JOIN  FALSE


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PUBLIC void vClearIncomingFrameCounter(uint16 u16JoinedAddr);
PRIVATE void vOpenPermitJoin(void);

PRIVATE void vPickChannel( void *pvNwk);
PRIVATE void vDiscoverNetworks(void);
PRIVATE void vTryNwkJoin(void);



/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
#if DBG_EVENT
PRIVATE char *apcStateNames[] = { "E_STARTUP", "E_NFN_START", "E_DISCOVERY",
        "E_NETWORK_INIT", "E_RUNNING" };
#endif


#ifndef CLD_OTA
PUBLIC const uint8 s_au8LnkKeyArray[16] = { 0x5a, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41,
        0x6c, 0x6c, 0x69, 0x61, 0x6e, 0x63, 0x65, 0x30, 0x39 };
#endif
// ZLL Commissioning trust centre link key
// 0x81, 0x42, 0x86, 0x86, 0x5D, 0xC1, 0xC8, 0xB2, 0xC8, 0xCB, 0xC5, 0x2E, 0x5D, 0x65, 0xD1, 0xB8

static const uint8 s_au8ZllLnkKeyArray[16] = {0x81, 0x42, 0x86, 0x86, 0x5D, 0xC1, 0xC8, 0xB2,
		0xC8, 0xCB, 0xC5, 0x2E, 0x5D, 0x65, 0xD1, 0xB8};



//static const uint8 s_au8ZllLnkKeyArray[16] = {0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
//                                     0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf};


#ifdef MK_CHANNEL
const uint8 u8DiscChannels[] = { MK_CHANNEL, MK_CHANNEL, MK_CHANNEL, MK_CHANNEL };
#else
const uint8 u8DiscChannels[] = { 11, 15, 20, 25, 12, 13, 14, 16, 17, 18, 19, 21, 22, 23, 24, 26 };
#endif

uint8 u8ChanIdx;
uint8 u8NwkIdx;
uint8 u8NwkCount;
ZPS_tsNwkNetworkDescr *psNwkDescTbl;

uint32 u32OldFrameCtr;


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
#ifdef PDM_EEPROM
    extern uint8 u8PDM_CalculateFileSystemCapacity();
    extern uint8 u8PDM_GetFileSystemOccupancy();
#endif
extern void zps_vNwkSecClearMatSet(void *psNwk);


/****************************************************************************
 *
 * NAME: vDiscoverNetworks
 *
 * DESCRIPTION:
 * Initialises discovery on each channel in turn, till successful join or all have been tried
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vDiscoverNetworks(void) {
uint8 u8Status;
void* pvNwk;

    pvNwk = ZPS_pvAplZdoGetNwkHandle();

    while (u8ChanIdx < sizeof(u8DiscChannels))
    {
        DBG_vPrintf(TRACE_CLASSIC, "\nDiscover on ch %d\n", u8DiscChannels[u8ChanIdx] );
        ZPS_vNwkNibClearDiscoveryNT( pvNwk);
        u8Status = ZPS_eAplZdoDiscoverNetworks( 1 << u8DiscChannels[u8ChanIdx]);
        DBG_vPrintf(TRACE_CLASSIC, "disc status %02x\n", u8Status );
        u8ChanIdx++;
        if (u8Status == 0) {
            return;
        }
    }
    if (u8ChanIdx == sizeof(u8DiscChannels)) {
        DBG_vPrintf(TRACE_CLASSIC, "Discover complete\n");

        vPickChannel( pvNwk);
    }
}

/****************************************************************************
 *
 * NAME: vTryNwkJoin
 *
 * DESCRIPTION:
 * Attempts to join each of the discovered networks, if unsuccessful initiate discovery on next channel
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vTryNwkJoin(void) {
uint8 u8Status;

    while (u8NwkIdx < u8NwkCount) {
        if (psNwkDescTbl[u8NwkIdx].u8PermitJoining  ) {
            DBG_vPrintf(TRACE_CLASSIC, "Try To join %016llx on Ch %d\n", psNwkDescTbl[u8NwkIdx].u64ExtPanId, psNwkDescTbl[u8NwkIdx].u8LogicalChan);


            u8Status = ZPS_eAplZdoJoinNetwork( &psNwkDescTbl[u8NwkIdx] );
            DBG_vPrintf(TRACE_CLASSIC, "Try join status %02x\n", u8Status);
            if (u8Status == 0) {
                u8NwkIdx++;
                return;
            }
#if TRACE_CLASSIC
            else if (u8Status == 0xc3) {
                ZPS_tsNwkNib * thisNib;
                thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());
                int i=0;
                while ( i<thisNib->sTblSize.u8NtDisc && thisNib->sTbl.psNtDisc[i].u64ExtPanId !=0 ) {
                    DBG_vPrintf(TRACE_CLASSIC, "DiscNT %d Pan %016llx Addr %04x Chan %d LQI %d Depth %d\n",
                            i,
                            thisNib->sTbl.psNtDisc[i].u64ExtPanId,
                            thisNib->sTbl.psNtDisc[i].u16NwkAddr,
                            thisNib->sTbl.psNtDisc[i].u8LogicalChan,
                            thisNib->sTbl.psNtDisc[i].u8LinkQuality,
                            thisNib->sTbl.psNtDisc[i].uAncAttrs.bfBitfields.u4Depth);
                    i++;
                }
            }
#endif
        }
        u8NwkIdx++;
    }
    if (u8NwkIdx >= u8NwkCount) {
        DBG_vPrintf(TRACE_CLASSIC, "No more nwks to try\n");
        vDiscoverNetworks();
    }
}
#ifdef CLD_OTA
PUBLIC teNODE_STATES eGetNodeState(void)
{
    return sZllState.eNodeState;
}
#endif

/****************************************************************************
 *
 * NAME: APP_vInitialiseNode
 *
 * DESCRIPTION:
 * Initialises the application related functions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vInitialiseNode(void) {
#if (defined DR1173) || (defined DR1175)
    bool_t bDeleteRecords = FALSE;
#endif
    PDM_teStatus eStatus;
    uint16 u16ByteRead;

    DBG_vPrintf(TRACE_APP, "\nAPP_vInitialiseNode*");

    /* Stay awake */
    PWRM_eStartActivity();


    sZllState.eNodeState = E_STARTUP;

    #if (defined DR1173) || (defined DR1175)
		bDeleteRecords = APP_bButtonInitialise();
    #endif



    eStatus = PDM_eReadDataFromRecord(PDM_ID_APP_ZLL_ROUTER,
            &sZllState,
            sizeof(tsZllState), &u16ByteRead);



    DBG_vPrintf(TRACE_APP, "\nAPP: PDM Load ZLL_ROUTER returned %d, RecState=%d\n", eStatus, sZllPDDesc.eState);
#ifdef CLD_OTA
    vLoadOTAPersistedData();
#endif

    vLoadScenesNVM();



#if DBG_EVENT
    DBG_vPrintf(DBG_EVENT, "\nContext state:%s(%d)\n",
            apcStateNames[sZllState.eNodeState], sZllState.eNodeState);
#endif
    DBG_vPrintf(TRACE_LIGHT_NODE, "\nZll recovered state %02x\n", sZllState.eState);

    //Inter-PAN Messaging is enabled in the ZPS diagram for each node
    ZPS_vDefaultKeyInit();
    // Set up the security link keys
    vSetKeys();

    /* Initialise ZBPro stack */
    ZPS_eAplAfInit();


#if (defined DR1175) || (defined DR1173)
    if (bDeleteRecords) {
        PDM_vDeleteDataRecord(PDM_ID_APP_SCENES_DATA);
        while (APP_bButtonInitialise());
    }
#endif



    APP_ZCL_vInitialise();



#if (defined DR1175) || (defined DR1173)
    /* If required, at this point delete the network context from flash, perhaps upon some condition
     * For example, check if a button is being held down at reset, and if so request the Persistent
     * Data Manager to delete all its records:
     * e.g. bDeleteRecords = vCheckButtons();
     * Alternatively, always call PDM_vDelete() if context saving is not required.
     */
    if (bDeleteRecords) {
        DBG_vPrintf(TRACE_APP, "\nAPP: Erasing flash records\n");

        vResetDataStructures();
        /* wait fior button release */
        while ( APP_bButtonInitialise());
        DBG_vPrintf(TRACE_APP, "Key released\n");

        vAHI_SwReset();
    }
#endif

    if (sZllState.eState == 0) {
        DBG_vPrintf(TRACE_LIGHT_NODE, "\nSet a Pan 0xcafe\n");
        ZPS_vNwkNibSetPanId(ZPS_pvAplZdoGetNwkHandle(), (uint16) RND_u32GetRand(1, 0xfff0) );

    }

    if ((sZllState.eNodeState >= E_RUNNING))
    {

        sZllState.eNodeState = E_NFN_START;
        /* Start the tick timer */
        OS_eActivateTask(APP_ZPR_Light_Task);
    } else {
        DBG_vPrintf(TRACE_LIGHT_NODE, "\nFN start\n");
        sZllState.eNodeState = E_STARTUP;
        u8ChanIdx = 0;
        //OS_eActivateTask(APP_ZPR_Light_Task);
        /* Start the tick timer */
    }



#ifndef CLD_COLOUR_CONTROL
    /* Second call to bulb initialisation.  This is required by the synchronus bulb      */
    /* Driver to enable the anti rebroadcast-flicker mechanism. Ignored by other drivers */
    DriverBulb_vInit();
#endif

#if (defined PDM_EEPROM)
#ifdef TRACE_APP
    /*
     * The functions u8PDM_CalculateFileSystemCapacity and u8PDM_GetFileSystemOccupancy
     * may be called at any time to monitor space available in  the eeprom
     */
    DBG_vPrintf(TRACE_APP, "PDM: Capacity %d\n", u8PDM_CalculateFileSystemCapacity() );
    DBG_vPrintf(TRACE_APP, "PDM: Occupancy %d\n", u8PDM_GetFileSystemOccupancy() );
#endif
#endif

}

/****************************************************************************
 *
 * NAME: vPickChannel
 *
 * DESCRIPTION:
 * picks a channel for a factory new light after a classical join did not find a network
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vPickChannel(void * pvNwk) {
    uint8 i;
#if RAND_CH
    i = (uint8) RND_u32GetRand(4, 8);
#else
    i = 0;
#endif
    DBG_vPrintf(TRACE_CLASSIC, "\nPicked Ch %d\n", au8ZLLChannelSet[i]);
    ZPS_vNwkNibSetChannel( pvNwk, au8ZLLChannelSet[i]);
#if NO_CLASSIC_JOIN
    DBG_vPrintf(TRACE_CLASSIC, "Set a Random Pan\n");
    ZPS_vNwkNibSetPanId( pvNwk, (uint16) RND_u32GetRand(1, 0xfff0));
#endif
    /* Set channel mask to primary channels */
    ZPS_eAplAibSetApsChannelMask( ZLL_CHANNEL_MASK);
    DBG_vPrintf(TRACE_CLASSIC, "Set channel mask %08x\n", ZLL_CHANNEL_MASK);
    sZllState.eNodeState = E_NETWORK_INIT;
}

/****************************************************************************
 *
 * NAME: APP_ZPR_Light_Task
 *
 * DESCRIPTION:
 * Task that handles application related functions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_ZPR_Light_Task)
{
    APP_tsLightEvent sAppEvent;
    ZPS_tsAfEvent sStackEvent;
    APP_CommissionEvent sCommissionEvent;
    PDUM_thAPduInstance hAPduInst;

    sStackEvent.eType = ZPS_EVENT_NONE;
    sAppEvent.eType = APP_E_EVENT_NONE;

    if (OS_eCollectMessage(APP_msgZpsEvents, &sStackEvent) == OS_E_OK)
    {
        switch (sStackEvent.eType)
        {

        case ZPS_EVENT_APS_DATA_INDICATION:
            DBG_vPrintf(TRACE_APP, "\nData Ind: Profile :%x Cluster :%x EP:%x\n",
                    sStackEvent.uEvent.sApsDataIndEvent.u16ProfileId,
                    sStackEvent.uEvent.sApsDataIndEvent.u16ClusterId,
                    sStackEvent.uEvent.sApsDataIndEvent.u8DstEndpoint);
            break;

        case ZPS_EVENT_NWK_STATUS_INDICATION:
            DBG_vPrintf(TRACE_APP, "\nNwkStat: Addr:%x Status:%x",
                    sStackEvent.uEvent.sNwkStatusIndicationEvent.u16NwkAddr,
                    sStackEvent.uEvent.sNwkStatusIndicationEvent.u8Status);
            break;

        default:
            break;

        }
    } else if (OS_eCollectMessage(APP_msgEvents, &sAppEvent) == OS_E_OK) {

        //  DBG_vPrintf(TRACE_APP, "\nE:%s(%d)\n",
        //              apcAPPEventStrings[sAppEvent.eType],sAppEvent.eType);
    }

    if (sStackEvent.eType == ZPS_EVENT_ERROR) {
        ZPS_tsAfErrorEvent *psErrEvt = &sStackEvent.uEvent.sAfErrorEvent;
        if (psErrEvt->eError == 3)
        {
            DBG_vPrintf(TRACE_APP, "\nERROR: Queue Over Flow %p",
                    psErrEvt->uErrorData.sAfErrorOsMessageOverrun.hMessage);
        } else {
            DBG_vPrintf(TRACE_APP, "\nStack Err: %d", psErrEvt->eError);
        }
    }

//    DBG_vPrintf(TRACE_APP, "\nAPP: NodeState=%d", sZllState.eNodeState);

    /* Handle events depending on node state */
    switch (sZllState.eNodeState) {

    case E_STARTUP:
        /* factory new start up */
#if NO_CLASSIC_JOIN==FALSE
        u8ChanIdx = 0;
        vDiscoverNetworks();
        sZllState.eNodeState = E_DISCOVERY;
#else
        vPickChannel( ZPS_pvAplZdoGetNwkHandle());
#endif
        break;

    case E_NFN_START:
        /* Non factory new start up */
        hAPduInst = PDUM_hAPduAllocateAPduInstance(apduZCL);

        if (hAPduInst != NULL) {
            ZPS_eAplZdoZllStartRouter();
            ZPS_tsAplZdpDeviceAnnceReq sZdpDeviceAnnceReq;
            sZdpDeviceAnnceReq.u16NwkAddr = ZPS_u16AplZdoGetNwkAddr();
            sZdpDeviceAnnceReq.u64IeeeAddr = ZPS_u64AplZdoGetIeeeAddr();
            sZdpDeviceAnnceReq.u8Capability = ZPS_eAplZdoGetMacCapability();
            uint8 u8Seq;
            volatile int i =0;
            for (i=0;i<1000000;i++);
            ZPS_eAplZdpDeviceAnnceRequest(hAPduInst, &u8Seq,
                    &sZdpDeviceAnnceReq);

            sZllState.eNodeState = E_RUNNING;
            PDM_eSaveRecordData(PDM_ID_APP_ZLL_ROUTER,&sZllState,sizeof(tsZllState));
        }
        sZllState.eNodeState = E_RUNNING;
        break;

    case E_DISCOVERY:
        if (sStackEvent.eType == ZPS_EVENT_NWK_DISCOVERY_COMPLETE) {
            DBG_vPrintf(TRACE_CLASSIC, "Disc st %d c %d sel %d\n",
                                sStackEvent.uEvent.sNwkDiscoveryEvent.eStatus,
                                sStackEvent.uEvent.sNwkDiscoveryEvent.u8NetworkCount,
                                sStackEvent.uEvent.sNwkDiscoveryEvent.u8SelectedNetwork);
                if (((sStackEvent.uEvent.sNwkDiscoveryEvent.eStatus == 0)
                        || (sStackEvent.uEvent.sNwkDiscoveryEvent.eStatus
                                == ZPS_NWK_ENUM_NEIGHBOR_TABLE_FULL))) {
        #if TRACE_CLASSIC
                    int i;
                    for (i=0; i<sStackEvent.uEvent.sNwkDiscoveryEvent.u8NetworkCount; i++) {
                        DBG_vPrintf(TRACE_CLASSIC, "Pan %016llx Ch %d RCap %d PJoin %d Sfpl %d ZBVer %d\n", sStackEvent.uEvent.sNwkDiscoveryEvent.psNwkDescriptors[i].u64ExtPanId
                                , sStackEvent.uEvent.sNwkDiscoveryEvent.psNwkDescriptors[i].u8LogicalChan
                                , sStackEvent.uEvent.sNwkDiscoveryEvent.psNwkDescriptors[i].u8RouterCapacity
                                , sStackEvent.uEvent.sNwkDiscoveryEvent.psNwkDescriptors[i].u8PermitJoining
                                , sStackEvent.uEvent.sNwkDiscoveryEvent.psNwkDescriptors[i].u8StackProfile
                                , sStackEvent.uEvent.sNwkDiscoveryEvent.psNwkDescriptors[i].u8ZigBeeVersion);
                    }
        #endif
                u8NwkIdx = 0;
            u8NwkCount = sStackEvent.uEvent.sNwkDiscoveryEvent.u8NetworkCount;
            psNwkDescTbl = sStackEvent.uEvent.sNwkDiscoveryEvent.psNwkDescriptors;
            vTryNwkJoin();

        } else {
            DBG_vPrintf(TRACE_CLASSIC, "Fail and not full 502x\n", sStackEvent.uEvent.sNwkDiscoveryEvent.eStatus);
            vDiscoverNetworks();
        }
    }               // end of discovery complete

        if (sStackEvent.eType == ZPS_EVENT_NWK_FAILED_TO_JOIN) {
            DBG_vPrintf(TRACE_CLASSIC, "Join failed %02x\n", sStackEvent.uEvent.sNwkJoinFailedEvent.u8Status  );
            vTryNwkJoin();
        }

        if (sStackEvent.eType == ZPS_EVENT_NWK_JOINED_AS_ROUTER) {
            sZllState.eNodeState = E_RUNNING;

            /* set the aps use pan id */
            ZPS_eAplAibSetApsUseExtendedPanId( ZPS_u64NwkNibGetEpid(ZPS_pvAplZdoGetNwkHandle()) );

            sZllState.eState = NOT_FACTORY_NEW;
            sZllState.u16MyAddr = sStackEvent.uEvent.sNwkJoinedEvent.u16Addr;

            PDM_eSaveRecordData(PDM_ID_APP_ZLL_ROUTER,&sZllState,sizeof(tsZllState));
            DBG_vPrintf(TRACE_CLASSIC, "Joined as Router\n");
            /* identify to signal the join */
         //   APP_ZCL_vSetIdentifyTime( 10);
        //    APP_vHandleIdentify( 10);
        }
        break;

    case E_NETWORK_INIT:
        /* Wait here for touch link */
        if ((sStackEvent.eType == ZPS_EVENT_NWK_DISCOVERY_COMPLETE)
                || (sStackEvent.eType == ZPS_EVENT_NWK_FAILED_TO_JOIN)) {
            sCommissionEvent.eType = APP_E_COMMISSION_DISCOVERY_DONE;
            OS_ePostMessage(APP_CommissionEvents, &sCommissionEvent);
        }

        if (sStackEvent.eType == ZPS_EVENT_NWK_LEAVE_CONFIRM) {
            if (sStackEvent.uEvent.sNwkLeaveConfirmEvent.u64ExtAddr == 0) {
                /* we left let commissioning task know */
                sCommissionEvent.eType = APP_E_COMMISSION_LEAVE_CFM;
                DBG_vPrintf(TRACE_LIGHT_NODE, "Send Leave cfm\n");
                OS_ePostMessage(APP_CommissionEvents, &sCommissionEvent);
            }
        }
        break;

    case E_RUNNING:
        if (sStackEvent.eType != ZPS_EVENT_NONE) {
            DBG_vPrintf(DBG_EVENT, "Zps event in running %d\n", sStackEvent.eType);
            if ((sStackEvent.eType == ZPS_EVENT_NWK_DISCOVERY_COMPLETE)
                    || (sStackEvent.eType == ZPS_EVENT_NWK_FAILED_TO_JOIN)) {
                /* let commissioning know discovery completed */
                sCommissionEvent.eType = APP_E_COMMISSION_DISCOVERY_DONE;
                OS_ePostMessage(APP_CommissionEvents, &sCommissionEvent);
            }

            if (sStackEvent.eType == ZPS_EVENT_NWK_LEAVE_CONFIRM) {
                if (sStackEvent.uEvent.sNwkLeaveConfirmEvent.u64ExtAddr == 0) {
                    /* we left let commissioning task know */
                    sCommissionEvent.eType = APP_E_COMMISSION_LEAVE_CFM;
                    //DBG_vPrintf(TRACE_LIGHT_NODE, "Send Leave cfm\n");
                    OS_ePostMessage(APP_CommissionEvents, &sCommissionEvent);
                }
            }


            if (sStackEvent.eType == ZPS_EVENT_NWK_NEW_NODE_HAS_JOINED) {
                DBG_vPrintf(TRACE_LIGHT_NODE, "\nNode joined %04x", sStackEvent.uEvent.sNwkJoinIndicationEvent.u16NwkAddr);
            }
#if 1
            /*Request response event, call the child age logic */
			if(ZPS_EVENT_APS_DATA_INDICATION==sStackEvent.eType)
			{
			    DBG_vPrintf(TRACE_APP, "\nData Ind: Profile :%x Cluster :%x EP:%x\n",
			                        sStackEvent.uEvent.sApsDataIndEvent.u16ProfileId,
			                        sStackEvent.uEvent.sApsDataIndEvent.u16ClusterId,
			                        sStackEvent.uEvent.sApsDataIndEvent.u8DstEndpoint);
#ifdef CLD_OTA


				if ((sStackEvent.uEvent.sApsDataIndEvent.eStatus == ZPS_E_SUCCESS) &&
				        (sStackEvent.uEvent.sApsDataIndEvent.u8DstEndpoint == 0))
				{
				    // Data Ind for ZDp Ep
				    if (ZPS_ZDP_MATCH_DESC_RSP_CLUSTER_ID == sStackEvent.uEvent.sApsDataIndEvent.u16ClusterId) {
				        vHandleMatchDescriptor(&sStackEvent);
				    } else if (ZPS_ZDP_IEEE_ADDR_RSP_CLUSTER_ID == sStackEvent.uEvent.sApsDataIndEvent.u16ClusterId) {
				        vHandleIeeeAddressRsp(&sStackEvent);
				    }
				}
#endif
				// let the buffer go
				PDUM_eAPduFreeAPduInstance(sStackEvent.uEvent.sApsDataIndEvent.hAPduInst);
			}
#endif

        }

        if (sAppEvent.eType == APP_E_EVENT_BUTTON_DOWN)
        {
            DBG_vPrintf(TRACE_LIGHT_NODE, "Button down %d\n", sAppEvent.uEvent.sButton.u8Button);
            vOpenPermitJoin();
        }
        break;

    }


    /*
     * Global clean up to make sure any APDUs have been freed
     */
    if (sStackEvent.eType == ZPS_EVENT_APS_DATA_INDICATION) {
        PDUM_eAPduFreeAPduInstance( sStackEvent.uEvent.sApsDataIndEvent.hAPduInst);
    }
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vOpenPermitJoin
 *
 * DESCRIPTION:
 * Toggle the PermitJoin between 0 and 254 seconds
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vOpenPermitJoin()
{
    ZPS_eAplZdoPermitJoining(180);
    DBG_vPrintf(TRACE_LIGHT_NODE, "Permit join 120 sec\n");
}

/****************************************************************************
 *
 * NAME: vResetDataStructures
 *
 * DESCRIPTION:
 * Resets persisted data structures to factory new state
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vResetDataStructures(void) {

    void * pvNwk = ZPS_pvAplZdoGetNwkHandle();
    ZPS_tsNwkNib *psNib = ZPS_psNwkNibGetHandle( pvNwk);
    int i;

    u32OldFrameCtr = psNib->sTbl.u32OutFC;
    ZPS_vNwkNibClearTables(pvNwk);
    (*(ZPS_tsNwkNibInitialValues *)psNib) = *(psNib->sTbl.psNibDefault);

   /* Clear Security Material Set - will set all 64bit addresses to ZPS_NWK_NULL_EXT_ADDR */
    zps_vNwkSecClearMatSet(pvNwk);

    /* Set default values in Material Set Table */
    for (i = 0; i < psNib->sTblSize.u8SecMatSet; i++)
    {
        psNib->sTbl.psSecMatSet[i].u8KeyType = ZPS_NWK_SEC_KEY_INVALID_KEY;
    }
    psNib->sPersist.u64ExtPanId = ZPS_NWK_NULL_EXT_PAN_ID; /* needs a default value? */
    psNib->sPersist.u16NwkAddr = 0xffff;
    psNib->sPersist.u8ActiveKeySeqNumber = 0xff;

    psNib->sTbl.u32OutFC = u32OldFrameCtr;

    vSetKeys();

    vRemoveAllGroupsAndScenes();
    /* set the aps use pan id */
    ZPS_eAplAibSetApsUseExtendedPanId( 0 );
    ZPS_eAplAibSetApsTrustCenterAddress( 0 );

    sZllState.eNodeState = E_STARTUP;
    sZllState.eState = FACTORY_NEW;
    sZllState.u16MyAddr = 0;
#ifdef CLD_OTA
    sZllState.bValid = FALSE;
    sZllState.u64IeeeAddrOfServer = 0;
    sZllState.u16NwkAddrOfServer = 0xffff;
    sZllState.u8OTAserverEP = 0xff;
    vOTAResetPersist();
#endif

    PDM_eSaveRecordData(PDM_ID_APP_ZLL_ROUTER,&sZllState,sizeof(tsZllState));
    ZPS_vSaveAllZpsRecords();
}

/****************************************************************************
 *
 * NAME: vSetKeys
 *
 * DESCRIPTION:
 * Initialise the ZHA and ZLL link keys
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vSetKeys(void) {

    ZPS_vAplSecSetInitialSecurityState(ZPS_ZDO_PRECONFIGURED_LINK_KEY, (uint8 *)&s_au8LnkKeyArray, 0x00, ZPS_APS_GLOBAL_LINK_KEY);
    ZPS_vAplSecSetInitialSecurityState(ZPS_ZDO_ZLL_LINK_KEY, (uint8 *)&s_au8ZllLnkKeyArray, 0x00, ZPS_APS_GLOBAL_LINK_KEY);
    DBG_vPrintf(TRACE_LIGHT_NODE, "Set Sec state\n");
}


/****************************************************************************
 *
 * NAME: sGetOTACallBackPersistdata
 *
 * DESCRIPTION:
 * returns persisted data
 *
 * RETURNS:
 * tsOTA_PersistedData
 *
 ****************************************************************************/
#ifdef CLD_OTA
PUBLIC tsOTA_PersistedData sGetOTACallBackPersistdata(void)
{
	return sLight.sCLD_OTA_CustomDataStruct.sOTACallBackMessage.sPersistedData;
}
#endif

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
