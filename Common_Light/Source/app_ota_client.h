/*****************************************************************************
 *
 * MODULE:             JN-AN-1189
 *
 * COMPONENT:          app_ota_client.h
 *
 * DESCRIPTION:        DK4 (DR1175/DR1199) App OTA client (Implementation)
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

#ifndef APP_OTA_CLIENT_H
#define APP_OTA_CLIENT_H

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef OTA_DEMO_TIMINGS
#define OTA_SERVER_QUERY_TIME_IN_SEC    ( 60*20  /*60*60*24*/)      /*To find the server*/
#define OTA_IMAGE_QUERY_TIME_IN_SEC     ( 60*20  /*60*60*24*/)      /*To query new image available*/
#define OTA_INITIAL_TIME_IN_SEC		RND_u32GetRand(1, 1150 /*60*60*/);    /* spread out 2 to 60 minutes */
#define OTA_IEEE_LOOKUP_TIME_IN_SEC     5           /* how longto delay the ieee look up after the match response */
#define OTA_IMAGE_QUERY_TIMEOUT_IN_SEC  15              /* To wait for the image response */
#define OTA_DISCOVERY_TIMEOUT_IN_SEC    15              /* To wait for the server discovery */
#define OTA_DL_IN_PROGRESS_TIME_IN_SEC  (15*60)             /* Time out period without block notifications */
#else
#define OTA_SERVER_QUERY_TIME_IN_SEC     60         /*To find the server*/
#define OTA_IMAGE_QUERY_TIME_IN_SEC     60         /*To query new image available*/
#define OTA_INITIAL_TIME_IN_SEC		RND_u32GetRand(0, 44);    /* spread out 10 to 59 seconds */
#define OTA_IEEE_LOOKUP_TIME_IN_SEC     5           /* how longto delay the ieee look up after the match response */
#define OTA_IMAGE_QUERY_TIMEOUT_IN_SEC  15         /* To wait for the image response */
#define OTA_DISCOVERY_TIMEOUT_IN_SEC    15         /* To wait for the server discovery */
#define OTA_DL_IN_PROGRESS_TIME_IN_SEC  (15*60)        /* Time out period without block notifications */
#endif

#define MAX_SERVER_EPs 2
#define MAX_SERVER_NODES 2
#define IGNORE_COORDINATOR_AS_OTA_SERVER
#define APP_OTA_OFFSET_WRITEN_BEFORE_LINKKEY_VERIFICATION 1100
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef struct {
	bool bValid;
    uint64 u64IeeeAddrOfServer;
    uint16 u16NwkAddrOfServer;
    uint8 u8MatchLength;
    uint8 u8MatchList[MAX_SERVER_EPs];
}tsDiscovedOTAServers;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void vAppInitOTA(void);
PUBLIC void vRunAppOTAStateMachine(void);
PUBLIC void vHandleMatchDescriptor( ZPS_tsAfEvent  * psStackEvent);
PUBLIC void vHandleIeeeAddressRsp( ZPS_tsAfEvent  * psStackEvent);
PUBLIC void vHandleAppOtaClient(tsOTA_CallBackMessage *psCallBackMessage);
PUBLIC void vLoadOTAPersistedData(void);
PUBLIC void vOTAResetPersist(void);

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/
PUBLIC	uint8 s_au8LnkKeyArray[16];
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#endif /*APP_OTA_CLIENT_H*/
