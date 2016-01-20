/******************************* learnmode.h *******************************
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
 * Revision:         $Revision: 15922 $
 * Last Changed:     $Date: 2009-12-07 10:11:28 +0100 (Mon, 07 Dec 2009) $
 *
 ****************************************************************************/
#ifndef _SLAVE_LEARN_H_
#define _SLAVE_LEARN_H_
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

#define LEARN_MODE_INCLUSION 1  //Enable the learn process to do an inclusion
#define LEARN_MODE_EXCLUSION 2  //Enable the learn process to do an exclusion
#define LEARN_MODE_DISABLE   0  //Disable learn process
/*============================   LearnCompleted   ===========================
**    Function description
**      Should be implemented by the Application.
**      Called when nodeID have been assigned or deleted.
**    Side effects:
**
**--------------------------------------------------------------------------*/
/* IN The nodeID assigned */
/* IN Completed flag */
//extern 
void LearnCompleted(BYTE nodeID, BOOL completed);



void LearnStarted(BYTE status);
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
extern BYTE learnState;
/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/*============================   StartLearnModeNow   ======================
**    Function description
**      Call this function whenever learnmode should be entered.
**      This function does the following:
**        - Set the Slave in Learnmode
**        - Starts a one second timeout after which learn mode is disabled
**        - Broadcast the NODEINFORMATION frame once each time unless the slave is
**        doing neighbor discovery.
**        - learnState will be TRUE until learnmode is done.
**      If the slave is added or removed to/from a network the function
**      LearnCompleted will be called.
**    Side effects:
**
**--------------------------------------------------------------------------*/
void StartLearnModeNow(BYTE bMode);

#ifdef EFH_PATCH_ENABLE
/*===============================   slaveLearnInit   ========================
**    This function initializes the slave_learn variables
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
extern void slaveLearnInit();
#endif

#endif /*_SLAVE_LEARN_H_*/
