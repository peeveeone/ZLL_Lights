/*****************************************************************************
 *
 * MODULE:             ZCL Options
 *
 * COMPONENT:          zcl_options.h
 *
 * DESCRIPTION:        Options Header for ZigBee Cluster Library functions
 *                     [Colored Light]
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

#ifndef ZCL_OPTIONS_H
#define ZCL_OPTIONS_H

#include <jendefs.h>


PUBLIC void vSaveScenesNVM(void);
PUBLIC void vLoadScenesNVM(void);
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/*
 * Define ONE of the following to set the Primary and Secondary ZLL Channels
 *
 *                      Primaries          Secondaries
 * ZLL_PRIMARY ->       [ 11 15 20 25] and [ 12 13 14 16 17 18 19 21 22 23 24 26]
 * ZLL_PRIMARY_PLUS1 -> [ 12 16 21 26] and [ 11 13 14 15 17 18 19 20 22 23 24 25]
 * ZLL_PRIMARY_PLUS2 -> [ 13 17 22 19] and [ 11 12 14 15 16 18 20 21 23 24 25 26]
 * ZLL_PRIMARY_PLUS3 -> [ 14 18 23 24] and [ 11 12 13 15 16 17 19 20 21 22 25 26]
 *
 */
#define ZLL_PRIMARY
//#define ZLL_PRIMARY_PLUS1
//#define ZLL_PRIMARY_PLUS2
//#define ZLL_PRIMARY_PLUS3

//#define ZLL_NO_APS_ACK

/* Sets the number of endpoints that will be created by the ZCL library */
#define ZLL_NUMBER_OF_ENDPOINTS                             2
#define ZLL_NUMBER_DEVICES                                  1

#define ZLL_MANUFACTURER_CODE                                0x1037

#define ZLL_DISABLE_DEFAULT_RESPONSES                       (TRUE)

/* Set this Tue to disable non error default responses from clusters */
#define ZLL_DISABLE_DEFAULT_RESPONSES       (TRUE)

#define ZLL_NUMBER_OF_ZCL_APPLICATION_TIMERS                 3

#define NUM_ENDPOINT_RECORDS         1
#define NUM_GROUP_RECORDS            4


/* Clusters used by this application */
#define CLD_BASIC
#define BASIC_SERVER

#define CLD_ZLL_COMMISSION
#define ZLL_COMMISSION_SERVER
//#define ZLL_COMMISSION_CLIENT

#define ZLL_CMD_RESPONSE_OPTION

#define CLD_IDENTIFY
#define IDENTIFY_SERVER
#define CLD_IDENTIFY_TICKS_PER_SECOND    10
#define CLD_IDENTIFY_SUPPORT_ZLL_ENHANCED_COMMANDS

#define CLD_GROUPS
#define GROUPS_SERVER
#define CLD_GROUPS_MAX_NUMBER_OF_GROUPS                     16
#define CLD_GROUPS_DISABLE_NAME_SUPPORT


#define CLD_ONOFF
#define ONOFF_SERVER

#define CLD_LEVEL_CONTROL
#define LEVEL_CONTROL_SERVER
#define CLD_LEVELCONTROL_TICKS_PER_SECOND                   10
#define CLD_LEVELCONTROL_MIN_LEVEL                          (1)
#define CLD_LEVELCONTROL_MAX_LEVEL                          (0xfe)


#define CLD_SCENES
#define SCENES_SERVER
#define CLD_SCENES_MAX_NUMBER_OF_SCENES                     9
#define CLD_SCENES_DISABLE_NAME_SUPPORT
#define CLD_SCENES_MAX_SCENE_NAME_LENGTH                    0
#define CLD_SCENES_MAX_SCENE_STORAGE_BYTES                  22
#define CLD_SCENES_ATTR_LAST_CONFIGURED_BY


#define CLD_COLOUR_CONTROL
#define COLOUR_CONTROL_SERVER

#ifdef BUILD_OTA
#define CLD_OTA
#endif
#ifdef  CLD_OTA
    #define OTA_DEMO_TIMINGS                            // define this for the fast timings for demo purposes
    #define OTA_CLIENT
    #define OTA_CLIENT_EP                               1
#define OTA_ACKS_ON FALSE
    #define OTA_MAX_CO_PROCESSOR_IMAGES             0
    //#define OTA_CLD_ATTR_CURRENT_FILE_VERSION
    #define OTA_MAX_BLOCK_SIZE                      48      // in multiples of 16 (internal flash requirement)
#ifdef OTA_DEMO_TIMINGS
    #define OTA_TIME_INTERVAL_BETWEEN_RETRIES       5       // Valid only if OTA_TIME_INTERVAL_BETWEEN_REQUESTS not defined
    #define CLD_OTA_MAX_BLOCK_PAGE_REQ_RETRIES      10      // count of block reqest failure before abandoning download
#else
    #define OTA_TIME_INTERVAL_BETWEEN_REQUESTS      RND_u32GetRand(10,20)
    #define OTA_TIME_INTERVAL_BETWEEN_RETRIES       RND_u32GetRand(10,20)      // Valid only if OTA_TIME_INTERVAL_BETWEEN_REQUESTS not defined
    #define CLD_OTA_MAX_BLOCK_PAGE_REQ_RETRIES      240                        // count of block reqest failure before abandoning download
#endif

#if JENNIC_CHIP == JN5169
    #define OTA_INTERNAL_STORAGE
    #ifdef OTA_ENCRYPTED
        #define INTERNAL_ENCRYPTED                                             // include if encrypted images are to be used for JN5169
    #endif
#endif
    #define OTA_MAX_IMAGES_PER_ENDPOINT             1
    #define OTA_STRING_COMPARE
#endif


/****************************************************************************/
/*             Basic Cluster - Optional Attributes                          */
/*                                                                          */
/* Add the following #define's to your zcl_options.h file to add optional   */
/* attributes to the basic cluster.                                         */
/****************************************************************************/

#define ZCL_ATTRIBUTE_READ_SERVER_SUPPORTED
#define ZCL_ATTRIBUTE_WRITE_SERVER_SUPPORTED

#define CLD_BAS_ATTR_APPLICATION_VERSION
#define CLD_BAS_ATTR_STACK_VERSION
#define CLD_BAS_ATTR_HARDWARE_VERSION
#define CLD_BAS_ATTR_MANUFACTURER_NAME
#define CLD_BAS_ATTR_MODEL_IDENTIFIER
#define CLD_BAS_ATTR_DATE_CODE
#define CLD_BAS_ATTR_SW_BUILD_ID


#define CLD_BAS_APP_VERSION         (1)
#define CLD_BAS_STACK_VERSION       (1)
#define CLD_BAS_HARDWARE_VERSION    (1)
#define CLD_BAS_MANUF_NAME_SIZE     (3)
#define CLD_BAS_MODEL_ID_SIZE       (16)
#define CLD_BAS_DATE_SIZE           (8)
#define CLD_BAS_POWER_SOURCE        E_CLD_BAS_PS_SINGLE_PHASE_MAINS
#define CLD_BAS_SW_BUILD_SIZE       (9)


#define CLD_IDENTIFY_SUPPORT_ZLL_ENHANCED_COMMANDS

#define CLD_SCENES_SUPPORT_ZLL_ENHANCED_COMMANDS
#define CLD_SCENES_SUPPORT_ZLL

#define CLD_ONOFF_ATTR_GLOBAL_SCENE_CONTROL
#define CLD_ONOFF_ATTR_ON_TIME
#define CLD_ONOFF_ATTR_OFF_WAIT_TIME
#define CLD_ONOFF_SUPPORT_ZLL_ENHANCED_COMMANDS

/****************************************************************************/
/*             Level Control Cluster - Optional Attributes                  */
/*                                                                          */
/* Add the following #define's to your zcl_options.h file to add optional   */
/* attributes to the level control cluster.                                 */
/****************************************************************************/

#define CLD_LEVELCONTROL_ATTR_REMAINING_TIME
#define CLD_LEVELCONTROL_ATTR_ON_OFF_TRANSITION_TIME  (10)
//#define CLD_LEVELCONTROL_ATTR_ON_LEVEL

/****************************************************************************/
/*             Scenes Cluster - Optional Attributes                         */
/*                                                                          */
/* Add the following #define's to your zcl_options.h file to add optional   */
/* attributes to the scenes cluster.                                        */
/****************************************************************************/



/****************************************************************************/
/*             Colour Control Cluster - Optional Attributes                 */
/*                                                                          */
/* Add the following #define's to your zcl_options.h file to add optional   */
/* attributes to the time cluster.                                          */
/****************************************************************************/



/*
 * Colour attributes, Optional in ZCL spec but Mandatory for ZLL
 */
/* Colour information attribute set attribute ID's (5.2.2.2.1) */


#define CLD_COLOURCONTROL_ATTR_REMAINING_TIME
#define CLD_COLOURCONTROL_ATTR_COLOUR_MODE
#define CLD_COLOURCONTROL_ATTR_ENHANCED_COLOUR_MODE
#define CLD_COLOURCONTROL_ATTR_COLOUR_CAPABILITIES

/* define capabilities of colour light */
#define CLD_COLOURCONTROL_COLOUR_CAPABILITIES           (COLOUR_CAPABILITY_HUE_SATURATION_SUPPORTED | \
                                                         COLOUR_CAPABILITY_ENHANCE_HUE_SUPPORTED    | \
                                                         COLOUR_CAPABILITY_COLOUR_LOOP_SUPPORTED    | \
                                                         COLOUR_CAPABILITY_XY_SUPPORTED)


/* Defined Primaries Information attribute attribute ID's set (5.2.2.2.2) */
#define CLD_COLOURCONTROL_ATTR_NUMBER_OF_PRIMARIES  3

/* Enable Primary (n) X, Y and Intensity attributes */
#define CLD_COLOURCONTROL_ATTR_PRIMARY_1
#define CLD_COLOURCONTROL_ATTR_PRIMARY_2
#define CLD_COLOURCONTROL_ATTR_PRIMARY_3
#define CLD_COLOURCONTROL_ATTR_PRIMARY_4
#define CLD_COLOURCONTROL_ATTR_PRIMARY_5
#define CLD_COLOURCONTROL_ATTR_PRIMARY_6

#define CLD_COLOURCONTROL_RED_X     (0.68)
#define CLD_COLOURCONTROL_RED_Y     (0.31)
#define CLD_COLOURCONTROL_GREEN_X   (0.11)
#define CLD_COLOURCONTROL_GREEN_Y   (0.82)
#define CLD_COLOURCONTROL_BLUE_X    (0.13)
#define CLD_COLOURCONTROL_BLUE_Y    (0.04)
#define CLD_COLOURCONTROL_WHITE_X   (0.33)
#define CLD_COLOURCONTROL_WHITE_Y   (0.33)

#define CLD_COLOURCONTROL_PRIMARY_1_X           CLD_COLOURCONTROL_RED_X
#define CLD_COLOURCONTROL_PRIMARY_1_Y           CLD_COLOURCONTROL_RED_Y
#define CLD_COLOURCONTROL_PRIMARY_1_INTENSITY   (254 / 3)

#define CLD_COLOURCONTROL_PRIMARY_2_X           CLD_COLOURCONTROL_GREEN_X
#define CLD_COLOURCONTROL_PRIMARY_2_Y           CLD_COLOURCONTROL_GREEN_Y
#define CLD_COLOURCONTROL_PRIMARY_2_INTENSITY   (254 / 3)

#define CLD_COLOURCONTROL_PRIMARY_3_X           CLD_COLOURCONTROL_BLUE_X
#define CLD_COLOURCONTROL_PRIMARY_3_Y           CLD_COLOURCONTROL_BLUE_Y
#define CLD_COLOURCONTROL_PRIMARY_3_INTENSITY   (254 / 3)

#define CLD_COLOURCONTROL_PRIMARY_4_X           (0)
#define CLD_COLOURCONTROL_PRIMARY_4_Y           (0)
#define CLD_COLOURCONTROL_PRIMARY_4_INTENSITY   (0xff)

#define CLD_COLOURCONTROL_PRIMARY_5_X           (0)
#define CLD_COLOURCONTROL_PRIMARY_5_Y           (0)
#define CLD_COLOURCONTROL_PRIMARY_5_INTENSITY   (0xff)

#define CLD_COLOURCONTROL_PRIMARY_6_X           (0)
#define CLD_COLOURCONTROL_PRIMARY_6_Y           (0)
#define CLD_COLOURCONTROL_PRIMARY_6_INTENSITY   (0xff)


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void* psGetDeviceTable(void);

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

#endif /* ZCL_OPTIONS_H */
