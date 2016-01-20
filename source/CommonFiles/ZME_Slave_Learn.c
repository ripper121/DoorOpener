
/******************************* slave_learn.c *******************************
 *           #######
 *           ##  ##
 *           #  ##    ####   #####    #####  ##  ##   #####
 *             ##    ##  ##  ##  ##  ##      ##  ##  ##
 *            ##  #  ######  ##  ##   ####   ##  ##   ####
 *           ##  ##  ##      ##  ##      ##   #####      ##
 *          #######   ####   ##  ##  #####       ##  #####
 *                                           #####
 *          Z-Wave, the wireless language.
 *
 *              Copyright (c) 2001
 *              Zensys A/S
 *              Denmark
 *
 *              All Rights Reserved
 *
 *    This source file is subject to the terms and conditions of the
 *    Zensys Software License Agreement which restricts the manner
 *    in which it may be used.
 *
 *---------------------------------------------------------------------------
 *
 * Description: This file contains a sample of how learn mode could be implemented
 *              on ZW0102 standard slave, routing slave and enhanced slave devices.
 *              The module works for both battery operated and always listening
 *              devices.
 *
 * Author:   Henrik Holm
 *
 * Last Changed By:  $Author: efh $
 * Revision:         $Revision: 23619 $
 * Last Changed:     $Date: 2012-11-04 19:49:35 +0200 (Вс, 04 ноя 2012) $
 *
 ****************************************************************************/

#ifdef ZW_SLAVE
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_slave_api.h>
#include <ZW_uart_api.h>
#include "ZME_Slave_Learn.h"

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_LEARNPLUS
#define ZW_DEBUG_LEARNPLUS_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_LEARNPLUS_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_LEARNPLUS_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_LEARNPLUS_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_LEARNPLUS_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_LEARNPLUS_SEND_BYTE(data)
#define ZW_DEBUG_LEARNPLUS_SEND_STR(STR)
#define ZW_DEBUG_LEARNPLUS_SEND_NUM(data)
#define ZW_DEBUG_LEARNPLUS_SEND_WORD_NUM(data)
#define ZW_DEBUG_LEARNPLUS_SEND_NL()
#endif


#define ZW_LEARN_NODE_STATE_TIMEOUT 100   /* The base learn timer timeout value */

#define LEARN_MODE_CLASSIC_TIMEOUT      2   /* Timeout count for classic innlusion */
#define LEARN_MODE_NWI_TIMEOUT          4  /* Timeout count for network wide innlusion */

static BYTE learnStateHandle = 0xFF;
static BYTE bInclusionTimeoutCount;
static BYTE bIncReqCount;
static BYTE bIncReqCountSave = 4;

static BOOL nodeInfoTransmitDone  = TRUE;

#define STATE_LEARN_IDLE     0
#define STATE_LEARN_STOP     1
#define STATE_LEARN_CLASSIC  2
#define STATE_LEARN_NWI      3
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
BYTE learnState = STATE_LEARN_IDLE;   /*Application can use this flag to check if learn mode is active*/

void HandleLearnState(void);
void ZCB_LearnNodeStateTimeout(void);
void ZCB_LearnModeCompleted(
  BYTE bStatus,         /* IN Current status of Learnmode*/
  BYTE nodeID);         /* IN resulting nodeID */

void ZCB_TransmitNodeInfoComplete( BYTE bTXStatus );
/****************************************************************************/
/*                               PROTOTYPES                                 */
/****************************************************************************/


/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

code const void (code * ZCB_LearnModeCompleted_p)(BYTE bStatus, BYTE nodeID) = &ZCB_LearnModeCompleted;
/*============================   LearnModeCompleted   ========================
**    Function description
**      Callback which is called on learnmode completes
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void                  /*RET Nothing */
ZCB_LearnModeCompleted(
  BYTE bStatus,         /* IN Current status of Learnmode*/
  BYTE nodeID           /* IN resulting nodeID */
)
{
  /* Learning in progress. Protocol will do timeout for us */
  if (learnStateHandle != 0xff)
  {
    TimerCancel(learnStateHandle);
    learnStateHandle = 0xff;
  }

  if (bStatus == ASSIGN_RANGE_INFO_UPDATE)
  {
    nodeInfoTransmitDone = FALSE;
  }
  else
  {
    nodeInfoTransmitDone = TRUE;
  }
  if (bStatus == ASSIGN_COMPLETE)
  {
    /* Assignment was complete. Tell application */
    if (learnState)
    {
      learnState = STATE_LEARN_STOP;
      HandleLearnState();
    }
    LearnCompleted(nodeID, TRUE);
  }

}

code const void (code * ZCB_TransmitNodeInfoComplete_p)(BYTE bTXStatus) = &ZCB_TransmitNodeInfoComplete;
/*========================   TransmitNodeInfoComplete   ======================
**    Function description
**      Callbackfunction called when the nodeinformation frame has
**      been transmitted. This function ensures that the Transmit Queue is not
**      flooded.
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_TransmitNodeInfoComplete(
  BYTE bTXStatus
)
{
  nodeInfoTransmitDone = TRUE;
}


code const void (code * ZCB_LearnNodeStateTimeout_p)(void) = &ZCB_LearnNodeStateTimeout;
/*============================   EndLearnNodeState   ========================
**    Function description
**      Timeout function that stop a learn mode
**      if we are in classic mode then switch to NWI else stop learn process
**      Should not be called directly.
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_LearnNodeStateTimeout(void)
{
  if (!(--bInclusionTimeoutCount))
  {
    if (learnStateHandle != 0xff)
    {
      TimerCancel(learnStateHandle);
      learnStateHandle = 0xff;
    }
    if (learnState == STATE_LEARN_CLASSIC)
    {
      ZW_SetLearnMode(ZW_SET_LEARN_MODE_DISABLE, NULL);
      learnState = STATE_LEARN_NWI;
      HandleLearnState();
    }
    else if (learnState == STATE_LEARN_NWI)
    {
      ZW_DEBUG_LEARNPLUS_SEND_BYTE('R');
      if (bIncReqCount)
      {
        ZW_ExploreRequestInclusion();
        bIncReqCount--;

      /* Start timer sending  out a explore inclusion request after 4 + random sec */
        bInclusionTimeoutCount = bIncReqCount? (LEARN_MODE_NWI_TIMEOUT + (ZW_Random() & 0x07)):LEARN_MODE_NWI_TIMEOUT;
        learnStateHandle = TimerStart(ZCB_LearnNodeStateTimeout,
                                      ZW_LEARN_NODE_STATE_TIMEOUT,
                                      TIMER_FOREVER);
      }
      else
      {
        learnState = STATE_LEARN_STOP;
        HandleLearnState();
        /*return nodeID 0xFF if the learn process timeout*/
        LearnCompleted(0xFF, FALSE);
      }

    }
  }
}

/*============================   StartLearnInternal   ======================
**    Function description
**      Call this function from the application whenever learnmode
**      should be enabled.
**      This function do the following:
**        - Set the Slave in Learnmode
**        - Starts a one second timeout after which learn mode is disabled
**        - Broadcast the NODEINFORMATION frame once when called.
**      LearnCompleted will be called if a controller performs an assignID.
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
HandleLearnState(void)
{
  if (learnState == STATE_LEARN_CLASSIC)
  {
    ZW_SetLearnMode(ZW_SET_LEARN_MODE_CLASSIC, ZCB_LearnModeCompleted);
    if (nodeInfoTransmitDone)
    {
      if (ZW_SendNodeInformation(NODE_BROADCAST, 0, ZCB_TransmitNodeInfoComplete))
      {
        nodeInfoTransmitDone = FALSE;
      }
    }
    /*Disable Learn mode after 2 sec.*/
    bInclusionTimeoutCount = LEARN_MODE_CLASSIC_TIMEOUT;
    learnStateHandle = TimerStart(ZCB_LearnNodeStateTimeout, ZW_LEARN_NODE_STATE_TIMEOUT, TIMER_FOREVER);
  }
  else if (learnState == STATE_LEARN_NWI)
  {
    ZW_SetLearnMode(ZW_SET_LEARN_MODE_NWI, ZCB_LearnModeCompleted);
    bInclusionTimeoutCount = 1;
    ZCB_LearnNodeStateTimeout();

  }
  else if (learnState == STATE_LEARN_STOP)
  {
    ZW_SetLearnMode(ZW_SET_LEARN_MODE_DISABLE, NULL);
    if (learnStateHandle != 0xff)
    {
      TimerCancel(learnStateHandle);
      learnStateHandle = 0xff;
    }

    learnState = STATE_LEARN_IDLE;
  }
}


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/
/*============================   StartLearnModeNow   ======================
**    Function description
**      Call this function from the application whenever learnmode
**      should be enabled / Disabled.
**      This function do the following:
**        If the node is not included in network
**          Set the Slave in classic Learnmode
**          Starts a two seconds timeout after which we switch to NWI mode
**          Broadcast the NODEINFORMATION frame once when called.
**          If classic learn mode timeout start NWI learn mode
**          if bInclusionReqCount > 1 send explorer inclusion frame
**            start a 4 + random time timer
**          if bInclusionReqCount == 1 send explorer inclusion request frame and wait 4 seconds
**          when timer timeout and bInclusionReqCount == 0 disable NWI mode and call LearnCompleted
**        if node is not included in a network
**          Set the Slave in classic Learnmode
**          Starts a two seconds timeout after which we stop learn mode
**
**       LearnCompleted will be also called after the end of learn process or a timeout
**        if LearnComplete called due timeout out the bNodeID parameter would be 0xFF
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
xStartLearnModeNow(BYTE bMode) /* The mode of the learn process
                                 LEARN_MODE_INCLUSION   Enable the learn mode to do an inclusion
                                 LEARN_MODE_EXCLUSION   Enable the learn mode to do an exclusion
                                 LEARN_MODE_DISABLE      Disable learn mode
                                */
{
  if (bMode)
  {
    if (learnState != STATE_LEARN_IDLE) /* Learn mode is started, stop it */
    {
      learnState = STATE_LEARN_STOP;
      HandleLearnState();
    }
    if (bMode == LEARN_MODE_INCLUSION)
      bIncReqCount = bIncReqCountSave;
    else
      bIncReqCount = 0;
    learnState = STATE_LEARN_CLASSIC;
    HandleLearnState();
  }
  else
  {
    learnState = STATE_LEARN_STOP;
    HandleLearnState();
  }
}

void
StartLearnModeNow(BYTE bMode) 
{
    BYTE bTmpNodeID;
        
    MemoryGetID(NULL, &bTmpNodeID);

    if(bMode != ZW_SET_LEARN_MODE_DISABLE)
    {
        if(bTmpNodeID != 0)
          xStartLearnModeNow(LEARN_MODE_EXCLUSION);
        else
          xStartLearnModeNow(LEARN_MODE_INCLUSION);
    }
    else
    {
        xStartLearnModeNow(LEARN_MODE_DISABLE);
    }
 }
/*===========================   SetInclusionRequestCount   =======================
**    Function description
**      Set the number of timer we send the explorer inclusion request frame
**
**    Side effects: None
**
**--------------------------------------------------------------------------------*/
void SetInclusionRequestCount(BYTE bInclusionRequestCount)
{
  bIncReqCountSave = bInclusionRequestCount;
}

/*===========================   GetLearnModeState   =======================
**    Function description
**      Check if the learning mode is active
**
** Side effects: None
**--------------------------------------------------------------------------------*/
BOOL                    /*RET TRUE if the learning mode is active, else FALSE*/
GetLearnModeState(void)
{
  return (learnState != STATE_LEARN_IDLE);
}

#endif // ZW_SLAVE


