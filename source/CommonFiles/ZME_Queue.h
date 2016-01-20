#ifndef _QUEUE_H_
#define _QUEUE_H_

#define SNDBUF_GROUP						0x01
#define SNDBUF_SECURITY 				0x02
#define SNDBUF_FSM_MANAGMENT		0x04
#define SNDBUF_PROCESSED				0x08
#define SNDBUF_NOROUTE_EXPLORE	0x10
#define SNDBUF_TEST_FRAME				0x20
#define SNDBUF_NIF							0x40
#define SNDBUF_WITH_CALLBACK		0x80

// For direct access to queue element (Association, Security)
typedef struct _SENDBUFFER {
	BYTE buf[(sizeof(ZW_APPLICATION_TX_BUFFER))];
	BYTE 	flags;		// Флаги пакета (SNDBUF_GROUP, SNDBUF_SECURITY, SNDBUF_FSM_MANAGMENT)
	BYTE 	dstID;		// nodeID или groupID если установлен флаг SNDBUF_GROUP
	BYTE 	bufSize;
	BYTE 	txOption;
	BYTE 	customID;  	// Здесь находится id обрабатывающего КА если установлен флаг SNDBUF_SECURITY
	BYTE 	next;	  	// Индекс следующего элемента очереди
} SENDBUFFER;

typedef void (code *QueueListenerFunc_t)(SENDBUFFER * packet, BYTE event);


extern BOOL queueSending;

ZW_APPLICATION_TX_BUFFER * CreatePacket(void);
void SetPacketFlags(BYTE flags);
void EnqueuePacket(BYTE nodeID, BYTE size, BYTE txOption);
void EnqueuePacketFSM(BYTE nodeID, BYTE size, BYTE txOption, BYTE fsmid);
void EnqueuePacketF(BYTE nodeID, BYTE size, BYTE txOption, BYTE flags);
#if WITH_CC_ASSOCIATION
#define CreatePacketToGroup() CreatePacket()
void EnqueuePacketToGroup(BYTE groupId, BYTE size);
// not listed here not to export to everyone. Those who need it will declare themself // void SendCompleteByExternalSend(void);
#endif
void SendQueue(void) reentrant;
BYTE QueueSize(void);
BOOL QueueHasPacketsForNode(BYTE nodeId, BYTE flags);

#if EXPLORER_FRAME_TIMOUT
void queue10msTick(void);
#endif

#if WITH_CC_MULTICHANNEL || WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
#define MULTICHANNEL_SRC_END_POINT 		mch_src_end_point
#define MULTICHANNEL_DST_END_POINT 		mch_dst_end_point
#define MULTICHANNEL_ENCAP_ENABLE 		mch_transform_enabled=TRUE
#define MULTICHANNEL_ENCAP_DISABLE 		mch_transform_enabled=FALSE
#define MULTICHANNEL_ENCAP_IS_ENABLED 	mch_transform_enabled

extern BYTE mch_src_end_point;   // Номер канала устройства отправителя, нужен для того, чтобы не передавать номер канала во все обработчики
								 // и упростить обработку многоканальных команд
extern BYTE mch_dst_end_point;   // Номер канала устройства получателя, нужен для того, чтобы не передавать номер канала во все обработчики
								 // и упростить обработку многоканальных команд
extern BYTE mch_transform_enabled; // Включение многоканальное преобразование (Обертка пакетов)
#else
#define MULTICHANNEL_SRC_END_POINT 0
#define MULTICHANNEL_DST_END_POINT 0
#define MULTICHANNEL_ENCAP_ENABLE
#define MULTICHANNEL_ENCAP_DISABLE
#define MULTICHANNEL_ENCAP_IS_ENABLED 	FALSE
#endif

// exported to application code to minimize function call parameters number
extern ZW_APPLICATION_TX_BUFFER * txBuf;  // pointer to the allocated buffer for new packets
extern BYTE txOption;

#define TRANSMIT_OPTION_USE_DEFAULT		0
#if PRODUCT_NO_ROUTES
// If we don't want to use routing in this device - only direct + Explorer Frames
#define TRANSMIT_OPTION_DEFAULT_VALUE		(TRANSMIT_OPTION_ACK)
#else
#define TRANSMIT_OPTION_DEFAULT_VALUE		(TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_EXPLORE)
#endif

void SendCallback(BYTE cbk, BYTE txStatus);

void SetPacketCallbackID(BYTE cbkID);

#endif /*_QUEUE_H_*/
