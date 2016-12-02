/****************************************************************************
 *
 * MODULE              AN1166 Smart Lamp Drivers
 *
 * DESCRIPTION         Driver Public Interface
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142, JN5139].
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
 ****************************************************************************/
#ifndef  DRIVERBULB_H_INCLUDED
#define  DRIVERBULB_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
/* SDK includes */
#include <jendefs.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

enum {E_RED_PWM,E_GREEN_PWM,E_BLUE_PWM};
/****************************************************************************/
/***        Public Function Prototypes                                    ***/
/****************************************************************************/

/* Mandatory Interface functions */
PUBLIC void 		DriverBulb_vInit(void);
PUBLIC void 		DriverBulb_vOn(void);
PUBLIC void 		DriverBulb_vOff(void);
PUBLIC bool_t		DriverBulb_bOn(void);

PUBLIC void 	 	DriverBulb_vSetLevel(uint32 u32Level);
PUBLIC void         DriverBulb_vSetOnOff(bool_t bOn);
PUBLIC void         DriverBulb_vSetColour(uint32 u32Red, uint32 u32Green, uint32 u32Blue)__attribute__((weak));

/* Optional Interface Functions                        */
/* Stub out in implementation if no behaviour required */
PUBLIC bool_t 	 	DriverBulb_bReady(void);
PUBLIC int16 		DriverBulb_i16Analogue(uint8 u8Adc, uint16 u16AdcRead);
PUBLIC void			DriverBulb_vCbSetLevel(uint8 u8Level);

/* No need to stub these out -only DR1192/DR1221 build variants support these */
PUBLIC void         DriverBulb_vSetBalance(uint8 u8Balance)__attribute__((weak));
PUBLIC void         DriverBulb_vSetTunableWhiteColourTemperature(int32 i32ColourTemperature)__attribute__((weak));
PUBLIC void         DriverBulb_vGetColourTempPhyMinMax(uint16 *pu16PhyMin, uint16 *pu16PhyMax)__attribute__((weak));

/* Auxillary interface for DR1223 (AAL RGB) */
PUBLIC uint16 DriverBulb_u16GetAdcValue(uint32 u32ChannelId)__attribute__((weak));
PUBLIC void DriverBulb_vSet12BitColour(uint32 u32Red, uint32 u32Green, uint32 u32Blue)__attribute__((weak));


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* DRIVERBULB_H_INCLUDED */
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
