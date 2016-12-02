/*****************************************************************************
 *
 * MODULE:             JN-AN-1171
 *
 * COMPONENT:          app_light_commission_task.c
 *
 * DESCRIPTION:        ZLL Demo: Commisioning Process -Implementation
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
#include <appapi.h>
#include "os.h"
#include "os_gen.h"
#include "pdum_apl.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "dbg.h"
#include "pwrm.h"
#include "zps_gen.h"
#include "zps_apl_af.h"
#include "zps_apl_zdo.h"
#include "zps_apl_aib.h"
#include "zps_apl_zdp.h"
#include "zps_nwk_nib.h"



#include "app_common.h"
#include "app_timer_driver.h"

#include "zpr_light_node.h"

#include "app_zcl_light_task.h"
#include "app_events.h"
#include "zcl_customcommand.h"

#include "zps_nwk_nib.h"

#include <string.h>
#include <rnd_pub.h>
#include <stdlib.h>

#include "PDM_IDs.h"
#include "app_scenes.h"

#define ADJUST_POWER        TRUE
#define ZLL_SCAN_LQI_MIN    (100)

#ifndef DEBUG_JOIN
#define TRACE_JOIN            FALSE
#else
#define TRACE_JOIN            TRUE
#endif

#ifndef DEBUG_COMMISSION
#define TRACE_COMMISSION      FALSE
#else
#define TRACE_COMMISSION      TRUE
#endif

#define PERMIT_JOIN       FALSE
#define PERMIT_JOIN_TIME  255

#if (defined DR1173) || (defined DR1175)
#define RSSI_CORRECTION     0x0d       // value for dk4 boards
#else
#define RSSI_CORRECTION     0x20       // max allowed value, for SSL bulbs
#endif

typedef enum {
    E_IDLE,
    E_ACTIVE,
    E_WAIT_DISCOVERY,
    E_SKIP_DISCOVERY,
    E_WAIT_LEAVE,
    E_WAIT_LEAVE_RESET,
    E_START_ROUTER
} eState;

typedef struct {
    ZPS_tsInterPanAddress sSrcAddr;
    tsCLD_ZllCommission_ScanRspCommandPayload sScanRspPayload;
} tsZllScanTable;

typedef struct {
    uint32 u8Count;
    tsZllScanTable sScanTable[2];
} tsZllScanResults;


typedef struct {
    uint64 u64ExtPanId;
    uint8 u8KeyIndex;
    uint8 au8NwkKey[16];
    uint8 u8LogicalChannel;
    uint16 u16PanId;
    uint16 u16NwkAddr;
    uint16 u16GroupIdBegin;
    uint16 u16GroupIdEnd;
    uint16 u16FreeNwkAddrBegin;
    uint16 u16FreeNwkAddrEnd;
    uint16 u16FreeGroupIdBegin;
    uint16 u16FreeGroupIdEnd;
    uint64 u64InitiatorIEEEAddr;
    uint16 u16InitiatorNwkAddr;
    uint8 u8NwkupdateId;
} tsStartParams;

tsStartParams sStartParams;

ZPS_tsInterPanAddress sDstAddr;
//PRIVATE tsReg128
//        sMasterKey = { 0x11223344, 0x55667788, 0x99aabbcc, 0xddeeff00 };

PRIVATE tsReg128
        sMasterKey = { 0x9F5595F1, 0x0257C8A4, 0x69CBF42B, 0xC93FEE31 };

PRIVATE tsReg128 sCertKey = { 0xc0c1c2c3, 0xc4c5c6c7, 0xc8c9cacb, 0xcccdcecf };

tsZllState sZllState = { FACTORY_NEW, E_STARTUP, ZLL_SKIP_CH1 };

PDM_tsRecordDescriptor sZllPDDesc;

extern tsCLD_ZllDeviceTable sDeviceTable;

PRIVATE uint8 eDecryptKey(uint8* au8InData, uint8* au8OutData,
        uint32 u32TransId, uint32 u32ResponseId, uint8 u8KeyIndex);
PRIVATE bool
        bSearchDiscNt(ZPS_tsNwkNib *psNib, uint64 u64EpId, uint16 u16PanId);
PRIVATE uint8 u8NewUpdateID(uint8 u8ID1, uint8 u8ID2);

PRIVATE teZCL_Status eSendScanResponse(ZPS_tsNwkNib *psNib,
                               ZPS_tsInterPanAddress       *psDstAddr,
                               uint32 u32TransactionId,
                               uint32 u32ResponseId);

/****************************************************************************
 *
 * NAME: APP_Commission_Task
 *
 * DESCRIPTION:
 * Task that handles touch link related events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_Commission_Task) {
    static eState eState = E_IDLE;
    static uint32 u32TransactionId;
    static uint32 u32ResponseId;
    static uint8 u8Flags;

    APP_CommissionEvent sEvent;
    tsZllPayloads sZllCommand;

    uint8 u8Seq;
    ZPS_tsNwkNib *psNib;

    if (OS_eCollectMessage(APP_CommissionEvents, &sEvent) != OS_E_OK)
    {
        //DBG_vPrintf(TRACE_COMMISSION, "\nError\n");
    }

    psNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());

    //DBG_vPrintf(TRACE_COMMISSION, "\nInterPan LQI %d\n", sEvent.u8Lqi);

    switch (eState)
    {
        case E_IDLE:
            if (sEvent.eType == APP_E_COMMISSION_MSG)
            {
                if (sEvent.sZllMessage.eCommand == E_CLD_COMMISSION_CMD_SCAN_REQ)
                {
                    if (sEvent.u8Lqi > ZLL_SCAN_LQI_MIN)
                    {
                        DBG_vPrintf(TRACE_COMMISSION, "\nScan Req LQI %d\n", sEvent.u8Lqi);
                        /* Turn down Tx power */
#if ADJUST_POWER
                        //phy_ePibSet(ZPS_pvAplZdoGetMacHandle(), PHY_PIB_ATTR_TX_POWER, TX_POWER_LOW);
                        eAppApiPlmeSet(PHY_PIB_ATTR_TX_POWER, TX_POWER_LOW);
#endif
                        u8Flags = 0;
                        if ((sEvent.sZllMessage.uPayload.sScanReqPayload.u8ZigbeeInfo & ZLL_TYPE_MASK) != ZLL_ZED)
                        {
                            // Not a ZED requester, set FFD bit in flags
                            u8Flags |= 0x02;
                        }
                        if (sEvent.sZllMessage.uPayload.sScanReqPayload.u8ZigbeeInfo
                                & ZLL_RXON_IDLE)
                        {
                            // RxOnWhenIdle, so set RXON and power source bits in the flags
                            u8Flags |= 0x0c;
                        }

                        sDstAddr = sEvent.sZllMessage.sSrcAddr;
                        DBG_vPrintf(TRACE_JOIN, "Back to %016llx Mode %d\n", sDstAddr.uAddress.u64Addr, sDstAddr.eMode);
                        sDstAddr.u16PanId = 0xffff;
                        u32TransactionId = sEvent.sZllMessage.uPayload.sScanReqPayload.u32TransactionId;
                        u32ResponseId = RND_u32GetRand(1, 0xffffffff);
                        if ( 0 == eSendScanResponse( psNib, &sDstAddr, u32TransactionId, u32ResponseId)) {
                            eState = E_ACTIVE;
                            /* Timer to end inter pan */
                            OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(ZLL_INTERPAN_LIFE_TIME_SEC), NULL);
                        }
                    }
                    else
                    {
                        /* LQI too low */
                        DBG_vPrintf(TRACE_COMMISSION, "\nDrop Scan LQI %d\n", sEvent.u8Lqi);
                    }
                } // end scan req
            }
            break;

        case E_ACTIVE:
            switch (sEvent.eType)
            {
                case APP_E_COMMISSION_TIMER_EXPIRED:
                    DBG_vPrintf(TRACE_COMMISSION, "Inter Pan Timed Out\n");
                    eState = E_IDLE;
                    u32TransactionId = 0;
                    u32ResponseId = 0;
                    /* Turn up Tx power */
#if ADJUST_POWER
                    //phy_ePibSet(ZPS_pvAplZdoGetMacHandle(), PHY_PIB_ATTR_TX_POWER, TX_POWER_NORMAL);
                    eAppApiPlmeSet(PHY_PIB_ATTR_TX_POWER, TX_POWER_NORMAL);
#endif
                    break;

                case APP_E_COMMISSION_MSG:
                    sDstAddr = sEvent.sZllMessage.sSrcAddr;
                    sDstAddr.u16PanId = 0xFFFF;
                    DBG_vPrintf(TRACE_JOIN, "IP cmd 0x%02x\n", sEvent.sZllMessage.eCommand);
                    if (sEvent.sZllMessage.uPayload.sScanReqPayload.u32TransactionId == u32TransactionId)
                    {
                        switch (sEvent.sZllMessage.eCommand)
                        {

                            /* if we receive the factory reset clear the persisted data and do a reset.
                             * Spoke to Philips and they say " Yup anyone can reset and bring nodes to factory settings"
                             */
                            case E_CLD_COMMISSION_CMD_FACTORY_RESET_REQ:
                                if (sZllState.eState == NOT_FACTORY_NEW)
                                {
                                    eState = E_WAIT_LEAVE_RESET;
                                    /* leave req */
                                    u32OldFrameCtr = psNib->sTbl.u32OutFC + 10;
                                    ZPS_eAplZdoLeaveNetwork(0, FALSE, FALSE);
                                }
                                break;

                            case E_CLD_COMMISSION_CMD_NETWORK_UPDATE_REQ:
                                DBG_vPrintf(TRACE_COMMISSION, "Got nwk up req\n");
                                if ((sEvent.sZllMessage.uPayload.sNwkUpdateReqPayload.u64ExtPanId == psNib->sPersist.u64ExtPanId)
                                        && (sEvent.sZllMessage.uPayload.sNwkUpdateReqPayload.u16PanId == psNib->sPersist.u16VsPanId)
                                        && (psNib->sPersist.u8UpdateId != u8NewUpdateID( psNib->sPersist.u8UpdateId,
                                                                                         sEvent.sZllMessage.uPayload.sNwkUpdateReqPayload.u8NwkUpdateId)))
                                {
                                    /* Update the UpdateId, Nwk addr and Channel */
                                    void * pvNwk = ZPS_pvAplZdoGetNwkHandle();
                                    ZPS_vNwkNibSetUpdateId( pvNwk,
                                                            sEvent.sZllMessage.uPayload.sNwkUpdateReqPayload.u8NwkUpdateId);
                                        ZPS_vNwkNibSetNwkAddr( pvNwk,
                                                               sEvent.sZllMessage.uPayload.sNwkUpdateReqPayload.u16NwkAddr);
                                    ZPS_vNwkNibSetChannel( pvNwk,
                                                           sEvent.sZllMessage.uPayload.sNwkUpdateReqPayload.u8LogicalChannel);
                                    DBG_vPrintf(TRACE_COMMISSION, "Nwk update to %d\n", sEvent.sZllMessage.uPayload.sNwkUpdateReqPayload.u8LogicalChannel);
                                }
                                break;

                            case E_CLD_COMMISSION_CMD_IDENTIFY_REQ:
                                if (sEvent.sZllMessage.uPayload.sIdentifyReqPayload.u16Duration == 0xFFFF)
                                {
                                    DBG_vPrintf(TRACE_COMMISSION, "Default Id time\n");
                                    sEvent.sZllMessage.uPayload.sIdentifyReqPayload.u16Duration = 3;
                                }

                                /** Identfiy time goes at 1Hz **/
                                APP_ZCL_vSetIdentifyTime( sEvent.sZllMessage.uPayload.sIdentifyReqPayload.u16Duration);
                                APP_vHandleIdentify(  sEvent.sZllMessage.uPayload.sIdentifyReqPayload.u16Duration);
                                break;

                            case E_CLD_COMMISSION_CMD_DEVICE_INFO_REQ:
                                DBG_vPrintf(TRACE_JOIN, "Device Info request\n");
                                memset( &sZllCommand.uPayload.sDeviceInfoRspPayload,
                                        0,
                                        sizeof(tsCLD_ZllCommission_DeviceInfoRspCommandPayload));
                                sZllCommand.uPayload.sDeviceInfoRspPayload.u32TransactionId = sEvent.sZllMessage.uPayload.sDeviceInfoReqPayload.u32TransactionId;
                                sZllCommand.uPayload.sDeviceInfoRspPayload.u8NumberSubDevices = ZLL_NUMBER_DEVICES;
                                sZllCommand.uPayload.sDeviceInfoRspPayload.u8StartIndex = sEvent.sZllMessage.uPayload.sDeviceInfoReqPayload.u8StartIndex;
                                sZllCommand.uPayload.sDeviceInfoRspPayload.u8DeviceInfoRecordCount = 0;
                                // copy from table sDeviceTable


                                int i,j;
                                j = sEvent.sZllMessage.uPayload.sDeviceInfoReqPayload.u8StartIndex;
                                for (i = 0; i < 16 && j < ZLL_NUMBER_DEVICES; i++, j++)
                                {
                                    sZllCommand.uPayload.sDeviceInfoRspPayload.asDeviceRecords[i] = sDeviceTable.asDeviceRecords[j];
                                    sZllCommand.uPayload.sDeviceInfoRspPayload.u8DeviceInfoRecordCount++;
                                }

                                eCLD_ZllCommissionCommandDeviceInfoRspCommandSend( &sDstAddr,
                                                                                   &u8Seq,
                                                                                   (tsCLD_ZllCommission_DeviceInfoRspCommandPayload*) &sZllCommand.uPayload);

                                break;

                            case E_CLD_COMMISSION_CMD_NETWORK_JOIN_END_DEVICE_REQ:
                                /* we are a ZR send error */
                                memset(&sZllCommand.uPayload.sNwkJoinEndDeviceRspPayload,
                                        0,
                                        sizeof(tsCLD_ZllCommission_NetworkJoinEndDeviceRspCommandPayload));
                                sZllCommand.uPayload.sNwkJoinEndDeviceRspPayload.u32TransactionId = u32TransactionId;
                                sZllCommand.uPayload.sNwkJoinEndDeviceRspPayload.u8Status = ZLL_ERROR;

                                eCLD_ZllCommissionCommandNetworkJoinEndDeviceRspCommandSend( &sDstAddr,
                                                                                             &u8Seq,
                                                                                             (tsCLD_ZllCommission_NetworkJoinEndDeviceRspCommandPayload*) &sZllCommand.uPayload);

                                break;

                            case E_CLD_COMMISSION_CMD_NETWORK_START_REQ:
                                DBG_vPrintf(TRACE_COMMISSION, "start request\n");

                                sStartParams.u64ExtPanId = sEvent.sZllMessage.uPayload.sNwkStartReqPayload.u64ExtPanId;
                                sStartParams.u8KeyIndex = sEvent.sZllMessage.uPayload.sNwkStartReqPayload.u8KeyIndex;
                                sStartParams.u8LogicalChannel = sEvent.sZllMessage.uPayload.sNwkStartReqPayload.u8LogicalChannel;
                                sStartParams.u16PanId = sEvent.sZllMessage.uPayload.sNwkStartReqPayload.u16PanId;
                                sStartParams.u16NwkAddr  = sEvent.sZllMessage.uPayload.sNwkStartReqPayload.u16NwkAddr;
                                sStartParams.u16GroupIdBegin = sEvent.sZllMessage.uPayload.sNwkStartReqPayload.u16GroupIdBegin;
                                sStartParams.u16GroupIdEnd = sEvent.sZllMessage.uPayload.sNwkStartReqPayload.u16GroupIdEnd;
                                sStartParams.u16FreeGroupIdBegin = sEvent.sZllMessage.uPayload.sNwkStartReqPayload.u16FreeGroupIdBegin;
                                sStartParams.u16FreeGroupIdEnd = sEvent.sZllMessage.uPayload.sNwkStartReqPayload.u16FreeGroupIdEnd;
                                sStartParams.u16FreeNwkAddrBegin = sEvent.sZllMessage.uPayload.sNwkStartReqPayload.u16FreeNwkAddrBegin;
                                sStartParams.u16FreeNwkAddrEnd  = sEvent.sZllMessage.uPayload.sNwkStartReqPayload.u16FreeNwkAddrEnd;
                                sStartParams.u64InitiatorIEEEAddr = sEvent.sZllMessage.uPayload.sNwkStartReqPayload.u64InitiatorIEEEAddr;
                                sStartParams.u16InitiatorNwkAddr = sEvent.sZllMessage.uPayload.sNwkStartReqPayload.u16InitiatorNwkAddr;
                                sStartParams.u8NwkupdateId = 0;
                                memcpy( sStartParams.au8NwkKey,
                                        sEvent.sZllMessage.uPayload.sNwkStartReqPayload.au8NwkKey,
                                        16);

                                /* Turn up Tx power */
#if ADJUST_POWER
                                //phy_ePibSet(ZPS_pvAplZdoGetMacHandle(), PHY_PIB_ATTR_TX_POWER, TX_POWER_NORMAL);
                                eAppApiPlmeSet(PHY_PIB_ATTR_TX_POWER, TX_POWER_NORMAL);
#endif

                                if ((sStartParams.u64ExtPanId == 0) || (sStartParams.u16PanId == 0))
                                {
                                    if (sStartParams.u64ExtPanId == 0)
                                    {
                                        sStartParams.u64ExtPanId = RND_u32GetRand(1, 0xffffffff);
                                        sStartParams.u64ExtPanId <<= 32;
                                        sStartParams.u64ExtPanId |= RND_u32GetRand(0, 0xffffffff);
                                        DBG_vPrintf(TRACE_COMMISSION, "Gen Epid\n");
                                    }
                                    if (sStartParams.u16PanId == 0)
                                    {
                                        sStartParams.u16PanId = RND_u32GetRand( 1, 0xfffe);
                                        DBG_vPrintf(TRACE_COMMISSION, "Gen pan\n");
                                    }
                                    DBG_vPrintf(TRACE_COMMISSION, "Do discovery\n");
                                    ZPS_eAplZdoDiscoverNetworks( ZLL_CHANNEL_MASK);
                                    eState = E_WAIT_DISCOVERY;
                                }
                                else
                                {
                                    eState = E_SKIP_DISCOVERY;
                                    DBG_vPrintf(TRACE_COMMISSION, "Skip discovery\n");
                                    OS_eStopSWTimer(APP_CommissionTimer);
                                    OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
                                }
                                break;

                            case E_CLD_COMMISSION_CMD_NETWORK_JOIN_ROUTER_REQ:
                                DBG_vPrintf(TRACE_JOIN, "Active join router req\n");
                                sZllCommand.uPayload.sNwkJoinRouterRspPayload.u32TransactionId = u32TransactionId;
                                sZllCommand.uPayload.sNwkJoinRouterRspPayload.u8Status  = ZLL_SUCCESS;
                                if ( (sEvent.sZllMessage.uPayload.sNwkJoinRouterReqPayload.u64ExtPanId == 0) ||
                                     (sEvent.sZllMessage.uPayload.sNwkJoinRouterReqPayload.u64ExtPanId == 0xffffffffff) ||
                                     (sEvent.sZllMessage.uPayload.sNwkJoinRouterReqPayload.u8LogicalChannel < 11) ||
                                     (sEvent.sZllMessage.uPayload.sNwkJoinRouterReqPayload.u8LogicalChannel > 26) )
                                {
                                    DBG_vPrintf( TRACE_COMMISSION, "Invalid start params reject\n");
                                    sZllCommand.uPayload.sNwkJoinRouterRspPayload.u8Status  = ZLL_ERROR;
                                    eCLD_ZllCommissionCommandNetworkJoinRouterRspCommandSend( &sDstAddr,
                                                                                              &u8Seq,
                                                                                              (tsCLD_ZllCommission_NetworkJoinRouterRspCommandPayload*) &sZllCommand.uPayload);
                                }
                                else
                                {

                                    /* Turn up Tx power */
#if ADJUST_POWER
                                    //phy_ePibSet(ZPS_pvAplZdoGetMacHandle(), PHY_PIB_ATTR_TX_POWER, TX_POWER_NORMAL);
                                    eAppApiPlmeSet(PHY_PIB_ATTR_TX_POWER, TX_POWER_NORMAL);
#endif

                                    DBG_vPrintf( TRACE_COMMISSION, "Set channel %d\n", sEvent.sZllMessage.uPayload.sNwkJoinRouterReqPayload.u8LogicalChannel);

                                    eCLD_ZllCommissionCommandNetworkJoinRouterRspCommandSend( &sDstAddr,
                                            &u8Seq,
                                            (tsCLD_ZllCommission_NetworkJoinRouterRspCommandPayload*) &sZllCommand.uPayload);

                                    sStartParams.u64ExtPanId  = sEvent.sZllMessage.uPayload.sNwkJoinRouterReqPayload.u64ExtPanId;
                                    sStartParams.u8KeyIndex = sEvent.sZllMessage.uPayload.sNwkJoinRouterReqPayload.u8KeyIndex;
                                    sStartParams.u8LogicalChannel = sEvent.sZllMessage.uPayload.sNwkJoinRouterReqPayload.u8LogicalChannel;
                                    sStartParams.u16PanId  = sEvent.sZllMessage.uPayload.sNwkJoinRouterReqPayload.u16PanId;
                                    sStartParams.u16NwkAddr = sEvent.sZllMessage.uPayload.sNwkJoinRouterReqPayload.u16NwkAddr;
                                    sStartParams.u16GroupIdBegin = sEvent.sZllMessage.uPayload.sNwkJoinRouterReqPayload.u16GroupIdBegin;
                                    sStartParams.u16GroupIdEnd = sEvent.sZllMessage.uPayload.sNwkJoinRouterReqPayload.u16GroupIdEnd;
                                    sStartParams.u16FreeGroupIdBegin = sEvent.sZllMessage.uPayload.sNwkJoinRouterReqPayload.u16FreeGroupIdBegin;
                                    sStartParams.u16FreeGroupIdEnd = sEvent.sZllMessage.uPayload.sNwkJoinRouterReqPayload.u16FreeGroupIdEnd;
                                    sStartParams.u16FreeNwkAddrBegin = sEvent.sZllMessage.uPayload.sNwkJoinRouterReqPayload.u16FreeNwkAddrBegin;
                                    sStartParams.u16FreeNwkAddrEnd = sEvent.sZllMessage.uPayload.sNwkJoinRouterReqPayload.u16FreeNwkAddrEnd;
                                    sStartParams.u64InitiatorIEEEAddr = 0;
                                    sStartParams.u16InitiatorNwkAddr = 0;
                                    sStartParams.u8NwkupdateId = sEvent.sZllMessage.uPayload.sNwkJoinRouterReqPayload.u8NwkUpdateId;
                                    memcpy( sStartParams.au8NwkKey,
                                            sEvent.sZllMessage.uPayload.sNwkJoinRouterReqPayload.au8NwkKey,
                                            16);

                                    if (sZllState.eState == FACTORY_NEW)
                                    {
                                        eState = E_START_ROUTER;
                                        //OS_eActivateTask(APP_Commission_Task);
                                    }
                                    else
                                    {
                                        eState = E_WAIT_LEAVE;
                                        /* leave req */
                                        /* save out FC to restore after the leave */
                                        u32OldFrameCtr = psNib->sTbl.u32OutFC + 10;
                                        ZPS_eAplZdoLeaveNetwork(0, FALSE, FALSE);
                                    }
                                    OS_eStopSWTimer(APP_CommissionTimer);
                                    OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
                                }
                                break;
                            default:
                                DBG_vPrintf(TRACE_JOIN, "Active unhandled Cmd %02x\n", sEvent.sZllMessage.eCommand);
                                break;

                        }
                    } else {
                        /*
                         * Mis match in transaction id
                         */
                        if (sEvent.sZllMessage.eCommand == E_CLD_COMMISSION_CMD_SCAN_REQ) {
                            /*
                             * New scan request
                             */
                            if (sEvent.u8Lqi > ZLL_SCAN_LQI_MIN) {
                                u8Flags = 0;
                                if ((sEvent.sZllMessage.uPayload.sScanReqPayload.u8ZigbeeInfo & ZLL_TYPE_MASK) != ZLL_ZED)
                                {
                                    // Not a ZED requester, set FFD bit in flags
                                    u8Flags |= 0x02;
                                }
                                if (sEvent.sZllMessage.uPayload.sScanReqPayload.u8ZigbeeInfo
                                        & ZLL_RXON_IDLE)
                                {
                                    // RxOnWhenIdle, so set RXON and power source bits in the flags
                                    u8Flags |= 0x0c;
                                }
                                sDstAddr = sEvent.sZllMessage.sSrcAddr;
                                DBG_vPrintf(TRACE_JOIN, "New scan Back to %016llx Mode %d\n", sDstAddr.uAddress.u64Addr, sDstAddr.eMode);
                                sDstAddr.u16PanId = 0xffff;
                                u32TransactionId = sEvent.sZllMessage.uPayload.sScanReqPayload.u32TransactionId;
                                u32ResponseId = RND_u32GetRand(1, 0xffffffff);
                                if ( 0 == eSendScanResponse( psNib, &sDstAddr, u32TransactionId, u32ResponseId)) {
                                    /* Timer to end inter pan */
                                    OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(ZLL_INTERPAN_LIFE_TIME_SEC), NULL);
                                }
                            }
                        }
                    }
                    break;

                default:
                    break;
            }
            break;

        case E_WAIT_DISCOVERY:
            if (sEvent.eType == APP_E_COMMISSION_DISCOVERY_DONE)
            {
                DBG_vPrintf(TRACE_COMMISSION, "discovery in commissioning\n");
                /* get unique set of pans */
                while (!bSearchDiscNt(psNib, sStartParams.u64ExtPanId,
                        sStartParams.u16PanId))
                {
                    sStartParams.u16PanId = RND_u32GetRand(1, 0xfffe);
                    sStartParams.u64ExtPanId = RND_u32GetRand(1, 0xffffffff);
                    sStartParams.u64ExtPanId <<= 32;
                    sStartParams.u64ExtPanId |= RND_u32GetRand(0, 0xffffffff);
                };
                //DBG_vPrintf(TRACE_JOIN, "New Epid %016llx Pan %04x\n", sStartParams.u64ExtPanId, sStartParams.u16PanId);
            }
            // Deliberate fall through

        case E_SKIP_DISCOVERY:
            DBG_vPrintf(TRACE_JOIN, "New Epid %016llx Pan %04x\n",
                    sStartParams.u64ExtPanId, sStartParams.u16PanId);
            DBG_vPrintf(TRACE_COMMISSION, "e_skip-discovery\n");
            if (sStartParams.u8LogicalChannel == 0)
            {
                // pick random
                int n;

                /* Get randon channel from set of channels
                 * 4 <= n < 8
                 * will give a channel from index 4 5 6 or 7 of the array
                 */
#if RAND_CH
                n = (uint8)RND_u32GetRand( 4, 8);
#else
                n = 0;
#endif
                DBG_vPrintf(TRACE_COMMISSION, "Picked Ch %d\n", au8ZLLChannelSet[n]);

                sStartParams.u8LogicalChannel = au8ZLLChannelSet[n];
            }

            /* send start rsp */
            sZllCommand.uPayload.sNwkStartRspPayload.u32TransactionId = u32TransactionId;
            sZllCommand.uPayload.sNwkStartRspPayload.u8Status = ZLL_SUCCESS;
            sZllCommand.uPayload.sNwkStartRspPayload.u64ExtPanId = sStartParams.u64ExtPanId;
            sZllCommand.uPayload.sNwkStartRspPayload.u8NwkUpdateId = 0;
            sZllCommand.uPayload.sNwkStartRspPayload.u8LogicalChannel = sStartParams.u8LogicalChannel;
            sZllCommand.uPayload.sNwkStartRspPayload.u16PanId = sStartParams.u16PanId;

            eCLD_ZllCommissionCommandNetworkStartRspCommandSend( &sDstAddr,
                                                                 &u8Seq,
                                                                 (tsCLD_ZllCommission_NetworkStartRspCommandPayload*) &sZllCommand.uPayload);

            if (sZllState.eState == FACTORY_NEW)
            {
                eState = E_START_ROUTER;
                /* give message time to go */
                OS_eStopSWTimer(APP_CommissionTimer);
                OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
            }
            else
            {
                eState = E_WAIT_LEAVE;

                DBG_vPrintf(TRACE_JOIN, "send leave outFC=%08x\n", psNib->sTbl.u32OutFC);
                /* save out FC to restore after the leave */
                u32OldFrameCtr = psNib->sTbl.u32OutFC + 10;
                ZPS_eAplZdoLeaveNetwork(0, FALSE, FALSE);
            }
            break;
        case E_WAIT_LEAVE:
            DBG_vPrintf(TRACE_JOIN, "Wait leave\n");
            if (sEvent.eType == APP_E_COMMISSION_LEAVE_CFM)
            {
                eState = E_START_ROUTER;
                /* restore the frame counter from before the leave */
                psNib->sTbl.u32OutFC = u32OldFrameCtr;
                DBG_vPrintf(TRACE_JOIN, "leave cfm outFC=%08x\n", psNib->sTbl.u32OutFC);
                OS_eStopSWTimer(APP_CommissionTimer);
                vRemoveAllGroupsAndScenes();
                vSetKeys();
                OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
            }
            break;

        case E_WAIT_LEAVE_RESET:
            if ((sEvent.eType == APP_E_COMMISSION_LEAVE_CFM) || (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED))
            {
                DBG_vPrintf(TRACE_JOIN, "WARNING: Received Factory reset \n");

                psNib->sTbl.u32OutFC = u32OldFrameCtr;
                ZPS_vNwkSaveSecMat(ZPS_pvAplZdoGetNwkHandle());
                vRemoveAllGroupsAndScenes();
                vResetDataStructures();
                vAHI_SwReset();
            }
            break;

        case E_START_ROUTER:
            DBG_vPrintf(TRACE_COMMISSION, "\n>Start router<\n");
            /* Set nwk params */
            void * pvNwk = ZPS_pvAplZdoGetNwkHandle();
            ZPS_vNwkNibSetNwkAddr(pvNwk, sStartParams.u16NwkAddr);
            DBG_vPrintf(TRACE_JOIN, "Given A %04x on %d\n", sStartParams.u16NwkAddr, sStartParams.u8LogicalChannel);
            ZPS_vNwkNibSetChannel(pvNwk, sStartParams.u8LogicalChannel);
            ZPS_vNwkNibSetPanId(pvNwk, sStartParams.u16PanId);
            ZPS_vNwkNibSetExtPanId(pvNwk, sStartParams.u64ExtPanId);
            ZPS_eAplAibSetApsUseExtendedPanId(sStartParams.u64ExtPanId);

            eDecryptKey( sStartParams.au8NwkKey,
                         psNib->sTbl.psSecMatSet[0].au8Key, u32TransactionId,
                         u32ResponseId, sStartParams.u8KeyIndex);

            psNib->sTbl.psSecMatSet[0].u8KeySeqNum = 0;
            memset(psNib->sTbl.pu32InFCSet, 0,
                    (sizeof(uint32) * psNib->sTblSize.u16NtActv));
            psNib->sTbl.psSecMatSet[0].u8KeyType = ZPS_NWK_SEC_NETWORK_KEY;

            /* save security material to flash */
            ZPS_vNwkSaveSecMat(pvNwk);

            /* Make this the Active Key */
            ZPS_vNwkNibSetKeySeqNum(pvNwk, 0);

            /* Start router */
            ZPS_eAplZdoZllStartRouter();

            PDUM_thAPduInstance hAPduInst = PDUM_hAPduAllocateAPduInstance( apduZCL);
            if (hAPduInst != NULL)
            {
                ZPS_tsAplZdpDeviceAnnceReq sZdpDeviceAnnceReq;
                sZdpDeviceAnnceReq.u16NwkAddr = sStartParams.u16NwkAddr;
                sZdpDeviceAnnceReq.u64IeeeAddr = ZPS_u64NwkNibGetExtAddr(pvNwk);
                sZdpDeviceAnnceReq.u8Capability = ZPS_eAplZdoGetMacCapability();

                ZPS_eAplZdpDeviceAnnceRequest(hAPduInst, &u8Seq, &sZdpDeviceAnnceReq);
                DBG_vPrintf(TRACE_JOIN, " Dev Annce Seq No = %d\n", u8Seq);
            }

            sZllState.eState = NOT_FACTORY_NEW;


            if (sStartParams.u16InitiatorNwkAddr != 0)
            {
                ZPS_eAplZdoDirectJoinNetwork(sStartParams.u64InitiatorIEEEAddr, sStartParams.u16InitiatorNwkAddr, u8Flags );
                DBG_vPrintf(TRACE_JOIN, "Direct join %02x\n", u8Flags);
            }

            sZllState.eNodeState = E_RUNNING;
            //PDM_vSaveRecord(&sZllPDDesc);
            PDM_eSaveRecordData(PDM_ID_APP_ZLL_ROUTER,&sZllState,sizeof(tsZllState));

            ZPS_eAplAibSetApsTrustCenterAddress(0xffffffffffffffffULL);
#if PERMIT_JOIN
            ZPS_eAplZdoPermitJoining( PERMIT_JOIN_TIME);
#endif

            eState = E_IDLE;
            u32TransactionId = 0;
            u32ResponseId = 0;
            OS_eStopSWTimer(APP_CommissionTimer);
            DBG_vPrintf(TRACE_JOIN, "All done\n");
            break;

        default:
            break;

    }

}

/****************************************************************************
 *
 * NAME: APP_CommissionTimerTask
 *
 * DESCRIPTION:
 * Handles commissioning timer expiry events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_CommissionTimerTask) {
    APP_CommissionEvent sEvent;
    sEvent.eType = APP_E_COMMISSION_TIMER_EXPIRED;
    OS_ePostMessage(APP_CommissionEvents, &sEvent);
}

/****************************************************************************
 *
 * NAME: eSendScanResponse
 *
 * DESCRIPTION:
 * sends the scan response message in response to a scan request
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE teZCL_Status eSendScanResponse(ZPS_tsNwkNib *psNib,
                               ZPS_tsInterPanAddress       *psDstAddr,
                               uint32 u32TransactionId,
                               uint32 u32ResponseId)
{
    tsCLD_ZllCommission_ScanRspCommandPayload sScanRsp;
    uint8 u8Seq;

    memset( &sScanRsp, 0, sizeof(tsCLD_ZllCommission_ScanRspCommandPayload));
    //u32TransactionId = sEvent.sZllMessage.uPayload.sScanReqPayload.u32TransactionId;
    sScanRsp.u32TransactionId = u32TransactionId;

    //u32ResponseId = RND_u32GetRand(1, 0xffffffff);
    sScanRsp.u32ResponseId = u32ResponseId;
    sScanRsp.u8RSSICorrection = RSSI_CORRECTION;
    sScanRsp.u8ZigbeeInfo = ZLL_ROUTER | ZLL_RXON_IDLE;
    sScanRsp.u16KeyMask = ZLL_SUPPORTED_KEYS;
    sScanRsp.u8ZllInfo = (sZllState.eState == FACTORY_NEW) ? (ZLL_FACTORY_NEW)
                    : (0);
    sScanRsp.u64ExtPanId  = psNib->sPersist.u64ExtPanId;
    sScanRsp.u8NwkUpdateId = psNib->sPersist.u8UpdateId;
    sScanRsp.u8LogicalChannel = psNib->sPersist.u8VsChannel;
    if (sZllState.eState == ZLL_FACTORY_NEW)
    {
        sScanRsp.u16PanId = 0xFFFF;
    }else
    {
        sScanRsp.u16PanId = psNib->sPersist.u16VsPanId;
    }
    sScanRsp.u16NwkAddr = psNib->sPersist.u16NwkAddr;

    sScanRsp.u8TotalGroupIds = 0;
    sScanRsp.u8NumberSubDevices = sDeviceTable.u8NumberDevices;
    if (sScanRsp.u8NumberSubDevices  == 1)
    {
        sScanRsp.u8Endpoint = sDeviceTable.asDeviceRecords[0].u8Endpoint;
        sScanRsp.u16ProfileId = sDeviceTable.asDeviceRecords[0].u16ProfileId;
        sScanRsp.u16DeviceId = sDeviceTable.asDeviceRecords[0].u16DeviceId;
        sScanRsp.u8Version = sDeviceTable.asDeviceRecords[0].u8Version;
        sScanRsp.u8GroupIdCount = 0;
    }
    //sDstAddr = sEvent.sZllMessage.sSrcAddr;
    DBG_vPrintf(TRACE_JOIN, "Back to %016llx Mode %d\n", sDstAddr.uAddress.u64Addr, sDstAddr.eMode);
    sDstAddr.u16PanId = 0xffff;
    return eCLD_ZllCommissionCommandScanRspCommandSend( psDstAddr /*&sEvent.sZllMessage.sSrcAddr*/,
                                                            &u8Seq,
                                                            &sScanRsp);

}

/****************************************************************************
 *
 * NAME: eDecryptKey
 *
 * DESCRIPTION:
 * Decrypt the nwk key use AES ECB
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC uint8 eDecryptKey(uint8* au8InData, uint8* au8OutData,
        uint32 u32TransId, uint32 u32ResponseId, uint8 u8KeyIndex) {
    tsReg128 sTransportKey;
    tsReg128 sExpanded;

    sExpanded.u32register0 = u32TransId;
    sExpanded.u32register1 = u32TransId;
    sExpanded.u32register2 = u32ResponseId;
    sExpanded.u32register3 = u32ResponseId;

    switch (u8KeyIndex)
    {
        case ZLL_TEST_KEY_INDEX:
            sTransportKey.u32register0 = 0x50684c69;
            sTransportKey.u32register1 = u32TransId;
            sTransportKey.u32register2 = 0x434c534e;
            sTransportKey.u32register3 = u32ResponseId;
            DBG_vPrintf(TRACE_COMMISSION, "Tests Key\n");
            break;
        case ZLL_MASTER_KEY_INDEX:
            bACI_ECBencodeStripe(&sMasterKey, TRUE, &sExpanded, &sTransportKey);
            break;
        case ZLL_CERTIFICATION_KEY_INDEX:
            DBG_vPrintf(TRACE_COMMISSION, "Cert Key\n");
            bACI_ECBencodeStripe(&sCertKey, TRUE, &sExpanded, &sTransportKey);

            break;

        default:
            return 3;
            break;
    }

    vECB_Decrypt((uint8*) &sTransportKey, au8InData, au8OutData);
#if SHOW_KEY
    int i;
    for (i = 0; i < 16; i++)
    {
        DBG_vPrintf(TRACE_COMMISSION, "%02x ", au8OutData[i]);
    }
    DBG_vPrintf(TRACE_COMMISSION, "\n");

#endif

    return 0;
}

PRIVATE bool bSearchDiscNt(ZPS_tsNwkNib *psNib, uint64 u64EpId, uint16 u16PanId) {

    int i;
    for (i = 0; i < psNib->sTblSize.u8NtDisc; i++)
    {
        if ((psNib->sTbl.psNtDisc[i].u64ExtPanId == u64EpId)
                || (psNib->sTbl.psNtDisc[i].u16PanId == u16PanId))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/****************************************************************************
 *
 * NAME: u8NewUpdateID
 *
 * DESCRIPTION:
 * Decides which of 2 update ids is thenewest
 *
 * RETURNS: uint8, the newest update id
 * void
 *
 ****************************************************************************/
PRIVATE uint8 u8NewUpdateID(uint8 u8ID1, uint8 u8ID2) {
    if ((abs(u8ID1 - u8ID2)) > 200)
    {
        return MIN(u8ID1, u8ID2);
    }
    return MAX(u8ID1, u8ID2);
}

