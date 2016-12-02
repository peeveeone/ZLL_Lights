/*****************************************************************************
 *
 * MODULE:             JN-AN-1171
 *
 * COMPONENT:          eventStrings.c
 *
 * DESCRIPTION:        ZLL Application/Stack event strings - Implementation
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
/* Description.                                                             */
/* If you do not need this file to be parsed by doxygen then delete @file   */
/****************************************************************************/

/** @file
 * Add brief description here.
 * Add more detailed description here
 */

/****************************************************************************/
/* Description End                                                          */
/****************************************************************************/


/****************************************************************************/
/*          Include files                                                   */
/****************************************************************************/

#include <jendefs.h>

/****************************************************************************/
/*          Macro Definitions                                               */
/****************************************************************************/


/****************************************************************************/
/***        Type Definitions                                                */
/****************************************************************************/


/****************************************************************************/
/*          Local Function Prototypes                                       */
/****************************************************************************/


/****************************************************************************/
/*          Exported Variables                                              */
/****************************************************************************/
#ifdef EVENT_STR
PUBLIC char *apcZPSEventStrings[] =
{
        "ZPS_EVENT_NONE",
        "ZPS_EVENT_APS_DATA_INDICATION",
        "ZPS_EVENT_APS_DATA_CONFIRM",
        "ZPS_EVENT_APS_DATA_ACK",
        "ZPS_EVENT_NWK_STARTED",
        "ZPS_EVENT_NWK_JOINED_AS_ROUTER",
        "ZPS_EVENT_NWK_JOINED_AS_ENDDEVICE",
        "ZPS_EVENT_NWK_FAILED_TO_START",
        "ZPS_EVENT_NWK_FAILED_TO_JOIN",
        "ZPS_EVENT_NWK_NEW_NODE_HAS_JOINED",
        "ZPS_EVENT_NWK_DISCOVERY_COMPLETE",
        "ZPS_EVENT_NWK_LEAVE_INDICATION",
        "ZPS_EVENT_NWK_LEAVE_CONFIRM",
        "ZPS_EVENT_NWK_STATUS_INDICATION",
        "ZPS_EVENT_NWK_ROUTE_DISCOVERY_CONFIRM",
        "ZPS_EVENT_NWK_POLL_CONFIRM",
        "ZPS_EVENT_NWK_ED_SCAN",
        "ZPS_EVENT_ZDO_BIND",
        "ZPS_EVENT_ZDO_UNBIND",
        "ZPS_EVENT_ZDO_LINK_KEY",
        "ZPS_EVENT_BIND_REQUEST_SERVER",
        "ZPS_EVENT_ERROR",
        "ZPS_EVENT_APS_INTERPAN_DATA_INDICATION",
        "ZPS_EVENT_APS_INTERPAN_DATA_CONFIRM",
        "ZPS_EVENT_APS_ZDP_RESPONSE"

};
#endif

#ifdef EVENT_STR
PUBLIC char *apcAPPEventStrings[] =
{
    "APP_E_EVENT_NONE",
    "APP_E_EVENT_BUTTON_UP",
    "APP_E_EVENT_BUTTON_DOWN",
    "APP_E_EVENT_PGM_BUTTON_UP",
    "APP_E_EVENT_PGM_BUTTON_DOWN",

};
#endif


/****************************************************************************/
/*          Local Variables                                                 */
/****************************************************************************/

/****************************************************************************/
/*          Exported Functions                                              */
/****************************************************************************/

/****************************************************************************/
/***        Local Functions                                                 */
/****************************************************************************/

/****************************************************************************/
/*          END OF FILE                                                     */
/****************************************************************************/
