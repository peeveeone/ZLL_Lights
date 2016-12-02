/*****************************************************************************
 *
 * MODULE:             JN-AN-1171
 *
 * COMPONENT:          zpr_light_node.h
 *
 * DESCRIPTION:        ZLL Demo: Light Node NWK Startup - Interface
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

#ifndef APP_LIGHT_NODE_H_
#define APP_LIGHT_NODE_H_

#ifdef CLD_OTA
#include "ota.h"
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/* set to flase to fix the channel chosen by light at network start
 * set to TRUE for release builds
 */
#define RAND_CH             TRUE
/* Set true to produce debug output to show the received network key
 * set FALSE for the release build
 */
#define SHOW_KEY            FALSE

typedef enum
{
    E_STARTUP,
    E_NFN_START,
    E_DISCOVERY,
    E_NETWORK_INIT,
    E_RUNNING
} teNODE_STATES;

typedef struct {
    enum {
        FACTORY_NEW = 0,
        NOT_FACTORY_NEW = 0xff
    }eState;
    teNODE_STATES eNodeState;
    uint16 u16MyAddr;
#ifdef CLD_OTA
    bool bValid;
    uint64 u64IeeeAddrOfServer;
    uint16 u16NwkAddrOfServer;
    uint8 u8OTAserverEP;
#endif
}tsZllState;

extern tsZllState sZllState;
extern PDM_tsRecordDescriptor sZllPDDesc;
extern PDM_tsRecordDescriptor sDevicePDDesc;
extern uint32 u32OldFrameCtr;



/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC void APP_vInitialiseNode(void);
PUBLIC void vResetDataStructures(void);
PUBLIC void vSetKeys(void);
#ifdef CLD_OTA
PUBLIC teNODE_STATES eGetNodeState(void);
PUBLIC tsOTA_PersistedData sGetOTACallBackPersistdata(void);
#endif

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#endif /*APP_LIGHT_NODE_H_*/
