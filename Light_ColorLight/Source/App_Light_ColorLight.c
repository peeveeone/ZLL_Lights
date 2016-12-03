/*****************************************************************************
 *
 * MODULE:             JN-AN-1171
 *
 * COMPONENT:          App_Light_ColorLight.c
 *
 * DESCRIPTION:        ZLL Demo: Colored Light - Implementation
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
#include "AppHardwareApi.h"
#include "zps_gen.h"
#include "App_Light_ColorLight.h"
#include "app_common.h"
#include "app_zcl_light_task.h"
#include "dbg.h"
#include <string.h>

#include "app_light_interpolation.h"
#include "DriverBulb_Shim.h"



#ifdef DEBUG_LIGHT_TASK
#define TRACE_LIGHT_TASK  TRUE
#else
#define TRACE_LIGHT_TASK FALSE
#endif

#ifdef DEBUG_PATH
#define TRACE_PATH  TRUE
#else
#define TRACE_PATH  FALSE
#endif



/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

tsZLL_ColourLightDevice sLight;
tsIdentifyColour sIdEffect;
tsCLD_ZllDeviceTable sDeviceTable = { ZLL_NUMBER_DEVICES,
                                      {
                                          { 0,
                                            ZLL_PROFILE_ID,
                                            COLOUR_LIGHT_DEVICE_ID,
                                            LIGHT_COLORLIGHT_LIGHT_00_ENDPOINT,
                                            2,
                                            0,
                                            0}
                                      }
};

uint16 u16CurrentLevel;
uint16 u16TargetLevel;
int16 i16DeltaLevel = 0;

uint8 u8Update = 0xff;

uint16 u16CurrentRed;
uint16 u16TargetRed;
int16 i16DeltaRed = 0;

uint16 u16CurrentBlue;
uint16 u16TargetBlue;
int16 i16DeltaBlue = 0;

uint16 u16CurrentGreen;
uint16 u16TargetGreen;
int16 i16DeltaGreen = 0;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PRIVATE void vOverideProfileId(uint16* pu16Profile, uint8 u8Ep);
/****************************************************************************
 **
 ** NAME: eApp_ZLL_RegisterEndpoint
 **
 ** DESCRIPTION:
 ** Register ZLL endpoints
 **
 ** PARAMETER
 ** Type                                Name                    Descirption
 ** tfpZCL_ZCLCallBackFunction            fptr                    Pointer to ZCL Callback function
 ** tsZLL_CommissionEndpoint            psCommissionEndpoint    Pointer to Commission Endpoint
 **
 **
 ** RETURNS:
 ** teZCL_Status
 *
 ****************************************************************************/
PUBLIC teZCL_Status eApp_ZLL_RegisterEndpoint(tfpZCL_ZCLCallBackFunction fptr,
                                       tsZLL_CommissionEndpoint* psCommissionEndpoint)
{
	ZPS_vAplZdoRegisterProfileCallback(vOverideProfileId);
	zps_vSetIgnoreProfileCheck();

	eZLL_RegisterCommissionEndPoint(LIGHT_COLORLIGHT_COMMISSION_ENDPOINT,
                                    fptr,
                                    psCommissionEndpoint);

    return eZLL_RegisterColourLightEndPoint(LIGHT_COLORLIGHT_LIGHT_00_ENDPOINT,
                                            fptr,
                                            &sLight);
}


/****************************************************************************
*
* NAME: vOverideProfileId
*
* DESCRIPTION: Allows the application to over ride the profile in the
* simple descriptor (0xc05e) with the ZHA profile id (0x0104)
* required for on air packets
*
*
* PARAMETER: pointer to the profile  to be used, the end point sending the data
*
* RETURNS: void
*
****************************************************************************/
PRIVATE void vOverideProfileId(uint16* pu16Profile, uint8 u8Ep)
{
    if (u8Ep == LIGHT_COLORLIGHT_LIGHT_00_ENDPOINT)
    {
        *pu16Profile = 0x0104;
    }
}

/****************************************************************************
 *
 * NAME: vApp_eCLD_ColourControl_GetRGB
 *
 * DESCRIPTION:
 * To get RGB value
 *
 * PARAMETER
 * Type                   Name                    Descirption
 * uint8 *                pu8Red                  Pointer to Red in RGB value
 * uint8 *                pu8Green                Pointer to Green in RGB value
 * uint8 *                pu8Blue                 Pointer to Blue in RGB value
 *
 * RETURNS:
 * teZCL_Status
 *
 ****************************************************************************/
PUBLIC void vApp_eCLD_ColourControl_GetRGB(uint8 *pu8Red,uint8 *pu8Green,uint8 *pu8Blue)
{
    eCLD_ColourControl_GetRGB(LIGHT_COLORLIGHT_LIGHT_00_ENDPOINT,
                              pu8Red,
                              pu8Green,
                              pu8Blue);
}

/****************************************************************************
 *
 * NAME: vAPP_ZCL_DeviceSpecific_Init
 *
 * DESCRIPTION:
 * ZLL Device Specific initialization
 *
 * PARAMETER: void
 *
 * RETURNS: void
 *
 ****************************************************************************/
PUBLIC void vAPP_ZCL_DeviceSpecific_Init(void)
{
    /* Initialise the strings in Basic */
    memcpy(sLight.sBasicServerCluster.au8ManufacturerName, "NXP", CLD_BAS_MANUF_NAME_SIZE);
    memcpy(sLight.sBasicServerCluster.au8ModelIdentifier, "ZLL-ColorLight", CLD_BAS_MODEL_ID_SIZE);
    memcpy(sLight.sBasicServerCluster.au8DateCode, "20150212", CLD_BAS_DATE_SIZE);
    memcpy(sLight.sBasicServerCluster.au8SWBuildID, "1000-0003", CLD_BAS_SW_BUILD_SIZE);

    sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_STOP_EFFECT;
    sIdEffect.u8Tick = 0;
}

/****************************************************************************
 *
 * NAME: APP_vHandleIdentify
 *
 * DESCRIPTION:
 * ZLL Device Specific identify
 *
 * PARAMETER: the identify time
 *
 * RETURNS: void
 *
 ****************************************************************************/
PUBLIC void APP_vHandleIdentify(uint16 u16Time) {

    uint8 u8Red, u8Green, u8Blue;

    DBG_vPrintf(TRACE_LIGHT_TASK, "JP Time %d\n", u16Time);

    if (sIdEffect.u8Effect < E_CLD_IDENTIFY_EFFECT_STOP_EFFECT) {
        /* do nothing */
        //DBG_vPrintf(TRACE_LIGHT_TASK, "Efect do nothing\n");
    }
    else if (u16Time == 0)
    {
            /*
             * Restore to off/off/colour state
             */
        DBG_vPrintf(TRACE_PATH, "\nPath 3");


        vApp_eCLD_ColourControl_GetRGB(&u8Red, &u8Green, &u8Blue);

        DBG_vPrintf(TRACE_LIGHT_TASK, "R %d G %d B %d L %d Hue %d Sat %d\n", u8Red, u8Green, u8Blue,
                            sLight.sLevelControlServerCluster.u8CurrentLevel,
                            sLight.sColourControlServerCluster.u8CurrentHue,
                            sLight.sColourControlServerCluster.u8CurrentSaturation);

        //DBG_vPrintf(TRACE_LIGHT_TASK, "\nidentify stop");

        vRGBLight_SetLevels(sLight.sOnOffServerCluster.bOnOff,
                            sLight.sLevelControlServerCluster.u8CurrentLevel,
                            u8Red,
                            u8Green,
                            u8Blue);
    }
    else
        {
            /* Set the Identify levels */
            DBG_vPrintf(TRACE_PATH, "\nPath 4");
            vRGBLight_SetLevels(TRUE, 159, 250, 0, 0);
        }
}

/****************************************************************************
 *
 * NAME: vIdEffectTick
 *
 * DESCRIPTION:
 * ZLL Device Specific identify tick
 *
 * PARAMETER: void
 *
 * RETURNS: void
 *
 ****************************************************************************/
PUBLIC void vIdEffectTick(uint8 u8Endpoint) {

    if (u8Endpoint != LIGHT_COLORLIGHT_LIGHT_00_ENDPOINT) {
        return;
    }

    if (sIdEffect.u8Effect < E_CLD_IDENTIFY_EFFECT_STOP_EFFECT)
    {
        if (sIdEffect.u8Tick > 0)
        {
            DBG_vPrintf(TRACE_PATH, "\nPath 5");

            sIdEffect.u8Tick--;
            /* Set the light parameters */
            vRGBLight_SetLevels(TRUE, sIdEffect.u8Level,sIdEffect.u8Red,sIdEffect.u8Green,sIdEffect.u8Blue);
            /* Now adjust parameters ready for for next round */
            switch (sIdEffect.u8Effect) {
                case E_CLD_IDENTIFY_EFFECT_BLINK:
                    break;

                case E_CLD_IDENTIFY_EFFECT_BREATHE:
                    if (sIdEffect.bDirection) {
                        if (sIdEffect.u8Level >= 250) {
                            sIdEffect.u8Level -= 50;
                            sIdEffect.bDirection = 0;
                        } else {
                            sIdEffect.u8Level += 50;
                        }
                    } else {
                        if (sIdEffect.u8Level == 0) {
                            // go back up, check for stop
                            sIdEffect.u8Count--;
                            if ((sIdEffect.u8Count) && ( !sIdEffect.bFinish)) {
                                sIdEffect.u8Level += 50;
                                sIdEffect.bDirection = 1;
                            } else {
                                //DBG_vPrintf(TRACE_LIGHT_TASK, "\n>>set tick 0<<");
                                /* lpsw2773 - stop the effect on the next tick */
                                sIdEffect.u8Tick = 0;
                            }
                        } else {
                            sIdEffect.u8Level -= 50;
                        }
                    }
                    break;
                default:
                    if ( sIdEffect.bFinish ) {
                        sIdEffect.u8Tick = 0;
                    }
                }
        } else {
            /*
             * Effect finished, restore the light
             */
            DBG_vPrintf(TRACE_PATH, "\nPath 6");
            sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_STOP_EFFECT;
            sIdEffect.bDirection = FALSE;
            APP_ZCL_vSetIdentifyTime(0);
                uint8 u8Red, u8Green, u8Blue;
                vApp_eCLD_ColourControl_GetRGB(&u8Red, &u8Green, &u8Blue);
                DBG_vPrintf(TRACE_LIGHT_TASK, "EF - R %d G %d B %d L %d Hue %d Sat %d\n",
                                    u8Red,
                                    u8Green,
                                    u8Blue,
                                    sLight.sLevelControlServerCluster.u8CurrentLevel,
                                    sLight.sColourControlServerCluster.u8CurrentHue,
                                    sLight.sColourControlServerCluster.u8CurrentSaturation);

                vRGBLight_SetLevels(sLight.sOnOffServerCluster.bOnOff,
                                    sLight.sLevelControlServerCluster.u8CurrentLevel,
                                    u8Red,
                                    u8Green,
                                    u8Blue);
        }
    }
}

/****************************************************************************
 *
 * NAME: vStartEffect
 *
 * DESCRIPTION:
 * ZLL Device Specific identify effect set up
 *
 * PARAMETER: void
 *
 * RETURNS: void
 *
 ****************************************************************************/
PUBLIC void vStartEffect(uint8 u8Effect) {
    switch (u8Effect) {
        case E_CLD_IDENTIFY_EFFECT_BLINK:
            sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_BLINK;
            sIdEffect.u8Level = 250;
            sIdEffect.u8Red = 255;
            sIdEffect.u8Green = 0;
            sIdEffect.u8Blue = 0;
            sIdEffect.bFinish = FALSE;
            APP_ZCL_vSetIdentifyTime(2);
            sIdEffect.u8Tick = 10;
            break;
        case E_CLD_IDENTIFY_EFFECT_BREATHE:
            sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_BREATHE;
            sIdEffect.bDirection = 1;
            sIdEffect.bFinish = FALSE;
            sIdEffect.u8Level = 0;
            sIdEffect.u8Count = 15;
            eCLD_ColourControl_GetRGB( LIGHT_COLORLIGHT_LIGHT_00_ENDPOINT, &sIdEffect.u8Red, &sIdEffect.u8Green, &sIdEffect.u8Blue);
            APP_ZCL_vSetIdentifyTime(17);
            sIdEffect.u8Tick = 200;
            break;
        case E_CLD_IDENTIFY_EFFECT_OKAY:
            sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_OKAY;
            sIdEffect.bFinish = FALSE;
            sIdEffect.u8Level = 250;
            sIdEffect.u8Red = 0;
            sIdEffect.u8Green = 255;
            sIdEffect.u8Blue = 0;
            APP_ZCL_vSetIdentifyTime(2);
            sIdEffect.u8Tick = 10;
            break;
        case E_CLD_IDENTIFY_EFFECT_CHANNEL_CHANGE:
            sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_CHANNEL_CHANGE;
            sIdEffect.u8Level = 250;
            sIdEffect.u8Red = 255;
            sIdEffect.u8Green = 127;
            sIdEffect.u8Blue = 4;
            sIdEffect.bFinish = FALSE;
            APP_ZCL_vSetIdentifyTime(9);
            sIdEffect.u8Tick = 80;
            break;

        case E_CLD_IDENTIFY_EFFECT_FINISH_EFFECT:
            if (sIdEffect.u8Effect < E_CLD_IDENTIFY_EFFECT_STOP_EFFECT)
            {
                DBG_vPrintf(TRACE_LIGHT_TASK, "\n<FINISH>");
                sIdEffect.bFinish = TRUE;
            }
            break;
        case E_CLD_IDENTIFY_EFFECT_STOP_EFFECT:
            sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_STOP_EFFECT;
            APP_ZCL_vSetIdentifyTime(1);
            break;
    }
}


/****************************************************************************
 *
 * NAME: vRGBLight_SetLevels
 *
 * DESCRIPTION:
 * Set the RGB and levels
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/

PUBLIC void vRGBLight_SetLevels(bool_t bOn, uint8 u8Level, uint8 u8Red, uint8 u8Green, uint8 u8Blue)
{
    if (bOn == TRUE)
    {
    	vLI_Start(u8Level, u8Red, u8Green, u8Blue, 0);
    }
    else
    {
        vLI_Stop();
    }
    vBULB_SetOnOff(bOn);
}

/****************************************************************/
/* OS Stub functions to allow single osconfig diagram (ZLL/ZHA) */
/* to be used for all driver variants (just clear interrupt)    */
/****************************************************************/

#ifndef DR1192

OS_ISR(vISR_Timer3)
{
	(void) u8AHI_TimerFired(E_AHI_TIMER_3);
}

OS_ISR(vISR_Timer4)
{
	(void) u8AHI_TimerFired(E_AHI_TIMER_4);
}

#endif
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
