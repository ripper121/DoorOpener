#ifndef ZME_POWER_LEVEL
#define ZME_POWER_LEVEL

typedef struct _POWERLEVEL_DATA_ {
   	BYTE fsmId;
   	BYTE source_nodeId;
   	BYTE test_nodeId;
   	BYTE level;
   	WORD retry_count;
	WORD ack_count;
	
} POWERLEVEL_DATA;

void PowerLevelInit();
void PowerLevel1SecTick();
void PowerLevelCommandHandler(BYTE sourceNode, ZW_APPLICATION_TX_BUFFER *pCmd, BYTE cmdLength) reentrant;

#endif // ZME_POWER_LEVEL
