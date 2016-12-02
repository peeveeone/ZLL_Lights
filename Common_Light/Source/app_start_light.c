/*****************************************************************************
 *
 * MODULE:             JN-AN-1171
 *
 * COMPONENT:          app_start_light.c
 *
 * DESCRIPTION:        ZLL Demo: Light Node Initialisation -Implementation
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
#include "os.h"
#include "os_gen.h"
#include "pwrm.h"
#include "pdum_nwk.h"
#include "pdum_apl.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "dbg.h"
#include "dbg_uart.h"

#include "zps_gen.h"
#include "zps_apl.h"
#include "zps_apl_af.h"
#include "zps_apl_zdo.h"
#include "string.h"

#include "appapi.h"
#include "zpr_light_node.h"


#include "zcl_options.h"

#include "app_common.h"
#include "app_light_interpolation.h"

#include "DriverBulb_Shim.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef DEBUG_APP
#define TRACE_APP   FALSE
#else
#define TRACE_APP   TRUE
#endif

#ifndef DEBUG_START_UP
#define TRACE_START FALSE
#else
#define TRACE_START TRUE
#endif

#ifndef DEBUG_EXCEPTIONS
#define TRACE_EXCEPTION FALSE
#else
#define TRACE_EXCEPTION FALSE
#endif


#define HALT_ON_EXCEPTION      FALSE

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/


PRIVATE void vInitialiseApp(void);
PRIVATE void vUnclaimedInterrupt(void);

PRIVATE void vOSError(OS_teStatus eStatus, void *hObject);

#if (defined PDM_EEPROM)
#if TRACE_APP
PRIVATE void vPdmEventHandlerCallback(uint32 u32EventNumber, PDM_eSystemEventCode eSystemEventCode);
#endif
#endif

void vfExtendedStatusCallBack (ZPS_teExtendedStatus eExtendedStatus);
extern uint8 g_u8ZpsExpiryMaxCount;
extern uint8 u8PDM_CalculateFileSystemCapacity();
extern uint8 u8PDM_GetFileSystemOccupancy();

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern void *_stack_low_water_mark;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: PreSleep
 *
 * DESCRIPTION:
 * Power manager callback.
 * Called just before the device is put to sleep.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PWRM_CALLBACK(PreSleep)
{
    DBG_vPrintf(TRACE_START, "Going to sleep ...\n");
}


/****************************************************************************
 *
 * NAME: Wakeup
 *
 * DESCRIPTION:
 * Power manager callback.
 * Called when the device wakes up.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PWRM_CALLBACK(Wakeup)
{
    DBG_vReset();
    DBG_vPrintf(TRACE_START, "Woken up\n");
}


/****************************************************************************
 *
 * NAME: vAppMain
 *
 * DESCRIPTION:
 * Entry point for application from a cold start.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppMain(void)
{
#if JENNIC_CHIP_FAMILY == JN516x
    // Wait until FALSE i.e. on XTAL  - otherwise uart data will be at wrong speed
     while (bAHI_GetClkSource() == TRUE);
    bAHI_SetClockRate(3); /* Move CPU to 32 MHz  vAHI_OptimiseWaitStates automatically called */
#endif

     DBG_vUartInit(DBG_E_UART_0, DBG_E_UART_BAUD_RATE_115200);

    /* Early call to Bulb initialisation to enable fast start up    */

    vBULB_Init();

    /* Bulb is now on 100% white (RGB or Mono) so ensure the LI     */
    /*  module's values are consistent with this initial state      */
#ifndef MONO_ON_OFF
     vLI_SetCurrentValues(CLD_LEVELCONTROL_MAX_LEVEL ,255,255,255,4000 );
#endif

    g_u8ZpsExpiryMaxCount = 1;
    /* Initialise the debug diagnostics module to use UART0 at 115K Baud;
     * Do not use UART 1 if LEDs are used, as it shares DIO with the LEDS
     */


    /*
     * Initialise the stack overflow exception to trigger if the end of the
     * stack is reached. See the linker command file to adjust the allocated
     * stack size.
     */
    vAHI_SetStackOverflow(TRUE, (uint32)&_stack_low_water_mark);


    /*
     * Catch resets due to watchdog timer expiry. Comment out to harden code.
     */
    if (bAHI_WatchdogResetEvent())
    {
        DBG_vPrintf(TRACE_EXCEPTION, "APP: Watchdog timer has reset device!\n");
#if HALT_ON_EXCEPTION
        vAHI_WatchdogStop();
        while (1);
#endif
    }

#ifndef JENNIC_MAC_MiniMacShim
    /* initialise ROM based software modules */
    u32AppApiInit(NULL, NULL, NULL, NULL, NULL, NULL);
#endif



    /* start the RTOS */
    OS_vStart(vInitialiseApp, vUnclaimedInterrupt, vOSError);
    DBG_vPrintf(TRACE_START, "OS started\n");

    /* idle task commences here */
    DBG_vPrintf(TRACE_START, "***********************************************\n");
    DBG_vPrintf(TRACE_START, "LIGHT NODE RESET                               \n");
    DBG_vPrintf(TRACE_START, "***********************************************\n");
    while (TRUE)
    {
        /* Re-load the watch-dog timer. Execution must return through the idle
         * task before the CPU is suspended by the power manager. This ensures
         * that at least one task / ISR has executed with in the watchdog period
         * otherwise the system will be reset.
         */
        vAHI_WatchdogRestart();

        /*
         * suspends CPU operation when the system is idle or puts the device to
         * sleep if there are no activities in progress
         */
        PWRM_vManagePower();
    }
}


/****************************************************************************
 *
 * NAME: vAppRegisterPWRMCallbacks
 *
 * DESCRIPTION:
 * Power manager callback.
 * Called to allow the application to register
 * sleep and wake callbacks.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void vAppRegisterPWRMCallbacks(void)
{
    /* nothing to register as device does not sleep */
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vInitialiseApp
 *
 * DESCRIPTION:
 * Initialises Zigbee stack, hardware and application.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vInitialiseApp(void)
{
    /* Initialise the debug diagnostics module to use UART0 at 115K Baud;
     * Do not use UART 1 if LEDs are used, as it shares DIO with the LEDS
     */
    DBG_vUartInit(DBG_E_UART_0, DBG_E_UART_BAUD_RATE_115200);

    /* Initialise JenOS modules. Initialise Power Manager even on non-sleeping nodes
     * as it allows the device to doze when in the idle task
     */
    PWRM_vInit(E_AHI_SLEEP_OSCON_RAMON);

    /* Initialize the Persistent Data Manager */

    PDM_eInitialise(63, NULL);
#if TRACE_APP
    PDM_vRegisterSystemCallback(vPdmEventHandlerCallback);
#endif


    /* Initialise Protocol Data Unit Manager */
    PDUM_vInit();
    ZPS_vExtendedStatusSetCallback(vfExtendedStatusCallBack);
    /* initialise application */
    APP_vInitialiseNode();

}

#if (defined PDM_EEPROM)
#if TRACE_APP
/****************************************************************************
 *
 * NAME: vPdmEventHandlerCallback
 *
 * DESCRIPTION:
 * Handles PDM callback, information the application of PDM conditions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vPdmEventHandlerCallback(uint32 u32EventNumber, PDM_eSystemEventCode eSystemEventCode)
{

    switch (eSystemEventCode) {
        /*
         * The next three events will require the application to take some action
         */
        case E_PDM_SYSTEM_EVENT_WEAR_COUNT_TRIGGER_VALUE_REACHED:
            DBG_vPrintf(TRACE_APP, "PDM: Segment %d reached trigger wear level\n", u32EventNumber);
            break;
        case E_PDM_SYSTEM_EVENT_DESCRIPTOR_SAVE_FAILED:
            DBG_vPrintf(TRACE_APP, "PDM: Record Id %d failed to save\n", u32EventNumber);
            DBG_vPrintf(TRACE_APP, "PDM: Capacity %d\n", u8PDM_CalculateFileSystemCapacity() );
            DBG_vPrintf(TRACE_APP, "PDM: Occupancy %d\n", u8PDM_GetFileSystemOccupancy() );
            break;
        case E_PDM_SYSTEM_EVENT_PDM_NOT_ENOUGH_SPACE:
            DBG_vPrintf(TRACE_APP, "PDM: Record %d not enough space\n", u32EventNumber);
            DBG_vPrintf(TRACE_APP, "PDM: Capacity %d\n", u8PDM_CalculateFileSystemCapacity() );
            DBG_vPrintf(TRACE_APP, "PDM: Occupancy %d\n", u8PDM_GetFileSystemOccupancy() );
            break;

        /*
         *  The following events are really for information only
         */
        case E_PDM_SYSTEM_EVENT_EEPROM_SEGMENT_HEADER_REPAIRED:
            DBG_vPrintf(TRACE_APP, "PDM: Segment %d header repaired\n", u32EventNumber);
            break;
        case E_PDM_SYSTEM_EVENT_SYSTEM_INTERNAL_BUFFER_WEAR_COUNT_SWAP:
            DBG_vPrintf(TRACE_APP, "PDM: Segment %d buffer wear count swap\n", u32EventNumber);
            break;
        case E_PDM_SYSTEM_EVENT_SYSTEM_DUPLICATE_FILE_SEGMENT_DETECTED:
            DBG_vPrintf(TRACE_APP, "PDM: Segement %d duplicate selected\n", u32EventNumber);
            break;
        default:
            DBG_vPrintf(TRACE_APP, "PDM: Unexpected call back Code %d Number %d\n", eSystemEventCode, u32EventNumber);
            break;
    }
}
#endif
#endif


/****************************************************************************
 *
 * NAME: vUnclaimedInterrupt
 *
 * DESCRIPTION:
 * Catches any unexpected interrupts
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vUnclaimedInterrupt(void)
{
    DBG_vPrintf(TRACE_EXCEPTION, "Unclaimed interrupt\n");
#if HALT_ON_EXCEPTION
    DBG_vDumpStack();
    while (1);
#else
    vAHI_SwReset();
#endif
}


/****************************************************************************
 *
 * NAME: APP_isrStackOverflowException
 *
 * DESCRIPTION:
 * Catches any stack overflow exceptions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_ISR(APP_isrStackOverflowException)
{
    DBG_vPrintf(TRACE_EXCEPTION, "Stack overflowed\n");
#if HALT_ON_EXCEPTION
    while (1);
#else
    vAHI_SwReset();
#endif
}


/****************************************************************************
 *
 * NAME: APP_isrUnimplementedModuleException
 *
 * DESCRIPTION:
 * Catches any unimplemented module exceptions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_ISR(APP_isrUnimplementedModuleException)
{
    DBG_vPrintf(TRACE_EXCEPTION, "Unimplemented module exception\n");
#if HALT_ON_EXCEPTION
    while (1);
#else
    vAHI_SwReset();
#endif
}


/****************************************************************************
 *
 * NAME: APP_isrBusErrorException
 *
 * DESCRIPTION:
 * Catches any bus error exceptions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_ISR(APP_isrBusErrorException)
{
    DBG_vPrintf(TRACE_EXCEPTION, "Bus error\n");
    //DBG_vDumpStack();
#if HALT_ON_EXCEPTION
    while(1);
#else
    vAHI_SwReset();
#endif
}


/****************************************************************************
 *
 * NAME: APP_isrAlignmentException
 *
 * DESCRIPTION:
 * Catches any alignment exceptions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_ISR(APP_isrAlignmentException)
{
    DBG_vPrintf(TRACE_EXCEPTION, "Alignment exception\n");
    DBG_vDumpStack();
#if HALT_ON_EXCEPTION
    DBG_vDumpStack();
    while(1);
#else
    vAHI_SwReset();
#endif
}


/****************************************************************************
 *
 * NAME: APP_isrIllegalInstruction
 *
 * DESCRIPTION:
 * Catches any illegal instruction exceptions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_ISR(APP_isrIllegalInstruction)
{
    DBG_vPrintf(TRACE_EXCEPTION, "Illegal instruction\n");
    DBG_vDumpStack();
#if HALT_ON_EXCEPTION
    DBG_vDumpStack();
    while(1);
#else
    vAHI_SwReset();
#endif
}


PUBLIC void vDebug(char *pcMessage)
{
    DBG_vPrintf(TRACE_EXCEPTION, "%s",pcMessage);
}

PUBLIC void vDebugHex(uint32 u32Data, int iSize)
{
    DBG_vPrintf(TRACE_EXCEPTION, "%x",u32Data);
}


/****************************************************************************
 *
 * NAME: vOSError
 *
 * DESCRIPTION:
 * Catches any unexpected OS errors
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vOSError(OS_teStatus eStatus, void *hObject)
{
    OS_thTask hTask;

    /* ignore queue underruns */
    if ((OS_E_QUEUE_EMPTY == eStatus) ||
        (( eStatus >= OS_E_SWTIMER_STOPPED) && ( eStatus <= OS_E_SWTIMER_RUNNING)) )
    {
        return;
    }

    DBG_vPrintf(TRACE_EXCEPTION, "OS Error %d, offending object handle = 0x%08x\n", eStatus, hObject);

    /* NB the task may have been pre-empted by an ISR which may be at fault */
    OS_eGetCurrentTask(&hTask);
    DBG_vPrintf(TRACE_EXCEPTION, "Currently active task handle = 0x%08x\n", hTask);
#ifdef OS_STRICT_CHECKS
//    DBG_vPrintf(TRACE_EXCEPTION, "Currently active ISR fn address = 0x%08x\n", OS_prGetActiveISR());
#endif

#if HALT_ON_EXCEPTION
    if ( (eStatus < OS_E_SWTIMER_STOPPED) || (eStatus > OS_E_SWTIMER_RUNNING) ) {
        while(1);
    }
#endif
}


/****************************************************************************
 *
 * NAME: vfExtendedStatusCallBack
 *
 * DESCRIPTION:
 * Displays any extended error codes
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void vfExtendedStatusCallBack (ZPS_teExtendedStatus eExtendedStatus)
{
	DBG_vPrintf(TRACE_EXCEPTION,"ERROR: Extended status %x\n", eExtendedStatus);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
