#ifndef _ASSOCIATION_H_	   
#define _ASSOCIATION_H_

// These functions should be implmented by application to Store, Clear and Read all group[] in non-volatile memory
void AssociationStoreAll(void);
void AssociationClearAll(void);
void AssociationInit(void);

typedef struct _ASSOCIATION_GROUPS_ {
	BYTE nodeID[ASSOCIATION_GROUP_SIZE];
} ASSOCIATION_GROUPS;

#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
typedef struct _MI_ASSOCIATION_GROUP_ {
	BYTE nodeID;
	BYTE endPoint;
} MI_ASSOCIATION_GROUP;

typedef struct _MI_ASSOCIATION_GROUPS_ {
	MI_ASSOCIATION_GROUP nodeIDEP[ASSOCIATION_GROUP_SIZE];
} MI_ASSOCIATION_GROUPS;
#endif

#if WITH_CC_SCENE_CONTROLLER_CONF
typedef struct _SCENE_ {
	BYTE sceneID;
	BYTE dimmingDuration;
} SCENE;
#endif

// definitions for main C file
extern ASSOCIATION_GROUPS groups[ASSOCIATION_NUM_GROUPS];
#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
extern MI_ASSOCIATION_GROUPS groupsMI[ASSOCIATION_NUM_GROUPS];
#endif
#if WITH_CC_SCENE_CONTROLLER_CONF
extern SCENE scenesCtrl[ASSOCIATION_NUM_GROUPS];
#endif

void AssociationSendGetReport(
		#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		BOOL mi,
		#endif
		BYTE groupNo, BYTE destNode);
void AssociationSendGroupings(
		#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		BOOL mi,
		#endif
		BYTE destNode);
void AssociationSendSpecificGroupReport(BYTE destNode);
BYTE AssociationAdd(
		#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		BOOL mi,
		#endif
		BYTE group, BYTE_P nodeIdP, BYTE nodeIdPSize);
void AssociationRemove(
		#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		BOOL mi,
		#endif
		BYTE group, BYTE_P nodeIdP, BYTE nodeIdPSize);
void AssociationRemoveNodeFromGroup(BYTE group, BYTE nodeId);

BYTE countAssociationGroupFullSize(BYTE group);


// -----------------------------------------
// Queue handling
BOOL AssociationSendPacketToGroup(void);

#endif /*_ASSOCIATION_H_*/
