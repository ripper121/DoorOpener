#include "ZME_Includes.h"

#if WITH_CC_ASSOCIATION

ASSOCIATION_GROUPS groups[ASSOCIATION_NUM_GROUPS];
#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
MI_ASSOCIATION_GROUPS groupsMI[ASSOCIATION_NUM_GROUPS];
#endif
#if WITH_CC_SCENE_CONTROLLER_CONF
SCENE scenesCtrl[ASSOCIATION_NUM_GROUPS];
#endif

static IBYTE indx; // do never use this variable in external for calling functions also using this variable!

static BYTE AssociationAddOneIDEP(BYTE group, BYTE nodeId, BYTE endPoint);
static void AssociationRemoveOneGroup(
		#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		BOOL mi,
		#endif
		BYTE group, BYTE_P nodeIdP, BYTE nodeIdPSize);
static void AssociationRemoveOneIDEP(BYTE group, BYTE nodeId, BYTE endPoint);

// variables for AssociationSendOne() loop

static IBYTE indxG = 0;
#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
static IBYTE indxMIG = 0;
#endif

extern SENDBUFFER * queuePacketAssoc; // exported from Queue.c
void SendCompleteByExternalSend(void); // exported from Queue.c
																				 
//--------------------------

ASSOCIATION_GROUPS * AssocGetGroupsPtr()
{
	return groups;
}
BYTE	AssocGetGroupSize()
{
	return ASSOCIATION_GROUP_SIZE;
}
BYTE	AssocGetGroupsCount()
{
	return ASSOCIATION_NUM_GROUPS;
}


// -------------------------
// Association Command Class Commands

/*============================ AssociationSendGetReport ======================
** Sends the ASSOCIATION_REPORT for the supplied Group to the node specified.
** Transmit options are taken from BYTE txOption
** RET TRUE if started FALSE if not
** IN MultiInstanceAssociation or Association
** IN Group Number to send report for
** IN Destination to send to
** IN Send Data transtmit options
**--------------------------------------------------------------------------*/
void AssociationSendGetReport(
		#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		BOOL mi,
		#endif
		BYTE groupNo, BYTE destNode) {
	BYTE byteCount;
	BYTE bGroupIndex;
	if ((txBuf = CreatePacket()) != NULL) {
#if ASSOCIATION_NUM_GROUPS > 1
		// If the node supports several groups. Set the correct group index.
		if (groupNo != 0) {
			bGroupIndex = groupNo-1;
		}
		if (groupNo == 0 || groupNo > ASSOCIATION_NUM_GROUPS) {
			return; // Do not send Report on non-existing groups
		}
#else
		// If node only support one group. Ignore the group ID requested (As defined in Device class specification)
		bGroupIndex = 0;
#endif
		txBuf->ZW_MultiInstanceAssociationReport1byteFrame.cmdClass = 
				#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
				mi ? COMMAND_CLASS_MULTI_INSTANCE_ASSOCIATION : 
				#endif
				COMMAND_CLASS_ASSOCIATION;
		txBuf->ZW_MultiInstanceAssociationReport1byteFrame.cmd = MULTI_INSTANCE_ASSOCIATION_REPORT; // === ASSOCIATION_REPORT
		txBuf->ZW_MultiInstanceAssociationReport1byteFrame.groupingIdentifier = bGroupIndex+1;
		txBuf->ZW_MultiInstanceAssociationReport1byteFrame.maxNodesSupported = ASSOCIATION_GROUP_SIZE;
		txBuf->ZW_MultiInstanceAssociationReport1byteFrame.reportsToFollow = 0; // Nodes fit in one report
#if ASSOCIATION_GROUP_SIZE > 14
		/*
		Check that we can fit in one report:
			48 bytes is the minimal available payload size
			2 bytes takes CC and CC command
			3 bytes groupId, Reports to follow and group size
			1 byte for the marker in MIA
			------------
			Remaining 42 bytes can be filled with 14 nodes (MIA records + A records).
		*/
		#error ASSOCIATION_GROUP_SIZE should not be bigger than 14 to fit in one REPORT!
#endif
		byteCount = 0;
		indx = ASSOCIATION_GROUP_SIZE - 1;
		do {
			if (groups[bGroupIndex].nodeID[indx] != 0) {
				*(&txBuf->ZW_MultiInstanceAssociationReport1byteFrame.nodeId1 + byteCount) = groups[bGroupIndex].nodeID[indx];
				byteCount++;
			}
			} while(indx--);
		#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		if (mi) {
			*(&txBuf->ZW_MultiInstanceAssociationReport1byteFrame.nodeId1 + byteCount) = MULTI_CHANNEL_ASSOCIATION_REPORT_MARKER_V2;
			byteCount++;
			indx = ASSOCIATION_GROUP_SIZE - 1;
			do {
				if (groupsMI[bGroupIndex].nodeIDEP[indx].nodeID != 0) {
					*(&txBuf->ZW_MultiInstanceAssociationReport1byteFrame.nodeId1 + byteCount) = groupsMI[bGroupIndex].nodeIDEP[indx].nodeID;
					*(&txBuf->ZW_MultiInstanceAssociationReport1byteFrame.nodeId1 + byteCount + 1) = groupsMI[bGroupIndex].nodeIDEP[indx].endPoint;
					byteCount += 2;
				}
			} while(indx--);
		}
		#endif

		EnqueuePacket(destNode, sizeof(ZW_MULTI_INSTANCE_ASSOCIATION_REPORT_1BYTE_FRAME) - 4 /* node, marker, node, instance */ + byteCount, txOption);
	}
}

/*======================== MIAssociationSendGroupings ======================
** Sends the number of groupings supported.
** RET TRUE If started, FALSE if not
** IN MultiInstanceAssociation or Association
** IN destination to send to
** IN Send data transmit options
**--------------------------------------------------------------------------*/
void AssociationSendGroupings(
		#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		BOOL mi,
		#endif
		BYTE destNode) {
	if ((txBuf = CreatePacket()) != NULL) {
		txBuf->ZW_MultiInstanceAssociationGroupingsReportFrame.cmdClass =
						#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
						mi ? COMMAND_CLASS_MULTI_INSTANCE_ASSOCIATION : 
						#endif
						COMMAND_CLASS_ASSOCIATION;
		txBuf->ZW_MultiInstanceAssociationGroupingsReportFrame.cmd = MULTI_INSTANCE_ASSOCIATION_GROUPINGS_REPORT;
		txBuf->ZW_MultiInstanceAssociationGroupingsReportFrame.supportedGroupings = ASSOCIATION_NUM_GROUPS;
		EnqueuePacket(destNode, sizeof(txBuf->ZW_MultiInstanceAssociationGroupingsReportFrame), txOption);
	}
}

/*======================== AssociationSendSpecificGroupReport ======================
** Sends the number of groupings supported.
** RET TRUE If started, FALSE if not
** // IN MultiInstanceAssociation or Association
** IN destination to send to
**--------------------------------------------------------------------------*/
void AssociationSendSpecificGroupReport(BYTE destNode) {
	if ((txBuf = CreatePacket()) != NULL) {
		txBuf->ZW_AssociationSpecificGroupReportV2Frame.cmdClass = /* mi ? COMMAND_CLASS_MULTI_INSTANCE_ASSOCIATION : */ COMMAND_CLASS_ASSOCIATION;
		txBuf->ZW_AssociationSpecificGroupReportV2Frame.cmd = ASSOCIATION_SPECIFIC_GROUP_REPORT_V2;
		txBuf->ZW_AssociationSpecificGroupReportV2Frame.group = 0; // we do not support this feature - no one do suppport it
		EnqueuePacket(destNode, sizeof(txBuf->ZW_AssociationSpecificGroupReportV2Frame), txOption);
	}
}

#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
/*============================ AssociationAddOneIDEP ======================
** Adds the node-instance pair to the group
** IN group number to add nodes to
** IN nodeId
** IN endPoint
**--------------------------------------------------------------------------*/
BYTE AssociationAddOneIDEP(BYTE group, BYTE nodeId, BYTE endPoint) {
	BYTE vacant = 0xFF;

	for (indx = 0; indx < ASSOCIATION_GROUP_SIZE; indx++) {
		if (groupsMI[group].nodeIDEP[indx].nodeID == nodeId && groupsMI[group].nodeIDEP[indx].endPoint == endPoint) {
			vacant = 0;
			break;	// Already in - break this loop
		}
	}
	if (vacant == 0)
		return vacant; // Alredy in - continue with next node-instance pair
	// Not in list: search for vacant place to add to the list
	for (indx = 0; indx < ASSOCIATION_GROUP_SIZE; indx++) {
		if (groupsMI[group].nodeIDEP[indx].nodeID == 0) {
				vacant = indx;
				break; // Found vacant place
		}
	}
	if (vacant != 0xFF) {
		groupsMI[group].nodeIDEP[vacant].nodeID = nodeId;
		groupsMI[group].nodeIDEP[vacant].endPoint = endPoint;
	}

	return vacant;
}
#endif

/*============================ AssociationAdd ======================
** Adds the nodes in nodeIdP to the group
** IN MultiInstanceAssociation or Association
** IN group number to add nodes to
** IN pointer to list of nodes
** IN size of the data
**--------------------------------------------------------------------------*/
BYTE AssociationAdd(
		#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		BOOL mi,
		#endif
		BYTE group, BYTE_P nodeIdP, BYTE nodeIdPSize) {
	BYTE i;
	BYTE res = 0xFF;
	BYTE tempID;
	#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
	BYTE tempEP;
	BYTE tempEP_num;
	#endif
	
#if ASSOCIATION_NUM_GROUPS > 1
	if (group < 1 || group > ASSOCIATION_NUM_GROUPS)
		return res; // Ignore invalid GroupId (GroupId can not be 0 for AssociationAdd)
	group--;
#else
	group = 0;
#endif
	
	// A part
	for (i = 0; i < nodeIdPSize; i++) {
		BYTE vacant = 0xFF;
		tempID = *(nodeIdP + i);
		#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		if (tempID == MULTI_CHANNEL_ASSOCIATION_SET_MARKER_V2 && mi) {
			// Got a Marker, go to MIA part
			i++; // Skip the marker
			break;
		}
		#endif
		if ((tempID == 0) || (tempID > ZW_MAX_NODES && tempID != NODE_BROADCAST))
			continue; // Add only legal nodeId: 1-232, 255
		for (indx = 0; indx < ASSOCIATION_GROUP_SIZE; indx++) {
			if (groups[group].nodeID[indx] == tempID) {
				vacant = 0;
				break;	// Already in - break this loop
			}
		}
		if (vacant == 0)
			continue; // Alredy in - continue with next node
		// Not in list: search for vacant place to add to the list
		for (indx = 0; indx < ASSOCIATION_GROUP_SIZE; indx++) {
			if (groups[group].nodeID[indx] == 0) {
					vacant = indx;
					break; // Found vacant place
			}
		}
		if (vacant != 0xff)
		{
			groups[group].nodeID[vacant] = tempID;
			res = vacant;
		}
	}

	#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
	if (mi) {
		// MIA part
		for (; i < nodeIdPSize; i+=2) {
			BYTE vacant = 0xff;
			tempID = *(nodeIdP + i);
			tempEP = *(nodeIdP + i + 1);
			if ((tempID == 0) || (tempID > ZW_MAX_NODES && tempID != NODE_BROADCAST))
				continue; // Add only legal nodeId: 1-232, 255
			if (tempEP & MULTI_CHANNEL_CMD_ENCAP_PROPERTIES2_BIT_ADDRESS_BIT_MASK_V2) {
				// multiple endpoints addressing: apply to first 7 end points
				// tempEP =& MULTI_CHANNEL_CMD_ENCAP_PROPERTIES2_DESTINATION_END_POINT_MASK_V2; - this is not need, since we do a loop from 0 to 6th bit only
				tempEP_num = 7;
				do {
					if (tempEP & (MULTI_CHANNEL_CMD_ENCAP_PROPERTIES2_BIT_ADDRESS_BIT_MASK_V2 >> 1)) // check 6th bit
						res = AssociationAddOneIDEP(group, tempID, tempEP_num);
					tempEP <<= 1;
				} while(--tempEP_num);
			} else {
				// Single endpoint addressing
				res = AssociationAddOneIDEP(group, tempID, tempEP);
			}
		}
	}
	#endif
	
	AssociationStoreAll();

	return res;
}

/*============================ AssociationRemoveNodeFromGroup ======================
** Removes the node from a group of A and MIA
** IN group number to remove node from
** IN nodeId
**--------------------------------------------------------------------------*/
void AssociationRemoveNodeFromGroup(BYTE group, BYTE nodeId) {
	indx = ASSOCIATION_GROUP_SIZE - 1;
	do {
		if (groups[group].nodeID[indx] == nodeId) {
			groups[group].nodeID[indx] = 0;
		}
		#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		if (groupsMI[group].nodeIDEP[indx].nodeID == nodeId) {
			groupsMI[group].nodeIDEP[indx].nodeID = 0;
		}
		#endif
	} while(indx--);
}

#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
/*============================ AssociationRemoveOneIDEP ======================
** Removes the node-instance pair to the group
** IN group number to add nodes to
** IN nodeId
** IN endPoint
**--------------------------------------------------------------------------*/
void AssociationRemoveOneIDEP(BYTE group, BYTE nodeId, BYTE endPoint) {
	indx = ASSOCIATION_GROUP_SIZE - 1;
	do {
		if (groupsMI[group].nodeIDEP[indx].nodeID == nodeId && groupsMI[group].nodeIDEP[indx].endPoint == endPoint) {
			groupsMI[group].nodeIDEP[indx].nodeID = 0;
			break;	// Already in - break this loop
		}
	} while(indx--);
}
#endif

/*============================ AssociationRemoveOneGroup ====================
** Removes association members from specified group
** IN MultiInstanceAssociation or Association
** IN group number to remove nodes from
** IN pointer to array of nodes to remove
** IN size of the data
** If nodeIdPSize = 0 group is removed
**--------------------------------------------------------------------------*/
void AssociationRemoveOneGroup(
		#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		BOOL mi,
		#endif
		BYTE group, BYTE_P nodeIdP, BYTE nodeIdPSize) {
	BYTE i;
	BYTE tempID;
	#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
	BYTE tempEP;
	BYTE tempEP_num;
	#endif
	
	// A part
	for (i = 0; i < nodeIdPSize; i++) {
		tempID = *(nodeIdP + i);
		#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		if (tempID == MULTI_CHANNEL_ASSOCIATION_SET_MARKER_V2 && mi) {
			i++;
			break; // got a Marker, go to MIA part
		}
		#endif
		indx = ASSOCIATION_GROUP_SIZE - 1;
		do {
			if (groups[group].nodeID[indx] == tempID) {
				groups[group].nodeID[indx] = 0;
				break;	/* Found */
			}
		} while (indx--);
	}

	#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
	if (mi) {
		// MIA part
		for (; i < nodeIdPSize; i+=2) {
			tempID = *(nodeIdP + i);
			tempEP = *(nodeIdP + i + 1);
			if (tempEP & MULTI_CHANNEL_CMD_ENCAP_PROPERTIES2_BIT_ADDRESS_BIT_MASK_V2) {
				// multiple endpoints addressing: apply to first 7 end points
				// tempEP =& MULTI_CHANNEL_CMD_ENCAP_PROPERTIES2_DESTINATION_END_POINT_MASK_V2; - this is not need, since we do a loop from 0 to 6th bit only
				tempEP_num = 7;
				do {
					if (tempEP & (MULTI_CHANNEL_CMD_ENCAP_PROPERTIES2_BIT_ADDRESS_BIT_MASK_V2 >> 1)) // check 6th bit
						AssociationRemoveOneIDEP(group, tempID, tempEP_num);
					tempEP <<= 1;
				} while(--tempEP_num);
			} else {
				// single endpoint addressing
				AssociationRemoveOneIDEP(group, tempID, tempEP);
			}
		}
	}
	#endif

	if (nodeIdPSize == 0) {
		// if nodeIDs are not supplied, all nodes should be removed from the group
		indx = ASSOCIATION_GROUP_SIZE;
		do {
			groups[group].nodeID[indx-1] = 0;
			#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
			if (mi)
				groupsMI[group].nodeIDEP[indx-1].nodeID = 0;
			#endif
		} while(--indx);
	}

	AssociationStoreAll();
}

/*============================ AssociationRemove ======================
** Removes association members from specified group
** IN MultiInstanceAssociation or Association
** IN group number to remove nodes from
** IN pointer to array of nodes to remove
** IN size of the data
** If nodeIdPSize = 0 group is removed
**--------------------------------------------------------------------------*/
void AssociationRemove(
		#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		BOOL mi,
		#endif
		BYTE group, BYTE_P nodeIdP, BYTE nodeIdPSize) {
	BYTE i;

#if ASSOCIATION_NUM_GROUPS > 1
	if (group == 0) {
		// apply to all groups
		i = ASSOCIATION_NUM_GROUPS;
		do {
			AssociationRemoveOneGroup(
				#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
				mi,
				#endif
				i-1, nodeIdP, nodeIdPSize);
		} while(--i);
		return;
	}
	if (group > ASSOCIATION_NUM_GROUPS)
		return; // ignore invalid GroupId
	group--;
#else
	group = 0;
#endif
	AssociationRemoveOneGroup(
		#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		mi,
		#endif
		group, nodeIdP, nodeIdPSize); 
}

BYTE countAssociationGroupFullSize(BYTE group)
{
	BYTE count = 0;

	BYTE i;
	for(i=0;i<AssocGetGroupsCount();i++)
	{	
			#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
			if(groupsMI[group-1].nodeIDEP[i].nodeID != 0)
				count++;
			#endif
						
			if(groups[group-1].nodeID[i] != 0)
				count++;
	}
	return count;
} 
// ---------------------------------------------------------------------
// Queue handling
 
 // Теперь все, что связано с посылкой сообщений группе находится в ZME_Queue.c


#endif // WITH_CC_ASSOCIATION