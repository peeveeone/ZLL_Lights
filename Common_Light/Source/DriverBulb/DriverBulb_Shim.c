/****************************************************************************
 *
 * MODULE              AN1166 Smart Lamp Drivers
 *
 * DESCRIPTION         Bulb Driver ZCL Shim Layer: Implementation
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

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

/* SDK includes */
#include <jendefs.h>


/* Device includes */
#include "DriverBulb_Shim.h"
#include "DriverBulb.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Global Variables                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME:       		vBULB_Init,vBULB_SetOnOff,vBULB_SetLevel,vBULB_SetColour
 *
 * DESCRIPTION:		Shim functions to provide generic interface to upper layer
 *
 ****************************************************************************/
PUBLIC void vBULB_Init(void)
{
	DriverBulb_vInit();
}

PUBLIC void vBULB_SetOnOff(bool_t bOn)
{
	DriverBulb_vSetOnOff(bOn);
}

PUBLIC void vBULB_SetLevel(uint32 u32Level)
{
	DriverBulb_vSetLevel(u32Level);
}

PUBLIC void vBULB_SetColour(uint32 u32Red, uint32 u32Green, uint32 u32Blue)
{
	if (DriverBulb_vSetColour)
	{
		DriverBulb_vSetColour(u32Red, u32Green, u32Blue);
	}
}

PUBLIC void vBULB_SetColourTemperature(uint32 u32ColTemp)
{
	if (DriverBulb_vSetTunableWhiteColourTemperature)
	{
		DriverBulb_vSetTunableWhiteColourTemperature(u32ColTemp);
	}
}
/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
