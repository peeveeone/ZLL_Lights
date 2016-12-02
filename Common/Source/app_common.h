/*****************************************************************************
 *
 * MODULE:             JN-AN-1171
 *
 * COMPONENT:          app_common.h
 *
 * DESCRIPTION:        ZLL Application common includes selector
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

#ifndef APP_COMMON_H_
#define APP_COMMON_H_

#include "app_timer_driver.h"




#ifdef RGB
#include "App_Light_ColorLight.h"
#else
#include "App_Light_DimmableLight.h"
#endif


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define APP_EXT_PAN_ID          0x1234567887654321ULL
#define NETWORK_RESTART_TIME    APP_TIME_MS(5000)
#define POLL_TIME               APP_TIME_MS(1000)
#define POLL_TIME_FAST          APP_TIME_MS(250)

#if JENNIC_CHIP==JN5169
#define TX_POWER_NORMAL     ((uint32)(8))
#define TX_POWER_LOW        ((uint32)(0))
#else
#define TX_POWER_NORMAL     ((uint32)(3))
#define TX_POWER_LOW        ((uint32)(-9))
#endif


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef struct {
	uint8 u8Effect;
	uint8 u8Tick;
	uint8 u8Red;
	uint8 u8Green;
	uint8 u8Blue;
	uint8 u8Level;
	uint8 u8Count;
	bool_t  bDirection;
	bool_t  bFinish;
} tsIdentifyColour;

typedef struct {
	uint8 u8Effect;
	uint8 u8Tick;
	uint8 u8Level;
	uint8 u8Count;
	bool_t  bDirection;
	bool_t  bFinish;
} tsIdentifyWhite;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void vECB_Decrypt(uint8* au8Key,
		uint8* au8InData,
		uint8* au8OutData);

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/
extern PUBLIC void vTAM_MLME_RxInCca(bool_t bEnable);

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#endif /*APP_COMMON_H_*/
