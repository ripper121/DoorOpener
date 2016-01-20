#ifndef ZME_FSM
#define ZME_FSM

typedef struct _FSM_RULE_
{
 	BYTE 				currentState;		// Cотояния автомата из которого осуществляется переход
 	BYTE 				nextState;			// Состояние автомата после перехода
 	BYTE 				event;				// Событие, по которому происходит переход
} FSM_RULE;

typedef struct _FSM_TIMEOUT_TRIGGER_
{
	BYTE		state;
	BYTE 		event;
	WORD		timeout;
} FSM_TIMEOUT_TRIGGER;

typedef struct _FSM_ACTION_
{
	BYTE 			fsm_id;
	FSM_RULE 	*	rule;
	BYTE 		* 	custom_data;
	BYTE 		* 	incoming_data;	
} FSM_ACTION;


typedef void (code *FSMActionFunc_t)(FSM_ACTION * action) reentrant;


typedef struct _FSM_MODEL_
{
	FSM_RULE 				* rules;				// 	Правила перехода
	BYTE       				  rules_count;	
	FSM_TIMEOUT_TRIGGER		* timeout_triggers;		//  Таймауты присоединненные к некоторым событиям перехода
	BYTE					  timeout_count;
	FSMActionFunc_t			  triggeredAction;      // Указатель на функцию, которая отрабатывает при переходе из одного состояния в другое		
	BYTE					  initial_state;		//  Начальное состояние автомата 
	BYTE					  final_state;			//  Конечное состояние автомата после которого он перестает обрабатываться 
													// (выбрасывается из списка активных автоматов)
	BYTE					  transmission_failed_state;  // Общее состояние для ошибок отправки/передачи пакетов
} FSM_MODEL;

#define INFINITY_TIMEOUT 				0xFFFF
#define SWITCH_TO_NEXT_TIMEOUT	        0x0000

typedef struct _FSM_ENTITY_
{
	BYTE 			state;				// Текущее состояние
	BYTE            timeout_event;      // Событие, которое произойдет по таймауту
	WORD    		time_remains;		// Время пребывания в этом состоянии, оставшееся до таймаута 
	BYTE 		*  	custom_data;		// Пользовательские данные, необходимые, совершения автоматом действий
	FSM_MODEL	*	model;
} FSM_ENTITY;


#define INVALID_FSM_ID 	0xFF
#define UNKNOWN_STATE	0xFF
#define UNKNOWN_EVENT 	0xFF

#define FSM_EV_ZW_DATA_SENDED         0xFC
#define FSM_EV_ZW_DATA_FAILED         0xFE
#define FSM_EV_ZW_DATA_TRAN_ACK    	  0xFB
#define FSM_EV_ZW_DATA_TRAN_FAILED    0xFA
#define FSM_EV_ZW_TRANSMISSION_FAILED 0xFD


BYTE getFirstActiveFSM();
// Добавить новый КА в список активных КА
// fsm_model 	-  Модель конечного автомата. Заранее заполненная структура, содержащая правила перехода, начальное и конечное сотояния и т.д.
// custom_data	-  Данные необходимые для работы действий КА
// Возвращает номер КА в списке активных КА или INVALID_FSM_ID, если не удалось добавить КА
BYTE ZMEFSM_addFsm(FSM_MODEL * fsm_model, BYTE  *  custom_data); 
// Посылает КА с fsm_id событие event
void ZMEFSM_TriggerEvent(BYTE fsm_id, BYTE event, BYTE * incoming_data) reentrant;
void ZMEFSM_TriggerTransmissionFailed(BYTE fsm_id);

// Возращает пользовательские данные КА
BYTE * ZMEFSM_getFSMCustomData(BYTE fsm_id);
// Посылает КА с fsm_id событие event
// При этом не происходит вызова хэндлера, метод используется внутри хэндлера.
BYTE ZMEFSM_TriggerEventLight(BYTE fsm_id, BYTE event, BYTE * incoming_data);

// Проверка активности автоматов
BOOL ZMEFSM_HaveActiveFSM();
BOOL ZMEFSM_IsActiveFSM(BYTE fsmId);
BYTE ZMEFSM_CalcActiveFSMS();

BYTE ZMEFSM_FSMState(BYTE fsm_id);

// Вызывается при начале работы с автоматами. Лучше всего в SoftwareInit в ZME_Common.cpp 
void ZMEFSM_Init(void);
// Эта функция должна быть вызвана внутри таймера. Лучше всего в ZME_Common.cpp 
void ZMEFSM_ProcessTimer(void);

#endif // ZME_FSM