#include "ZME_Includes.h"

#if WITH_CC_SECURITY
#warning CHECK "Составные пакеты неверно обрабатываются."

#include "ZME_Security.h"
#include "ZME_FSM.h"
#include "ZME_Queue.h"

#include <ZW_aes_api.h>

#define ZMESC_GEN_NONCE(B) genRNDNonce(B)// genSimpleNonce(B)

#define ZMESC_AES_ECB(K, I, O) AES_ECB_Hardware(K, I, O)

// Encrupted packet structure
#define SEC_FIRST_PART_SIZE         10
#define SEC_SECOND_PART_SIZE        9
#define SEC_EMPTY_PACKET_SIZE       (SEC_FIRST_PART_SIZE + SEC_SECOND_PART_SIZE)
#define SEC_DATA_SIZE(L)            (L - SEC_EMPTY_PACKET_SIZE)
#define SEC_GET_RI(D, L)            D[SEC_FIRST_PART_SIZE + SEC_DATA_SIZE(L)]                       
#define SEC_GET_MAC(D, L)           &(D[SEC_FIRST_PART_SIZE + SEC_DATA_SIZE(L) + 1])                        
#define SEC_GET_IV(D)               &(D[2])
#define SEC_GET_DATA(D)             &(D[10])
#define SEC_LAST_PACKET(P)          (((P & 0x18) == 0)||((P & 0x18) == 0x18))
                        
#define AES_DATA_LENGTH                 16
#define AES_DATA_HLENGTH                8

XBYTE aes_tmp[AES_DATA_LENGTH * 3] _at_ 2048; // these three arrays MUST be in lower 4k
#define aes_tmp_key   				(&aes_tmp[AES_DATA_LENGTH * 0])
#define aes_tmp_input 				(&aes_tmp[AES_DATA_LENGTH * 1])
#define aes_tmp_output				(&aes_tmp[AES_DATA_LENGTH * 2])

#define RND_HW_ITER                     2

#define MAX_SECURITY_SINGLEPACK             (40 - 2 - 8 - 1 - 1 - 8) // maximum size of single secure packet = Explorer Fram packet (40 byes) - size of Security CC overhead
#define SECURITY_SCHEME_0_MASK          0x01
#define ZME_SECURITY_SUPPORTED_SCHEMES	0x00 // the only supported security scheme is SCHEME_0 is bit 0 is not 1

enum {
    SEC_CONTEXT_EMPTY=0x00,
    SEC_CONTEXT_TRANSMITTER,
    SEC_CONTEXT_RECEIVER
    #ifdef ZW_CONTROLLER
    ,SEC_CONTEXT_CONTROLLER_INCLUDE 
    #endif

};

typedef struct _SECURITY_CONTEXT_ {
    BYTE nodeId; // == 0xff - пустая запись
    BYTE fsmId;
    BYTE contextType;
} SECURITY_CONTEXT;

#define INACTIVE_STRUCT                 0xFF
    
#define MAX_FRAME_FLAT_DATA             56
#define MAX_FRAME_ENCRYPTED_DATA        28

BYTE untrackedNonceRequests[NODE_MASK_LENGTH];

typedef struct _SECURITY_SENDER_DATA_ {
    BYTE size; // == 0xff - пустая запись
    BYTE flat_data[MAX_FRAME_FLAT_DATA];
    BYTE txOptions;
    BYTE crypted_offset;
} SECURITY_SENDER_DATA;

typedef struct _SECURITY_RECEIVER_DATA_ {
    BYTE size; // == 0xff - пустая запись
    BYTE flat_data[MAX_FRAME_FLAT_DATA];
    BYTE offset;
} SECURITY_RECEIVER_DATA;

#define NONCE_LIVETIME              30 // in 100ms

typedef struct _SECURITY_NONCE_ {
    BYTE nonce[AES_DATA_HLENGTH];
    WORD remains; // == 0 - пустой (невалидный)
} SECURITY_NONCE;

BYTE g_encrypt_buff[AES_DATA_LENGTH];
BYTE g_KaKey[AES_DATA_LENGTH];
BYTE g_KeKey[AES_DATA_LENGTH];
BYTE g_IV[AES_DATA_LENGTH];
BYTE g_TempBuff[MAX_FRAME_FLAT_DATA];
BYTE g_RandState[AES_DATA_LENGTH];

BYTE fsmInclusionId;
BYTE fsmInclusionControllerNodeId;

BYTE askedSecurely; // used to track if packed was asked securely or not (used by EnqueuePacket, group send operation and Security test)

#define MAX_SECURITY_CONTEXTS           (MAX_DATA_SENDERS + MAX_DATA_RECEIVERS) // number of security contexts to keep

SECURITY_CONTEXT        g_security_context[MAX_SECURITY_CONTEXTS];
SECURITY_SENDER_DATA    g_security_senderdata[MAX_DATA_SENDERS];
SECURITY_RECEIVER_DATA  g_security_receiverdata[MAX_DATA_RECEIVERS];
SECURITY_NONCE          g_security_nonces[MAX_NONCES];   

// Функции из других модулей
BYTE* SecurityKeyPtr(void);
BYTE* SecurityModePtr(void);
void SecureNodeListAdd(BYTE nodeId);
void SecureNodeListRemove(BYTE nodeId);
void SecureNodeListZero(void);
void SecureTestedNodeListAdd(BYTE nodeId);
void SecureTestedNodeListRemove(BYTE nodeId);
BOOL IsNodeTested(BYTE nodeId);
BYTE SecureNIFCopy(BYTE * dst);
void SecurityStoreAll(void);
void SecurityClearAll(void);
BYTE SecurityNodeId(void);
void SetSecureNIF(void);
void SetUnsecureNIF(void);
BOOL SecurityIncomingSecurePacketsPolicy(BYTE classcmd);
BYTE SecurityOutgoingPacketsPolicy(BYTE nodeId, BYTE cmdclass, BYTE askedSecurely) ;

void __ApplicationCommandHandler(BYTE rxStatus, BYTE sourceNode,  ZW_APPLICATION_TX_BUFFER *pCmd, BYTE cmdLength) reentrant;



/******
                                     Конечные автоматы
******/

// Автомат для включения в безопасную сеть
enum {
    FSMSI_Init=0x01,
    FSMSI_WaitForSchemeGet,
    FSMSI_SendSchemeReport,
    FSMSI_WaitForNonceGet,
    FSMSI_SendNonceReport,
    FSMSI_WaitForKey,
    FSMSI_SendNonceGet,
    FSMSI_WaitForNonceReport,
    FSMSI_SendKeyVerify,
    FSMSI_Done,
    FSMSI_Failed,   
    FSMSI_Destroy
} fsm_security_includer_states;

enum {
    FSMSI_E_Start=0x01,
    FSMSI_E_SchemeGetReceived,
    FSMSI_E_SchemeGetTimeout,
    FSMSI_E_NonceGetReceived,
    FSMSI_E_SchemeNotSupported,
    FSMSI_E_NonceGetTimeout,
    FSMSI_E_KeyReceived,
    FSMSI_E_KeyTimeout,
    FSMSI_E_NonceReportReceived,
    FSMSI_E_NonceReportTimeout,
    FSMSI_E_Release         
} fsm_security_includer_events;

#define MAX_FSM_SECURITY_INCLUDER_RULES                                                 20
FSM_RULE  fsm_security_includer_rules[MAX_FSM_SECURITY_INCLUDER_RULES] = {          
    //  State1        ->        State2                      Event                                           
    {FSMSI_Init,                FSMSI_WaitForSchemeGet,     FSMSI_E_Start               }, // 0
    {FSMSI_WaitForSchemeGet,    FSMSI_Failed,               FSMSI_E_SchemeGetTimeout    }, // 1
    {FSMSI_WaitForSchemeGet,    FSMSI_SendSchemeReport,     FSMSI_E_SchemeGetReceived   }, // 2
    {FSMSI_SendSchemeReport,    FSMSI_WaitForNonceGet,      FSM_EV_ZW_DATA_SENDED       }, // 3
    {FSMSI_SendSchemeReport,    FSMSI_Failed,               FSM_EV_ZW_DATA_FAILED       }, // 4
    {FSMSI_SendSchemeReport,    FSMSI_Failed,               FSMSI_E_SchemeNotSupported  }, // 5
    {FSMSI_WaitForNonceGet,     FSMSI_Failed,               FSMSI_E_NonceGetTimeout     }, // 6
    {FSMSI_WaitForNonceGet,     FSMSI_SendNonceReport,      FSMSI_E_NonceGetReceived    }, // 7
    {FSMSI_SendNonceReport,     FSMSI_WaitForKey,           FSM_EV_ZW_DATA_SENDED       }, // 8
    {FSMSI_SendNonceReport,     FSMSI_Failed,               FSM_EV_ZW_DATA_FAILED       }, // 9
    {FSMSI_WaitForKey,          FSMSI_Failed,               FSMSI_E_KeyTimeout          }, // 10
    {FSMSI_WaitForKey,          FSMSI_SendNonceGet,         FSMSI_E_KeyReceived         }, // 11
    {FSMSI_SendNonceGet,        FSMSI_WaitForNonceReport,   FSM_EV_ZW_DATA_SENDED       }, // 12
    {FSMSI_SendNonceGet,        FSMSI_Failed,               FSM_EV_ZW_DATA_FAILED       }, // 13
    {FSMSI_WaitForNonceReport,  FSMSI_Failed,               FSMSI_E_NonceReportTimeout  }, // 14
    {FSMSI_WaitForNonceReport,  FSMSI_SendKeyVerify,        FSMSI_E_NonceReportReceived }, // 15
    {FSMSI_SendKeyVerify,       FSMSI_Done,                 FSM_EV_ZW_DATA_SENDED       }, // 16
    {FSMSI_SendKeyVerify,       FSMSI_Failed,               FSM_EV_ZW_DATA_FAILED       }, // 17
    {FSMSI_Failed,              FSMSI_Destroy,              FSMSI_E_Release             }, // 18
    {FSMSI_Done,                FSMSI_Destroy,              FSMSI_E_Release             }  // 19    
};

#define MAX_FSM_SECURITY_INCLUDER_TIMEOUTS                              6
FSM_TIMEOUT_TRIGGER fsm_security_includer_timeouts[MAX_FSM_SECURITY_INCLUDER_TIMEOUTS] = {
    //  State                   Event                       TimeoutInterval (*100ms)
    {FSMSI_WaitForSchemeGet,    FSMSI_E_SchemeGetTimeout,   100 }, 
    {FSMSI_WaitForNonceGet,     FSMSI_E_NonceGetTimeout,    100 },
    {FSMSI_WaitForKey,          FSMSI_E_KeyTimeout,         100 },
    {FSMSI_WaitForNonceReport,  FSMSI_E_NonceReportTimeout, 100 },
    {FSMSI_Done,                FSMSI_E_Release,            SWITCH_TO_NEXT_TIMEOUT  },
    {FSMSI_Failed,              FSMSI_E_Release,            SWITCH_TO_NEXT_TIMEOUT  }
};

void fsmSecurityIncluderHandler(FSM_ACTION * action) reentrant;
code const FSMActionFunc_t fsmSecurityIncluderHandler_p = &fsmSecurityIncluderHandler;

// Автомат для посылки зашифрованных сообщений
enum {
    FSMSS_Init=0x01,
    FSMSS_GetNonce,
    FSMSS_WaitForAck,
    FSMSS_WaitForNonce,
    FSMSS_CheckPolicy,
    FSMSS_SendPlain,
    FSMSS_CheckData,
    FSMSS_SendSeqData,
    FSMSS_SendFinalData,
    FSMSS_Failed,
    FSMSS_Done,
    FSMSS_Destroy   
} fsm_security_sender_states;

enum {
    FSMSS_E_Start=0x01,
    FSMSS_E_StartNonceRequested,
    FSMSS_E_AckTimeout,
    FSMSS_E_Plain,
    FSMSS_E_Secure,
    FSMSS_E_NonceTimeout,
    FSMSS_E_NonceReceived,
    FSMSS_E_DataRemains,
    FSMSS_E_DataPrepared,
    FSMSS_E_Release,
    FSMSS_E_SendTimeout
} fsm_security_sender_events;

#define MAX_FSM_SECURITY_SENDER_RULES                                           20
FSM_RULE  fsm_security_sender_rules[MAX_FSM_SECURITY_SENDER_RULES] = {          
    //  State1       ->     State2                  Event                                           
    {FSMSS_Init,            FSMSS_GetNonce,         FSMSS_E_Start               },
    {FSMSS_Init,            FSMSS_WaitForNonce,     FSMSS_E_StartNonceRequested },
    {FSMSS_GetNonce,        FSMSS_WaitForAck,       FSM_EV_ZW_DATA_SENDED       },
    {FSMSS_GetNonce,        FSMSS_Failed,           FSM_EV_ZW_DATA_FAILED       },
    {FSMSS_WaitForAck,      FSMSS_WaitForNonce,     FSM_EV_ZW_DATA_TRAN_ACK     },
    {FSMSS_WaitForAck,      FSMSS_Failed,           FSM_EV_ZW_DATA_TRAN_FAILED  },
    {FSMSS_WaitForNonce,    FSMSS_CheckPolicy,      FSMSS_E_NonceTimeout        },
    {FSMSS_WaitForNonce,    FSMSS_CheckData,        FSMSS_E_NonceReceived       },
    {FSMSS_CheckPolicy,     FSMSS_SendPlain,        FSMSS_E_Plain               },
    {FSMSS_CheckPolicy,     FSMSS_Failed,           FSMSS_E_Secure              },
    {FSMSS_SendPlain,       FSMSS_Done,             FSM_EV_ZW_DATA_SENDED       },
    {FSMSS_SendPlain,       FSMSS_Failed,           FSM_EV_ZW_DATA_FAILED       },
    {FSMSS_CheckData,       FSMSS_SendSeqData,      FSMSS_E_DataRemains         },
    {FSMSS_CheckData,       FSMSS_SendFinalData,    FSMSS_E_DataPrepared        },
    {FSMSS_SendSeqData,     FSMSS_WaitForNonce,     FSM_EV_ZW_DATA_SENDED       },
    {FSMSS_SendSeqData,     FSMSS_Failed,           FSM_EV_ZW_DATA_FAILED       },
    {FSMSS_SendFinalData,   FSMSS_Done,             FSM_EV_ZW_DATA_SENDED       },
    {FSMSS_SendFinalData,   FSMSS_Failed,           FSM_EV_ZW_DATA_FAILED       },
    {FSMSS_Done,            FSMSS_Destroy,          FSMSS_E_Release             },
    {FSMSS_Failed,          FSMSS_Destroy,          FSMSS_E_Release             }
};

#define MAX_FSM_SECURITY_SENDER_TIMEOUTS                                        3//4
FSM_TIMEOUT_TRIGGER fsm_security_sender_timeouts[MAX_FSM_SECURITY_SENDER_TIMEOUTS] = {
    //  State                   Event                   TimeoutInterval (*100ms)
    {FSMSS_WaitForNonce,        FSMSS_E_NonceTimeout,   200                     },
    //{FSMSS_WaitForAck,          FSMSS_E_AckTimeout,     100                     },
    {FSMSS_Done,                FSMSS_E_Release,        SWITCH_TO_NEXT_TIMEOUT  },
    {FSMSS_Failed,              FSMSS_E_Release,        SWITCH_TO_NEXT_TIMEOUT  }
};

void fsmSecuritySenderHandler(FSM_ACTION * action) reentrant;
code const FSMActionFunc_t fsmSecuritySenderHandler_p = &fsmSecuritySenderHandler;

// Автомат для получения зашифрованных сообщений
enum {
    FSMSR_Init=0x01,
    FSMSR_SendNonceReport,
    FSMSR_WaitForData,
    FSMSR_CheckData,
    FSMSR_Failed,
    FSMSR_Done,
    FSMSR_Destroy   
} fsm_security_receiver_states;

enum {
    FSMSR_E_Start=0x01,
    FSMSR_E_DataTimeout,
    FSMSR_E_DataReceived,
    FSMSR_E_NextData,
    FSMSR_E_FinalData,
    FSMSR_E_InvalidData,
    FSMSR_E_Release
} fsm_security_receiver_events;

#define MAX_FSM_SECURITY_RECEIVER_RULES                                     10
FSM_RULE  fsm_security_receiver_rules[MAX_FSM_SECURITY_RECEIVER_RULES] = {
    //  State1       ->     State2                  Event                                           
    {FSMSR_Init,            FSMSR_SendNonceReport,  FSMSR_E_Start               },
    {FSMSR_SendNonceReport, FSMSR_WaitForData,      FSM_EV_ZW_DATA_SENDED       },
    {FSMSR_SendNonceReport, FSMSR_Failed,           FSM_EV_ZW_DATA_FAILED       },
    {FSMSR_WaitForData,     FSMSR_CheckData,        FSMSR_E_DataReceived        },
    {FSMSR_WaitForData,     FSMSR_Failed,           FSMSR_E_DataTimeout         },
    {FSMSR_CheckData,       FSMSR_Done,             FSMSR_E_FinalData           },
    {FSMSR_CheckData,       FSMSR_SendNonceReport,  FSMSR_E_NextData            },
    {FSMSR_CheckData,       FSMSR_Failed,           FSMSR_E_InvalidData         },
    {FSMSR_Done,            FSMSR_Destroy,          FSMSR_E_Release             },
    {FSMSR_Failed,          FSMSR_Destroy,          FSMSR_E_Release             }
};

#define MAX_FSM_SECURITY_RECEIVER_TIMEOUTS                                  3
FSM_TIMEOUT_TRIGGER fsm_security_receiver_timeouts[MAX_FSM_SECURITY_RECEIVER_TIMEOUTS] = {
    //  State                   Event                   TimeoutInterval (*100ms)
    {FSMSR_WaitForData,         FSMSR_E_DataTimeout,    200 },
    {FSMSR_Done,                FSMSR_E_Release,        SWITCH_TO_NEXT_TIMEOUT  },
    {FSMSR_Failed,              FSMSR_E_Release,        SWITCH_TO_NEXT_TIMEOUT  }
};

void fsmSecurityReceiverHandler(FSM_ACTION * action) reentrant;
code const FSMActionFunc_t fsmSecurityReceiverHandler_p = &fsmSecurityReceiverHandler;


#ifdef ZW_CONTROLLER 

// Автомат для включения контроллером в безопасную сеть
enum {
    FSMSCI_Init=0x01,
    FSMSCI_SendSchemeGet,
    FSMSCI_WaitForSchemeReport,
    FSMSCI_SendNonceGet,
    FSMSCI_WaitForNonceReport,
    FSMSCI_SendKeySet,
    FSMSCI_WaitForNonceGet,
    FSMSCI_SendNoceReport,
    FSMSCI_WaitForKeyVerify,
    FSMSCI_CheckKey,
    FSMSCI_Done,
    FSMSCI_Failed,   
    FSMSCI_Destroy
} fsm_security_controller_inclusion_states;

enum {
    FSMSCI_E_Start=0x01,
    FSMSCI_E_ScemeReportReceived,
    FSMSCI_E_ScemeReportTimeout,  
    FSMSCI_E_NonceReportReceived,
    FSMSCI_E_NonceReportTimeout,
    FSMSCI_E_NonceGetReceived,
    FSMSCI_E_NonceGetTimeout,
    FSMSCI_E_KeyVerifyReceived,
    FSMSCI_E_KeyVerifyTimeout,
    FSMSCI_E_KeyValid,
    FSMSCI_E_KeyInvalid,
    FSMSCI_E_Release         
} fsm_security_controller_inclusion_events;

#define MAX_FSM_SECURITY_CONTROLLER_INCLUDE_RULES                                     21

FSM_RULE  fsm_security_controller_include_rules[MAX_FSM_SECURITY_CONTROLLER_INCLUDE_RULES] = {          
    //  State1        ->        State2                          Event                                           
    {FSMSCI_Init,                   FSMSCI_SendSchemeGet,           FSMSCI_E_Start                  }, // 0
    {FSMSCI_SendSchemeGet,          FSMSCI_WaitForSchemeReport,     FSM_EV_ZW_DATA_SENDED           }, // 1
    {FSMSCI_SendSchemeGet,          FSMSCI_Failed,                  FSM_EV_ZW_DATA_FAILED           }, // 2
    {FSMSCI_WaitForSchemeReport,    FSMSCI_SendNonceGet,            FSMSCI_E_ScemeReportReceived    }, // 3
    {FSMSCI_WaitForSchemeReport,    FSMSCI_Failed,                  FSMSCI_E_ScemeReportTimeout     }, // 4
    {FSMSCI_SendNonceGet,           FSMSCI_WaitForNonceReport,      FSM_EV_ZW_DATA_SENDED           }, // 5
    {FSMSCI_SendNonceGet,           FSMSCI_Failed,                  FSM_EV_ZW_DATA_FAILED           }, // 6
    {FSMSCI_WaitForNonceReport,     FSMSCI_SendKeySet,              FSMSCI_E_NonceReportReceived    }, // 7
    {FSMSCI_WaitForNonceReport,     FSMSCI_Failed,                  FSMSCI_E_NonceReportTimeout     }, // 8
    {FSMSCI_SendKeySet,             FSMSCI_WaitForNonceGet,         FSM_EV_ZW_DATA_SENDED           }, // 9
    {FSMSCI_SendKeySet,             FSMSCI_Failed,                  FSM_EV_ZW_DATA_FAILED           }, // 10
    {FSMSCI_WaitForNonceGet,        FSMSCI_SendNoceReport,          FSMSCI_E_NonceGetReceived       }, // 11
    {FSMSCI_WaitForNonceGet,        FSMSCI_Failed,                  FSMSCI_E_NonceGetTimeout        }, // 12
    {FSMSCI_SendNoceReport,         FSMSCI_WaitForKeyVerify,        FSM_EV_ZW_DATA_SENDED           }, // 13
    {FSMSCI_SendNoceReport,         FSMSCI_Failed,                  FSM_EV_ZW_DATA_FAILED           }, // 14
    {FSMSCI_WaitForKeyVerify,       FSMSCI_CheckKey,                FSMSCI_E_KeyVerifyReceived      }, // 15
    {FSMSCI_WaitForKeyVerify,       FSMSCI_Failed,                  FSMSCI_E_KeyVerifyTimeout       }, // 16
    {FSMSCI_CheckKey,               FSMSCI_Done,                    FSMSCI_E_KeyValid               }, // 17
    {FSMSCI_CheckKey,               FSMSCI_Failed,                  FSMSCI_E_KeyInvalid             }, // 18
    {FSMSCI_Failed,                 FSMSCI_Destroy,                 FSMSCI_E_Release                }, // 19
    {FSMSCI_Done,                   FSMSCI_Destroy,                 FSMSCI_E_Release                }  // 20    
};

#define MAX_FSM_SECURITY_CONTROLLER_INCLUDE_TIMEOUTS                              6

FSM_TIMEOUT_TRIGGER fsm_security_controller_include_timeouts[MAX_FSM_SECURITY_CONTROLLER_INCLUDE_TIMEOUTS] = {
    //  State                   Event                       TimeoutInterval (*100ms)
    {FSMSCI_WaitForSchemeReport,    FSMSCI_E_ScemeReportTimeout,    1200                    },
    {FSMSCI_WaitForNonceReport,     FSMSCI_E_NonceReportTimeout,    200                     },
    {FSMSCI_WaitForNonceGet,        FSMSCI_E_NonceGetTimeout,       200                     },
    {FSMSCI_WaitForKeyVerify,       FSMSCI_E_KeyVerifyTimeout,      200                     },
    {FSMSCI_Done,                   FSMSCI_E_Release,               SWITCH_TO_NEXT_TIMEOUT  },
    {FSMSCI_Failed,                 FSMSCI_E_Release,               SWITCH_TO_NEXT_TIMEOUT  }
};

void fsmSecurityControllerIncludeHandler(FSM_ACTION * action) reentrant;
code const FSMActionFunc_t fsmSecurityCotrollerIncludeHandler_p = &fsmSecurityControllerIncludeHandler;

BYTE g_ControllerIncluding = FALSE; 
#endif



// Список всех используемых моделей КА
#ifdef ZW_CONTROLLER 
    #define MAX_SC_FSM_MODELS       4
#else
    #define MAX_SC_FSM_MODELS       3
#endif

// Индексы моделей КА в списке
#define SC_FSM_INCLUDER                   0
#define SC_FSM_SENDER                     1
#define SC_FSM_RECEIVER                   2

#ifdef ZW_CONTROLLER
#define SC_FSM_CONTROLLER_INCLUDE         3 
#endif



FSM_MODEL security_fsm_models[MAX_SC_FSM_MODELS]= {
    // Автомат включения в сеть
    {   
        fsm_security_includer_rules,    MAX_FSM_SECURITY_INCLUDER_RULES,
        fsm_security_includer_timeouts, MAX_FSM_SECURITY_INCLUDER_TIMEOUTS,
        0, //fsmSecurityIncluderHandler_p,
        FSMSI_Init, FSMSI_Destroy, FSMSI_Failed
    },
    
    // Автомат отправки сообщений
    {   
        fsm_security_sender_rules,      MAX_FSM_SECURITY_SENDER_RULES, 
        fsm_security_sender_timeouts,   MAX_FSM_SECURITY_SENDER_TIMEOUTS,
        0, //fsmSecuritySenderHandler_p,
        FSMSS_Init, FSMSS_Destroy, FSMSS_Failed
    },
    
    // Автомат получения сообщений
    {
        fsm_security_receiver_rules,    MAX_FSM_SECURITY_RECEIVER_RULES,
        fsm_security_receiver_timeouts, MAX_FSM_SECURITY_RECEIVER_TIMEOUTS,
        0, //fsmSecurityReceiverHandler_p,
        FSMSR_Init, FSMSR_Destroy, FSMSR_Failed
    }

    #ifdef ZW_CONTROLLER
    ,
    {
        fsm_security_controller_include_rules,    MAX_FSM_SECURITY_CONTROLLER_INCLUDE_RULES,
        fsm_security_controller_include_timeouts,  MAX_FSM_SECURITY_CONTROLLER_INCLUDE_TIMEOUTS,
        0, //fsmSecurityCotrollerIncludeHandler_p,
        FSMSCI_Init, FSMSCI_Destroy, FSMSCI_Failed

    }
    #endif

};

// ------------------------------------------------------------------------
// DBG ОТЛАДКА
// ------------------------------------------------------------------------
#if SECURITY_DEBUG

BYTE dbgSecurityPulses = 0;
// Прототипы из ZME_Commmon.c
void SendReportToNode(BYTE dstNode, BYTE classcmd, BYTE reportparam);
void SendDataReportToNode(BYTE dstNode, BYTE classcmd, BYTE cmd, BYTE * reportdata, BYTE numdata);
void dbgSendSecurityMasks();

BOOL dbgSecurityEchoFilter(BYTE source, BYTE * d, BYTE cmdlen)
{
    if(d[0] == SECURITY_DEBUG_CC_ECHO && cmdlen > SECURITY_DEBUG_CC_ECHO_SIZE)
    {
        BYTE data_count = cmdlen-2;
        switch(d[2])
        {
            case 0x00:
                // Тестируем инкапсуляцию
                // Объединение только в случае data_count <= 28
                // Создаем два собщения 1-ое короткое
                // Создаем два собщения 2-ое исходной длины
                SendDataReportToNode(source, d[0], d[1], &d[1], 1);
                SendDataReportToNode(source, d[0], d[1], &d[2], data_count);
                break;
            case 0x02:
                {
                    BYTE i;
                    // Тестируем инкапсуляцию
                    // Много коротких команд
                    for(i=0; i<data_count; i++)
                        SendDataReportToNode(source, d[0], d[1], &d[i], 1);
                }
                break;
            default:    
                // Обычное эхо
                SendDataReportToNode(source, d[0], d[1], &d[2], cmdlen-2);
        }
        return TRUE;
    }
    return FALSE;
}

static BYTE countAvailiableStructs(BYTE * list, BYTE element_size, BYTE maxcount) {
    BYTE i;
    BYTE count = 0;
    for (i = 0; i < maxcount; i++, list += element_size)
        if(list[0] != INACTIVE_STRUCT)
            count++;
            
    return count;
}
static BYTE countActiveNonces(void) {

    BYTE i;
    BYTE count = 0;

    for (i = 0; i < MAX_NONCES; i++)
        if(g_security_nonces[i].remains != 0)
            count++;
            
    return count;
}
static void stateOfActiveFSM(BYTE * edge)
{

}
static void dbgDumpSecurityState()
{
    BYTE dumpdata[8];
    BYTE fsm_id;

    dumpdata[0] =   0xfa;
    dumpdata[1] =   ZMEFSM_CalcActiveFSMS();
    dumpdata[2] =   countAvailiableStructs((BYTE*)g_security_context,       sizeof(SECURITY_CONTEXT),       MAX_SECURITY_CONTEXTS);
    dumpdata[3] =   countAvailiableStructs((BYTE*)g_security_senderdata,    sizeof(SECURITY_SENDER_DATA),   MAX_DATA_SENDERS);
    dumpdata[4] =   countAvailiableStructs((BYTE*)g_security_receiverdata,  sizeof(SECURITY_RECEIVER_DATA), MAX_DATA_RECEIVERS);
    dumpdata[5] =   countActiveNonces();
    dumpdata[6] =   QueueSize();
    dumpdata[7] =   0;

    fsm_id = getFirstActiveFSM();
    if(fsm_id != INVALID_FSM_ID)
        dumpdata[7] = ZMEFSM_FSMState(fsm_id);

    SendDataReportToNode(1, COMMAND_CLASS_SECURITY, 0xde, dumpdata, sizeof(dumpdata));

}
/**
    genSimpleNonce Это только тестовый вариант гененрации псевдослучайного nonce!
**/
/*
void genSimpleNonce(BYTE * nonce) {
    BYTE i;
    
    for (i=0; i < AES_DATA_HLENGTH; i++)
        nonce[i] = ZW_Random();
}
*/
#endif
// ------------------------------------------------------------------------



/**
    AES_ECB_Hardware Реализация AES на чипе 5-го поколения
**/
void AES_ECB_Hardware(BYTE * key, BYTE * input, BYTE * output) {    
    memcpy(aes_tmp_key, key, AES_DATA_LENGTH);
    memcpy(aes_tmp_input, input, AES_DATA_LENGTH);
    ZW_AES_ECB(aes_tmp_key, aes_tmp_input, aes_tmp_output);
    memcpy(output, aes_tmp_output, AES_DATA_LENGTH);
}


/**
    Генерация псевдослучайных чисел
    Полноценная версия
    Адаптировано из ProductPlus сигмы
**/

void reqRND(BYTE *pData, BYTE count) {
  for(;count>0;count--, pData+=2)
    ZW_GetRandomWord((BYTE *) (pData));
  
}
void updateRND(void) {
  BYTE h[AES_DATA_LENGTH];
  BYTE k[AES_DATA_LENGTH];
  BYTE i,j;
  
  // H = 0xA5 (x16) 
  memset(h, 0xA5, AES_DATA_LENGTH);
  
  ZW_SetRFReceiveMode(FALSE);

  // Несколько итераций настоящего случайного генератора
  for(i=0; i<RND_HW_ITER; i++)
  {
      reqRND(k, AES_DATA_LENGTH);

      AES_ECB_Hardware(k, h, g_TempBuff);
      for(j=0; j<AES_DATA_LENGTH; j++)
        h[j] ^= g_TempBuff[j];
  } 
  ZW_SetRFReceiveMode(TRUE);

  // Внутреннее состояние S = S ^ H 
  for(i=0; i<AES_DATA_LENGTH; i++)
        g_RandState[i] ^= h[i];

  memset(g_TempBuff, 0x36, AES_DATA_LENGTH);

  AES_ECB_Hardware(g_RandState, g_TempBuff, g_RandState);
}
void genRNDNonce(BYTE * nonce) {

  // Генерация выходной последовательности
  memset(g_TempBuff, 0x5C/*0xA5*/, AES_DATA_LENGTH);
  AES_ECB_Hardware(g_RandState, g_TempBuff, g_TempBuff);
  memcpy(nonce, g_TempBuff, AES_DATA_HLENGTH);

  // Переходим в следующее состояние
  memset(g_TempBuff, 0x36, 16);
  AES_ECB_Hardware(g_RandState, g_TempBuff, g_RandState);
      
}
void initRND(){

    memset(g_RandState, 0, AES_DATA_LENGTH);
    updateRND();
}

/**
    ZMESC_Encrypt Использует для шифрования и расшифровки данных (процедура симметричная за счет XOR)
    Написана "по мотивам" реализации для контроллера _SecurityEncrypt из Security.c 
**/
void ZMESC_Encrypt(BYTE * key, BYTE * iv, BYTE * input, BYTE len, BYTE * output) {
    BYTE padded_len = len;
    BYTE i, j;
    BYTE rl = len % AES_DATA_LENGTH;
    
    if (rl != 0)
        padded_len += AES_DATA_LENGTH - rl;
    
    memcpy(g_encrypt_buff, iv, AES_DATA_LENGTH);
        
    for (i=0; i < padded_len; i+= AES_DATA_LENGTH) {
        ZMESC_AES_ECB(key, g_encrypt_buff, g_encrypt_buff);
        
        for (j=0; j<AES_DATA_LENGTH; j++) {
            BYTE index = i + j;
            if (index >= len)
                break;
            output[index] = input[index] ^ g_encrypt_buff[j];
        }   
    }
}

/**
    ZMESC_GenMAC Использует для шифрования и расшифровки данных (процедура симметричная за счет XOR)
    Написана "по мотивам" реализации для контроллера _SecurityHash из Security.c 
**/

void ZMESC_GenMAC(BYTE * key, BYTE * iv, BYTE * input, BYTE len, BYTE * mac) {
    BYTE padded_len = len;
    BYTE i, j;
    BYTE rl = len % AES_DATA_LENGTH;
    
    if (rl != 0)
        padded_len += AES_DATA_LENGTH - rl;
    
    ZMESC_AES_ECB(key, iv, g_encrypt_buff);
            
    for (i=0; i < padded_len; i+= AES_DATA_LENGTH) {
        for (j=0; j < AES_DATA_LENGTH; j++) {
            BYTE index = i + j;
            g_encrypt_buff[j] ^= (index < len) ? input[index]: 0;
        }
        
        ZMESC_AES_ECB(key, g_encrypt_buff, g_encrypt_buff);
    }
    
    memcpy(mac, g_encrypt_buff, AES_DATA_HLENGTH);
}


/***
    Вспомогательные функции
***/
#define UNAVAILIABLE_INDEX 0xFF

static BYTE allocAvailiableStruct(BYTE * list, BYTE element_size, BYTE maxcount) {
    BYTE i;
    
    for (i = 0; i < maxcount; i++, list += element_size)
        if(list[0] == INACTIVE_STRUCT)
            return i;
            
    return UNAVAILIABLE_INDEX;
}

static void initStructs(BYTE * list, BYTE element_size, BYTE maxcount) {
    BYTE i;
    
    for (i = 0; i < maxcount; i++, list += element_size)
        list[0] = INACTIVE_STRUCT;
}

static BYTE findContextForNode(BYTE nodeId, BYTE contextType) {
    BYTE i;
    
    for (i = 0; i < MAX_SECURITY_CONTEXTS; i++)
        if (g_security_context[i].nodeId == nodeId && g_security_context[i].contextType == contextType)
            return i;
            
    return UNAVAILIABLE_INDEX;
}

static BYTE findContextForFSM(BYTE fsmId) {
    BYTE i;
    
    for(i = 0; i < MAX_SECURITY_CONTEXTS; i++)
        if(g_security_context[i].fsmId == fsmId)
            return i;
            
    return UNAVAILIABLE_INDEX;
}

static BYTE findNonce(BYTE key) {

    BYTE i;
    
    for (i = 0; i < MAX_NONCES; i++)
        if((g_security_nonces[i].remains >0) && (g_security_nonces[i].nonce[0] == key))
            return i;
            
    return UNAVAILIABLE_INDEX;
}

static BYTE allocNonce(void) {

    BYTE i;
    
    for (i = 0; i < MAX_NONCES; i++)
        if(g_security_nonces[i].remains == 0)
            return i;
            
    return UNAVAILIABLE_INDEX;
}

static BYTE genNonce(void) {
    BYTE index = allocNonce();
    BYTE new_index = index;
    
    if (index == UNAVAILIABLE_INDEX)
        return UNAVAILIABLE_INDEX;
        
    while (new_index != UNAVAILIABLE_INDEX) {
        ZMESC_GEN_NONCE(g_security_nonces[index].nonce);
        new_index = findNonce(g_security_nonces[index].nonce[0]);
    }
    
    g_security_nonces[index].remains = NONCE_LIVETIME;
    
    return index;
}

static void Create_MAC_Data(BYTE srcnodeId, BYTE dstnodeId, BYTE * inputpacket, BYTE len, BYTE * d) {
    d[0] = inputpacket[1];
    d[1] = srcnodeId;
    d[2] = dstnodeId;
    d[3] = len - 19;
    
    memcpy(&d[4], &inputpacket[10], d[3]);  
}

static BOOL sendNonce(BYTE nodeId, BYTE fsmId) {
    // Пробуем сгенерировать NONCE...
    BYTE nonce_index = genNonce();
    if(nonce_index == UNAVAILIABLE_INDEX) {
            // Индекс не может быть получен - кончились записи под NONC'ы
            // Такая ситуация очень маловероятна при включении...
            // Скорее всего - БАГ
            // НО вполне вероятно при получении сообщения (NONCE просрочен) 
            return FALSE;
    }
    
    // Далее отправить NONCE_REPORT
            
    if((txBuf = CreatePacket()) == NULL)
        return FALSE; // Обрабатывается уже внутри автомата
        
    txBuf->ZW_SecurityNonceReportFrame.cmdClass = COMMAND_CLASS_SECURITY;
    txBuf->ZW_SecurityNonceReportFrame.cmd = SECURITY_NONCE_REPORT;
            
    memcpy(&txBuf->ZW_SecurityNonceReportFrame.nonceByte1, g_security_nonces[nonce_index].nonce, AES_DATA_HLENGTH);
            
    EnqueuePacketFSM(nodeId, sizeof(txBuf->ZW_SecurityNonceReportFrame), 
                            TRANSMIT_OPTION_USE_DEFAULT, fsmId);
                            
    return TRUE;
}

static BOOL decryptMessage(BYTE nodeId, BYTE cmd,  BYTE * input, BYTE * output, BYTE * len) {
    BYTE data_len = SEC_DATA_SIZE(*len);
    BYTE nonce_key = SEC_GET_RI(input, *len);
    
    BYTE nonce_index = findNonce(nonce_key);
    
    if(nonce_index == UNAVAILIABLE_INDEX) {
            // Нет такого Nonc'a
            return FALSE;
    }
    
    // Заполняем IV
    memcpy(g_IV, SEC_GET_IV(input), AES_DATA_HLENGTH);
    memcpy(g_IV+AES_DATA_HLENGTH, g_security_nonces[nonce_index].nonce, AES_DATA_HLENGTH);
    
    // Генерируем MAC
    Create_MAC_Data(nodeId, SecurityNodeId(), input, *len, g_TempBuff);
    ZMESC_GenMAC(g_KaKey, g_IV, g_TempBuff, g_TempBuff[3]+4, g_TempBuff);   
    
    if(memcmp(g_TempBuff, SEC_GET_MAC(input, *len), AES_DATA_HLENGTH) != 0 ) {
        // Не совпадает код авторизации 
        return FALSE;
    }
    
    ZMESC_Encrypt(g_KeKey, g_IV, SEC_GET_DATA(input), data_len, output);
    
    *len = data_len;
    
    return TRUE;
}

static void encryptMessage(BYTE nodeId, BYTE * nonce, BYTE cmd, BYTE * d, BYTE len) {
    d[0] = COMMAND_CLASS_SECURITY;
    d[1] = cmd;
    
    // RI
    d[len - 9] = nonce[0];
    
    ZMESC_GEN_NONCE(SEC_GET_IV(d));
    
    // Заполняем IV
    memcpy(g_IV, SEC_GET_IV(d), AES_DATA_HLENGTH);
    memcpy(g_IV+AES_DATA_HLENGTH, nonce, AES_DATA_HLENGTH);
    
    // Шифруем сообщение
    ZMESC_Encrypt(g_KeKey, g_IV, SEC_GET_DATA(d), SEC_DATA_SIZE(len), SEC_GET_DATA(d)); 
    
    // Генерируем MAC
    Create_MAC_Data(SecurityNodeId(), nodeId, d, len, g_TempBuff);
    ZMESC_GenMAC(g_KaKey, g_IV, g_TempBuff, g_TempBuff[3]+4, SEC_GET_MAC(d, len));
}

#define INCLUSION_PROCESS  (CURRENT_SECURITY_MODE == SECURITY_MODE_NOT_YET)
                // Workaround! Commented due to a bug in SDK in Assign Complete calling back to late
                // Это правильный вариант, но когда поправят СДК
                // (fsmInclusionId != INVALID_FSM_ID && CURRENT_SECURITY_MODE == SECURITY_MODE_NOT_YET)

BYTE g_decrypted_key[AES_DATA_LENGTH];
BYTE g_key_of_key[AES_DATA_LENGTH]={    0x00,   0x00,   0x00,   0x00,
                                        0x00,   0x00,   0x00,   0x00,
                                        0x00,   0x00,   0x00,   0x00,
                                        0x00,   0x00,   0x00,   0x00    };
BYTE g_iv_of_key[AES_DATA_LENGTH];


BYTE * SecurityDecKeyPtr()
{
    return g_decrypted_key;
}
void SecurityEncryptDecryptSeq(BYTE * key, BYTE *iv, BYTE * in, BYTE * out)
{
    BYTE i;
    ZMESC_AES_ECB(key, iv, out);
    for(i=0;i<AES_DATA_LENGTH;i++)
        out[i] ^= in[i];
}
void SecurityDecryptMainKey()
{
    SecurityEncryptDecryptSeq(g_key_of_key, g_iv_of_key,  SecurityKeyPtr(), SecurityDecKeyPtr());   
}
void SecurityEncryptMainKey()
{

    SecurityEncryptDecryptSeq(g_key_of_key, g_iv_of_key,  SecurityDecKeyPtr(), SecurityKeyPtr());   
}
void SecurityClearMainKey()
{
    memset(SecurityDecKeyPtr(), 0, AES_DATA_LENGTH);
    SecurityEncryptMainKey();

}

void InitKeys(void) {

    memset(g_iv_of_key, 0xBB, AES_DATA_LENGTH);
    // Сначала извлекаем основной ключ и расшифровываем ключ из EEPROM...
    SecurityDecryptMainKey();
    // Расшифровываем 
    memset(g_KaKey, 0x55, AES_DATA_LENGTH);
    memset(g_KeKey, 0xAA, AES_DATA_LENGTH);
    ZMESC_AES_ECB(SecurityDecKeyPtr(), g_KaKey, g_KaKey);
    ZMESC_AES_ECB(SecurityDecKeyPtr(), g_KeKey, g_KeKey);
}

/***
    Внешние Security-функции
***/
void SecurityInit(void) {
    BYTE i;

    security_fsm_models[SC_FSM_INCLUDER].triggeredAction    =   fsmSecurityIncluderHandler_p;
    security_fsm_models[SC_FSM_SENDER].triggeredAction      =   fsmSecuritySenderHandler_p;
    security_fsm_models[SC_FSM_RECEIVER].triggeredAction    =   fsmSecurityReceiverHandler_p;

    #ifdef ZW_CONTROLLER
    security_fsm_models[SC_FSM_CONTROLLER_INCLUDE].triggeredAction   =   fsmSecurityCotrollerIncludeHandler_p;
    #endif
        
    fsmInclusionId = INVALID_FSM_ID;
    
    #ifndef ZW_CONTROLLER
    if (CURRENT_SECURITY_MODE == SECURITY_MODE_NOT_YET && SecurityNodeId())
            CURRENT_SECURITY_MODE = SECURITY_MODE_DISABLED; // Do not allow to pass security incluson again. 
    #endif

    switch (CURRENT_SECURITY_MODE) {
        case SECURITY_MODE_DISABLED:
            SetUnsecureNIF();
            // the key does not really matter - we will never start security interview again
            break;
        case SECURITY_MODE_ENABLED:
            SetSecureNIF();
            // Network KEY was already loaded with over variables from EEPROM
            break;
        case SECURITY_MODE_NOT_YET:
        default:
            SetSecureNIF();
            SecurityClearMainKey(); // init scheme0 key
            break;
        

    }
    
    InitKeys();

    // Инициализация генератора случайных чисел.
    // Это повлечет за собой выключение выключение/включение радиомодуля
    initRND();

    for(i=0; i < MAX_NONCES; i++)
        g_security_nonces[i].remains =  0;
    
    initStructs((BYTE*)g_security_context, sizeof(SECURITY_CONTEXT), MAX_SECURITY_CONTEXTS);
    initStructs((BYTE*)g_security_senderdata, sizeof(SECURITY_SENDER_DATA), MAX_DATA_SENDERS);
    initStructs((BYTE*)g_security_receiverdata, sizeof(SECURITY_RECEIVER_DATA), MAX_DATA_RECEIVERS);
    
    NODE_MASK_ZERO(untrackedNonceRequests);
    
    askedSecurely = ASKED_UNSECURELY;
}

void SecurityInclusion(void) {
    if (CURRENT_SECURITY_MODE == SECURITY_MODE_NOT_YET) {
        fsmInclusionId = ZMEFSM_addFsm(&security_fsm_models[SC_FSM_INCLUDER], NULL);
        ZMEFSM_TriggerEvent(fsmInclusionId, FSMSI_E_Start, NULL);
    }
}

BYTE SecurityRepack(BYTE nodeID, BYTE sz, BYTE * d, BYTE txOptions) {
    
    BYTE data_id = allocAvailiableStruct((BYTE*)g_security_senderdata, sizeof(SECURITY_SENDER_DATA), MAX_DATA_SENDERS);
    BYTE context_id;
    BYTE fsm_id;
    
    if (data_id == UNAVAILIABLE_INDEX)
        return UNAVAILIABLE_INDEX;
        
    memcpy(g_security_senderdata[data_id].flat_data, d, sz);
    g_security_senderdata[data_id].size = sz;
    g_security_senderdata[data_id].txOptions = txOptions;
    g_security_senderdata[data_id].crypted_offset = 0;

    context_id = allocAvailiableStruct((BYTE*)g_security_context, sizeof(SECURITY_CONTEXT), MAX_SECURITY_CONTEXTS);
    if(context_id == UNAVAILIABLE_INDEX)
        return UNAVAILIABLE_INDEX;
        
    fsm_id  =   ZMEFSM_addFsm(&security_fsm_models[SC_FSM_SENDER], (BYTE*)&g_security_senderdata[data_id]);
    if(fsm_id == INVALID_FSM_ID)
        return UNAVAILIABLE_INDEX;
        
    g_security_context[context_id].fsmId = fsm_id;
    g_security_context[context_id].contextType = SEC_CONTEXT_TRANSMITTER;
    g_security_context[context_id].nodeId = nodeID;
        
    if (NODE_MASK_IS_SET(untrackedNonceRequests, nodeID)) {
        ZMEFSM_TriggerEvent(fsm_id, FSMSS_E_StartNonceRequested, NULL);
        NODE_MASK_CLEAR(untrackedNonceRequests, nodeID);
    } else
        ZMEFSM_TriggerEvent(fsm_id, FSMSS_E_Start, NULL);
    
    return fsm_id;
}

BYTE SecurityStartReceiver(BYTE nodeID) {

    BYTE data_id = allocAvailiableStruct((BYTE*)g_security_receiverdata, sizeof(SECURITY_RECEIVER_DATA), MAX_DATA_RECEIVERS);
    BYTE context_id;
    BYTE fsm_id;

     

    if(data_id == UNAVAILIABLE_INDEX)
        return UNAVAILIABLE_INDEX;

    g_security_receiverdata[data_id].size = 0;
    g_security_receiverdata[data_id].offset = 0;
    
    context_id = allocAvailiableStruct((BYTE*)g_security_context, sizeof(SECURITY_CONTEXT), MAX_SECURITY_CONTEXTS);
    if(context_id == UNAVAILIABLE_INDEX)
        return UNAVAILIABLE_INDEX;
    
    fsm_id  =   ZMEFSM_addFsm(&security_fsm_models[SC_FSM_RECEIVER], (BYTE*)&g_security_receiverdata[data_id]);
    if(fsm_id == INVALID_FSM_ID)
        return UNAVAILIABLE_INDEX;
        
    g_security_context[context_id].fsmId = fsm_id;
    g_security_context[context_id].contextType = SEC_CONTEXT_RECEIVER;
    g_security_context[context_id].nodeId = nodeID;
    
    ZMEFSM_TriggerEvent(fsm_id, FSMSR_E_Start, NULL);
    
    return fsm_id;  
}

BYTE SecurityAllocFSMSender(BYTE nodeID, BYTE sz, BYTE * d, BYTE txOptions) {
    BYTE contex_id = findContextForNode(nodeID, SEC_CONTEXT_TRANSMITTER);   

    // Автомат для работы с этим узлом уже есть - мы не можем создать еще один
    if (contex_id != UNAVAILIABLE_INDEX)
        return UNAVAILIABLE_INDEX;
    
    return SecurityRepack(nodeID, sz, d, txOptions);
}

void SecurityNonceTimer(void) {
    BYTE i;
    
    for (i=0; i < MAX_NONCES; i++)
        if (g_security_nonces[i].remains != 0)
            g_security_nonces[i].remains--;    
    
    #if SECURITY_DEBUG && SECURITY_DEBUG_DUMP_STATE
        if((CURRENT_SECURITY_MODE == SECURITY_MODE_ENABLED) &&((dbgSecurityPulses % 10) == 0))
        {
            //dbgSendSecurityMasks();
            dbgDumpSecurityState();
        }
        dbgSecurityPulses++;
    #endif              
}

/****
    Обработчик входящих сообщений для security
****/
BYTE dbg_test_data[32];//={0xaa, 0xbb, 0xcc, 0xdd};
void SendDataReportToNode(BYTE dstNode, BYTE classcmd, BYTE cmd, BYTE * reportdata, BYTE numdata);

BOOL securityInclusionInProcess()
{
    return (fsmInclusionId != INVALID_FSM_ID);
}
void SecurityCommandHandler(BYTE sourceNode, ZW_APPLICATION_TX_BUFFER *pCmd, BYTE cmdLength) reentrant {
    BYTE command = ((BYTE*)pCmd)[1];
    BYTE ci;

    if (INCLUSION_PROCESS) {
        fsmInclusionControllerNodeId = sourceNode;
        switch (command) {
           

            case SECURITY_SCHEME_GET:
                // Workaround! Moved here due to a bug in SDK in Assign Complete calling back to late
                // LearnCompleted is called after few commands from the controller already arrived, including SECURITY_SCHEME_GET
                // This call should be in ZME_Common.c,
                // !!!
                if(fsmInclusionId == INVALID_FSM_ID)
                {
                    if(SecurityNodeId() != 0)
                        SecurityInclusion();

                }
                ZMEFSM_TriggerEvent(fsmInclusionId, FSMSI_E_SchemeGetReceived, ((BYTE*)pCmd));
                break; 
            case SECURITY_NONCE_GET:
                ZMEFSM_TriggerEvent(fsmInclusionId, FSMSI_E_NonceGetReceived, NULL);
                break;
            case SECURITY_MESSAGE_ENCAPSULATION:
                *((BYTE*)&pCmd[0]) = cmdLength; // Оптимально используем каждый байт :)
                ZMEFSM_TriggerEvent(fsmInclusionId, FSMSI_E_KeyReceived, ((BYTE*)pCmd));
                break;
            case SECURITY_NONCE_REPORT:
                ZMEFSM_TriggerEvent(fsmInclusionId, FSMSI_E_NonceReportReceived, ((BYTE*)pCmd));
                break;  
        }
    } else if (CURRENT_SECURITY_MODE == SECURITY_MODE_ENABLED) {
        BYTE ci;
        
        switch (command) {
             #ifdef ZW_CONTROLLER
            case  SECURITY_SCHEME_REPORT:
                 ci = findContextForNode(sourceNode, SEC_CONTEXT_CONTROLLER_INCLUDE);
                 if(ci != UNAVAILIABLE_INDEX)
                 {
                    ZMEFSM_TriggerEvent(g_security_context[ci].fsmId, 
                                        FSMSCI_E_ScemeReportReceived,
                                        ((BYTE*)pCmd) );
                        return;
                 }
                 break;
            #endif

            case SECURITY_MESSAGE_ENCAPSULATION:
            case SECURITY_MESSAGE_ENCAPSULATION_NONCE_GET:
            {
                    BYTE fsmId;

                    *((BYTE*)&pCmd[0]) = cmdLength; 

                    #ifdef ZW_CONTROLLER
                    ci = findContextForNode(sourceNode, SEC_CONTEXT_CONTROLLER_INCLUDE);
                    if(ci != UNAVAILIABLE_INDEX)
                    {
                        ZMEFSM_TriggerEvent(g_security_context[ci].fsmId, 
                                            FSMSCI_E_KeyVerifyReceived,
                                            ((BYTE*)pCmd) );
                        return;
                    }
                    #endif

                    ci = findContextForNode(sourceNode, SEC_CONTEXT_RECEIVER);
                    if (ci == UNAVAILIABLE_INDEX)
                        return;
                    fsmId = g_security_context[ci].fsmId;

                    
                    
                    ZMEFSM_TriggerEvent(fsmId, 
                                        FSMSR_E_DataReceived,
                                        ((BYTE*)pCmd) );
                
                    if (ZMEFSM_IsActiveFSM(fsmId) || command==SECURITY_MESSAGE_ENCAPSULATION)
                        break;
            }
                    // else -- no break - this is the NONCE GET for next packet
            case SECURITY_NONCE_GET:
                #ifdef ZW_CONTROLLER
                ci = findContextForNode(sourceNode, SEC_CONTEXT_CONTROLLER_INCLUDE);
                if(ci != UNAVAILIABLE_INDEX)
                {
                    ZMEFSM_TriggerEvent(g_security_context[ci].fsmId, 
                                            FSMSCI_E_NonceGetReceived,
                                            ((BYTE*)pCmd) );
                    return;
                }
                #endif
                SecurityStartReceiver(sourceNode);
                break;
            case SECURITY_NONCE_REPORT:
                {
                    SECURITY_SENDER_DATA * pSenderData;
                    BYTE    data_remains;

                    #ifdef ZW_CONTROLLER
                    ci = findContextForNode(sourceNode, SEC_CONTEXT_CONTROLLER_INCLUDE);
                    if(ci != UNAVAILIABLE_INDEX)
                    {
                        ZMEFSM_TriggerEvent(g_security_context[ci].fsmId, 
                                            FSMSCI_E_NonceReportReceived,
                                            ((BYTE*)pCmd) );
                        return;
                    }
                    #endif

                    ci = findContextForNode(sourceNode, SEC_CONTEXT_TRANSMITTER);
                    if (ci == UNAVAILIABLE_INDEX)
                        return;
                    
                    ZMEFSM_TriggerEvent(g_security_context[ci].fsmId, 
                                        FSMSS_E_NonceReceived,                      
                                        ((BYTE*)pCmd));
                }   
                break;
            case SECURITY_COMMANDS_SUPPORTED_GET:
                {
                    BYTE sz;
                    
                    if((txBuf = CreatePacket()) == NULL)
                    {
                        return;  
                    }
                
                    txBuf->ZW_SecurityCommandsSupportedReport1byteFrame.cmdClass = COMMAND_CLASS_SECURITY;
                    txBuf->ZW_SecurityCommandsSupportedReport1byteFrame.cmd = SECURITY_COMMANDS_SUPPORTED_REPORT;
                    txBuf->ZW_SecurityCommandsSupportedReport1byteFrame.reportsToFollow = 0;
                    
                    sz = SecureNIFCopy(&(txBuf->ZW_SecurityCommandsSupportedReport1byteFrame.commandClassSupport1));

                    EnqueuePacketF(sourceNode,
                                 sizeof(txBuf->ZW_SecurityCommandsSupportedReport1byteFrame) - 3 + sz, 
                                 TRANSMIT_OPTION_USE_DEFAULT, SNDBUF_SECURITY); 
                }
                break;
            #ifdef ZW_CONTROLLER
            case  SECURITY_SCHEME_INHERIT:
                {
                    BYTE main_controller_scheme = ((BYTE*)pCmd)[2];
                    if((txBuf = CreatePacket()) == NULL)
                    {
                        return;  
                    }
                
                    // Всегда отвечаем, что поддерживает только схему 0
                    // В дальнейшем этот код при появлении других схем придется  усложнять
                    // Но т.к. на данный момент мы поддерживаем только scheme0 нет смысла пересекать 
                    // битовые маски. Это младшая схема и она ОБЯЗАНА поддерживаться основным контроллером если 
                    // этот узел включили.
                    txBuf->ZW_SecuritySchemeReportFrame.cmdClass = COMMAND_CLASS_SECURITY;
                    txBuf->ZW_SecuritySchemeReportFrame.cmd = SECURITY_SCHEME_REPORT;
                    txBuf->ZW_SecuritySchemeReportFrame.supportedSecuritySchemes = (main_controller_scheme & ZME_SECURITY_SUPPORTED_SCHEMES);
                
                    EnqueuePacketF(sourceNode, sizeof(txBuf->ZW_SecuritySchemeReportFrame), 
                                TRANSMIT_OPTION_USE_DEFAULT, SNDBUF_SECURITY);
                
                }
                break;
                    
            #endif

        }
    }
}

/***
    Обработчики КА
****/
void fsmSecurityIncluderHandler(FSM_ACTION * action) reentrant {  
    BYTE nonce_index;
    
    switch (action->rule->nextState) {
        case FSMSI_WaitForSchemeGet:
            break;
        case FSMSI_SendSchemeReport:
            {
                if ((txBuf = CreatePacket()) == NULL)
                {
                    ZMEFSM_TriggerTransmissionFailed(action->fsm_id);    
                    return;  
                }
                txBuf->ZW_SecuritySchemeReportFrame.cmdClass = COMMAND_CLASS_SECURITY;
                txBuf->ZW_SecuritySchemeReportFrame.cmd = SECURITY_SCHEME_REPORT;
                txBuf->ZW_SecuritySchemeReportFrame.supportedSecuritySchemes = ZME_SECURITY_SUPPORTED_SCHEMES;
                
                EnqueuePacketFSM(fsmInclusionControllerNodeId, sizeof(txBuf->ZW_SecuritySchemeReportFrame), 
                                TRANSMIT_OPTION_USE_DEFAULT, action->fsm_id);
                
                if (!
                    (
                        ((~(action->incoming_data[2]) & ~(ZME_SECURITY_SUPPORTED_SCHEMES)) & SECURITY_SCHEME_0_MASK) ||
                        ((action->incoming_data[2] & ZME_SECURITY_SUPPORTED_SCHEMES) & (~(SECURITY_SCHEME_0_MASK)))
                    )
                ) {
                    // there are no intersection of schemes on both sides
                    // SECURITY_SCHEME_0 requires special handling due to Danish brain-breakers ;)
                    ZMEFSM_TriggerEvent(action->fsm_id, FSMSI_E_SchemeNotSupported, NULL);
                }
            }
            break;
        case FSMSI_WaitForNonceGet:
            break;
        case FSMSI_SendNonceReport:
            {
                if (!sendNonce(fsmInclusionControllerNodeId, action->fsm_id)) {
                    // Индекс не может быть получен - кончились записи под NONC'ы
                    // Такая ситуация очень маловероятна при включении...
                    // Скорее всего - БАГ
                    ZMEFSM_TriggerTransmissionFailed(action->fsm_id);
                    return;
                }           
            }
            break;
        case FSMSI_WaitForKey:
            break;
        case FSMSI_SendNonceGet:
            {
                BYTE packet_len = action->incoming_data[0];
                
                SecurityClearMainKey();
                InitKeys();
                
                if(!decryptMessage(fsmInclusionControllerNodeId, SECURITY_MESSAGE_ENCAPSULATION,
                                action->incoming_data, SEC_GET_DATA(action->incoming_data),
                                &packet_len))
                {
                        ZMEFSM_TriggerTransmissionFailed(action->fsm_id);
                        return; 
                }
                // Записать ключ сети...
                memcpy(SecurityDecKeyPtr(), SEC_GET_DATA(action->incoming_data)+3, AES_DATA_LENGTH);
                SecurityEncryptMainKey();
                // Переинициализировать ключи
                InitKeys();
                
                // Далее отправить NONCE_GET
                
                if ((txBuf = CreatePacket()) == NULL)
                {
                    ZMEFSM_TriggerTransmissionFailed(action->fsm_id);    
                    return; 
                }
                txBuf->ZW_SecurityNonceGetFrame.cmdClass = COMMAND_CLASS_SECURITY;
                txBuf->ZW_SecurityNonceGetFrame.cmd = SECURITY_NONCE_GET;
                
                EnqueuePacketFSM(fsmInclusionControllerNodeId, sizeof(txBuf->ZW_SecurityNonceGetFrame), 
                                TRANSMIT_OPTION_USE_DEFAULT, action->fsm_id);
            }
            break;
        case FSMSI_WaitForNonceReport:
            break;
        case FSMSI_SendKeyVerify:
            {
                if ((txBuf = CreatePacket()) == NULL)
                {
                    ZMEFSM_TriggerTransmissionFailed(action->fsm_id);
                    return; 
                }
                txBuf->ZW_SecurityMessageEncapsulation1byteFrame.properties1 = 0;
                txBuf->ZW_SecurityMessageEncapsulation1byteFrame.commandClassIdentifier = COMMAND_CLASS_SECURITY;
                txBuf->ZW_SecurityMessageEncapsulation1byteFrame.commandIdentifier = NETWORK_KEY_VERIFY;
                    
                encryptMessage(fsmInclusionControllerNodeId, &(action->incoming_data[2]), SECURITY_MESSAGE_ENCAPSULATION,
                                (BYTE*)txBuf, sizeof(txBuf->ZW_SecurityMessageEncapsulation1byteFrame)-1);
                    
                EnqueuePacketFSM(fsmInclusionControllerNodeId,
                                 sizeof(txBuf->ZW_SecurityMessageEncapsulation1byteFrame)-1, 
                                 TRANSMIT_OPTION_USE_DEFAULT, action->fsm_id);  
            }
            break;
        case FSMSI_Done:
            {
                // Включить Security и сохранить данные 
                CURRENT_SECURITY_MODE = SECURITY_MODE_ENABLED;
                SecurityStoreAll();
								#ifdef SECURITY_INTERVIEW_DONE_ACTION
								SECURITY_INTERVIEW_DONE_ACTION
								#endif
                #warning CHECK if SecureNodeListAdd and SecureTestedNodeListAdd are now in SecurityStoreAll - delete the commented code below
                // SecureNodeListAdd(fsmInclusionControllerNodeId);
                // SecureTestedNodeListAdd(fsmInclusionControllerNodeId);
            }
            break;
        case FSMSI_Failed:
            {
                CURRENT_SECURITY_MODE = SECURITY_MODE_DISABLED;
                SecurityStoreAll();
                // not needed, since it is cleared on Reset and was never set // SecureNodeListZero();
                SetUnsecureNIF();
								#ifdef SECURITY_INTERVIEW_FAILED_ACTION
								SECURITY_INTERVIEW_FAILED_ACTION
								#endif
            }
            break;
        case FSMSI_Destroy:
            fsmInclusionId = INVALID_FSM_ID;
            break;
    }
}

void fsmSecuritySenderHandler(FSM_ACTION * action) reentrant {    
    SECURITY_SENDER_DATA * sender_data = (SECURITY_SENDER_DATA *) ZMEFSM_getFSMCustomData(action->fsm_id);
    BYTE ci = findContextForFSM(action->fsm_id);    
    
    switch (action->rule->nextState) {
        case FSMSS_GetNonce:
            // Далее отправить NONCE_GET
            if((txBuf = CreatePacket()) == NULL)
            {
                ZMEFSM_TriggerTransmissionFailed(action->fsm_id);
                return; 
            }
            txBuf->ZW_SecurityNonceGetFrame.cmdClass = COMMAND_CLASS_SECURITY;
            txBuf->ZW_SecurityNonceGetFrame.cmd = SECURITY_NONCE_GET;
            
            EnqueuePacketFSM(g_security_context[ci].nodeId, sizeof(txBuf->ZW_SecurityNonceGetFrame), 
                            TRANSMIT_OPTION_USE_DEFAULT, action->fsm_id);
            break;
        case FSMSS_WaitForNonce:
            break;
        case FSMSS_CheckPolicy:
            {

                BYTE nodeID = g_security_context[ci].nodeId;
                if(!IsNodeTested(nodeID))
                {
                    // ---
                    // ---
                    SecureNodeListRemove(nodeID);
                    SecureTestedNodeListAdd(nodeID);

                    if((SecurityOutgoingPacketsPolicy(nodeID, sender_data->flat_data[0], ASKED_UNSOLICITED) == PACKET_POLICY_SEND_PLAIN) &&
                        (sender_data->flat_data[1] != SECURITY_SCHEME_INHERIT)) 
                        ZMEFSM_TriggerEvent(action->fsm_id, FSMSS_E_Plain, NULL);
                    else
                    {
                     
                        ZMEFSM_TriggerEvent(action->fsm_id, FSMSS_E_Secure, NULL);
                    }
                 }  
                 else
                 {
                     ZMEFSM_TriggerEvent(action->fsm_id, FSMSS_E_Secure, NULL);  
                 }    

            }
            break;

        case FSMSS_SendPlain:
            if((txBuf = CreatePacket()) == NULL)
            {
                ZMEFSM_TriggerTransmissionFailed(action->fsm_id);
                return; 
            }
            memcpy(txBuf, sender_data->flat_data, sender_data->size);
            EnqueuePacketFSM(g_security_context[ci].nodeId, sender_data->size, TRANSMIT_OPTION_USE_DEFAULT, action->fsm_id);
                    
            break;
        case FSMSS_Failed:
            break;
        case FSMSS_CheckData:
                {
                    BYTE data_remains = sender_data->size - sender_data->crypted_offset;
                    BYTE sec_data_size = data_remains;
                    BYTE packet_len = SEC_EMPTY_PACKET_SIZE;
                    BYTE next_event = FSMSS_E_DataPrepared;
                    BYTE cmd = SECURITY_MESSAGE_ENCAPSULATION;

                   

                    BYTE nodeID = g_security_context[ci].nodeId;
					
                    if(!IsNodeTested(nodeID))
                    {
                        SecureNodeListAdd(nodeID);
                        SecureTestedNodeListAdd(nodeID);
                    }
                                        
                    if((txBuf = CreatePacket()) == NULL)
                    {
                        ZMEFSM_TriggerTransmissionFailed(action->fsm_id);
                        return; 
                    }
                    // Отсылаем все одним пакетом
                    if(data_remains <= MAX_SECURITY_SINGLEPACK && sender_data->crypted_offset == 0) {
                        txBuf->ZW_SecurityMessageEncapsulation1byteFrame.properties1 = 0;
                        if (QueueHasPacketsForNode(g_security_context[ci].nodeId, SNDBUF_SECURITY)) {
                            cmd = SECURITY_MESSAGE_ENCAPSULATION_NONCE_GET;
                            NODE_MASK_SET(untrackedNonceRequests, g_security_context[ci].nodeId);
                        }
                    } else {
                      // Отсылка в несколько пакетов
                      if(sender_data->crypted_offset == 0) {
                            // Это первый пакет
                            cmd = SECURITY_MESSAGE_ENCAPSULATION_NONCE_GET;
                            sec_data_size = MAX_SECURITY_SINGLEPACK;
                            txBuf->ZW_SecurityMessageEncapsulation1byteFrame.properties1 = 0x11;
                            next_event = FSMSS_E_DataRemains;
                      } else {
                            // Это второй пакет
                            txBuf->ZW_SecurityMessageEncapsulation1byteFrame.properties1 = 0x32;
                        }
                    }
                    
                    packet_len += sec_data_size + 1;
                    
                    memcpy(&(txBuf->ZW_SecurityMessageEncapsulation1byteFrame.commandClassIdentifier), 
                            &(sender_data->flat_data[sender_data->crypted_offset]),
                            sec_data_size);
                            
                    // Шифруем сообщение и заполняем все вспомогательные структуры (IV/MAC/RI и т.д.)
                    encryptMessage(g_security_context[ci].nodeId, &(action->incoming_data[2]), cmd, (BYTE*)txBuf, packet_len);
                
                    sender_data->crypted_offset += data_remains > 0 ? MAX_SECURITY_SINGLEPACK : 0;
            
                    // Ставим в очередь на отправку
                    EnqueuePacketFSM(g_security_context[ci].nodeId, packet_len, TRANSMIT_OPTION_USE_DEFAULT, action->fsm_id);
                                        
                    // Дергаем автомат, без повторного вызова хэндлера
                    ZMEFSM_TriggerEventLight(action->fsm_id, next_event, NULL);
                }
            
            break;
        case FSMSS_Destroy:
            g_security_context[ci].nodeId = INACTIVE_STRUCT;
            sender_data->size = INACTIVE_STRUCT;  
            break;
    }
}

void fsmSecurityReceiverHandler(FSM_ACTION * action) reentrant {
    SECURITY_RECEIVER_DATA * receiver_data = (SECURITY_RECEIVER_DATA *) ZMEFSM_getFSMCustomData(action->fsm_id);
    BYTE ci = findContextForFSM(action->fsm_id);    
    
    BYTE len  = 0;
    
    if (action->incoming_data != NULL)
            len = action->incoming_data[0];
    
    switch(action->rule->nextState) {
        case FSMSR_SendNonceReport:
            // У нас просят NONCE       
            
            if(!sendNonce(g_security_context[ci].nodeId, action->fsm_id)) {   
                ZMEFSM_TriggerTransmissionFailed(action->fsm_id);          
                return;
            }   
            break;
        case FSMSR_CheckData:
        {
                // Нам прислали зашифрованное сообщение
                // - Расшифровываем
                if(!decryptMessage(g_security_context[ci].nodeId, 
                                SECURITY_MESSAGE_ENCAPSULATION_NONCE_GET,
                                action->incoming_data, 
                                action->incoming_data,
                                &len))
                {    
                    // Проблема с расшифровкой данных
                    // Автомат нужно корректно освободить
                    ZMEFSM_TriggerEvent(action->fsm_id, FSMSR_E_InvalidData, NULL);
                    return;
                }

                // - Добавляем данные в буффер
                len--;
                memcpy(&(receiver_data->flat_data[receiver_data->offset]), action->incoming_data+1, len);
                receiver_data->size     +=   len;
                receiver_data->offset   +=   len;    
         
                if(SEC_LAST_PACKET(action->incoming_data[0])) // Это последний пакет - переходим в завершающее состояние
                    ZMEFSM_TriggerEvent(action->fsm_id, FSMSR_E_FinalData, NULL);
                else
                    ZMEFSM_TriggerEvent(action->fsm_id, FSMSR_E_NextData, NULL);    
          }   
          break;
        case FSMSR_Done:
            {
               
                //SendReportToNode(3, COMMAND_CLASS_BASIC, receiver_data->flat_data[0]);
           
                if (SecurityIncomingSecurePacketsPolicy(receiver_data->flat_data[0]) != PACKET_POLICY_DROP) {
                    // Передаем сообщение на исполнение в обычный хэндлер   
                    askedSecurely = ASKED_SECURELY;

                    __ApplicationCommandHandler(RECEIVE_STATUS_SECURE_PACKET, g_security_context[ci].nodeId, 
                                            (ZW_APPLICATION_TX_BUFFER *)(receiver_data->flat_data),
                                            receiver_data->size);
                    askedSecurely = ASKED_UNSECURELY;
                }
            }
            break;
        case FSMSR_Destroy:
            g_security_context[ci].nodeId = INACTIVE_STRUCT;
            receiver_data->size = INACTIVE_STRUCT;

            
            break;
    }
}

#ifdef ZW_CONTROLLER

void setControllerNodeInclusion(BYTE bConcroller)
{
    g_ControllerIncluding = bConcroller;
}

BYTE startSecurityInclusion(BYTE nodeID) {
    BYTE context_id;
    BYTE fsm_id;
  
    
    
    if(CURRENT_SECURITY_MODE == SECURITY_MODE_NOT_YET)
    {
        // Мы первый раз кого-то включаем как контроллер
        // Генерируем случайный ключ сети        
        genRNDNonce(SecurityDecKeyPtr());
        genRNDNonce(SecurityDecKeyPtr() + 8);
        SecurityEncryptMainKey();
        // Включаем шифрование
        CURRENT_SECURITY_MODE = SECURITY_MODE_ENABLED;
        // Сохраняем все...
        SecurityStoreAll();
        // Реинициализация Security        
        SecurityInit();
    }



    context_id = allocAvailiableStruct((BYTE*)g_security_context, sizeof(SECURITY_CONTEXT), MAX_SECURITY_CONTEXTS);
    if(context_id == UNAVAILIABLE_INDEX)
        return UNAVAILIABLE_INDEX;
    

    fsm_id  =   ZMEFSM_addFsm(&security_fsm_models[SC_FSM_CONTROLLER_INCLUDE], NULL);
    if(fsm_id == INVALID_FSM_ID)
        return UNAVAILIABLE_INDEX;


        
    g_security_context[context_id].fsmId        = fsm_id;
    g_security_context[context_id].contextType  = SEC_CONTEXT_CONTROLLER_INCLUDE;
    g_security_context[context_id].nodeId       = nodeID;
    
    ZMEFSM_TriggerEvent(fsm_id, FSMSCI_E_Start, NULL);
    
    return fsm_id;  


}

void fsmSecurityControllerIncludeHandler(FSM_ACTION * action) reentrant{

    BYTE ci     = findContextForFSM(action->fsm_id);   
    BYTE nodeID = g_security_context[ci].nodeId; 
    
    switch (action->rule->nextState) {
        case FSMSCI_SendSchemeGet:

                if ((txBuf = CreatePacket()) == NULL)
                {
                    ZMEFSM_TriggerTransmissionFailed(action->fsm_id);    
                    return;  
                }
                txBuf->ZW_SecuritySchemeGetFrame.cmdClass                    =   COMMAND_CLASS_SECURITY;
                txBuf->ZW_SecuritySchemeGetFrame.cmd                         =   SECURITY_SCHEME_GET;
                txBuf->ZW_SecuritySchemeGetFrame.supportedSecuritySchemes    =   ZME_SECURITY_SUPPORTED_SCHEMES;
                
                

                EnqueuePacketFSM(nodeID, sizeof(txBuf->ZW_SecuritySchemeGetFrame), 
                                TRANSMIT_OPTION_USE_DEFAULT, action->fsm_id);
    

            break;
        case FSMSCI_SendNonceGet:
                if ((txBuf = CreatePacket()) == NULL)
                {
                    ZMEFSM_TriggerTransmissionFailed(action->fsm_id);    
                    return; 
                }
                txBuf->ZW_SecurityNonceGetFrame.cmdClass = COMMAND_CLASS_SECURITY;
                txBuf->ZW_SecurityNonceGetFrame.cmd = SECURITY_NONCE_GET;
                
                EnqueuePacketFSM(nodeID, sizeof(txBuf->ZW_SecurityNonceGetFrame), 
                                TRANSMIT_OPTION_USE_DEFAULT, action->fsm_id);

            break;
        case FSMSCI_SendKeySet:
        {
            BYTE null_key[AES_DATA_LENGTH];
            BYTE temp_key[AES_DATA_LENGTH];
            BYTE size =  sizeof(txBuf->ZW_SecurityMessageEncapsulation1byteFrame)-1+AES_DATA_LENGTH;   
            

            memset(null_key, 0, AES_DATA_LENGTH);
            memcpy(temp_key, SecurityDecKeyPtr(), AES_DATA_LENGTH);

            if ((txBuf = CreatePacket()) == NULL)
            {
                    ZMEFSM_TriggerTransmissionFailed(action->fsm_id);
                    return; 
            }
               
             txBuf->ZW_SecurityMessageEncapsulation1byteFrame.properties1 = 0;
             txBuf->ZW_SecurityMessageEncapsulation1byteFrame.commandClassIdentifier    =   COMMAND_CLASS_SECURITY;
             txBuf->ZW_SecurityMessageEncapsulation1byteFrame.commandIdentifier         =   NETWORK_KEY_SET;

             memcpy(((BYTE*)&txBuf->ZW_SecurityMessageEncapsulation1byteFrame.commandIdentifier) + 1,
                   SecurityDecKeyPtr(), 
                   AES_DATA_LENGTH);   
         
            memcpy(SecurityDecKeyPtr(), null_key, AES_DATA_LENGTH);
            SecurityEncryptMainKey();
            // Переинициализировать ключи
            InitKeys();

            encryptMessage(nodeID, &(action->incoming_data[2]), SECURITY_MESSAGE_ENCAPSULATION,
                            (BYTE*)txBuf, size);

            memcpy(SecurityDecKeyPtr(), temp_key, AES_DATA_LENGTH);
            SecurityEncryptMainKey();
            // Переинициализировать ключи
            InitKeys();

                    
            EnqueuePacketFSM(nodeID,
                                 size, 
                                 TRANSMIT_OPTION_USE_DEFAULT, action->fsm_id);
        }    
        break;
        case FSMSCI_SendNoceReport:
             if (!sendNonce(nodeID, action->fsm_id)) {
                    ZMEFSM_TriggerTransmissionFailed(action->fsm_id);
                    return;
            }           
        break;
        case FSMSCI_CheckKey:
        {
                BYTE packet_len = action->incoming_data[0];
               
                if(!decryptMessage(nodeID, SECURITY_MESSAGE_ENCAPSULATION,
                                action->incoming_data, SEC_GET_DATA(action->incoming_data),
                                &packet_len))
                {
                        ZMEFSM_TriggerTransmissionFailed(action->fsm_id);
                        return; 
                }

                if((SEC_GET_DATA(action->incoming_data))[2] == NETWORK_KEY_VERIFY)
                {
                    if(g_ControllerIncluding)
                    {
                        if ((txBuf = CreatePacket()) == NULL)
                        {
                            ZMEFSM_TriggerTransmissionFailed(action->fsm_id);    
                            return;  
                        }
                        txBuf->ZW_SecuritySchemeGetFrame.cmdClass                    =   COMMAND_CLASS_SECURITY;
                        txBuf->ZW_SecuritySchemeGetFrame.cmd                         =   SECURITY_SCHEME_INHERIT;
                        txBuf->ZW_SecuritySchemeGetFrame.supportedSecuritySchemes    =   ZME_SECURITY_SUPPORTED_SCHEMES;
                
                        EnqueuePacketF(nodeID, sizeof(txBuf->ZW_SecuritySchemeGetFrame), 
                                TRANSMIT_OPTION_USE_DEFAULT, SNDBUF_SECURITY);
                    } 
                    ZMEFSM_TriggerEvent(action->fsm_id, FSMSCI_E_KeyValid, NULL);    
                } 
                else
                   ZMEFSM_TriggerEvent(action->fsm_id, FSMSCI_E_KeyInvalid, NULL);
        }
        break;
        
        case FSMSCI_Done:
        {

        }
        break;
        case FSMSCI_Failed:
        {

        }
        break;    
        case FSMSCI_Destroy:
        {
            g_security_context[ci].nodeId = INACTIVE_STRUCT;   
            g_ControllerIncluding = FALSE;
        }
        break;    

    }

}
#endif // ZW_CONTROLLER


#endif //  WITH_CC_SECURITY
