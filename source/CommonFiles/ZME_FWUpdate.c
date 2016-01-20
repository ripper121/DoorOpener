
#include "ZME_Includes.h"

#if WITH_CC_FIRMWARE_UPGRADE

#include "ZME_FWUpdate.h"
#include "ZME_FSM.h"

#include <ZW_crc.h>
#include <ZW_firmware_descriptor.h>
#include <ZW_Firmware_bootloader_defs.h>
#include <ZW_firmware_update_nvm_api.h>



#define  MAX_RETRY_COUNT            4
#define  PACKET_SIZE                40
#define  DATA_PTR(D)                (D+4)
#define  DATA_SIZE(S)               (S-6)

#define  FWUPDATE_DEBUG             0

#define  STATEVECTOR_SIZE           9

// в Custom.h адрес NVM для прошивки
// FWUPDATE_FIRMWARE_START(CHIP_ID) 0x8000

enum
{
    CHECKPACKET_LOC_CRCERROR=0x00,
    CHECKPACKET_WHL_CRCERROR=0x01,
    CHECKPACKET_SUCESS=0x02,
    CHECKPACKET_REQ_NEXT=0x03,
    CHECKPACKET_GO_SLEEP=0x04
};

/****
            Дополнительные структуры
 ****/

typedef struct _FWU_FSM_DATA_
{
    BYTE    retryRemains;       // Сколько осталось попыток для текущего пакета
    // vvvvvvvvvvvvvvvvvvvvvvvvvvv
    // Эти данные нужно сохранять в EEPROM для возобновления
    // загрузки по WAKEUP
    BYTE    senderNode;             // Узел, который будет присылать обновление
    WORD    packetIndex;            // Индекс  текущего пакета
    WORD    wholeChecksum;          // Результирующая контрольная сумма прошивки 
    WORD    accumulatedCheckSum;    // Накопленная контрольная сумма (до текущего пакета)
    BYTE    update_packetsize;      // Размер пакета обновления
	BYTE 	chipId;					// Выбранный чип для обновления 0 для Z-Wave
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^
    #if FIRMWARE_UPGRADE_BATTERY_SAVING
    WORD    packetBatteryRemains;   // Количесво пакетов, которые осталось получить на текущем просыпании
    #endif

} FWU_FSM_DATA;



/******
                             Конечные автоматы
******/
                      

// Автомат для загрузки обновления прошивки
enum {
	FSMFW_Init=0x01,
    FSMFW_CheckFWVersion,         // 02
    FSMFW_SendInvalidVersion,     // 03
    FSMFW_WaitForAuth,            // 04
    FSMFW_SendAuthFailed,         // 05
    FSMFW_SendAccepted,           // 06
    FSMFW_SendRequestPacket,      // 07
    FSMFW_WaitForPacket,          // 08
    FSMFW_CheckRetryCount,        // 09
    FSMFW_CheckPacket,            // 0A
    FSMFW_SendTransmissionFails,  // 0B
    FSMFW_SendTransmissionInvalidCRC,
    FSMFW_SendTransmissionDone,
    FSMFW_WaitForCompletion,
    FSMFW_GoSleep,
    FSMFW_Failed,
    FSMFW_Done,
    FSMFW_Destroy
} fsm_fwupdater_states;

enum {
	FSMFW_E_Start=0x01,
    FSMFW_E_Invalid,          // 02
    FSMFW_E_Valid,            // 03
    FSMFW_E_Timeout,          // 04
    FSMFW_E_Authentificated,  // 05  
    FSMFW_E_PacketReceived,   // 06
    FSMFW_E_Next,             // 07
    FSMFW_E_Final,            // 08  
    FSMFW_E_InvalidChecksum,  // 09   
    FSMFW_E_Ready,            // 0A
    FSMFW_E_TimeoutCompletion,// 0B
    FSMFW_E_ResumeAfterWakeup,// 0С
    FSMFW_E_BlockReady        // 0D       

		
} fsm_fwupdater_events;

#define MAX_FSM_FWUPDATER_RULES                                                                 33
 
// FSMFW_E_PacketReceived
FSM_RULE  fsm_fwupdater_rules[MAX_FSM_FWUPDATER_RULES] = {			
     // State1            ->           State2                             Event
    {FSMFW_Init,                       FSMFW_CheckFWVersion,              FSMFW_E_Start             }, // 0
    {FSMFW_Init,                       FSMFW_SendRequestPacket,           FSMFW_E_ResumeAfterWakeup }, // 1
    {FSMFW_CheckFWVersion,             FSMFW_SendInvalidVersion,          FSMFW_E_Invalid           }, // 2
    {FSMFW_CheckFWVersion,             FSMFW_WaitForAuth,                 FSMFW_E_Valid             }, // 3
    {FSMFW_WaitForAuth,                FSMFW_SendAuthFailed,              FSMFW_E_Timeout           }, // 4
    {FSMFW_SendAuthFailed,             FSMFW_Failed,                      FSM_EV_ZW_DATA_SENDED     }, // 5
    {FSMFW_SendAuthFailed,             FSMFW_Failed,                      FSM_EV_ZW_DATA_FAILED     }, // 6
    {FSMFW_SendInvalidVersion,         FSMFW_Failed,                      FSM_EV_ZW_DATA_SENDED     }, // 7
    {FSMFW_SendInvalidVersion,         FSMFW_Failed,                      FSM_EV_ZW_DATA_FAILED     }, // 8
    {FSMFW_WaitForAuth,                FSMFW_SendAccepted,                FSMFW_E_Authentificated   }, // 9
    {FSMFW_SendAccepted,               FSMFW_SendRequestPacket,           FSM_EV_ZW_DATA_SENDED     }, // 10
    {FSMFW_SendAccepted,               FSMFW_Failed,                      FSM_EV_ZW_DATA_FAILED     }, // 11
    {FSMFW_SendRequestPacket,          FSMFW_WaitForPacket,               FSM_EV_ZW_DATA_SENDED     }, // 12
    {FSMFW_SendRequestPacket,          FSMFW_Failed,                      FSM_EV_ZW_DATA_FAILED     }, // 13
    {FSMFW_WaitForPacket,              FSMFW_CheckPacket,                 FSMFW_E_PacketReceived    }, // 14
    {FSMFW_WaitForPacket,              FSMFW_CheckRetryCount,             FSMFW_E_Timeout           }, // 15
    {FSMFW_CheckPacket,                FSMFW_SendRequestPacket,           FSMFW_E_Next              }, // 16
    {FSMFW_CheckPacket,                FSMFW_CheckRetryCount,             FSMFW_E_Invalid           }, // 17
    {FSMFW_CheckPacket,                FSMFW_SendTransmissionDone,        FSMFW_E_Final             }, // 18
    {FSMFW_CheckPacket,                FSMFW_SendTransmissionInvalidCRC,  FSMFW_E_InvalidChecksum   }, // 19
    {FSMFW_CheckPacket,                FSMFW_GoSleep,                     FSMFW_E_BlockReady        }, // 20
    {FSMFW_SendTransmissionInvalidCRC, FSMFW_Failed,                      FSM_EV_ZW_DATA_SENDED     }, // 21
    {FSMFW_SendTransmissionInvalidCRC, FSMFW_Failed,                      FSM_EV_ZW_DATA_FAILED     }, // 22 
    {FSMFW_CheckRetryCount,            FSMFW_SendRequestPacket,           FSMFW_E_Valid             }, // 23
    {FSMFW_CheckRetryCount,            FSMFW_SendTransmissionFails,       FSMFW_E_Invalid           }, // 24
    {FSMFW_SendTransmissionDone,       FSMFW_WaitForCompletion,           FSM_EV_ZW_DATA_SENDED     }, // 25
    {FSMFW_WaitForCompletion,          FSMFW_Done,                        FSMFW_E_TimeoutCompletion }, // 26
    {FSMFW_SendTransmissionDone,       FSMFW_Failed,                      FSM_EV_ZW_DATA_FAILED     }, // 27
    {FSMFW_SendTransmissionFails,      FSMFW_Failed,                      FSM_EV_ZW_DATA_SENDED     }, // 28
    {FSMFW_SendTransmissionFails,      FSMFW_Failed,                      FSM_EV_ZW_DATA_FAILED     }, // 29
    {FSMFW_GoSleep,                    FSMFW_Destroy,                     FSMFW_E_Ready             }, // 30
    {FSMFW_Failed,                     FSMFW_Destroy,                     FSMFW_E_Ready             }, // 31
    {FSMFW_Done,                       FSMFW_Destroy,                     FSMFW_E_Ready             }  // 32

};

#define MAX_FSM_FWUPDATER_TIMEOUTS                                                                 5

FSM_TIMEOUT_TRIGGER fsm_fwupdater_timeouts[MAX_FSM_FWUPDATER_TIMEOUTS] = {
     // State                Event                    TimeoutInterval (*100ms)
    {FSMFW_WaitForAuth,      FSMFW_E_Timeout,                   200                     },
    {FSMFW_WaitForPacket,    FSMFW_E_Timeout,                   200                     },
    {FSMFW_WaitForCompletion,FSMFW_E_TimeoutCompletion,         10                      },
    {FSMFW_Failed,           FSMFW_E_Ready,                     0                       },
    {FSMFW_GoSleep,          FSMFW_E_Ready,                     0                       }
        
};

void fsmFWUpdaterHandler(FSM_ACTION * action) reentrant;
code const FSMActionFunc_t fsmFWUpdaterHandler_p = &fsmFWUpdaterHandler;

FSM_MODEL fw_fsm_model[1]= 
   {	
      fsm_fwupdater_rules, 	    MAX_FSM_FWUPDATER_RULES,
      fsm_fwupdater_timeouts,	  MAX_FSM_FWUPDATER_TIMEOUTS,
      0, //fsmFWUpdaterHandler_p,
      FSMFW_Init, FSMFW_Destroy, FSMFW_Failed
	};


/****
            Глобальные переменные
 ****/

BYTE                g_fwupdate_fsmid;
FWU_FSM_DATA        g_fwu_data;   
BYTE                g_chk_packet_events[] = {   FSMFW_E_Invalid, 
                                                FSMFW_E_InvalidChecksum, 
                                                FSMFW_E_Final, 
                                                FSMFW_E_Next,
                                                FSMFW_E_BlockReady}; 



#define BOOTCRYPTO_FW_START         FIRMWARE_NVM_OFFSET_1MBIT 

void zmeDecryptFWSegment(DWORD offset, BYTE *buf, WORD length);

//static BYTE firmware_update_packetsize = PACKET_SIZE;

#ifdef FWUPDATE_DEBUG
void SendDataReportToNode(BYTE dstNode, BYTE classcmd, BYTE cmd, BYTE * reportdata, BYTE numdata);
#endif	

#if FIRMWARE_UPGRADE_BATTERY_SAVING
BYTE* FWUpdate_GetStatePtr(); 
void FWUpdate_SaveState();
void FWUpdate_Clear();
#endif

/****
                    Внешние функции FWUpdate
 ****/

void FWUpdate_Init()
{
        fw_fsm_model[0].triggeredAction    =   fsmFWUpdaterHandler_p;
        g_fwupdate_fsmid                   =   INVALID_FSM_ID;

        ZW_FirmwareUpdate_NVM_Init();
}
void FWUpdate_Authentificate()
{
        if( g_fwupdate_fsmid != INVALID_FSM_ID )
            ZMEFSM_TriggerEvent(g_fwupdate_fsmid, FSMFW_E_Authentificated, NULL);
}
/****
                    Внутренние функции 
 ****/

void zmeDecryptFWSegment(DWORD offset, BYTE *buf, WORD length);
WORD calcFWCRC(void);
void startCRCTest();
void getCRCTestResult(BYTE * buff);

//BYTE dbg_global_crc_vec[32];
//BYTE g_dbg_buff[48];

extern BYTE g_crypto_initialized;
extern WORD g_crc_param;
extern WORD g_crc_addr;

extern BYTE g_crypto_initialized;

#if FIRMWARE_UPGRADE_BATTERY_SAVING 
#message "Compiling with FirmwareUpdateBatterySaving flag"

BOOL isFWUInProgress()
{
    return  (g_fwu_data.senderNode != 0);
}

 void fwuWakeup()
 {
      BYTE * p_state_vec = FWUpdate_GetStatePtr();

      // Проверяем, что в сохраненное состояние содержит корректные данные
      if(*p_state_vec == 0)
        return; // Не нужно возобновлять загрузку

      // Загружаем последнее состояние
      memcpy(((BYTE*)&g_fwu_data) + 1, p_state_vec, STATEVECTOR_SIZE);  

      // Начальная инициализация
      g_fwu_data.retryRemains           =   MAX_RETRY_COUNT;
      g_fwu_data.packetBatteryRemains   =   FIRMWARE_UPGRADE_MAX_WAKEUP_PACKETS;

      // Запускаем КА
      g_fwupdate_fsmid        =   ZMEFSM_addFsm(&fw_fsm_model[0], (BYTE*)&g_fwu_data);
      if(g_fwupdate_fsmid == INVALID_FSM_ID)
        return;
      ZMEFSM_TriggerEvent(g_fwupdate_fsmid, FSMFW_E_ResumeAfterWakeup, NULL);
 }

 void fwuSaveState()
 {
    BYTE * p_state_vec = FWUpdate_GetStatePtr();
    memcpy(p_state_vec, ((BYTE*)&g_fwu_data) + 1, STATEVECTOR_SIZE);  
    FWUpdate_SaveState();
 }
 #endif

 BYTE fwuSendCurrentFWVersion(BYTE dstNode)
 {
	BYTE i = 0;
        
        if((txBuf = CreatePacket()) == NULL)
                return FALSE; 

    // report V3 
	txBuf->ZW_FirmwareMdReport4byteV3Frame.cmdClass = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V3;
	txBuf->ZW_FirmwareMdReport4byteV3Frame.cmd = FIRMWARE_MD_REPORT_V3;

	txBuf->ZW_FirmwareMdReport4byteV3Frame.manufacturerId1 = firmwareDescriptor.manufacturerID >> 8;
	txBuf->ZW_FirmwareMdReport4byteV3Frame.manufacturerId2 = firmwareDescriptor.manufacturerID & 0xFF;
	txBuf->ZW_FirmwareMdReport4byteV3Frame.firmware0Id1 = firmwareDescriptor.firmwareID >> 8;
	txBuf->ZW_FirmwareMdReport4byteV3Frame.firmware0Id2 = firmwareDescriptor.firmwareID & 0xFF;
	txBuf->ZW_FirmwareMdReport4byteV3Frame.firmware0Checksum1 = firmwareDescriptor.checksum >> 8;
	txBuf->ZW_FirmwareMdReport4byteV3Frame.firmware0Checksum2 = firmwareDescriptor.checksum & 0xFF;
	txBuf->ZW_FirmwareMdReport4byteV3Frame.firmwareUpgradable = FIRMWARE_UPGRADE_ZWAVE_UPGRADABLE & 0xFF;
	txBuf->ZW_FirmwareMdReport4byteV3Frame.numberOfFirmwareTargets = FIRMWARE_UPGRADE_NUMBER_OF_FIRMWARE_TARGETS & 0xFF;
    txBuf->ZW_FirmwareMdReport4byteV3Frame.maxFragmentSize1 = FIRMWARE_UPGRADE_MAX_FRAGMENT_SIZE >> 8;
    txBuf->ZW_FirmwareMdReport4byteV3Frame.maxFragmentSize2 = FIRMWARE_UPGRADE_MAX_FRAGMENT_SIZE & 0xFF;
    for (; i < FIRMWARE_UPGRADE_NUMBER_OF_FIRMWARE_TARGETS; i++)  // max 8
    	*((WORD*)(&txBuf->ZW_FirmwareMdReport4byteV3Frame.variantgroup1) + i) = 
        (WORD)(FIRMWARE_UPGRADE_EXTERNAL_FWID(i) >> 8) | (FIRMWARE_UPGRADE_EXTERNAL_FWID(i) & 0xFF); //Firmware i ID 1
        
	EnqueuePacket(dstNode, sizeof(txBuf->ZW_FirmwareMdReport1byteV3Frame) + 2 * FIRMWARE_UPGRADE_NUMBER_OF_FIRMWARE_TARGETS, TRANSMIT_OPTION_USE_DEFAULT);

        //test_func_link = 0x1300;
        //test_func_link(g_dbg_buff);
        //startCRCTest();
        /*
        g_dbg_buff[0] = ZW_FirmwareUpdate_NVM_isValidCRC16((WORD*)(g_dbg_buff + 1)) ? 1 : 0;
        g_dbg_buff[3] = g_crypto_initialized;
        g_dbg_buff[4] = g_crc_addr >> 8;
        g_dbg_buff[5] = g_crc_addr & 0xFF;
        g_dbg_buff[6] = g_crc_param >> 8;
        g_dbg_buff[7] = g_crc_param & 0xFF;
	 */
        //getCRCTestResult(g_dbg_buff+5);
        //zmeDecryptFWSegment(0x7808, g_dbg_buff + 3, 2);
        
	//SendDataReportToNode(1, COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2, 0xab, g_dbg_buff, 30);
        /*
        ZW_FirmwareUpdate_NVM_Set_NEWIMAGE(1);
        //Перезагружаем устройство
        ZW_WatchDogEnable(); 
        while(1);
        */
        
        return TRUE;        
 }   

 void fwuStartProcess(BYTE senderNode, BYTE * reqcmd)
 {
    g_fwu_data.senderNode   =   senderNode;
    g_fwupdate_fsmid        =   ZMEFSM_addFsm(&fw_fsm_model[0], (BYTE*)&g_fwu_data);

    if(g_fwupdate_fsmid == INVALID_FSM_ID)
        return;
    
    ZMEFSM_TriggerEvent(g_fwupdate_fsmid, FSMFW_E_Start, reqcmd);
    
 }

 BYTE fwuCheckFWVersion(BYTE * pCmd)
 {
        if( (pCmd[2] == (firmwareDescriptor.manufacturerID >> 8) )&&
            (pCmd[3] == (firmwareDescriptor.manufacturerID & 0xFF)) &&
            (pCmd[4] == (firmwareDescriptor.firmwareID >> 8) )&&
            (pCmd[5] == (firmwareDescriptor.firmwareID & 0xFF))
            )
        {
            g_fwu_data.wholeChecksum = (((WORD)pCmd[6]) << 8) + pCmd[7];
            return TRUE;
        }

        return FALSE;

 }  

 BYTE fwuRequestReport(BYTE status)
 {
        if((txBuf = CreatePacket()) == NULL)
                return FALSE; 

	txBuf->ZW_FirmwareUpdateMdRequestReportV3Frame.cmdClass = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V3;
	txBuf->ZW_FirmwareUpdateMdRequestReportV3Frame.cmd = FIRMWARE_UPDATE_MD_REQUEST_REPORT_V3;
    txBuf->ZW_FirmwareUpdateMdRequestReportV3Frame.status = status;
            
	EnqueuePacketFSM(g_fwu_data.senderNode, sizeof(txBuf->ZW_FirmwareUpdateMdRequestReportV3Frame), TRANSMIT_OPTION_USE_DEFAULT, g_fwupdate_fsmid);

        return TRUE;
 }    

BYTE fwuStatusReport(BYTE status) {
        BYTE result = 1;
	BYTE packet_size = sizeof(txBuf->ZW_FirmwareUpdateMdStatusReportV3Frame);
        char const code* p_cc_params;

        if((txBuf = CreatePacket()) == NULL)
                return FALSE; 

	txBuf->ZW_FirmwareUpdateMdStatusReportV3Frame.cmdClass = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V3;
	txBuf->ZW_FirmwareUpdateMdStatusReportV3Frame.cmd = FIRMWARE_UPDATE_MD_STATUS_REPORT_V3;
	txBuf->ZW_FirmwareUpdateMdStatusReportV3Frame.status = status;
    // V3
    txBuf->ZW_FirmwareUpdateMdStatusReportV3Frame.waittime1 = FIRMWARE_UPGRADE_WAIT_TIME >> 8 & 0xFF;
    txBuf->ZW_FirmwareUpdateMdStatusReportV3Frame.waittime2 = FIRMWARE_UPGRADE_WAIT_TIME & 0xFF;

	if(status == 0xFF) {
		BYTE * aux_res = ((BYTE*)&txBuf->ZW_FirmwareUpdateMdStatusReportV3Frame.waittime2) + 1;
          g_crypto_initialized = 0;
		result = ZW_FirmwareUpdate_NVM_isValidCRC16((WORD*)(aux_res + 1));
          aux_res[0] = result;
          packet_size  += 3;
        }    
                
            
        EnqueuePacketFSM(g_fwu_data.senderNode, packet_size, 
                            TRANSMIT_OPTION_USE_DEFAULT, g_fwupdate_fsmid);



        #if FIRMWARE_UPGRADE_BATTERY_SAVING
        // Нужно обнулить узел с которого все обновляется, иначе продолжит обновляться...
        g_fwu_data.senderNode   =   0;
        fwuSaveState();
        #endif

        return (result != 0);
 }    
 
 BOOL fwuRequestPacket()
 {
        if((txBuf = CreatePacket()) == NULL)
                return FALSE; 

        txBuf->ZW_FirmwareUpdateMdGetV2Frame.cmdClass          =   COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2;
        txBuf->ZW_FirmwareUpdateMdGetV2Frame.cmd               =   FIRMWARE_UPDATE_MD_GET_V2;

        txBuf->ZW_FirmwareUpdateMdGetV2Frame.numberOfReports   =   1;
        txBuf->ZW_FirmwareUpdateMdGetV2Frame.properties1       =   g_fwu_data.packetIndex >> 8;
        txBuf->ZW_FirmwareUpdateMdGetV2Frame.reportNumber2     =   g_fwu_data.packetIndex & 0xFF;
         
        EnqueuePacketFSM(g_fwu_data.senderNode, sizeof(txBuf->ZW_FirmwareUpdateMdGetV2Frame), 
                            TRANSMIT_OPTION_USE_DEFAULT, g_fwupdate_fsmid);

        return TRUE;
 }   

BYTE fwuCheckPacket(BYTE * pCmd) {
       BYTE     len             =   pCmd[0];
       BYTE     packetlen       =   DATA_SIZE(len);
       BYTE     frm_index;
       WORD     crc16Result; 
       DWORD    addr;

       pCmd[0]  =  COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2;
       crc16Result = ZW_CheckCrc16(0x1D0F, pCmd, len);

       if(g_fwu_data.packetIndex == 1)
          g_fwu_data.update_packetsize = packetlen;

       if(crc16Result != 0)
            return CHECKPACKET_LOC_CRCERROR;

	g_fwu_data.accumulatedCheckSum = ZW_CheckCrc16(g_fwu_data.accumulatedCheckSum, DATA_PTR(pCmd), packetlen);
       addr  = ((DWORD)(g_fwu_data.packetIndex-1))*((DWORD)g_fwu_data.update_packetsize);

    // chipId 0 - Z-Wave
	if (g_fwu_data.chipId == 0)
       ZW_FirmwareUpdate_NVM_Write(DATA_PTR(pCmd), packetlen, addr);
	else
		NVM_ext_write_long_buffer(FWUPDATE_FIRMWARE_START(g_fwu_data.chipId) + addr, DATA_PTR(pCmd), packetlen);

	if(pCmd[2] & FIRMWARE_UPDATE_MD_REPORT_PROPERTIES1_LAST_BIT_MASK_V2){
               
            if(g_fwu_data.accumulatedCheckSum  != g_fwu_data.wholeChecksum) 
               return CHECKPACKET_WHL_CRCERROR;
                
            return CHECKPACKET_SUCESS;
      }
     
      g_fwu_data.packetIndex++;
      
      #if FIRMWARE_UPGRADE_BATTERY_SAVING
      g_fwu_data.packetBatteryRemains--;
      if(g_fwu_data.packetBatteryRemains == 0)
        return CHECKPACKET_GO_SLEEP;
      #endif
      g_fwu_data.retryRemains           =       MAX_RETRY_COUNT;

      return CHECKPACKET_REQ_NEXT;
 }       

/****
	Обработчик входящих сообщений для FWUpdate
****/
void FWUpdateCommandHandler(BYTE sourceNode, ZW_APPLICATION_TX_BUFFER *pCmd, BYTE cmdLength){
	BYTE command = ((BYTE*)pCmd)[1];

    //memset(g_dbg_buff, 0, 32);
	
	switch (command) {
			case FIRMWARE_MD_GET_V2:
                fwuSendCurrentFWVersion(sourceNode);
				break; 
			case FIRMWARE_UPDATE_MD_REQUEST_GET_V2:
                {
                        BYTE * pbCmd =  (BYTE*)pCmd;
                        pbCmd[0]  =    cmdLength;
                        #if FIRMWARE_UPGRADE_BATTERY_SAVING   
                        FWUpdate_Clear();
                        #endif
                        fwuStartProcess(sourceNode, pbCmd);
                }
				break;
			case FIRMWARE_UPDATE_MD_REPORT_V2:
                {
                    BYTE * pbCmd            =  (BYTE*)pCmd;
                    WORD   packetNumber     =  (((WORD)(pbCmd[2]&FIRMWARE_UPDATE_MD_REPORT_PROPERTIES1_REPORT_NUMBER_1_MASK_V2)) <<  8) | pbCmd[3];

                    pbCmd[0]  =    cmdLength;

			if( g_fwupdate_fsmid != INVALID_FSM_ID 
                &&  packetNumber == g_fwu_data.packetIndex)
                            ZMEFSM_TriggerEvent(g_fwupdate_fsmid, FSMFW_E_PacketReceived, pbCmd);
                        }
				break;
	}
	
}

void fsmFWUpdaterHandler(FSM_ACTION * action) reentrant {
	BYTE actionDataLength = action->incoming_data[0];
	switch(action->rule->nextState) {
		case FSMFW_CheckFWVersion: {
			g_fwu_data.chipId = 0;
            //sendBuffer2Uart(&(action->incoming_data[0]), 11, 'I');
			if(actionDataLength >= 8 && action->incoming_data[8] != 1) {
                g_fwu_data.chipId = action->incoming_data[8];
                g_fwu_data.wholeChecksum = (((WORD)action->incoming_data[6]) << 8) + action->incoming_data[7];
                ZMEFSM_TriggerEvent(g_fwupdate_fsmid, FSMFW_E_Valid, NULL);
                #ifdef FWUPDATE_AUTH_HANDLER
				FWUPDATE_AUTH_HANDLER;
                #endif									
			} else {
                 if(fwuCheckFWVersion(action->incoming_data)) {
					ZW_FirmwareUpdate_NVM_Set_NEWIMAGE(0);
                    ZMEFSM_TriggerEvent(g_fwupdate_fsmid, FSMFW_E_Valid, NULL);
                    #ifdef FWUPDATE_AUTH_HANDLER
                    FWUPDATE_AUTH_HANDLER;
                    #endif
				} else
                    ZMEFSM_TriggerEvent(g_fwupdate_fsmid, FSMFW_E_Invalid, NULL); 
			}
		}
                 break;
            case FSMFW_WaitForAuth:
                 #if !FWUPDATE_MANUAL_AUTHORIZATION
                    #warning "Please Call this function MANUALLY! When you are ready to update. For certification"
                    FWUpdate_Authentificate();
                 #endif
                 break;

            case  FSMFW_SendAuthFailed:
                 fwuRequestReport(0x01);  
                 break;

            case  FSMFW_SendInvalidVersion:
                 fwuRequestReport(0x00);  
                 break;

            case  FSMFW_SendAccepted:
                 g_fwu_data.retryRemains           =       MAX_RETRY_COUNT;
                 g_fwu_data.packetIndex            =       1; 
                 g_fwu_data.accumulatedCheckSum    =       0x1D0F;
                 g_fwu_data.update_packetsize      =       PACKET_SIZE;
                 #if FIRMWARE_UPGRADE_BATTERY_SAVING 
                 g_fwu_data.packetBatteryRemains   =       FIRMWARE_UPGRADE_MAX_WAKEUP_PACKETS;
                 #endif

                 fwuRequestReport(0xFF);   
                 break;

            case  FSMFW_SendRequestPacket:
                 fwuRequestPacket();
                 break;

            case  FSMFW_CheckPacket:
                 {
                     BYTE rs = fwuCheckPacket(action->incoming_data);
                     ZMEFSM_TriggerEvent(g_fwupdate_fsmid, g_chk_packet_events[rs], NULL);    
                 }
                 break;

            case  FSMFW_CheckRetryCount:
                 {
                     g_fwu_data.retryRemains--;
				ZMEFSM_TriggerEvent(g_fwupdate_fsmid, g_fwu_data.retryRemains == 0 ? FSMFW_E_Invalid : FSMFW_E_Valid, NULL);
                 }
                 break;

            case  FSMFW_SendTransmissionFails:
                 fwuStatusReport(0x01);    
                 break;

            case  FSMFW_SendTransmissionInvalidCRC:
                 fwuStatusReport(0x00);    
                 break;

            case  FSMFW_SendTransmissionDone:
			if(fwuStatusReport(0xFF)) {
				if (g_fwu_data.chipId == 0)
                        ZW_FirmwareUpdate_NVM_Set_NEWIMAGE(1);
				else {
					#ifdef CUSTOM_FW_FLASH
                    #warning "Custom flash update..."
					CUSTOM_FW_FLASH(g_fwu_data.chipId, ((DWORD)(g_fwu_data.packetIndex-1))*((DWORD)g_fwu_data.update_packetsize));
					#endif
				}
			}
                 break;  
            case FSMFW_WaitForCompletion:
                  {
                  }
                  break;          

            case FSMFW_GoSleep:
                {
                    #if FIRMWARE_UPGRADE_BATTERY_SAVING   
                    fwuSaveState();
                    #endif
                }
                break;
            case  FSMFW_Done:
                 {

                     #ifdef FWUPDATE_DONE_HANDLER
                        #warning "Custom update handler..."
                        FWUPDATE_DONE_HANDLER;
                     #else
    			if(g_fwu_data.chipId == 0) {
                        // Перезагружаем устройство
                        ZW_WatchDogEnable(); 
                        while(1);
    			}
                     #endif
                 }
                 break;  
              
            case  FSMFW_Failed:
			if(g_fwu_data.chipId == 0)
				        ZW_FirmwareUpdate_NVM_Set_NEWIMAGE(0);
            break;
                   
            case  FSMFW_Destroy:
                    g_fwupdate_fsmid = INVALID_FSM_ID;
                break;
    }
} 

#endif // WITH_CC_FIRMWARE_UPDATE