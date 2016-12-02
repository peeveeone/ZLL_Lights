/****************************************************************************
 *
 * MODULE              JN-AN-1171 ZigBee Light Link Application
 *
 * COMPONENT:          app_scenes.c
 *
 * DESCRIPTION         Application Scenes save and load functionality
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5164].
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
 * Copyright NXP B.V. 2014. All rights reserved
 *
 ***************************************************************************/
#ifndef APP_SCENES_H_
#define APP_SCENES_H_

#include "zcl.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
#if (defined CLD_SCENES) && (defined SCENES_SERVER)
typedef struct
{
    bool_t  bIsSceneValid;
    uint16  u16GroupId;
    uint8   u8SceneId;
    uint16  u16TransitionTime;
    uint16  u16SceneDataLength;
    uint8   au8SceneData[CLD_SCENES_MAX_SCENE_STORAGE_BYTES];
    #ifdef CLD_SCENES_SUPPORT_ZLL_ENHANCED_COMMANDS
        uint8 u8TransitionTime100ms;
    #endif
} tsAPP_ScenesCustomTableEntry;

/* Scenes data structure for PDM saving */
typedef struct
{
    tsAPP_ScenesCustomTableEntry  asScenesCustomTableEntry[CLD_SCENES_MAX_NUMBER_OF_SCENES];
} tsAPP_ScenesCustomData;
#endif

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
#if (defined CLD_SCENES) && (defined SCENES_SERVER)
PUBLIC void vLoadScenesNVM(void);
PUBLIC void vSaveScenesNVM(void);
#endif

#ifdef CLD_GROUPS
PUBLIC void vRemoveAllGroupsAndScenes(void);
#endif

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#endif //APP_SCENES_H_
