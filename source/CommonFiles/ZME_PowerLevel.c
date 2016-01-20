#include "ZME_Includes.h"
#include "ZME_FSM.h"

#include <ZW_slave_32_api.h>
#include <ZW_basis_api.h>

#if WITH_CC_POWERLEVEL

/******
                             Конечные автоматы
******/
                      

// Автомат для тестирования узлов сети
enum {
	FSMPL_Init=0x01,
	FSMPL_SetupPowerLevel,
	FSMPL_SendTestFrame,
	FSMPL_TestOK,
	FSMPL_CheckRetryCount,
	FSMPL_ReleasePowerLevel,
	FSMPL_SendReport,
    FSMPL_Failed,
    FSMPL_Destroy
} fsm_powerlevel_states;

enum {
	FSMPL_E_Start=0x01,
	FSMPL_E_CustomPL,
	FSMPL_E_Accepted,
	FSMPL_E_TryNext,
	FSMPL_E_Final,
	FSMPL_E_NormalPL,
    FSMPL_E_Ready         
} fsm_powerlevel_events;

#define MAX_FSM_POWERLEVEL_RULES                                                                 12
 
// FSMFW_E_PacketReceived
FSM_RULE  fsm_powerlevel_rules[MAX_FSM_POWERLEVEL_RULES] = {			
     // State1            ->           State2                             Event
    {FSMPL_Init,                       FSMPL_SetupPowerLevel,             FSMPL_E_Start              }, // 0
    {FSMPL_SetupPowerLevel,            FSMPL_SendTestFrame,               FSMPL_E_CustomPL           }, // 1
    {FSMPL_SendTestFrame,              FSMPL_TestOK,               		  FSM_EV_ZW_DATA_TRAN_ACK    }, // 2
    {FSMPL_SendTestFrame,              FSMPL_CheckRetryCount,             FSM_EV_ZW_DATA_FAILED      }, // 3
    {FSMPL_SendTestFrame,              FSMPL_CheckRetryCount,             FSM_EV_ZW_DATA_TRAN_FAILED }, // 4
    {FSMPL_TestOK,             		   FSMPL_CheckRetryCount,             FSMPL_E_Accepted   		 }, // 5
    {FSMPL_CheckRetryCount,            FSMPL_SendTestFrame,               FSMPL_E_TryNext   		 }, // 6
    {FSMPL_CheckRetryCount,            FSMPL_ReleasePowerLevel,           FSMPL_E_Final   		     }, // 7
    {FSMPL_ReleasePowerLevel,          FSMPL_SendReport,           		  FSMPL_E_NormalPL   		 }, // 8
    {FSMPL_SendReport,          	   FSMPL_Destroy,           		  FSM_EV_ZW_DATA_SENDED      }, // 9
    {FSMPL_SendReport,          	   FSMPL_Failed,           		  	  FSM_EV_ZW_DATA_FAILED      }, // 10
    {FSMPL_Failed,          	   	   FSMPL_Destroy,           		  FSMPL_E_Ready      		 }  // 11
    
};

#define MAX_FSM_POWERLEVEL_TIMEOUTS                                                               1

FSM_TIMEOUT_TRIGGER fsm_powerlevel_timeouts[MAX_FSM_POWERLEVEL_TIMEOUTS] = {

    {FSMPL_Failed,           FSMPL_E_Ready,           0                       }
      
};
// Из Common.c
BYTE * getPowerLevelData();
void savePowerLevelData();

void fsmPowerLevelHandler(FSM_ACTION * action) reentrant;
code const FSMActionFunc_t fsmPowerLevelHandler_p = &fsmPowerLevelHandler;

FSM_MODEL pl_fsm_model[1]= 
   {	
      fsm_powerlevel_rules, 	    MAX_FSM_POWERLEVEL_RULES,
      fsm_powerlevel_timeouts,	  	MAX_FSM_POWERLEVEL_TIMEOUTS,
      0, //fsmPowerLevelHandler_p,
      FSMPL_Init, FSMPL_Destroy, 	FSMPL_Failed
	};




#define MAX_POWERLEVEL_TEST		10
#define UNAVAILIABLE_INDEX 		0xFF

POWERLEVEL_DATA 				g_powerlevel_tests[MAX_POWERLEVEL_TEST];
BYTE							g_last_powerlevel_test_index = 0;
BYTE							g_current_powerlevel         = normalPower;
BYTE							g_current_powertimer		   = 0;

#define 						SAVED_PL_DATA_SIZE			6


void PowerLevelInit()
{
	BYTE i;
	BYTE * ppldata;
	pl_fsm_model[0].triggeredAction		=	fsmPowerLevelHandler_p; 

	for(i=0; i<MAX_POWERLEVEL_TEST; i++)
		g_powerlevel_tests[i].fsmId = INVALID_FSM_ID;

	// Загружаем результат последнего теста из EEPROM
	ppldata = getPowerLevelData();
	memcpy(((BYTE*)g_powerlevel_tests) + 2, ppldata, SAVED_PL_DATA_SIZE); 		
}	
void PowerLevel1SecTick()
{
	if(g_current_powertimer)
	{
		g_current_powertimer--;
		if(!g_current_powertimer)
		{
			ZW_RF_POWERLEVEL_SET(normalPower);
			g_current_powerlevel  = normalPower;
		}
	}
	
}
static BYTE allocPowerLevelTest()
{
	BYTE i;

	for(i=0; i<MAX_POWERLEVEL_TEST; i++)
		if(g_powerlevel_tests[i].fsmId == INVALID_FSM_ID)
			return i;

	return UNAVAILIABLE_INDEX;
}
static void freePowerLevelTest(POWERLEVEL_DATA * d)
{
	d->fsmId = INVALID_FSM_ID;
}	
static BYTE runPowerLevelTest(BYTE sourceNode, BYTE testNode, BYTE level, WORD retryCount)
{

	BYTE			  data_index;
	BYTE 			  fsm_id;


	data_index = allocPowerLevelTest();
     
    if(data_index == UNAVAILIABLE_INDEX)
        return UNAVAILIABLE_INDEX;

    
    fsm_id  =   ZMEFSM_addFsm(pl_fsm_model, (BYTE*)&g_powerlevel_tests[data_index]);
    
    if(fsm_id == INVALID_FSM_ID)
        return UNAVAILIABLE_INDEX;

        
    g_powerlevel_tests[data_index].source_nodeId 	= 	sourceNode;
    g_powerlevel_tests[data_index].test_nodeId 		= 	testNode;
    g_powerlevel_tests[data_index].level 			= 	level;
    g_powerlevel_tests[data_index].retry_count 		= 	retryCount;
    g_powerlevel_tests[data_index].ack_count 		= 	0;
    g_powerlevel_tests[data_index].fsmId 			=	fsm_id;
    
    ZMEFSM_TriggerEvent(fsm_id, FSMPL_E_Start, NULL);
    
    g_last_powerlevel_test_index = data_index;

    return fsm_id;  

}
static BOOL sendPowerLevelTestReport(POWERLEVEL_DATA * d, BYTE fsm_id, BYTE nodeId)
{
	if ((txBuf = CreatePacket()) == NULL)
            return FALSE; 
    
    txBuf->ZW_PowerlevelTestNodeReportFrame.cmdClass 	= COMMAND_CLASS_POWERLEVEL;
    txBuf->ZW_PowerlevelTestNodeReportFrame.cmd 		= POWERLEVEL_TEST_NODE_REPORT;
    txBuf->ZW_PowerlevelTestNodeReportFrame.testNodeid 	= d->test_nodeId;

    if(d->retry_count == 0)
    {
    	if(d->ack_count == 0)
    		txBuf->ZW_PowerlevelTestNodeReportFrame.statusOfOperation 	= POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_FAILED;
    	else
    		txBuf->ZW_PowerlevelTestNodeReportFrame.statusOfOperation 	= POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_SUCCES;		
    }
    else
    	txBuf->ZW_PowerlevelTestNodeReportFrame.statusOfOperation 	= POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_INPROGRESS;
    
    txBuf->ZW_PowerlevelTestNodeReportFrame.testFrameCount1		=	d->ack_count >> 8;	
    txBuf->ZW_PowerlevelTestNodeReportFrame.testFrameCount2		=	d->ack_count & 0xFF;
    
    if(nodeId == 0)
    	nodeId = d->source_nodeId;

    if(fsm_id == INVALID_FSM_ID)                
    	EnqueuePacket(nodeId,
                      sizeof(txBuf->ZW_PowerlevelTestNodeReportFrame), 
                      TRANSMIT_OPTION_USE_DEFAULT);
    else
    	EnqueuePacketFSM(nodeId,
                  		 sizeof(txBuf->ZW_PowerlevelTestNodeReportFrame), 
                  		 TRANSMIT_OPTION_USE_DEFAULT, 
                         fsm_id);

    return TRUE;
}
void PowerLevelCommandHandler(BYTE sourceNode, ZW_APPLICATION_TX_BUFFER *pCmd, BYTE cmdLength) reentrant
{
	BYTE cmd	= *((BYTE*)pCmd + 1);
	BYTE param1 = *((BYTE*)pCmd + 2);
	BYTE param2 = *((BYTE*)pCmd + 3);
	BYTE param3 = *((BYTE*)pCmd + 4);
	BYTE param4 = *((BYTE*)pCmd + 5);

	switch (cmd) {
				
				case POWERLEVEL_SET:
					if ((param1==normalPower) || (param1==minus1dBm) || (param1==minus2dBm) || (param1==minus3dBm) ||
							(param1==minus4dBm) || (param1==minus5dBm) || (param1==minus6dBm) ||
							(param1==minus7dBm) || (param1==minus8dBm) || (param1==minus9dBm)) {
						if (param2 || (param1 == normalPower)) { // Only if timeout defined or powerlevel is normalPower
							param1 				= 	ZW_RF_POWERLEVEL_SET(param1); // Set powerlevel according to command
							if (param1 != normalPower)// If normalPower then no timer is started
								g_current_powertimer = param2;
						}
					}
					break;

				case POWERLEVEL_GET:
						if((txBuf = CreatePacket()) == NULL)
                    	{
                        	return;  
                    	}

						txBuf->ZW_PowerlevelReportFrame.cmdClass 	= COMMAND_CLASS_POWERLEVEL;
						txBuf->ZW_PowerlevelReportFrame.cmd 		= POWERLEVEL_REPORT;
						txBuf->ZW_PowerlevelReportFrame.powerLevel  = ZW_RF_POWERLEVEL_GET();
						txBuf->ZW_PowerlevelReportFrame.timeout 	= g_current_powertimer;
						
						EnqueuePacket(sourceNode,
                                 sizeof(txBuf->ZW_PowerlevelReportFrame), 
                                 TRANSMIT_OPTION_USE_DEFAULT); 
          
					break;

				case POWERLEVEL_TEST_NODE_SET:
					runPowerLevelTest(sourceNode, param1, param2, (WORD)((WORD)param3 << 8) + param4);
					break;

				case POWERLEVEL_TEST_NODE_GET:
					sendPowerLevelTestReport(&(g_powerlevel_tests[g_last_powerlevel_test_index]), 
									INVALID_FSM_ID, sourceNode);
					break;
			}
			
}
void SendDataReportToNode(BYTE dstNode, BYTE classcmd, BYTE cmd, BYTE * reportdata, BYTE numdata);
void fsmPowerLevelHandler(FSM_ACTION * action) reentrant
{
	POWERLEVEL_DATA * pl_data = (POWERLEVEL_DATA *) ZMEFSM_getFSMCustomData(action->fsm_id);
   	
	switch (action->rule->nextState) {
    
			case FSMPL_SetupPowerLevel:
				ZW_RF_POWERLEVEL_SET(pl_data->level);
				ZMEFSM_TriggerEvent(action->fsm_id, FSMPL_E_CustomPL, NULL);
				//SendDataReportToNode(1, COMMAND_CLASS_POWERLEVEL, 0xda, pl_data, sizeof(POWERLEVEL_DATA));
				break;
			case FSMPL_SendTestFrame:
				{
					if ((txBuf = CreatePacket()) == NULL)
                	{
                    	ZMEFSM_TriggerTransmissionFailed(action->fsm_id);
                    	return; 
                	}
                	// Это тестовый пакет
                	SetPacketFlags(SNDBUF_TEST_FRAME);
                	// В данных передается только уровень мощности	
                	((BYTE*)txBuf)[0] = pl_data->level;
                   
                	EnqueuePacketFSM(pl_data->test_nodeId,
                                 	 1, 
                                 	 TRANSMIT_OPTION_USE_DEFAULT, 
                                 	 action->fsm_id);
            	}
				break;
			case FSMPL_TestOK:
				pl_data->ack_count = pl_data->ack_count+1;
				ZMEFSM_TriggerEvent(action->fsm_id, FSMPL_E_Accepted, NULL);
				break;
			case FSMPL_CheckRetryCount:
				//SendDataReportToNode(1, COMMAND_CLASS_POWERLEVEL, 0xda, action->rule, 3);
				
				pl_data->retry_count--;
				if(pl_data->retry_count)
					ZMEFSM_TriggerEvent(action->fsm_id, FSMPL_E_TryNext, NULL);
				else
					ZMEFSM_TriggerEvent(action->fsm_id, FSMPL_E_Final, NULL);
				break;
			case FSMPL_ReleasePowerLevel:
				ZW_RF_POWERLEVEL_SET(normalPower);
				ZMEFSM_TriggerEvent(action->fsm_id, FSMPL_E_NormalPL, NULL);
				break;		

			case FSMPL_SendReport:
				{
					BYTE * ppl_ee_data;
					//SendDataReportToNode(1, COMMAND_CLASS_POWERLEVEL, 0xda, pl_data, sizeof(POWERLEVEL_DATA));
				
					// Сохраняем данные последнего теста в EEPROM
					ppl_ee_data = getPowerLevelData();	
					memcpy(ppl_ee_data, ((BYTE*)pl_data) + 2, SAVED_PL_DATA_SIZE); 		
	                savePowerLevelData();

					if(!sendPowerLevelTestReport(pl_data, action->fsm_id, 0))
					{
						ZMEFSM_TriggerTransmissionFailed(action->fsm_id);
                    	return; 
                	}
				}
				break;
			case FSMPL_Failed:
				break;	
			case FSMPL_Destroy:
				freePowerLevelTest(pl_data);
				break;			
						
    }
}

#endif // WITH_CC_POWERLEVEL

