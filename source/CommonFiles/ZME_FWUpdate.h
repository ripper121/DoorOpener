#ifndef ZME_FIRMWARE_UPDATE
#define ZME_FIRMWARE_UPDATE

void FWUpdate_Init();
void FWUpdate_Authentificate();
void FWUpdateCommandHandler(BYTE sourceNode, ZW_APPLICATION_TX_BUFFER *pCmd, BYTE cmdLength);

#if FIRMWARE_UPGRADE_BATTERY_SAVING    
void fwuWakeup();
BOOL isFWUInProgress();

#endif 

#endif // ZME_FIRMWARE_UPDATE
