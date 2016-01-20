#include "ZME_Includes.h"

#if WITH_CC_SECURITY
#include "ZME_Security.h"
#endif
#if WITH_FSM
#include "ZME_FSM.h"
#endif
#if WITH_CC_ASSOCIATION
#include "ZME_Association.h"
#endif

#if WITH_CC_ASSOCIATION
SENDBUFFER * queuePacketAssoc; // Public to be visible from Association
#endif
keil_static SENDBUFFER queue[QUEUE_SIZE];

#define UNALLOCATED_INDEX  0xFF

keil_static BYTE queueFirst 		= UNALLOCATED_INDEX; // Index of first buffer in queue
keil_static BYTE queueLast 			= UNALLOCATED_INDEX; // Index of lasr free buffer in queue
keil_static BYTE queueLen 			= 0;
keil_static BYTE queueLastSended 	= UNALLOCATED_INDEX;
keil_static BYTE queueAllocIndex 	= UNALLOCATED_INDEX;

ZW_APPLICATION_TX_BUFFER * txBuf;  // Pointer to the next free buffer or NULL if queue full

BOOL queueSending = FALSE; // In progress flag

#if EXPLORER_FRAME_TIMOUT
BYTE abortExplorerFrameSendTimout = 0;
#endif
			
// Forward declarations
static void SendComplete(BYTE txStatus);
static void UnsetPacketFlags(BYTE flags);

// Exported functions
#if WITH_CC_ASSOCIATION
BOOL AssociationSendPacketToGroup(void);
#endif

#define QUEUE_PACKET queue[queueFirst]

////////////////////////////////////////

#if WITH_CC_SECURITY
	// Прототипы необходимые для Security из других модулей...
	BYTE SecurityOutgoingPacketsPolicy(BYTE nodeId, BYTE cmdclass, BYTE askedSecurely);
#endif

#if WITH_CC_MULTI_COMMAND
BOOL IsNodeSupportMultiCmd(BYTE nodeId);
#endif

#if WITH_CC_ASSOCIATION
	// ZME_Association.c
	ASSOCIATION_GROUPS * AssocGetGroupsPtr();
	BYTE AssocGetGroupSize();
	BYTE AssocGetGroupsCount();
#endif

static void EnqueuePacketFPos(BYTE p, BYTE nodeID, BYTE size, BYTE txOption, BYTE flags);


#if WITH_CC_MULTICHANNEL || WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
// Adds MultiChannel data to the currently allocated packet
void ProcessMultichannelPacket(void) {
	if (MULTICHANNEL_ENCAP_IS_ENABLED && (MULTICHANNEL_SRC_END_POINT != 0 || MULTICHANNEL_DST_END_POINT != 0)) {
		// Не нужно делать Multichannel поверх Multichannel
		if(queue[queueAllocIndex].buf[0] == COMMAND_CLASS_MULTI_CHANNEL_V3)
			return;

		zme_memmove(queue[queueAllocIndex].buf + 4, queue[queueAllocIndex].buf, queue[queueAllocIndex].bufSize);

		queue[queueAllocIndex].bufSize += 4;

		queue[queueAllocIndex].buf[0] = COMMAND_CLASS_MULTI_CHANNEL_V3;
		queue[queueAllocIndex].buf[1] = MULTI_CHANNEL_CMD_ENCAP_V3;
		queue[queueAllocIndex].buf[2] = MULTICHANNEL_DST_END_POINT; // incoming destiation becomes outgoing source
		queue[queueAllocIndex].buf[3] = MULTICHANNEL_SRC_END_POINT;
	}
}
#endif

#if WITH_CC_MULTI_COMMAND
#define MCOMMAND_PACKET_OVERHEAD				  	4
#define MAX_MCOMMAND_PACKET_SIZE_SECURITY 			(20 - MCOMMAND_PACKET_OVERHEAD)
#define MAX_MCOMMAND_PACKET_SIZE_PLAIN	  			(40 - MCOMMAND_PACKET_OVERHEAD)

// Tries to encapsulate the currently allocated packet in MultiCmd
// Parameter move instructs to move the resulting packet on a specific position
// Returns TRUE if packet queued, FALSE if caller need to queue it
BOOL ProcessMulticommand(BYTE move) {
	BYTE current_packet;
	BOOL encapsulated = FALSE;
	BOOL mcommand_packet;
	BYTE packet_size  		=  queue[queueAllocIndex].bufSize;
	BYTE packet_flags 		=  queue[queueAllocIndex].flags;
	BYTE mch_offset;	

	BYTE max_packet_size = (packet_flags & SNDBUF_SECURITY) ? MAX_MCOMMAND_PACKET_SIZE_SECURITY : MAX_MCOMMAND_PACKET_SIZE_PLAIN;
	BYTE current_size;

	// packet is to a specific node (not to group) && destination does not support MultiCmd - we will not encapsulate it
	// we allow MultiCmd encapsulation to the same Group!
	if(!(packet_flags & SNDBUF_GROUP) && !IsNodeSupportMultiCmd(queue[queueAllocIndex].dstID))
		return FALSE;
	
	for(current_packet = queueFirst; current_packet != UNALLOCATED_INDEX; current_packet = queue[current_packet].next) {
		// Не рассматриваем уже обработанные пакеты
		if(queue[current_packet].flags & SNDBUF_PROCESSED)
			continue;

		current_size = 	queue[current_packet].bufSize;

		// Необходимо совпадение типа пакета (Security к Security, Group к Group)
		if((queue[current_packet].dstID != queue[queueAllocIndex].dstID) || (queue[current_packet].flags  != packet_flags) || (packet_flags & SNDBUF_WITH_CALLBACK))
			continue;

		mch_offset = 0;
		#if WITH_CC_MULTICHANNEL || WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		// Нужно отправлять пакет через Multichannel?
		if (queue[queueAllocIndex].buf[0] == COMMAND_CLASS_MULTI_CHANNEL_V3) {
			// Нужно!
			if (
					(queue[current_packet].buf[0] != queue[queueAllocIndex].buf[0]) || // Текущий пакет не содержит Multichannel... 
					(queue[current_packet].buf[2] != queue[queueAllocIndex].buf[2]) || // Канал не соответствует
					(queue[current_packet].buf[3] != queue[queueAllocIndex].buf[3])
			) {
				continue;
			} else {
				mch_offset = 4;
				packet_size -= 4; // we will add data from new packet without MultiChannel header
			}
		} else {
			if (queue[current_packet].buf[0] == COMMAND_CLASS_MULTI_CHANNEL_V3) {
				continue;
			}
		}
		#endif
		
		mcommand_packet = (queue[current_packet].buf[mch_offset] == COMMAND_CLASS_MULTI_CMD && queue[current_packet].buf[mch_offset+1] == MULTI_CMD_ENCAP);
		
		// already contains MultiCmd, so no need to account for overhead twice
		if(mcommand_packet)
			max_packet_size += MCOMMAND_PACKET_OVERHEAD;
		
		// Можно ли впихнуть еще одну команду, хватает ли нам оставшегося места внутри пакета
		// +1 из-за добавления размера пакета
		if( (current_size + packet_size + 1) > max_packet_size )
			continue; // Не хватает места - ищим следующий подходящий

		// Места хватает
		// Пакет не содержит MultiCommand
		if(!mcommand_packet){
			// Преобразуем пакет
			// Сдвигаем данные
			zme_memmove(queue[current_packet].buf+4+mch_offset, queue[current_packet].buf+mch_offset, current_size);
			// Добавляем данные MultiCommand
			// Класс команд и команда
			queue[current_packet].buf[mch_offset    ]	= 	COMMAND_CLASS_MULTI_CMD;
			queue[current_packet].buf[mch_offset + 1]	= 	MULTI_CMD_ENCAP;
			// Количество команд
			queue[current_packet].buf[mch_offset + 2]	=	1;	
			// размер 1-ой команды
			queue[current_packet].buf[mch_offset + 3]	=	current_size - mch_offset;

			// размер пакета
			queue[current_packet].bufSize += MCOMMAND_PACKET_OVERHEAD;
			current_size = queue[current_packet].bufSize;
		}
		
		// Добавляем данные в конец
		queue[current_packet].buf[current_size]	= packet_size;
		memcpy(queue[current_packet].buf + current_size + 1, queue[queueAllocIndex].buf + mch_offset, packet_size);
		// Обновляем счетчик команд
		queue[current_packet].buf[mch_offset + 2]++;
		// Обновляем размер пакета
		queue[current_packet].bufSize += packet_size + 1;

		encapsulated = TRUE;

		if (move != UNALLOCATED_INDEX) {
			// Нам нужно чтобы итоговый пакет был определенным в очереди
			if (current_packet != move) { // Если пакет уже не первый
				// Копируем старый пакет целиком на место нового
				memcpy((BYTE*)&queue[queueAllocIndex], (BYTE*)&queue[current_packet], sizeof(SENDBUFFER));
				// Старый помечаем, как обработанный
				queue[current_packet].flags |= SNDBUF_PROCESSED;
				// this instruct to add new packet, while previous was marked as done (this means we moved repacked packet
				encapsulated = FALSE; // Нам все-таки придется добавлять новый пакет
			}
		}
		break;
	}

	return encapsulated;
}
#endif

// Defines the order of encapsulation and tried to encapsulate
// Returns TRUE if packet was encapsulated
static BOOL processEncapsulation(BYTE move){
	BOOL result = FALSE;
	
	// Add MultiChannel to the packet - it will be used by MultiCmd too
	#if (WITH_CC_MULTICHANNEL || WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2)
		ProcessMultichannelPacket();
	#endif

	// Now trying to add packet into MultiCmd
	#if WITH_CC_MULTI_COMMAND
		result = ProcessMulticommand(move);
	#endif

	return result;
}

static void InsertToQueueFirst(BYTE cell) {
	queue[cell].next = queueFirst;
	if(queueFirst == UNALLOCATED_INDEX)
		queueLast = cell;	
	queueFirst = cell;
	queueLen++;
}

static void InsertToQueueLast(BYTE cell) {
	queue[cell].next = UNALLOCATED_INDEX;
	if(queueLast == UNALLOCATED_INDEX)
		queueFirst = cell;
	else
		queue[queueLast].next = cell;
	queueLast = cell;
	queueLen++;
}
static BOOL ElementInQueue(BYTE pos) {
    BYTE curr;
    for(curr=queueFirst; curr!= UNALLOCATED_INDEX; curr=queue[curr].next)
        if(curr == pos)
            return TRUE;
    return FALSE;
}
static void InsertToQueueAfter(BYTE pos, BYTE cell) {
	BYTE prev_next;
	
	if (pos == UNALLOCATED_INDEX || pos == queueLast || !ElementInQueue(pos)) {
		InsertToQueueLast(cell);
		return;
	}
	
	prev_next = queue[pos].next;
	queue[pos].next = cell;
	queue[cell].next = prev_next;
	queueLen++;
}

static void RemoveFromQueue(BYTE index) {
	BYTE deleted = UNALLOCATED_INDEX;
	
	if (queueFirst == UNALLOCATED_INDEX)
		return;
	
	if (index == queueFirst) {
		deleted = queueFirst;
		
		if (queue[queueFirst].next != UNALLOCATED_INDEX)
			queueFirst = queue[queueFirst].next;
		else {
			queueFirst = UNALLOCATED_INDEX;
			queueLast = UNALLOCATED_INDEX;
		}
	} else {
		register BYTE curr;
		for (curr = queueFirst; curr != UNALLOCATED_INDEX; curr = queue[curr].next)
			if (queue[curr].next == index) {
				deleted = index;
				
				if (queue[index].next == UNALLOCATED_INDEX) { // Это последний элемент списка
					queue[curr].next = UNALLOCATED_INDEX;
					queueLast = curr;	
				} else
					queue[curr].next = queue[index].next;
				
				break;
			}	
	}
	
	if(deleted != UNALLOCATED_INDEX) {
		queue[deleted].bufSize = 0;
		queueLen--;
	}
}

static void FillPacket(BYTE dstID, BYTE sz, BYTE txOptions, BYTE flags) {
	queue[queueAllocIndex].dstID = dstID;
	queue[queueAllocIndex].bufSize = sz;
	queue[queueAllocIndex].txOption = (txOptions != TRANSMIT_OPTION_USE_DEFAULT) ? txOptions : TRANSMIT_OPTION_DEFAULT_VALUE;
	queue[queueAllocIndex].flags |= flags;
}

void SetPacketCallbackID(BYTE cbkID) {
	queue[queueAllocIndex].customID = cbkID;
	queue[queueAllocIndex].flags |= SNDBUF_WITH_CALLBACK;
}

static void cleanUpPacket(BYTE index) {
	queue[index].bufSize = 0;
}

#if WITH_CC_ASSOCIATION
// This function queue packet to Association group to unfold it in future
void EnqueuePacketToGroup(BYTE groupID, BYTE size) {
	// NB! if this function is called without CreatePAcket*, buffer override will happen and queue will be filled with ghost packets!

	EnqueuePacketF(groupID, size, 0, SNDBUF_GROUP);
}
#endif

// Bypass security policy and allows additional flags
// Should be used by Security CC only or to send NIF
void EnqueuePacketF(BYTE nodeID, BYTE size, BYTE txOption, BYTE flags) {
	// NB! if this function is called without CreatePacket*, buffer override will happen and queue will be filled with ghost packets!

	EnqueuePacketFPos(UNALLOCATED_INDEX, nodeID, size, txOption, flags);
}

// Used to place a packet at a specific place, private function
static void EnqueuePacketFPos(BYTE p, BYTE nodeID, BYTE size, BYTE txOption, BYTE flags) {
	// NB! if this function is called without CreatePacket*, buffer override will happen and queue will be filled with ghost packets!

	assert(size); // size shoudl never be 0. Even for Test and NIF packets
	
	// Если мы не используем таблицы маршрутизации, то явно говорим об этом в опциях передачи пакета
	// и отдельно помечаем этот пакет в очереди, для повторной отправки с explore-фреймом в случае неудачи
	#if PRODUCT_NO_ROUTES
	if(((flags & SNDBUF_GROUP) == 0) && ((flags & SNDBUF_NIF) == 0))
	{
		txOption = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_NO_ROUTE;
		flags |= SNDBUF_NOROUTE_EXPLORE;
	}
	#endif
	FillPacket(nodeID, size, txOption, flags);	

	
	if(!processEncapsulation(p))
		InsertToQueueAfter(p, queueAllocIndex);
	else
		cleanUpPacket(queueAllocIndex);

	queueAllocIndex = UNALLOCATED_INDEX;
}

// This is the most used function
// Used to send reports in Secure and Unsecure modes (as asked policy applies)
void EnqueuePacket(BYTE nodeID, BYTE size, BYTE txOption) {
	// NB! if this function is called without CreatePacket*, buffer override will happen and queue will be filled with ghost packets!
	BYTE flags = 0;
	#if WITH_CC_SECURITY
		switch (SecurityOutgoingPacketsPolicy(nodeID, queue[queueAllocIndex].buf[0], askedSecurely)) {
			case PACKET_POLICY_DROP:
				return;
			case PACKET_POLICY_SEND_ENCRYPT:
				// flags are ORed
				flags = SNDBUF_SECURITY;
				break;
			case PACKET_POLICY_SEND_PLAIN:
				// nothing to do here
				break;
			}
	#endif
	EnqueuePacketF(nodeID, size, txOption, flags);
}

#if WITH_FSM
// Used by different FSM (Security, PowerLevel, FWUpdate)
void EnqueuePacketFSM(BYTE nodeID, BYTE size, BYTE txOption, BYTE fsmid) {
	
	BYTE flags = SNDBUF_FSM_MANAGMENT;
	// Если мы не используем таблицы маршрутизации, то явно говорим об этом в опциях передачи пакета
	// и отдельно помечаем этот пакет в очереди, для повторной отправки с explore-фреймом в случае неудачи
	#if PRODUCT_NO_ROUTES
	txOption |= TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_NO_ROUTE;
	flags |= SNDBUF_NOROUTE_EXPLORE;
	#endif
	
	FillPacket(nodeID, size, txOption, flags);
	
	assert(size); // size shoudl never be 0. Even for Test and NIF packets
	
	queue[queueAllocIndex].customID = fsmid;
	
	if(!processEncapsulation(queueFirst))
		InsertToQueueFirst(queueAllocIndex);

	queueAllocIndex = UNALLOCATED_INDEX;	
}

// #warning "What for?" !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void QueueFastCleanup(BYTE except_packet) {	
	BYTE current_packet = queueFirst;
	SENDBUFFER * pPacket;
	BYTE prev;

	while (current_packet != UNALLOCATED_INDEX) {
		pPacket = &queue[current_packet];
		
		// Удаляем уже обработанные пакеты
		if ((pPacket->flags & SNDBUF_PROCESSED) && (current_packet != except_packet)) {
      prev = current_packet;
      current_packet = queue[current_packet].next;	
      RemoveFromQueue(prev);
      continue;
		}
		// Обрабатываем следующий....
		current_packet = queue[current_packet].next;
	}
}
#endif

#if EXPLORER_FRAME_TIMOUT
void queue10msTick(void) {
	if (abortExplorerFrameSendTimout) {
		if (abortExplorerFrameSendTimout-- == 1) {
				ZW_SEND_DATA_ABORT();
		}
	}
}
#endif

CBK_DEFINE( , void, SendComplete, (BYTE txStatus)) {
	#if EXPLORER_FRAME_TIMOUT
		abortExplorerFrameSendTimout = 0;
	#endif
	
	assert(queueLastSended != UNALLOCATED_INDEX);
	
	#if PRODUCT_NO_ROUTES
	// Продукт не использует таблицы маршрутизации
	// Проверяем, что пакет не был доставлен, но при этом
	// очередь работает корректно и у нас помечен отправляемый пакет
	if ((txStatus != TRANSMIT_COMPLETE_OK) && 
		((queue[queueLastSended].flags & (SNDBUF_NOROUTE_EXPLORE | SNDBUF_PROCESSED)) == (SNDBUF_NOROUTE_EXPLORE | SNDBUF_PROCESSED))) {
		// Скидываем эти флаги и обновляем опции передачи...
		queue[queueLastSended].flags 		&= 		~(SNDBUF_NOROUTE_EXPLORE | SNDBUF_PROCESSED);
		queue[queueLastSended].txOption 	=		TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_EXPLORE;
	} else
	#endif
	{			
		if (queue[queueLastSended].flags & SNDBUF_WITH_CALLBACK) {
			SendCallback(queue[queueLastSended].customID, txStatus);
		}
		#if WITH_FSM
		if (queue[queueLastSended].flags & SNDBUF_FSM_MANAGMENT) {
			QueueFastCleanup(queueLastSended);
			ZMEFSM_TriggerEvent(queue[queueLastSended].customID, (txStatus == TRANSMIT_COMPLETE_OK) ? FSM_EV_ZW_DATA_TRAN_ACK : FSM_EV_ZW_DATA_TRAN_FAILED, queue[queueLastSended].buf);	
		}
		#endif
	}

	switch (txStatus) {
		case TRANSMIT_COMPLETE_OK:
			#if defined(INFORM_SEND_DONE)
			INFORM_SEND_DONE;
			#endif
			break;
		default:
			#if defined(INFORM_SEND_FAILED)
			INFORM_SEND_FAILED;
			#endif
			break;
	}
	
	queueSending = FALSE;
	SendQueue();
}

#if WITH_CC_ASSOCIATION
void SendCompleteByExternalSend(void) {
	queueSending = FALSE;
	SendQueue();
	#if defined(CC_ASSOCIATION_CHECK_COMPLETE)
		CC_ASSOCIATION_CHECK_COMPLETE
	#endif
}
#endif

void SendQueue(void) reentrant {	
	BYTE current_packet;
	SENDBUFFER * pPacket;
	BYTE hasNonTestPacketsInQueue;
	BYTE prev;
	BYTE i;
	
	if (queueSending) // Два раза не запускать SendQueue
		return;

	hasNonTestPacketsInQueue = FALSE;
	current_packet = queueFirst;
	while (current_packet != UNALLOCATED_INDEX) {
		if (!(queue[current_packet].flags & (SNDBUF_TEST_FRAME | SNDBUF_PROCESSED)))
			hasNonTestPacketsInQueue = TRUE;
		current_packet = queue[current_packet].next;
	}
	
	queueLastSended = UNALLOCATED_INDEX;
	
	current_packet = queueFirst;
	while (current_packet != UNALLOCATED_INDEX) {
		pPacket = &queue[current_packet];
		
		// Удаляем уже обработанные пакеты
		if (pPacket->flags & SNDBUF_PROCESSED) {
      prev = current_packet;
      current_packet = queue[current_packet].next;	
      RemoveFromQueue(prev);
      continue;
		}

		if (!(pPacket->flags & SNDBUF_TEST_FRAME) || !hasNonTestPacketsInQueue) { // skip Test packets if there are others to send
			if (0) {}
			#if WITH_CC_ASSOCIATION
			else if (pPacket->flags & SNDBUF_GROUP)  {
					// Разворачиваем группу
					
					if (pPacket->dstID < AssocGetGroupsCount()) {
						ASSOCIATION_GROUPS * groups =  AssocGetGroupsPtr();
						BYTE nodeID;
						BYTE flags = 0, newTxOption = 0;
						BYTE pos = current_packet;

						#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
						BYTE not_mca = 2; // to repeat the loop for MCA after A
						while (--not_mca != 0xff)
						#endif
						{
							for (i=0; i < AssocGetGroupSize(); i++) {
								#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
								if (!not_mca)
									nodeID = groupsMI[pPacket->dstID].nodeIDEP[i].nodeID;
								else
								#endif
									nodeID = groups[pPacket->dstID].nodeID[i];
								
								if (nodeID != 0) {
									// Добавляем пакет для конкретного узла

									// packet might be MultiCommand - unfold it
									BYTE buf_indx = 0;
									BYTE buf_sz = pPacket->bufSize;

									#if WITH_CC_MULTI_COMMAND
									if (queue[current_packet].buf[0] == COMMAND_CLASS_MULTI_CMD && queue[current_packet].buf[1] == MULTI_CMD_ENCAP) {
										buf_indx = 4;
										buf_sz = queue[current_packet].buf[3];
									}
									#endif

									while (buf_indx < pPacket->bufSize) {
										if (CreatePacket() == NULL) // В очереди кончилось место
											break;
										// Немного избыточно, но просто - копируем текущий пакет целиком
										prev = queueAllocIndex; // запомнить только что выделенный пакет
										memcpy(&queue[queueAllocIndex], pPacket, sizeof(SENDBUFFER));
										if (buf_indx) { // move payload of encapsulated packet
												memcpy(queue[queueAllocIndex].buf, queue[queueAllocIndex].buf + buf_indx, buf_sz);
										}

										flags = 0;

										#if WITH_CC_SECURITY
										{
											BYTE go_out = FALSE;
											switch (SecurityOutgoingPacketsPolicy(nodeID, queue[queueAllocIndex].buf[0], ASKED_UNSOLICITED)) {
												case PACKET_POLICY_DROP:
													{
														go_out = TRUE;
													}
													break;
												case PACKET_POLICY_SEND_ENCRYPT:
													flags = SNDBUF_SECURITY;
													break;
												case PACKET_POLICY_SEND_PLAIN:
													// nothing to do here
													break;
											}

											if(go_out)
												break;

										}
										#endif // WITH_CC_SECURITY

										UnsetPacketFlags(SNDBUF_GROUP); // unfolded packet is not to group

										#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
										if (!not_mca) {
											MULTICHANNEL_ENCAP_IS_ENABLED  = TRUE;
											MULTICHANNEL_SRC_END_POINT     = groupsMI[pPacket->dstID].nodeIDEP[i].endPoint;
											MULTICHANNEL_DST_END_POINT     = 0;

											if ((queue[queueAllocIndex].buf[0] == COMMAND_CLASS_MULTI_CHANNEL_V3) && 
												(queue[queueAllocIndex].buf[1] == MULTI_CHANNEL_CMD_ENCAP_V3)) {
												queue[queueAllocIndex].buf[3] = groupsMI[pPacket->dstID].nodeIDEP[i].endPoint;
											}

										}
										#endif

										EnqueuePacketFPos(pos, nodeID, buf_sz, newTxOption, flags);
										
										#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
										if (!not_mca) {
											MULTICHANNEL_ENCAP_IS_ENABLED  = FALSE;
										}
										#endif

										pos = prev;
										
										buf_indx += buf_sz + 1;
										if (buf_indx < pPacket->bufSize) { // not to access beyond last byte - not critical, but QueueUnitTest will complain
											buf_sz = queue[current_packet].buf[buf_indx - 1];
										}
									}
								}
							}
						}
					}
					
					// Помечаем пакет как обработанный
					pPacket->flags |= SNDBUF_PROCESSED;
					
					continue; // повторим цикл с этим пакетом, чтоб удалить его сборщиком мусора
			}
			#endif
			#if WITH_CC_SECURITY
			else if (pPacket->flags & SNDBUF_SECURITY) {	
					// Пробуем выделить автомат
					if (SecurityAllocFSMSender(pPacket->dstID, pPacket->bufSize, (BYTE*)pPacket->buf, pPacket->txOption) != UNALLOCATED_INDEX) {
						// Получилось - пакет обработан. Теперь за него отвечает КА
						pPacket->flags |= SNDBUF_PROCESSED;
						current_packet = queueFirst; // пакет вставлен в начало очереди - начнём сначала
						continue;
					}
					// Оставляем пакет где он был - попробуем исполнить позже...
					// ? Нужно время жизни пакета
			}
			#endif
			else {
				if (pPacket->flags & SNDBUF_TEST_FRAME) {
					queueSending = ZW_SEND_TEST_FRAME(pPacket->dstID,  pPacket->buf[0], CBK_POINTER(SendComplete));
				} else if (pPacket->flags & SNDBUF_NIF) {	
					queueSending = ZW_SEND_NODE_INFO(pPacket->dstID, pPacket->txOption, CBK_POINTER(SendComplete));
				} else {
					queueSending = ZW_SEND_DATA(pPacket->dstID, pPacket->buf, pPacket->bufSize, pPacket->txOption, CBK_POINTER(SendComplete));
					// For Explorer Frame packet we want to start a timer to stop it earlier and save battery
					#if EXPLORER_FRAME_TIMOUT
						#if EXPLORER_FRAME_TIMOUT > 254
							#error "EXPLORER_FRAME_TIMOUT must be from 1 to 254 or 0 (unlimited)"
						#endif
						if (pPacket->txOption & TRANSMIT_OPTION_EXPLORE) {
							abortExplorerFrameSendTimout = EXPLORER_FRAME_TIMOUT + 1;
						}
					#endif
				}
				pPacket->flags |= SNDBUF_PROCESSED;

				#if WITH_FSM
				if (pPacket->flags & SNDBUF_FSM_MANAGMENT) {
					ZMEFSM_TriggerEvent(pPacket->customID, (queueSending) ? FSM_EV_ZW_DATA_SENDED : FSM_EV_ZW_DATA_FAILED, pPacket->buf);	
				}
				#endif

				if (queueSending) {	
					queueLastSended = current_packet;
				} else {
					#if defined(INFORM_SEND_FAILED)
					INFORM_SEND_FAILED;
					#endif
				}
			}
		}

		if (queueSending)
				return;
		
		// Обрабатываем следующий....
		current_packet = queue[current_packet].next;
	}
}

BOOL QueueHasPacketsForNode(BYTE nodeId, BYTE flags)
{
	BYTE current_packet;
	
	for(current_packet = queueFirst; current_packet != UNALLOCATED_INDEX; current_packet = queue[current_packet].next)
	{
		// Не рассматриваем уже обработанные пакеты
		if(queue[current_packet].flags & SNDBUF_PROCESSED)
			continue;

		if((queue[current_packet].dstID == nodeId)&&(queue[current_packet].flags &  flags))
			return TRUE;
	}

	return FALSE;
}

// Returns pointer to first (FIFO) free buffer for a packet to be sent to a node or NULL if queue is full
ZW_APPLICATION_TX_BUFFER * CreatePacket(void) {
	BYTE i;
	
	if(queueAllocIndex != UNALLOCATED_INDEX)
		queue[queueAllocIndex].bufSize = 0;

	queueAllocIndex = UNALLOCATED_INDEX;
	
	for (i=0; i<QUEUE_SIZE; i++)
		if (queue[i].bufSize == 0) {
			queueAllocIndex = i;
			txBuf	=	(ZW_APPLICATION_TX_BUFFER *)&(queue[i].buf);
			 // FillPacket will add flags using OR
			queue[queueAllocIndex].flags = 0;
			return txBuf;
		}
			
	return NULL;	
}

void SetPacketFlags(BYTE flags) {
	queue[queueAllocIndex].flags |= flags;
}

static void UnsetPacketFlags(BYTE flags) {
	queue[queueAllocIndex].flags &= ~flags;
}

BYTE QueueSize(void) {
	return queueLen;
}

