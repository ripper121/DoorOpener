#ifndef ZME_SECURITY
#define ZME_SECURITY

#define RECEIVE_STATUS_SECURE_PACKET	0x80 // not used by other RECEIVE_STATUS_* defined in ZW_transport_api.h

enum
{
	SECURITY_MODE_DISABLED,
	SECURITY_MODE_ENABLED,
	SECURITY_MODE_NOT_YET
};

enum
{
	PACKET_POLICY_DROP = 0,
	PACKET_POLICY_SEND_PLAIN = 1,
	PACKET_POLICY_ACCEPT = 1,
	PACKET_POLICY_SEND_ENCRYPT = 2
};

enum
{
	 ASKED_UNSECURELY = 0,
	 ASKED_SECURELY = 1,
	 ASKED_UNSOLICITED = 2
};

void SecurityInit(void);
void SecurityInclusion(void);
BOOL securityInclusionInProcess(void);
BYTE SecurityAllocFSMSender(BYTE nodeID, BYTE sz, BYTE * d, BYTE txOptions);
void SecurityCommandHandler(BYTE sourceNode, ZW_APPLICATION_TX_BUFFER *pCmd, BYTE cmdLength) reentrant;  
void SecurityNonceTimer(void);

extern BYTE askedSecurely;

#ifdef ZW_CONTROLLER
BYTE startSecurityInclusion(BYTE nodeID);
void setControllerNodeInclusion(BYTE bConcroller);

#endif

#define CURRENT_SECURITY_MODE	(*(SecurityModePtr()))

#endif // ZME_SECURITY