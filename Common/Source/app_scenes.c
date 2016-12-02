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

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include <string.h>
#include "os.h"
#include "os_gen.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "dbg.h"
#include "zps_gen.h"
#include "PDM_IDs.h"
#include "zcl_options.h"
#include "zps_apl_af.h"

#include "app_common.h"

#include "scenes.h"
#include "app_scenes.h"
#ifdef CLD_GROUPS
#include "Groups_internal.h"
#endif
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define SCENES_SEARCH_GROUP_ID      (1 << 0)
#define SCENES_SEARCH_SCENE_ID      (1 << 1)


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef struct
{
    uint8   u8SearchOptions;
    uint16  u16GroupId;
    uint8   u8SceneId;
} tsSearchParameter;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
#if (defined CLD_SCENES) && (defined SCENES_SERVER)
PRIVATE  bool bCLD_ScenesSearchForScene(void *pvSearchParam, void *psNodeUnderTest);
#endif

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
#if (defined CLD_SCENES) && (defined SCENES_SERVER)
PRIVATE tsAPP_ScenesCustomData sScenesCustomData;
#endif


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

#if (defined CLD_SCENES) && (defined SCENES_SERVER)
/****************************************************************************
 *
 * NAME: vSaveScenesNVM
 *
 * DESCRIPTION:
 * To save scenes data to EEPROM
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vSaveScenesNVM(void)
{
    uint8 i=0, j=0;
    tsCLD_ScenesTableEntry *psTableEntry;
    tsSearchParameter sSearchParameter;

    for(i=0; i<CLD_SCENES_MAX_NUMBER_OF_SCENES; i++)
    {
        /* Find for valid scene */
        sSearchParameter.u8SearchOptions = (SCENES_SEARCH_GROUP_ID|SCENES_SEARCH_SCENE_ID);
        sSearchParameter.u16GroupId = sLight.sScenesServerCustomDataStructure.asScenesTableEntry[i].u16GroupId;
        sSearchParameter.u8SceneId  = sLight.sScenesServerCustomDataStructure.asScenesTableEntry[i].u8SceneId;
        psTableEntry = (tsCLD_ScenesTableEntry*)psDLISTsearchFromHead(&sLight.sScenesServerCustomDataStructure.lScenesAllocList,
                                                                        bCLD_ScenesSearchForScene, (void*)&sSearchParameter);

        /* GroupId 0 really does not exist; it's for ZLL GlobalScene */
    #if (defined CLD_SCENES_SUPPORT_ZLL_ENHANCED_COMMANDS)
        if((psTableEntry==NULL) || (sSearchParameter.u16GroupId == 0 && i != 0))
    #else
        if((psTableEntry==NULL) || (sSearchParameter.u16GroupId == 0))
    #endif
        {
            sScenesCustomData.asScenesCustomTableEntry[i].bIsSceneValid = FALSE;
        }
        else
        {
            sScenesCustomData.asScenesCustomTableEntry[i].bIsSceneValid = TRUE;
            sScenesCustomData.asScenesCustomTableEntry[i].u16GroupId = sLight.sScenesServerCustomDataStructure.asScenesTableEntry[i].u16GroupId;
            sScenesCustomData.asScenesCustomTableEntry[i].u8SceneId = sLight.sScenesServerCustomDataStructure.asScenesTableEntry[i].u8SceneId;
            sScenesCustomData.asScenesCustomTableEntry[i].u16TransitionTime = sLight.sScenesServerCustomDataStructure.asScenesTableEntry[i].u16TransitionTime;
            sScenesCustomData.asScenesCustomTableEntry[i].u16SceneDataLength = sLight.sScenesServerCustomDataStructure.asScenesTableEntry[i].u16SceneDataLength;
            for(j=0; j<CLD_SCENES_MAX_SCENE_STORAGE_BYTES; j++)
            {
                sScenesCustomData.asScenesCustomTableEntry[i].au8SceneData[j] = sLight.sScenesServerCustomDataStructure.asScenesTableEntry[i].au8SceneData[j];
            }
            #ifdef CLD_SCENES_SUPPORT_ZLL_ENHANCED_COMMANDS
                sScenesCustomData.asScenesCustomTableEntry[i].u8TransitionTime100ms = sLight.sScenesServerCustomDataStructure.asScenesTableEntry[i].u8TransitionTime100ms;
            #endif
        }
    }
    PDM_eSaveRecordData(PDM_ID_APP_SCENES_DATA,&sScenesCustomData,sizeof(tsAPP_ScenesCustomData));
}

PRIVATE  bool bCLD_ScenesSearchForScene(void *pvSearchParam, void *psNodeUnderTest)
{
    tsSearchParameter *psSearchParameter = (tsSearchParameter*)pvSearchParam;
    tsCLD_ScenesTableEntry *psSearchEntry = (tsCLD_ScenesTableEntry*)psNodeUnderTest;

    if((psSearchParameter->u8SearchOptions & SCENES_SEARCH_GROUP_ID) && (psSearchParameter->u16GroupId != psSearchEntry->u16GroupId))
    {
        return FALSE;
    }

    if((psSearchParameter->u8SearchOptions & SCENES_SEARCH_SCENE_ID) && (psSearchParameter->u8SceneId != psSearchEntry->u8SceneId))
    {
    return FALSE;
    }

    return TRUE;
}
#endif

#if (defined CLD_SCENES) && (defined SCENES_SERVER)
/****************************************************************************
 *
 * NAME: vLoadScenesNVM
 *
 * DESCRIPTION:
 * To load scenes data from EEPROM
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vLoadScenesNVM(void)
{
    uint8 i=0,j=0;
    uint16 u16ByteRead;

    PDM_eReadDataFromRecord(PDM_ID_APP_SCENES_DATA,
                            &sScenesCustomData,
                            sizeof(tsAPP_ScenesCustomData), &u16ByteRead);

    /* initialise lists */
    vDLISTinitialise(&sLight.sScenesServerCustomDataStructure.lScenesAllocList);
    vDLISTinitialise(&sLight.sScenesServerCustomDataStructure.lScenesDeAllocList);

#if (defined CLD_SCENES) && (defined SCENES_SERVER)
    for(i=0; i<CLD_SCENES_MAX_NUMBER_OF_SCENES; i++)
    {
        /* Rebuild the scene list to avoid scene loss after re-flashing */
        if(sScenesCustomData.asScenesCustomTableEntry[i].bIsSceneValid == TRUE)
        {
            vDLISTaddToTail(&sLight.sScenesServerCustomDataStructure.lScenesAllocList,
                            (DNODE *)&sLight.sScenesServerCustomDataStructure.asScenesTableEntry[i]);
        }
        else
        {
            vDLISTaddToTail(&sLight.sScenesServerCustomDataStructure.lScenesDeAllocList,
                            (DNODE *)&sLight.sScenesServerCustomDataStructure.asScenesTableEntry[i]);
        }
        
        sLight.sScenesServerCustomDataStructure.asScenesTableEntry[i].u16GroupId = sScenesCustomData.asScenesCustomTableEntry[i].u16GroupId;
        sLight.sScenesServerCustomDataStructure.asScenesTableEntry[i].u8SceneId = sScenesCustomData.asScenesCustomTableEntry[i].u8SceneId;
        sLight.sScenesServerCustomDataStructure.asScenesTableEntry[i].u16TransitionTime = sScenesCustomData.asScenesCustomTableEntry[i].u16TransitionTime;
        sLight.sScenesServerCustomDataStructure.asScenesTableEntry[i].u16SceneDataLength = sScenesCustomData.asScenesCustomTableEntry[i].u16SceneDataLength;
        for(j=0; j<CLD_SCENES_MAX_SCENE_STORAGE_BYTES; j++)
        {
            sLight.sScenesServerCustomDataStructure.asScenesTableEntry[i].au8SceneData[j] = sScenesCustomData.asScenesCustomTableEntry[i].au8SceneData[j];
        }
    #ifdef CLD_SCENES_SUPPORT_ZLL_ENHANCED_COMMANDS
        sLight.sScenesServerCustomDataStructure.asScenesTableEntry[i].u8TransitionTime100ms = sScenesCustomData.asScenesCustomTableEntry[i].u8TransitionTime100ms;
    #endif
    }
#endif
}
#endif

#ifdef CLD_GROUPS
/****************************************************************************
 *
 * NAME: vRemoveAllGroupsAndScenes
 *
 * DESCRIPTION:
 * to remove all scenes and groups after a leave or factory reset
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vRemoveAllGroupsAndScenes(void)
{
    eCLD_GroupsRemoveAllGroups(&sLight.sEndPoint,
                               &sLight.sClusterInstance.sGroupsServer,
                               (uint64)0xffffffffffffffffLL);

}
#endif

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
