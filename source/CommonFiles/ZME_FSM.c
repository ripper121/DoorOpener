#include "ZME_Includes.h"

#if WITH_FSM
#include "ZME_FSM.h"

FSM_ENTITY fsm_list[MAX_FSM];
BYTE fsm_list_count;

void ZMEFSM_Init(void) {
	BYTE i;
	fsm_list_count = 0; 
	
	for (i=0; i < MAX_FSM; i++)
		fsm_list[i].model = NULL; // Если нулевой указатель на модель - то fsm не используется
}

BOOL ZMEFSM_HaveActiveFSM() {
    /*BYTE i;
    for (i=0; i < MAX_FSM; i++)
		if(fsm_list[i].model != NULL)
            return TRUE;*/
 
    return ZMEFSM_CalcActiveFSMS() != 0;
}
BYTE getFirstActiveFSM()
{
	BYTE i;
    BYTE count = 0;
    for (i=0; i < MAX_FSM; i++)
		if(fsm_list[i].model != NULL)
            return i;
 
    return INVALID_FSM_ID;
}
BYTE ZMEFSM_CalcActiveFSMS() {
    BYTE i;
    BYTE count = 0;
    for (i=0; i < MAX_FSM; i++)
		if(fsm_list[i].model != NULL)
            count++;
 
    return count;
}

BOOL ZMEFSM_IsActiveFSM(BYTE fsmId)
{
	if(fsmId == INVALID_FSM_ID)
		return FALSE;
	if(fsm_list[fsmId].model == NULL)
		return FALSE;
	return TRUE;
}

void ZMEFSM_ProcessTimer(void) {
	BYTE i;
	
	for (i=0; i < fsm_list_count; i++) {
		if (fsm_list[i].model == NULL)
			continue; // Пустая запись - пропускаем ее 
		if (fsm_list[i].time_remains != INFINITY_TIMEOUT) { // Есть ли у текущего состояния таймаут?
			// Он есть
			fsm_list[i].time_remains--;
			if (fsm_list[i].time_remains == 0) // Наступил таймаут
				ZMEFSM_TriggerEvent(i, fsm_list[i].timeout_event, NULL);
		}
	}
}

void ZMEFSM_ProcessSwitchToNextState(void) {
	BYTE i;
	
	for (i=0; i < fsm_list_count; i++) {
		if (fsm_list[i].model == NULL)
			continue; // Пустая запись - пропускаем ее 
		if (fsm_list[i].time_remains == SWITCH_TO_NEXT_TIMEOUT) { // Есть ли у текущего состояния специальный таймаут?
			// Он есть
			ZMEFSM_TriggerEvent(i, fsm_list[i].timeout_event, NULL);
		}
	}
}

void FSMAUX_initializeTimeout(FSM_ENTITY * fsm) {
	BYTE i;
	
	fsm->time_remains = INFINITY_TIMEOUT;
	
	for(i=0; i < fsm->model->timeout_count; i++)
		if(fsm->model->timeout_triggers[i].state == fsm->state) {
			fsm->timeout_event = fsm->model->timeout_triggers[i].event;
			fsm->time_remains = fsm->model->timeout_triggers[i].timeout;
			break;
		}
}

BYTE ZMEFSM_addFsm(FSM_MODEL * fsm_model, BYTE  *  custom_data) {
	BYTE i;
	BYTE id = INVALID_FSM_ID;
	
	// Мы не добавляем пустые КА
	if (fsm_model == NULL)
		return id;
	
	// Сначала пробегаемся по текущим автоматам, вдруг кто освободился
	
	for (i=0; i < fsm_list_count; i++)
	  if (fsm_list[i].model == NULL)
	  	id = i;
	
	// Такого не нашлось - нужно увеличивать счетчик
	if (id == INVALID_FSM_ID)	{
		if (fsm_list_count >= MAX_FSM)
			return id;
		id = fsm_list_count;
		fsm_list_count++;
	}
	
	// Заполняем поля КА	
	fsm_list[id].model = fsm_model;	
	fsm_list[id].state = fsm_model->initial_state;
	fsm_list[id].custom_data = custom_data;
		
	FSMAUX_initializeTimeout(&fsm_list[id]);
	
	return id;
}

BYTE ZMEFSM_TriggerEventLight(BYTE fsm_id, BYTE event, BYTE * incoming_data) {
	BYTE i = UNKNOWN_STATE;
	BYTE next_state = UNKNOWN_STATE;

	if (fsm_id > fsm_list_count)
		return UNKNOWN_STATE;
	if (fsm_list[fsm_id].model == NULL)
		return UNKNOWN_STATE;
	
	if(event == FSM_EV_ZW_TRANSMISSION_FAILED) // Универсальное правило перехода из любого состояния в завершающее с отриц. рез-ом
	{
		next_state = fsm_list[fsm_id].model->transmission_failed_state;	
	}
	else
	{
		// Есть ли переход по такому событию из текущего состояния
		// Проходим по правилам перехода
		for (i=0; i < fsm_list[fsm_id].model->rules_count; i++)
			if ((fsm_list[fsm_id].model->rules[i].currentState == fsm_list[fsm_id].state) &&
		   		(fsm_list[fsm_id].model->rules[i].event == event)) {
		   			next_state = fsm_list[fsm_id].model->rules[i].nextState;
		   			break;
		   		}

	}
	// Нельзя никуда переходить по такому событию из текущего состояния
	if(next_state == UNKNOWN_STATE) // Тут нужно хотя бы предупреждение
		return UNKNOWN_STATE;
	
	// Переходим в новое состояние
	fsm_list[fsm_id].state = next_state;
	
	FSMAUX_initializeTimeout(&fsm_list[fsm_id]);
	
	return i;
}
BYTE ZMEFSM_FSMState(BYTE fsm_id)
{
	return fsm_list[fsm_id].state;
}
void ZMEFSM_TriggerEvent(BYTE fsm_id, BYTE event, BYTE * incoming_data) reentrant {
	FSM_ACTION action;
		
	BYTE rule = ZMEFSM_TriggerEventLight(fsm_id, event, incoming_data);
	BYTE next_state;	

	if (rule == UNKNOWN_STATE)
		return;
		
	next_state = fsm_list[fsm_id].model->rules[rule].nextState;
		
	// Вызываем пользовательский обработчик
	action.fsm_id = fsm_id;
	action.rule = &(fsm_list[fsm_id].model->rules[rule]);
	action.custom_data = fsm_list[fsm_id].custom_data;
	action.incoming_data = incoming_data;
	
	fsm_list[fsm_id].model->triggeredAction(&action); 
		
	// Автомат дошел до конечного состояния
	if (next_state == fsm_list[fsm_id].model->final_state) {
		// Удаляем его из списка
		fsm_list[fsm_id].model = NULL;
		if (fsm_id == (fsm_list_count-1))
			fsm_list_count--;		
	}
	ZMEFSM_ProcessSwitchToNextState();
}
void ZMEFSM_TriggerTransmissionFailed(BYTE fsm_id)
{
	ZMEFSM_TriggerEvent(fsm_id, FSM_EV_ZW_TRANSMISSION_FAILED, NULL);
}
BYTE * ZMEFSM_getFSMCustomData(BYTE fsm_id) {
	if (fsm_id > fsm_list_count)
		return NULL;
	if (fsm_list[fsm_id].model == NULL)
		return NULL;
		
	return fsm_list[fsm_id].custom_data;
}

#endif