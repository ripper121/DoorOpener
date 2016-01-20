#include "ZME_Includes.h"

#include "ZME_Common.h"
#if WITH_CC_SECURITY
#include "ZME_Security.h"
#endif

#if WITH_CC_FIRMWARE_UPGRADE
#include "ZME_FWUpdate.h"
#endif

#if WITH_FSM
#include "ZME_FSM.h"
#endif

#ifdef ZW050x
#include <ZW050x.h>
#else
#ifdef ZW030x
#include <ZW030x.h>
#endif
#endif
#include <ZW_typedefs.h>
#include <ZW_nvr_api.h>
#include <ZW_nvm_ext_api.h>

BYTE txOption;

#if WITH_PROPRIETARY_EXTENTIONS
void FreqChange(BYTE freq);
#endif

#define EXTERN_CONST_CODE(var)			extern BYTE const code var
//#define EXTERN_CONST_CODE(var)			extern BYTE var

#define CONST_CODE(var, val)			EXTERN_CONST_CODE(var)  

EXTERN_CONST_CODE(_MANUFACTURER_ID_MAJOR);
EXTERN_CONST_CODE(_MANUFACTURER_ID_MINOR);
EXTERN_CONST_CODE(_PRODUCT_TYPE_ID_MAJOR);
EXTERN_CONST_CODE(_PRODUCT_TYPE_ID_MINOR);
EXTERN_CONST_CODE(_APP_VERSION_MAJOR_);
EXTERN_CONST_CODE(_APP_VERSION_MINOR_);
EXTERN_CONST_CODE(_PRODUCT_FIRMWARE_VERSION_MAJOR);
EXTERN_CONST_CODE(_PRODUCT_FIRMWARE_VERSION_MINOR);
EXTERN_CONST_CODE(_PRODUCT_GENERIC_TYPE);
EXTERN_CONST_CODE(_PRODUCT_SPECIFIC_TYPE);



#ifdef CUSTOM_CONST_CODE_PARAMETERS
CUSTOM_CONST_CODE_PARAMETERS
#endif

#include "Custom_Code.inc"

#if WITH_CC_MULTICHANNEL || WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
BYTE mch_src_end_point = 0;   // Номер канала устройства отправителя, нужен для того, чтобы не передавать номер канала во все обработчики
							  	 // и упростить обработку многоканальных команд
BYTE mch_dst_end_point = 0;   // Номер канала устройства получателя, нужен для того, чтобы не передавать номер канала во все обработчики
							  	 // и упростить обработку многоканальных команд
BYTE mch_transform_enabled = FALSE;
#endif

BYTE g_one_poll = FALSE; 
static BYTE timer10msHandler = 0xff;




void SendDataReportToNode(BYTE dstNode, BYTE classcmd, BYTE cmd, BYTE * reportdata, BYTE numdata); // ! debug

#if WITH_TRIAC
void TRIAC_10MSTimer();
#endif
CBK_DEFINE(static, void, Timer10ms, (void)) {
	static BYTE tickCounter = 1;

	#if NUM_BUTTONS > 0
	ButtonsPoll(); // Do buttons poll every 10 ms
	#endif

	#if defined(T10MS_TICK)
	// #warning "Timer: User defined 10ms timer."
	T10MS_TICK();
	#endif

	#if EXPLORER_FRAME_TIMOUT
	queue10msTick();
	#endif

	tickCounter--;

	if (tickCounter % 10 == 0) {
		LEDTick();
		#if WITH_FSM
		ZMEFSM_ProcessTimer();
		#endif
		#if WITH_CC_SECURITY
		SecurityNonceTimer();
		#endif
		#if WITH_CC_CENTRAL_SCENE
    	CentralSceneRepeat();
		#endif
		#ifdef T100MS_TICK
    	T100MS_TICK();
		#endif
	}
	
	#if defined(T50MS_TICK)
	if (tickCounter % 5 == 0) {
    	T50MS_TICK();		
	}
  #endif
	
  #if defined(T20MS_TICK)
	if (tickCounter % 2 == 0) {
    	T20MS_TICK();		
	}
  #endif
	
	#if defined(QUARTER_SECOND_TICK)
	if (tickCounter % 25 == 0) {
		QUARTER_SECOND_TICK();
	}
	#endif

	if (tickCounter == 0) {
		tickCounter = 100; // one second
		#if WITH_CC_PROTECTION
		if (protectionSecondsDoCount && protectionSecondsCounter < PROTECTION_LOCK_TIMEOUT) {
			protectionSecondsCounter++;
			if (protectionSecondsCounter == PROTECTION_LOCK_TIMEOUT) {
				protectionSecondsDoCount = FALSE;
				protectedUnlocked = FALSE;
			}
		}
		#endif
		#if defined(ONE_SECOND_TICK)
		ONE_SECOND_TICK();
		#endif
		#if WITH_CC_POWERLEVEL
		PowerLevel1SecTick();
		#endif
		#if WITH_CC_DOOR_LOCK
		if (VAR(DOOR_LOCK_TIMEOUT_FLAG)) {
			if (VAR(DOOR_LOCK_TIMEOUT_MINUTES) > 0) {
				if (VAR(DOOR_LOCK_TIMEOUT_SECONDS) > 0) {
					VAR(DOOR_LOCK_TIMEOUT_SECONDS)--;
				} else {
					VAR(DOOR_LOCK_TIMEOUT_SECONDS) = 59;
					VAR(DOOR_LOCK_TIMEOUT_MINUTES)--;
				}
			} else {
				if (VAR(DOOR_LOCK_TIMEOUT_SECONDS) > 0) {
					VAR(DOOR_LOCK_TIMEOUT_SECONDS)--;
				} else {
					VAR(DOOR_LOCK_TIMEOUT_FLAG) = FALSE;
					pDoorLockData->mode = DOOR_LOCK_OPERATION_SET_DOOR_SECURED;
					DOOR_LOCK_OPERATION_SET_ACTION(DOOR_LOCK_OPERATION_SET_DOOR_SECURED_V2);
					sendDoorLockOperationReportToGroup(ASSOCIATION_REPORTS_GROUP_NUM);
				}
			}
			SAVE_EEPROM_BYTE(DOOR_LOCK_TIMEOUT_SECONDS);
			SAVE_EEPROM_BYTE(DOOR_LOCK_TIMEOUT_MINUTES);
		}
		#endif
	}
}

void RebootChip(void) {
	ZW_WATCHDOG_ENABLE; // Enable contains Kick
	while ( TRUE ) {};
}

#if WITH_PROPRIETARY_EXTENTIONS
#if defined(ZW050x)

CBK_DEFINE( , void, FreqChangeDo, (void)) {
  BYTE    i;
  WORD    j;
  BYTE    unlockString[4] = {0xDE, 0xAD, 0xBE, 0xEF};
  BYTE    sector_num = REFLASH_SEG_ADDR / 2048;
  XBYTE  * seg_data_ptr = 0x3000; // at 12 KB

  // Выключаем все, что может помешать...
  EA = 0;

  // Считываем весь сегмент, который будем перезаписывать
  for(j=0;j<2048;j++)
        seg_data_ptr[j] = ((char const code *)(REFLASH_SEG_ADDR))[j];


  // Очищаем сегмент
  ZW_FLASH_code_prog_unlock(unlockString);
  if(ZW_FLASH_code_sector_erase(sector_num)){
  }
  ZW_FLASH_code_prog_lock();
  

  if(ZW_FLASH_prog_state())
  {
  }

      
  
  // Пишим страницы сектора
  for(i=0; i<8; i++, seg_data_ptr += 256)
  {
        
      // Копируем часть сектора в 1-ые 4k
      for(j=0;j<256;j++)
        reflashPage[j] = seg_data_ptr[j];


      if(i == 0) // Данные первой страницы содержат таблицу частот - меняем их
      {
          // EU   000D45DA000D403000000000020C0001
          // US   000DFA20000DDC7000000000020C0001
          // RU   000D4288000D428800000000020C0001
          // IN   000D33B0000D33B000000000020C0001
          // ANZ  000E08F8000E0F3800000000020C0000
          // HK   000E08F8000E08F800000000020C0001
          // CN   000D4030000D403000000000020C0001
          // JP   000E1384000E18FC000E225C02020200
          // KR   000E0894000E1064000E15DC02020206
          // IL   000DFA20000DFA2000000000020C0001
          // MY   000D3F04000D3F0400000000020C0001


          //      0 1 2 3 4 5 6 7 8 9 a b c d e f
          //          xxxx    xxxx

          // EU по-умолчанию
          reflashPage[REFLASH_CODE_SHIFT + 0x00] = 0x00;
          reflashPage[REFLASH_CODE_SHIFT + 0x01] = 0x0d;
          reflashPage[REFLASH_CODE_SHIFT + 0x02] = 0x45;
          reflashPage[REFLASH_CODE_SHIFT + 0x03] = 0xDA;
          reflashPage[REFLASH_CODE_SHIFT + 0x04] = 0x00;
          reflashPage[REFLASH_CODE_SHIFT + 0x05] = 0x0D;
          reflashPage[REFLASH_CODE_SHIFT + 0x06] = 0x40;
          reflashPage[REFLASH_CODE_SHIFT + 0x07] = 0x30;
          reflashPage[REFLASH_CODE_SHIFT + 0x08] = 0x00;
          reflashPage[REFLASH_CODE_SHIFT + 0x09] = 0x00;
          reflashPage[REFLASH_CODE_SHIFT + 0x0A] = 0x00;
          reflashPage[REFLASH_CODE_SHIFT + 0x0B] = 0x00;
          reflashPage[REFLASH_CODE_SHIFT + 0x0C] = 0x02;
          reflashPage[REFLASH_CODE_SHIFT + 0x0D] = 0x0C;
          reflashPage[REFLASH_CODE_SHIFT + 0x0E] = 0x00;
          reflashPage[REFLASH_CODE_SHIFT + 0x0F] = 0x01;

          switch (freqChangeCode) {
              case FREQ_EU:
                  // Не нужно ничего менять - это уже по-умолчанию...
                  break;
              case FREQ_RU:
                  reflashPage[REFLASH_CODE_SHIFT + 0x02] = 0x42;
                  reflashPage[REFLASH_CODE_SHIFT + 0x03] = 0x88;
                  reflashPage[REFLASH_CODE_SHIFT + 0x06] = 0x42;
                  reflashPage[REFLASH_CODE_SHIFT + 0x07] = 0x88;
                  break;
              case FREQ_IN:
                  reflashPage[REFLASH_CODE_SHIFT + 0x02] = 0x33;
                  reflashPage[REFLASH_CODE_SHIFT + 0x03] = 0xB0;
                  reflashPage[REFLASH_CODE_SHIFT + 0x06] = 0x33;
                  reflashPage[REFLASH_CODE_SHIFT + 0x07] = 0xB0;
                  break;
              case FREQ_US:
                  reflashPage[REFLASH_CODE_SHIFT + 0x02] = 0xFA;
                  reflashPage[REFLASH_CODE_SHIFT + 0x03] = 0x20;
                  reflashPage[REFLASH_CODE_SHIFT + 0x06] = 0xDC;
                  reflashPage[REFLASH_CODE_SHIFT + 0x07] = 0x70;
                  break;
              case FREQ_ANZ:
                  reflashPage[REFLASH_CODE_SHIFT + 0x01] = 0x0E;
                  reflashPage[REFLASH_CODE_SHIFT + 0x02] = 0x08;
                  reflashPage[REFLASH_CODE_SHIFT + 0x03] = 0xF8;
                  reflashPage[REFLASH_CODE_SHIFT + 0x05] = 0x0E;
                  reflashPage[REFLASH_CODE_SHIFT + 0x06] = 0x0F;
                  reflashPage[REFLASH_CODE_SHIFT + 0x07] = 0x38;
                  reflashPage[REFLASH_CODE_SHIFT + 0x0E] = 0x01;
                  break;
              case FREQ_HK:
                  reflashPage[REFLASH_CODE_SHIFT + 0x01] = 0x0E;
                  reflashPage[REFLASH_CODE_SHIFT + 0x02] = 0x08;
                  reflashPage[REFLASH_CODE_SHIFT + 0x03] = 0xF8;
                  reflashPage[REFLASH_CODE_SHIFT + 0x05] = 0x0E;
                  reflashPage[REFLASH_CODE_SHIFT + 0x06] = 0x08;
                  reflashPage[REFLASH_CODE_SHIFT + 0x07] = 0xF8;
                  break;
               case FREQ_CN:
                  reflashPage[REFLASH_CODE_SHIFT + 0x02] = 0x40;
                  reflashPage[REFLASH_CODE_SHIFT + 0x03] = 0x30;
                  break;  
               case FREQ_JP:
                  reflashPage[REFLASH_CODE_SHIFT + 0x01] = 0x0E;
                  reflashPage[REFLASH_CODE_SHIFT + 0x02] = 0x13;
                  reflashPage[REFLASH_CODE_SHIFT + 0x03] = 0x84;
                  reflashPage[REFLASH_CODE_SHIFT + 0x05] = 0x0E;
                  reflashPage[REFLASH_CODE_SHIFT + 0x06] = 0x18;
                  reflashPage[REFLASH_CODE_SHIFT + 0x07] = 0xFC;
                  reflashPage[REFLASH_CODE_SHIFT + 0x09] = 0x0E;
                  reflashPage[REFLASH_CODE_SHIFT + 0x0A] = 0x22;
                  reflashPage[REFLASH_CODE_SHIFT + 0x0B] = 0x5C;
                  reflashPage[REFLASH_CODE_SHIFT + 0x0D] = 0x02;
                  reflashPage[REFLASH_CODE_SHIFT + 0x0E] = 0x02;
                  reflashPage[REFLASH_CODE_SHIFT + 0x0F] = 0x00;
                  break; 
               case FREQ_KR:
                  reflashPage[REFLASH_CODE_SHIFT + 0x01] = 0x0E;
                  reflashPage[REFLASH_CODE_SHIFT + 0x02] = 0x08;
                  reflashPage[REFLASH_CODE_SHIFT + 0x03] = 0x94;
                  reflashPage[REFLASH_CODE_SHIFT + 0x05] = 0x0E;
                  reflashPage[REFLASH_CODE_SHIFT + 0x06] = 0x10;
                  reflashPage[REFLASH_CODE_SHIFT + 0x07] = 0x64;
                  reflashPage[REFLASH_CODE_SHIFT + 0x09] = 0x0E;
                  reflashPage[REFLASH_CODE_SHIFT + 0x0A] = 0x15;
                  reflashPage[REFLASH_CODE_SHIFT + 0x0B] = 0xDC;
                  reflashPage[REFLASH_CODE_SHIFT + 0x0D] = 0x02;
                  reflashPage[REFLASH_CODE_SHIFT + 0x0E] = 0x02;
                  reflashPage[REFLASH_CODE_SHIFT + 0x0F] = 0x06;
                  break;  
                case FREQ_IL:
                  reflashPage[REFLASH_CODE_SHIFT + 0x02] = 0xFA;
                  reflashPage[REFLASH_CODE_SHIFT + 0x03] = 0x20;
                  reflashPage[REFLASH_CODE_SHIFT + 0x06] = 0xFA;
                  reflashPage[REFLASH_CODE_SHIFT + 0x07] = 0x20;
                  break;
                case FREQ_MY:
                  reflashPage[REFLASH_CODE_SHIFT + 0x02] = 0x3F;
                  reflashPage[REFLASH_CODE_SHIFT + 0x03] = 0x04;
                  reflashPage[REFLASH_CODE_SHIFT + 0x06] = 0x3F;
                  reflashPage[REFLASH_CODE_SHIFT + 0x07] = 0x04;
                  break;

          }
        }
        
        
  
        // Пишим страницу
        ZW_FLASH_code_prog_unlock(unlockString);
        if(ZW_FLASH_code_page_prog(reflashPage, sector_num, i)){
           
           
        }
        ZW_FLASH_code_prog_lock();

  }

  RebootChip();
}
#else // 3rd generation freq change code
void FreqChangeDo(void) {
	BYTE i;

	EA=0;

	ZW_WATCHDOG_ENABLE; // Enable contains Kick

	FLCON = 0; // setup the xdata offset - at the beginning of XDATA
	FLADR = 0x7f; // setup the flash offset (page) register - we want only last page
	i = 0;
	do {
		((XBYTE *)0)[i] = ((char const code *)(0x7f00))[i]; // write to the beginning of XDATA
		i++;
	} while (i != 0);

	switch (freqChangeCode) {
		case FREQ_EU:
			((XBYTE *)0)[0x89] = 0x0a;
			((XBYTE *)0)[0x8b] = 0x08;
			((XBYTE *)0)[0xb1] = 0x00;
			break;

	case FREQ_RU:
			((XBYTE *)0)[0x89] = 0x07;
			((XBYTE *)0)[0x8b] = 0x05;
			((XBYTE *)0)[0xb1] = 0x0a;
			break;

	case FREQ_IN:
			((XBYTE *)0)[0x89] = 0x5a;
			((XBYTE *)0)[0x8b] = 0x58;
			((XBYTE *)0)[0xb1] = 0x09;
			break;
	}

	// start the write operation
	FLCON |= FLCON_FLPRGI;
	FLCON |= FLCON_FLPRGS;

	RebootChip();
}
#endif

void FreqChange(BYTE freq) {
	freqChangeCode = freq;
	hardBlock = TRUE;
	if (ZW_TIMER_START(CBK_POINTER(FreqChangeDo), 250, TIMER_ONE_TIME) != 0xff)
		LEDSet(LED_DONE);
	else
		LEDSet(LED_FAILED);
}
#endif

BYTE ApplicationInitHW(BYTE bStatus) {	
	#if !PRODUCT_LISTENING_TYPE || PRODUCT_DYNAMIC_TYPE
		EX1 = 0; // disable INT1
	#endif
	
	INIT_CUSTOM_HW();

	LED_HW_INIT();
	LEDSet(LED_NO);
	LED_HW_SET_COLOR(LED_COLOR_NO);
	
	#if NUM_BUTTONS > 0
	ButtonsInit();
	ButtonsPoll(); // to trap pressed buttons on wakeup by buttons before we go into sleep
	#endif

	#if WITH_I2C_INTERFACE
	#warning "I2C Init"
	i2c_init();
	#endif

	if ((timer10msHandler = ZW_TIMER_START(CBK_POINTER(Timer10ms), 1, TIMER_FOREVER) == 0xFF))
		return FALSE;


	#if !PRODUCT_LISTENING_TYPE  || PRODUCT_DYNAMIC_TYPE
		wakeupReason = bStatus;
	#endif
	
	return TRUE;
}

#if WITH_TRIAC
/*
	#define TRIAC_IGBT 					FALSE
	#define TRIAC_PULSE_WIDTH 			45
	#define TRIAC_PULSE_PERIODQ 		12
	#define TRIAC_BRIDGE_TYPE	 		TRIAC_HALFBRIDGE_A
	#define TRIAC_ZEROX_MASK	 		TRUE
	#define TRIAC_ZEROX_INVERT	 		FALSE
	#define TRIAC_FREQUENCY	 			FREQUENCY_50HZ
	#define TRIAC_ZEROX_CORRECTION		0
	#define TRIAC_ZEROX_COR_PRESCALE	0
	#define TRIAC_KEEP_OFF				0
*/

void TRIAC_Init(void) {
	ZW_TRIAC_init(TRIAC_IGBT, 				//FALSE,  // Мы используем реальный triac,
				  TRIAC_PULSE_WIDTH,		//45,//50,//((WORD)VAR(TRIAC_T_PULSE_WIDTH) << 2), // Совместимость поведение пока не понятно
				  TRIAC_PULSE_PERIODQ,		//12,//15, //VAR(TRIAC_T_PULSE_WIDTH) + 10,
				  TRIAC_BRIDGE_TYPE,		//TRIAC_HALFBRIDGE_A,
				  TRIAC_ZEROX_MASK,			//TRUE,
				  TRIAC_ZEROX_INVERT,		//FALSE,
				  TRIAC_FREQUENCY,			//FREQUENCY_50HZ,
				  TRIAC_ZEROX_CORRECTION,	//0,
				  TRIAC_ZEROX_COR_PRESCALE,	//0,
				  TRIAC_KEEP_OFF			//0
				  );

	ZW_TRIAC_dimlevel_set(0);
	ZW_TRIAC_enable(TRUE);
}

BYTE 	g_triac_accepted 	= 	TRUE;
WORD    g_triac_last_level	=	0xFFFF;

void TRIAC_10MSTimer() {
	if(!g_triac_accepted) {
		g_triac_accepted = TRUE;
		if(!ZW_TRIAC_dimlevel_set(g_triac_last_level)) {
			g_triac_accepted = FALSE;
		}
	}
}

// Set TRIAC to level (0-99)
void TRIAC_Set(BYTE level) {
	WORD real_level = 0;
	
	if(level > 99)
		level = 99;

	if(level != 0) {
		real_level = level;
		real_level *= 7;
		real_level += 150;
		
	}

	if(g_triac_accepted && (real_level == g_triac_last_level))
		return;

	g_triac_last_level = real_level;
	g_triac_accepted = TRUE;
	if(!ZW_TRIAC_dimlevel_set(g_triac_last_level))
	{
			g_triac_accepted = FALSE;
	}
	#warning Strange place !!!!
	ZW_TRIAC_dimlevel_set(g_triac_last_level);
	//g_triac_accepted = ZW_TRIAC_dimlevel_set(real_level);	
}
#endif

void auxExcludeCCFromNIF(BYTE cc, BYTE * arr, BYTE* size) {
	BYTE i;
	BYTE index = 0xff; //To find the needed CC 

	for(i = 0; i < (*size); i++) { //run untill  the end 
		if (index == 0xff) {
			if (arr[i] == cc) //if we found needed CC 
				index = i;			//Mark it for us.
		} 

		if((index != 0xff) && ((i+1) < (*size)))	{ //if we fund something ad not on te end of the array
			arr[i] =  arr[i+1];	 											// move it 
		}	
	}

	if (index != 0xff)
		(*size)--;			//return the new size for the correct NIF
}

void auxIncludeCCToNIF(BYTE cc, BYTE * arr, BYTE* size) {
	BYTE i;
	
	for(i = ((*size)-1); i != 0xff; i--) { //run untill  the end 
		arr[i+1] =  arr[i];	 											// move it 
		if (arr[i] == COMMAND_CLASS_MARK) { //if we found mark
			arr[i] = cc;			// insert the ned CC
			break;
		}
	}

	(*size)++;			//return the new size for the correct NIF
}

void excludeCCFromNIF(BYTE cc) {
	auxExcludeCCFromNIF(cc, nodeInfoCCsNormal, &nodeInfoCCsNormal_size);
	#if WITH_CC_SECURITY
		auxExcludeCCFromNIF(cc, nodeInfoCCsOutsideSecurity, &nodeInfoCCsOutsideSecurity_size);
		auxExcludeCCFromNIF(cc, nodeInfoCCsInsideSecurity, 	&nodeInfoCCsInsideSecurity_size);
	#endif
}

/*
enum {
	CC_IS_UNSECURE,
	CC_IS_SECURE,
	CC_IS_UNSECURE_AND_SECURE,
};
*/

void includeCCToNIF(BYTE cc,BYTE ifSecure) { //Use this func to add the CC to NIF the definitions are above
	switch(ifSecure) {
		case CC_IS_UNSECURE:
		auxIncludeCCToNIF(cc, nodeInfoCCsNormal, &nodeInfoCCsNormal_size);
	#if WITH_CC_SECURITY
		auxIncludeCCToNIF(cc, nodeInfoCCsOutsideSecurity, &nodeInfoCCsOutsideSecurity_size);
	#endif
		break;
		
		case CC_IS_SECURE:
	#if WITH_CC_SECURITY
		auxIncludeCCToNIF(cc, nodeInfoCCsInsideSecurity, 	&nodeInfoCCsInsideSecurity_size);
	#endif
		break;
		
		case CC_IS_UNSECURE_AND_SECURE:
		auxIncludeCCToNIF(cc, nodeInfoCCsNormal, &nodeInfoCCsNormal_size);
	#if WITH_CC_SECURITY
		auxIncludeCCToNIF(cc, nodeInfoCCsOutsideSecurity, &nodeInfoCCsOutsideSecurity_size);
		auxIncludeCCToNIF(cc, nodeInfoCCsInsideSecurity, 	&nodeInfoCCsInsideSecurity_size);
	#endif
		break;
		
		default:
		break;
	}
}

#if WITH_CC_MULTI_COMMAND
void MultiCmdListZero(void) {
	NODE_MASK_ZERO(&(VAR(MULTICMD_NODE_LIST)));
	SAVE_EEPROM_ARRAY(MULTICMD_NODE_LIST, NODE_MASK_LENGTH);
	
}

BOOL IsNodeSupportMultiCmd(BYTE nodeId) {
	return NODE_MASK_IS_SET(&(VAR(MULTICMD_NODE_LIST)), nodeId);
}

void MultiCmdListAdd(BYTE nodeId) {
	NODE_MASK_SET(&(VAR(MULTICMD_NODE_LIST)), nodeId);
	SAVE_EEPROM_ARRAY(MULTICMD_NODE_LIST, NODE_MASK_LENGTH);
}
#endif

#if WITH_CC_FIRMWARE_UPGRADE

#if FIRMWARE_UPGRADE_BATTERY_SAVING
BYTE* FWUpdate_GetStatePtr() {
	BYTE * p_data;
	p_data = &VAR(FWUPGRADE_STATEVECTOR);
	return p_data;
}
void FWUpdate_SaveState() {

	SAVE_EEPROM_ARRAY(FWUPGRADE_STATEVECTOR, FWUPDATE_VECTOR_SIZE);
}
void FWUpdate_Clear()
{
	VAR(FWUPGRADE_STATEVECTOR) = 0;
	SAVE_EEPROM_ARRAY(FWUPGRADE_STATEVECTOR, FWUPDATE_VECTOR_SIZE);	
}

#endif

#endif

#if WITH_CC_SECURITY
#if SECURITY_DEBUG
void dbgSendSecurityMasks()
{
    SendDataReportToNode(255, SECURITY_DEBUG_CC, 0xda, &(VAR(SECURE_NODE_LIST)), NODE_MASK_LENGTH);
    SendDataReportToNode(255, SECURITY_DEBUG_CC, 0xdb, &(VAR(SECURE_NODE_LIST_TEST)), NODE_MASK_LENGTH);   
}
#endif

BYTE SecurityNodeId(void) {
	// Workaround! Added one more check due to a bug in SDK in Assign Complete calling back to late
	#ifndef ZW_CONTROLLER
	if(myNodeId == 0)
	#endif
	  MemoryGetID(NULL, &myNodeId);
	return myNodeId;
}

BYTE* SecurityKeyPtr(void) {
	return (BYTE *)(&(VAR(NETWORK_KEY)));
}

BYTE* SecurityModePtr(void) {
	return (BYTE *)(&(VAR(SECURITY_MODE)));
}

void SecureNodeListAdd(BYTE nodeId) {
	NODE_MASK_SET(&(VAR(SECURE_NODE_LIST)), nodeId);
	SAVE_EEPROM_ARRAY(SECURE_NODE_LIST, NODE_MASK_LENGTH);
}

void SecureNodeListRemove(BYTE nodeId) {
	NODE_MASK_CLEAR(&(VAR(SECURE_NODE_LIST)), nodeId);
	SAVE_EEPROM_ARRAY(SECURE_NODE_LIST, NODE_MASK_LENGTH);
}
void SecureTestedNodeListAdd(BYTE nodeId) {
	NODE_MASK_SET(&(VAR(SECURE_NODE_LIST_TEST)), nodeId);
	SAVE_EEPROM_ARRAY(SECURE_NODE_LIST_TEST, NODE_MASK_LENGTH);
}

void SecureTestedNodeListRemove(BYTE nodeId) {
	NODE_MASK_CLEAR(&(VAR(SECURE_NODE_LIST_TEST)), nodeId);
	SAVE_EEPROM_ARRAY(SECURE_NODE_LIST_TEST, NODE_MASK_LENGTH);
}
void SecureNodeListZero(void) {
	NODE_MASK_ZERO(&(VAR(SECURE_NODE_LIST)));
	NODE_MASK_ZERO(&(VAR(SECURE_NODE_LIST_TEST)));
}

BOOL IsNodeSecure(BYTE nodeId) {
	return NODE_MASK_IS_SET(&(VAR(SECURE_NODE_LIST)), nodeId);
}

BOOL IsNodeTested(BYTE nodeId) {
	return NODE_MASK_IS_SET(&(VAR(SECURE_NODE_LIST_TEST)), nodeId);
}

void SecurityStoreAll() {
	SAVE_EEPROM_BYTE(SECURITY_MODE);
	SAVE_EEPROM_ARRAY(NETWORK_KEY, 16);
	SAVE_EEPROM_ARRAY(SECURE_NODE_LIST, NODE_MASK_LENGTH);
	SAVE_EEPROM_ARRAY(SECURE_NODE_LIST_TEST, NODE_MASK_LENGTH);
}

void SecurityClearAll() {
	SAVE_EEPROM_BYTE_DEFAULT(SECURITY_MODE);
	memset(SecurityKeyPtr(), 0, 16);
	SecureNodeListZero();
	SecurityStoreAll();
}

void SetSecureNIF(void) {
	nodeInfoCCs 		= nodeInfoCCsOutsideSecurity;
	p_nodeInfoCCs_size 	= &nodeInfoCCsOutsideSecurity_size;
}


void SetUnsecureNIF(void) {
	nodeInfoCCs 		= nodeInfoCCsNormal;
	p_nodeInfoCCs_size 	= &nodeInfoCCsNormal_size;
}

BYTE SecureNIFCopy(BYTE * dst) {
	memcpy(dst, nodeInfoCCsInsideSecurity, nodeInfoCCsInsideSecurity_size);
	return nodeInfoCCsInsideSecurity_size;
}

BYTE SecurityOutgoingPacketsPolicy(BYTE nodeId, BYTE cmdclass, BYTE askedSecurely) {
	/*
        
                Secure mode On   Node secure     Unsec incl NIF    Sec incl NIF    Sec Supported   Policy
                not yet                                           -                                DROP
                not yet                                           +                                PLAIN
                -                                -                                                 DROP
                -                                +                                                 PLAIN
                +                -                                -                                DROP
                +                -                                +                                PLAIN
                +                +                                -                -               DROP
                +                +                                -                +               ENCRYPT
                +                +                                +                -               AS ASKED (PLAIN || ENCRYPT) or PALIN (if not asked)
                +                +                                +                +               AS ASKED (PLAIN || ENCRYPT) or ENCRYPT (if not asked)
        */
	
	#if SECURITY_DEBUG
		if(cmdclass == SECURITY_DEBUG_CC && nodeId == 255)
			return PACKET_POLICY_SEND_PLAIN;
	#endif

	if (CURRENT_SECURITY_MODE == SECURITY_MODE_ENABLED) {
		if (IsNodeSecure(nodeId) || !IsNodeTested(nodeId)) {
			if (inArray(nodeInfoCCsOutsideSecurity, sizeof(nodeInfoCCsOutsideSecurity), cmdclass)) {
				switch (askedSecurely) {
					case ASKED_UNSECURELY:
						return PACKET_POLICY_SEND_PLAIN;
					case ASKED_SECURELY:
						return PACKET_POLICY_SEND_ENCRYPT;
					case ASKED_UNSOLICITED:
					{
						if (inArray(nodeInfoCCsInsideSecurity, sizeof(nodeInfoCCsInsideSecurity), cmdclass))
							return PACKET_POLICY_SEND_ENCRYPT;
						else
							return PACKET_POLICY_SEND_PLAIN;
					}
				}
			} else {
				if (inArray(nodeInfoCCsInsideSecurity, sizeof(nodeInfoCCsInsideSecurity), cmdclass))
					return PACKET_POLICY_SEND_ENCRYPT;
				else
					return PACKET_POLICY_DROP;
			}
		} else {
			if (inArray(nodeInfoCCsOutsideSecurity, sizeof(nodeInfoCCsOutsideSecurity), cmdclass))
				return PACKET_POLICY_SEND_PLAIN;
			else
				return PACKET_POLICY_DROP;
		}
	}

	if (CURRENT_SECURITY_MODE == SECURITY_MODE_DISABLED) {
		if (inArray(nodeInfoCCsNormal, sizeof(nodeInfoCCsNormal), cmdclass))
			return PACKET_POLICY_SEND_PLAIN;
		else
			return PACKET_POLICY_DROP;
	}

	if (CURRENT_SECURITY_MODE == SECURITY_MODE_NOT_YET) {
		if (inArray(nodeInfoCCsOutsideSecurity, sizeof(nodeInfoCCsOutsideSecurity), cmdclass))
			return PACKET_POLICY_SEND_PLAIN;
		else
			return PACKET_POLICY_DROP;
	}
	
	return PACKET_POLICY_DROP; // just in case
}

BOOL SecurityIncomingUnsecurePacketsPolicy(BYTE cmdclass) {
	if (
				((CURRENT_SECURITY_MODE == SECURITY_MODE_ENABLED || CURRENT_SECURITY_MODE == SECURITY_MODE_NOT_YET) && inCCArray(nodeInfoCCsOutsideSecurity, sizeof(nodeInfoCCsOutsideSecurity), cmdclass, TRUE))
				||
				(CURRENT_SECURITY_MODE == SECURITY_MODE_DISABLED && inCCArray(nodeInfoCCsNormal, sizeof(nodeInfoCCsNormal), cmdclass, TRUE))
				#if WITH_PROPRIETARY_EXTENTIONS && !WITH_PROPRIETARY_EXTENTIONS_IN_NIF
				||
				cmdclass == COMMAND_CLASS_MANUFACTURER_PROPRIETARY
				#endif

		)
		return PACKET_POLICY_ACCEPT;
	else
		return PACKET_POLICY_DROP;
}

BOOL SecurityIncomingSecurePacketsPolicy(BYTE cmdclass) {
	if (
			CURRENT_SECURITY_MODE == SECURITY_MODE_ENABLED
			&&
			(
				inCCArray(nodeInfoCCsInsideSecurity, sizeof(nodeInfoCCsInsideSecurity), cmdclass, TRUE)
				||
				inCCArray(nodeInfoCCsOutsideSecurity, sizeof(nodeInfoCCsOutsideSecurity), cmdclass, TRUE) // also allow unsecure CCs to be asked securelly
				#if !WITH_PROPRIETARY_EXTENTIONS_IN_NIF
				||
				cmdclass == COMMAND_CLASS_MANUFACTURER_PROPRIETARY
				#endif
			)
		)
		return PACKET_POLICY_ACCEPT;
	else
		return PACKET_POLICY_DROP;
}
#endif

#if WITH_CC_POWERLEVEL
BYTE * getPowerLevelData()
{
	return &VAR(POWERLEVEL_TEST_NODE);
}
void savePowerLevelData()
{
	SAVE_EEPROM_ARRAY(POWERLEVEL_TEST_NODE, 6);
}
void clearPowerLevelData() {
	memset(&VAR(POWERLEVEL_TEST_NODE), 0, 6);
	savePowerLevelData();
}

#endif		

void RestoreToDefaultNonExcludableValues(void)
{
	#ifdef CUSTOM_RESET_EEPROM_TO_DEFAULT_NON_EXCLUDABLE
		// #warning "Defined non excludable ResetToDefault()"
		CUSTOM_RESET_EEPROM_TO_DEFAULT_NON_EXCLUDABLE();
	#endif	

	#ifdef ZW_SLAVE_32
	{
	BYTE dummy[8];
	ZW_NVRGetValue(0x70,8,&dummy);
	ZW_MEM_PUT_BUFFER((WORD)&eeConstantProductionParameters,&dummy,8,NULL);
	}
	#endif
}

CBK_DEFINE( , void, RestoreToDefault, (void)) {
	// ZW_SetDefault might delete full EEPROM content. Check this
	#ifdef ZW_SLAVE
	ZW_SetDefault(); // Set SDK part of EEPROM to default too
	#else
	ZW_SetDefault(0); // Set SDK part of EEPROM to default too
	#endif
	myNodeId = 0; // to let know we are not in a network
	

	#if WITH_TRIAC
	SAVE_EEPROM_BYTE_DEFAULT(TRIAC_T_BEFORE_PULSE);
	SAVE_EEPROM_BYTE_DEFAULT(TRIAC_T_AFTER_PULSE);
	SAVE_EEPROM_BYTE_DEFAULT(TRIAC_T_PULSE_WIDTH);
	SAVE_EEPROM_BYTE_DEFAULT(TRIAC_PULSE_TYPE);	
	#endif
	
	// keep same order as in enum for EE_VAR

	#if WITH_CC_PROTECTION
	SAVE_EEPROM_BYTE_DEFAULT(PROTECTION);
	#endif
	
	// Security is below in a separate function
	
	#if WITH_CC_SWITCH_ALL 
	SAVE_EEPROM_BYTE_DEFAULT(SWITCH_ALL_MODE);
	#endif

	#if CONTROL_CC_SWITCH_ALL 
	SAVE_EEPROM_BYTE_DEFAULT(CONTROL_SWITCH_ALL_MODE);
	#endif

	// WakeUp is below in a separate function

	#if WITH_CC_WAKE_UP && WITH_CC_BATTERY
	SAVE_EEPROM_BYTE_DEFAULT(UNSOLICITED_BATTERY_REPORT);
	SAVE_EEPROM_BYTE_DEFAULT(ENABLE_WAKEUPS);
	#endif

	#if WITH_CC_BATTERY && defined(ZW050x)
	SAVE_EEPROM_BYTE_DEFAULT(BATTERY_REPORT_ON_POR_SENT)
	#endif

	#if WITH_CC_THERMOSTAT_MODE
	SAVE_EEPROM_BYTE_DEFAULT(THERMOSTAT_MODE);
	#endif

	#if WITH_CC_THERMOSTAT_SETPOINT
	#if CC_THERMOSTAT_SETPOINT_WITH_HEATING_1_V2
	SAVE_EEPROM_WORD_DEFAULT(THERMOSTAT_SETPOINT_HEAT);
	#endif
	#if CC_THERMOSTAT_SETPOINT_WITH_COOLING_1_V2
	SAVE_EEPROM_WORD_DEFAULT(THERMOSTAT_SETPOINT_COOL);
	#endif
	#if CC_THERMOSTAT_SETPOINT_WITH_FURNACE_V2
	SAVE_EEPROM_WORD_DEFAULT(THERMOSTAT_SETPOINT_FURNACE);
	#endif
	#if CC_THERMOSTAT_SETPOINT_WITH_DRY_AIR_V2
	SAVE_EEPROM_WORD_DEFAULT(THERMOSTAT_SETPOINT_DRY_AIR);
	#endif
	#if CC_THERMOSTAT_SETPOINT_WITH_MOIST_AIR_V2
	SAVE_EEPROM_WORD_DEFAULT(THERMOSTAT_SETPOINT_MOIST_AIR);
	#endif
	#if CC_THERMOSTAT_SETPOINT_WITH_AUTO_CHANGEOVER_V2
	SAVE_EEPROM_WORD_DEFAULT(THERMOSTAT_SETPOINT_AUTO_CHANGEOVER);
	#endif
	#if CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_HEATING_V2
	SAVE_EEPROM_WORD_DEFAULT(THERMOSTAT_SETPOINT_ENERGY_SAVE_HEAT);
	#endif
	#if CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_COOLING_V2
	SAVE_EEPROM_WORD_DEFAULT(THERMOSTAT_SETPOINT_ENERGY_SAVE_COOL);
	#endif
	#if CC_THERMOSTAT_SETPOINT_WITH_AWAY_HEATING_V2
	SAVE_EEPROM_WORD_DEFAULT(THERMOSTAT_SETPOINT_AWAY);
	#endif
	#endif

	CUSTOM_RESET_EEPROM_TO_DEFAULT();
	        	        	        		
	#if WITH_CC_ASSOCIATION
	AssociationClearAll();
	#endif
	#if WITH_CC_SCENE_CONTROLLER_CONF
	ScenesClearAll();
	#endif
	#if WITH_CC_NODE_NAMING
	NodeNamingClear();
	#endif
	#if WITH_CC_WAKE_UP
	SetDefaultWakeupConfiguration();
	#endif
	#if WITH_CC_SCENE_ACTUATOR_CONF
	ScenesActuatorClearAll();
	#endif

	#if WITH_CC_SECURITY
	SecurityClearAll();
	#endif

	#if WITH_CC_FIRMWARE_UPGRADE
	#if FIRMWARE_UPGRADE_BATTERY_SAVING
	FWUpdate_Clear();
	#endif
	#endif

	#if WITH_CC_POWERLEVEL
	clearPowerLevelData();
	#endif

	#if WITH_CC_DOOR_LOCK
	SAVE_EEPROM_ARRAY_DEFAULT(DOOR_LOCK_DATA,DOOR_LOCK_DATA_SIZE);
	#endif

	#if WITH_CC_DOOR_LOCK_LOGGING
	VAR(DOOR_LOCK_LOGGING_BUF_INDEX) = 0xFF;
	SAVE_EEPROM_BYTE(DOOR_LOCK_LOGGING_BUF_INDEX);
	VAR(DOOR_LOCK_LOGGING_LAST_REPORT) = 0;
	SAVE_EEPROM_BYTE(DOOR_LOCK_LOGGING_LAST_REPORT);
	createDoorLockLoggingReport(DOOR_LOCK_LOGGING_EVENT_LOCK_RESET);
	#endif

	ZME_PUT_BYTE(EE(MAGIC), MAGIC_VALUE);
	
	#if WITH_PENDING_BATTERY
	// to give "fake" battery value first time on next inclusion
	NZ(pendingBatteryCount) = 0;
	#endif

	#if WITH_CC_MULTI_COMMAND
	MultiCmdListZero();
	#endif

	#ifndef ZW_SLAVE_32
	ZW_MemoryFlush();
	#endif
	// We will reboot to correctly clean all structures in App Code and in SDK
	RebootChip();
}

void ResetDeviceLocally(void) {
	// Give DeviceResetLocally a chance to be delivered
	#if WITH_CC_DEVICE_RESET_LOCALLY
	if (ZW_TIMER_START(CBK_POINTER(RestoreToDefault), 250, TIMER_ONE_TIME) != 0xff) {
		deviceResetLocallyNotify();
		LEDSet(LED_NWI);
		hardBlock = TRUE;
	} else
	#endif
	{
		RestoreToDefault();
	}
}

#if WITH_CC_ASSOCIATION
void AssociationClearAll(void) {
	register BYTE i;
	register BYTE j;
	i = ASSOCIATION_NUM_GROUPS - 1;
	do {
		j = ASSOCIATION_GROUP_SIZE - 1;
		do {
			groups[i].nodeID[j] = 0;
			#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
			groupsMI[i].nodeIDEP[j].nodeID = 0;
			#endif
		} while (j--);
	} while (i--);
	AssociationStoreAll();
}

void AssociationStoreAll(void) {
	register BYTE i;
	register BYTE j;
	
	i = ASSOCIATION_NUM_GROUPS - 1;
	do {
		j = ASSOCIATION_GROUP_SIZE - 1;
		do {
			#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
			ZME_PUT_BYTE(EE(ASSOCIATIONS) + i * 3*ASSOCIATION_GROUP_SIZE + j, groups[i].nodeID[j]);
			ZME_PUT_BYTE(EE(ASSOCIATIONS) + i * 3*ASSOCIATION_GROUP_SIZE + ASSOCIATION_GROUP_SIZE + 2 * j, groupsMI[i].nodeIDEP[j].nodeID);
			ZME_PUT_BYTE(EE(ASSOCIATIONS) + i * 3*ASSOCIATION_GROUP_SIZE + ASSOCIATION_GROUP_SIZE + 2 * j + 1, groupsMI[i].nodeIDEP[j].endPoint);
			#else
			ZME_PUT_BYTE(EE(ASSOCIATIONS) + i * 1*ASSOCIATION_GROUP_SIZE + j, groups[i].nodeID[j]);
			#endif
		} while (j--);
	} while (i--);
	LEDSet(LED_DONE);
}

void AssociationInit(void) {
	register BYTE i;
	register BYTE j;
	i = ASSOCIATION_NUM_GROUPS - 1;
	do {
		j = ASSOCIATION_GROUP_SIZE - 1;
		do {
			#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
			groups[i].nodeID[j] =  ZME_GET_BYTE(EE(ASSOCIATIONS) + i * 3*ASSOCIATION_GROUP_SIZE + j);
			ZME_GET_BUFFER(EE(ASSOCIATIONS) + i * 3*ASSOCIATION_GROUP_SIZE + ASSOCIATION_GROUP_SIZE + 2 * j, &(groupsMI[i].nodeIDEP[j].nodeID), 2);
			#else
			groups[i].nodeID[j] =  ZME_GET_BYTE(EE(ASSOCIATIONS) + i * 1*ASSOCIATION_GROUP_SIZE + j);
			#endif
		} while (j--);
	} while (i--);
}
#endif



#if WITH_CC_SCENE_CONTROLLER_CONF
void ScenesInit(void) {
	register BYTE i;
	i = ASSOCIATION_NUM_GROUPS - 1;
	do {
		scenesCtrl[i].sceneID =  ZME_GET_BYTE(EE(SCENE_CONTROLLER_CONF) + i * SCENE_HOLDER_SIZE);
		scenesCtrl[i].dimmingDuration =  ZME_GET_BYTE(EE(SCENE_CONTROLLER_CONF) + i * SCENE_HOLDER_SIZE + 1);
	} while (i--);
}

void ScenesClearAll(void) {
	register BYTE i;
	i = ASSOCIATION_NUM_GROUPS - 1;
	do {
		scenesCtrl[i].sceneID = 0;
		scenesCtrl[i].dimmingDuration = 0;
		ZME_PUT_BYTE(EE(SCENE_CONTROLLER_CONF) + i * SCENE_HOLDER_SIZE    , 0);
		ZME_PUT_BYTE(EE(SCENE_CONTROLLER_CONF) + i * SCENE_HOLDER_SIZE + 1, 0);
	} while (i--);
}
#endif

#if WITH_CC_SCENE_ACTUATOR_CONF
void ScenesActuatorClearAll(void) {
	register BYTE i;
	i = SCENE_ACTUATOR_EE_SIZE;
	do {
		scenesAct[i-1] = 0x00;
		ZME_PUT_BYTE(EE(SCENE_ACTUATOR_CONF_LEVEL) + (WORD)i - 1, 0x00);
	} while (--i);
}
#endif

#if WITH_CC_NODE_NAMING
void NodeNamingClear(void) {
	register BYTE i;
	// We suppose nodenaming and encodings to be one after other // just to minimize the code
	i = 1 + 16; // LOCATION_ENCODING + LOCATION
	do {
		ZME_PUT_BYTE(EE(NODE_LOCATION_ENCODING) + i - 1, 0x00);
	} while (--i);

	i = 1 + 16; // NAME_ENCODING + NAME
	do {
		ZME_PUT_BYTE(EE(NODE_NAME_ENCODING) + i - 1, NODE_NAMING_DEFAULT_NAME[i - 1]);
	} while (--i);
}
#endif

#ifdef ZW_SLAVE
void LearnCompleted(BYTE nodeID, BOOL completed) {
	if (completed) {
		if (nodeID == 0) {
			RestoreToDefault(); // Reset configuration to default
		}
		else {
			#if WITH_CC_SECURITY
				if (myNodeId == 0) { // Inclusion from default state
						// Workaround! Moved here due to a bug in SDK in Assign Complete calling back to late
						// LearnCompleted is called after few commands from the controller already arrived, including SECURITY_SCHEME_GET
						// See ZME_Security.c SecurityCommandHandler
						// На данный момент считаем оптимальным началом Security-interview 
					    // первый SCHEME_GET
						//if(!securityInclusionInProcess()) 
						//	SecurityInclusion();
				}
			#endif

			#if !PRODUCT_LISTENING_TYPE  || PRODUCT_DYNAMIC_TYPE
					#if PRODUCT_DYNAMIC_TYPE
					if (DYNAMIC_TYPE_CURRENT_VALUE != DYNAMIC_MODE_ALWAYS_LISTENING) {
					#endif
							LongKickSleep(); // Wait for more packets from including node
					#if PRODUCT_DYNAMIC_TYPE
					}
					#endif
			#endif
		}
		myNodeId = nodeID;
		LEDSet(LED_DONE);
		
		LEARN_COMPLETE_ACTION(nodeID);
	} else {
		LEDSet(LED_FAILED);
	}
	LEDNWIOff(); // turn off long term NWI LED codes
	mode = MODE_IDLE;
}
void LearnStarted(BYTE status) //reentrant 
{  
	mode = MODE_LEARN_IN_PROGRESS;
}
#else
// ZW_CONTROLLER
void LearnCompleted(BYTE nodeID, BOOL completed) {
	if (completed) {
		if (nodeID == 0) {
			RestoreToDefault(); // Reset configuration to default
		}
		else {
			#if WITH_CC_SECURITY
				if (myNodeId == 0) { // Inclusion from default state
						// Workaround! Moved here due to a bug in SDK in Assign Complete calling back to late
						// LearnCompleted is called after few commands from the controller already arrived, including SECURITY_SCHEME_GET
						// See ZME_Security.c SecurityCommandHandler
						// На данный момент считаем оптимальным началом Security-interview 
					    // первый SCHEME_GET
						//if(!securityInclusionInProcess()) 
						//	SecurityInclusion();
				}
			#endif

			#if !PRODUCT_LISTENING_TYPE
			LongKickSleep(); // Wait for more packets from including node
			#endif
		}
		myNodeId = nodeID;
		LEDSet(LED_DONE);
		
		LEARN_COMPLETE_ACTION(nodeID);
	} else {
		LEDSet(LED_FAILED);
	}
	LEDNWIOff(); // turn off long term NWI LED codes
	mode = MODE_IDLE;
}	

#endif

// ------------------------------------------------------------------------------------
// 			Функционал контроллера
// ------------------------------------------------------------------------------------

#ifdef ZW_CONTROLLER

// CONTROLLER_LEARN_INCLUSION_ACTION
BYTE g_bSecureInclusion = FALSE;

BYTE g_controllerInterviewNode = 0;

BYTE g_ControllerCanLearn = FALSE;

BYTE g_IncludingSecureDevice = FALSE;

BYTE sucNodeId;
//BYTE dbg_fdata[]={0xaa, 0xbb, 0xcc, 0xdd};

void RemoveFromAllGroups(BYTE NodeId)
{
	#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
	AssociationRemove(FALSE, 0, &NodeId, 1);
	#else
	AssociationRemove(0, &NodeId, 1);
	#endif
}


BOOL checkNIFForCC(BYTE * nif, BYTE nif_size, BYTE cc)
{
	BYTE i;

	for(i=0; i<nif_size; i++)		
		if(nif[i] == cc)
			return TRUE;

	return FALSE;
}
CBK_DEFINE(static, void, GetNetworkRole, (BYTE txStatus)) {
	sucNodeId = ZW_GetSUCNodeID();
}

CBK_DEFINE(static, void, controllerLearnInclusionHandler, (LEARN_INFO * learnNodeInfo)) {
	BYTE i;

	static BYTE addedControllerId = 0;

	switch(learnNodeInfo->bStatus)
	{
		// --- Inclusion
		case ADD_NODE_STATUS_LEARN_READY:
			#ifdef CONTROLLER_LEARN_INCLUSION_ACTION
			g_IncludingSecureDevice = FALSE;
			CONTROLLER_LEARN_INCLUSION_ACTION;
			#endif
			g_controllerInterviewNode = 0;
			break;
		case ADD_NODE_STATUS_NODE_FOUND:
			break;

		case ADD_NODE_STATUS_ADDING_CONTROLLER:	
			g_IncludingSecureDevice  = checkNIFForCC(learnNodeInfo->pCmd, learnNodeInfo->bLen, COMMAND_CLASS_SECURITY);

			#if WITH_CC_SECURITY
			setControllerNodeInclusion(TRUE);
			#endif

			#ifdef CONTROLLER_LEARN_INCLUSION_ACTION_DONE_CNTRL
			CONTROLLER_LEARN_INCLUSION_ACTION_DONE_CNTRL(learnNodeInfo->bSource, learnNodeInfo->pCmd, learnNodeInfo->bLen);
			#endif

			if (learnNodeInfo->bLen > 0 && learnNodeInfo->pCmd[0] == BASIC_TYPE_STATIC_CONTROLLER) // node is a Static Controller - try it as SUC
				addedControllerId = learnNodeInfo->bSource;

			LongKickSleep();
			break;

		case ADD_NODE_STATUS_ADDING_SLAVE:
			g_IncludingSecureDevice  = checkNIFForCC(learnNodeInfo->pCmd, learnNodeInfo->bLen, COMMAND_CLASS_SECURITY);
			#ifdef CONTROLLER_LEARN_INCLUSION_ACTION_DONE_SLAVE
			CONTROLLER_LEARN_INCLUSION_ACTION_DONE_SLAVE(learnNodeInfo->bSource, learnNodeInfo->pCmd, learnNodeInfo->bLen);
			#endif
			LongKickSleep();
			break;

		case ADD_NODE_STATUS_PROTOCOL_DONE:
			{

				ZW_ADD_NODE_TO_NETWORK(ADD_NODE_STOP, CBK_POINTER(controllerLearnInclusionHandler));
				//g_controllerInterviewNode = learnNodeInfo->bSource;
			}
			break;
		case ADD_NODE_STATUS_NOT_PRIMARY:
		case ADD_NODE_STATUS_FAILED:
			LEDSet(LED_NO);
			LEDSet(LED_FAILED);
			ZW_ADD_NODE_TO_NETWORK(ADD_NODE_STOP, NULL);
			mode = MODE_IDLE;
	
			addedControllerId = 0;
			g_controllerInterviewNode = 0;
			g_bSecureInclusion = FALSE;
			break;
		case ADD_NODE_STATUS_DONE:

			if (addedControllerId != 0 && sucNodeId == 0)
				ZW_SetSUCNodeID(addedControllerId, TRUE, FALSE, ZW_SUC_FUNC_NODEID_SERVER, CBK_POINTER(GetNetworkRole));
			addedControllerId = 0;

			controllerUpdateState();

			mode = MODE_IDLE;
			LEDSet(LED_NO);
			LEDSet(LED_DONE);
			
			ZW_ADD_NODE_TO_NETWORK(ADD_NODE_STOP, NULL);
			
			#if WITH_CC_SECURITY
				if(g_IncludingSecureDevice && g_bSecureInclusion)
						startSecurityInclusion(learnNodeInfo->bSource);	
			#endif


			
			g_bSecureInclusion = FALSE;
			break;
		
	}

}

CBK_DEFINE(static, void, controllerLearnExclusionHandler, (LEARN_INFO * learnNodeInfo)) {

	switch(learnNodeInfo->bStatus)
	{

		// --- Exclusion

		case REMOVE_NODE_STATUS_LEARN_READY:
			#ifdef CONTROLLER_LEARN_EXCLUSION_ACTION
			CONTROLLER_LEARN_EXCLUSION_ACTION;
			#endif
			break;
		case REMOVE_NODE_STATUS_NODE_FOUND:
			break;
		case REMOVE_NODE_STATUS_REMOVING_SLAVE:
		case REMOVE_NODE_STATUS_REMOVING_CONTROLLER:
			#ifdef WITH_SMART_REMOVE_EXCLUDED_FROM_GROUPS
			RemoveFromAllGroups(learnNodeInfo->bSource);
			#endif
			break;
		case REMOVE_NODE_STATUS_FAILED:
			mode = MODE_IDLE;
			LEDSet(LED_NO);
			LEDSet(LED_FAILED);
			ZW_REMOVE_NODE_FROM_NETWORK(REMOVE_NODE_STOP, NULL);
			break;
		case REMOVE_NODE_STATUS_DONE:
			controllerUpdateState();
			mode = MODE_IDLE;
			LEDSet(LED_NO);
			LEDSet(LED_DONE);
			ZW_REMOVE_NODE_FROM_NETWORK(REMOVE_NODE_STOP, NULL);
			break;
	
			

	}

}
CBK_DEFINE(static, void, controllerIncludeHandler, (LEARN_INFO * learnNodeInfo)) {

	switch(learnNodeInfo->bStatus)
	{
		case LEARN_MODE_STARTED:
			break;
		case LEARN_MODE_FAILED:
			LEDSet(LED_FAILED);
			break;
		case LEARN_MODE_DONE:
			LEDSet(LED_DONE);
			break;
	}
}
/*
BOOL controllerInclusionInterviewFilter(BYTE sourceNode, ZW_APPLICATION_TX_BUFFER *pCmd, BYTE cmdLength)
{
	BYTE * pbCmd = (BYTE*) pCmd;
	
	if(sourceNode != g_controllerInterviewNode)
		return FALSE;
	if(pbCmd[0] == COMMAND_CLASS_SECURITY)
		return FALSE;

	switch(pbCmd[0])
	{
		case 
	}

	return TRUE;



}*/
void controllerStartInclusion(BYTE bSecure) {

	g_bSecureInclusion = bSecure;
	ZW_ADD_NODE_TO_NETWORK((ADD_NODE_ANY | ADD_NODE_OPTION_NORMAL_POWER |
                        ADD_NODE_OPTION_NETWORK_WIDE), CBK_POINTER(controllerLearnInclusionHandler));

}
void controllerStopInclusion() {

	ZW_ADD_NODE_TO_NETWORK(ADD_NODE_STOP, CBK_POINTER(controllerLearnInclusionHandler));

}

void controllerStartExclusion() {

	ZW_REMOVE_NODE_FROM_NETWORK(REMOVE_NODE_ANY, CBK_POINTER(controllerLearnExclusionHandler));
			
}
void controllerStopExclusion() {

	ZW_REMOVE_NODE_FROM_NETWORK(REMOVE_NODE_STOP, CBK_POINTER(controllerLearnExclusionHandler));
			
}

void controllerStartShift()
{
	ZW_CONTROLLER_CHANGE(CONTROLLER_CHANGE_START, CBK_POINTER(controllerLearnInclusionHandler));

}

void controllerStopShift()
{
	ZW_CONTROLLER_CHANGE(CONTROLLER_CHANGE_STOP, CBK_POINTER(controllerLearnInclusionHandler));

}



BOOL controllerCanLearn(void) {
	BYTE node;
	NODEINFO nodeInfo;

	if (realPrimaryController) {
		MemoryGetID(NULL, &myNodeId);
		for (node = 1; node <= ZW_MAX_NODES; node++)
			if (node != myNodeId) {
				ZW_GetNodeProtocolInfo(node, &nodeInfo);
				if (nodeInfo.nodeType.generic)
					return FALSE;
				//ZW_POLL();
			}
	}

	return TRUE;
}

void controllerUpdateState()
{
	g_ControllerCanLearn = controllerCanLearn();
	sucNodeId = ZW_GetSUCNodeID();
}

void controllerStartLearn(){
	if(controllerCanLearn())
		ZW_SetLearnMode(TRUE, CBK_POINTER(controllerIncludeHandler));
}



#endif

#if WITH_CC_PROTECTION
void ProtectionCounterStart(void) {
	protectionSecondsDoCount = TRUE;
	protectionSecondsCounter = 0;
}
#endif

#if WITH_CC_WAKE_UP
// Saves the current configuration to EEPROM
void StoreWakeupConfiguration(void) {
	SAVE_EEPROM_BYTE(WAKEUP_NODEID);
	ZME_PUT_BYTE(EE(SLEEP_PERIOD) + 1, VAR(SLEEP_PERIOD + 1));
	ZME_PUT_BYTE(EE(SLEEP_PERIOD) + 2, VAR(SLEEP_PERIOD + 2));
	ZME_PUT_BYTE(EE(SLEEP_PERIOD) + 3, VAR(SLEEP_PERIOD + 3));

}

// Convert the 24-bit timeout value to a big endian DWORD value that determines how long the sensor sleeps.
void SetWakeupPeriod(void) {
	DWORD sleepPeriod;
	WORD  sleepRest;
	
	((BYTE *)&sleepPeriod)[0] = 0;
	((BYTE *)&sleepPeriod)[1] = VAR(SLEEP_PERIOD + 1);
	((BYTE *)&sleepPeriod)[2] = VAR(SLEEP_PERIOD + 2);
	((BYTE *)&sleepPeriod)[3] = VAR(SLEEP_PERIOD + 3);
	

	sleepRest   				= sleepPeriod % SLEEP_TIME;
	sleepPeriod    			   -= sleepRest;
	
	NZ(wakeupCount) = (WORD)(sleepPeriod / ((DWORD)SLEEP_TIME));
	NZ(wakeupCounterMagic) = WAKE_UP_MAGIC_VALUE;
	
	// change back sleepSeconds* to adopt rounded value
	VAR(SLEEP_PERIOD + 1) = ((BYTE_P)&sleepPeriod)[1];
	VAR(SLEEP_PERIOD + 2) = ((BYTE_P)&sleepPeriod)[2];
	VAR(SLEEP_PERIOD + 3) = ((BYTE_P)&sleepPeriod)[3];

	StoreWakeupConfiguration();
}

// Reset configuration to default values.
void SetDefaultWakeupConfiguration(void) {
	VAR(WAKEUP_NODEID) = DEFAULT_WAKEUP_NODE;
	
	VAR(SLEEP_PERIOD + 1) = (BYTE)(((DWORD)DEFAULT_WAKE_UP_INTERVAL) >> 16) & 0xFF;
	VAR(SLEEP_PERIOD + 2) = (BYTE)(((DWORD)DEFAULT_WAKE_UP_INTERVAL) >> 8) & 0xFF;
	VAR(SLEEP_PERIOD + 3) = (BYTE)(((DWORD)DEFAULT_WAKE_UP_INTERVAL)) & 0xFF;
	
	SetWakeupPeriod();
}

#if WITH_CC_BATTERY
void SendUnsolicitedBatteryReport(void) {
	BYTE dstBatteryReport;

	if (VAR(UNSOLICITED_BATTERY_REPORT) == UNSOLICITED_BATTERY_REPORT_NO || (VAR(UNSOLICITED_BATTERY_REPORT) == UNSOLICITED_BATTERY_REPORT_WAKEUP_NODE && VAR(WAKEUP_NODEID) == ZW_TEST_NOT_A_NODEID))
		return;
	
	// send unsolicited Battery Report
	dstBatteryReport = (VAR(UNSOLICITED_BATTERY_REPORT) == UNSOLICITED_BATTERY_REPORT_WAKEUP_NODE) ? VAR(WAKEUP_NODEID) : NODE_BROADCAST;
	
	
	#if WITH_CC_ASSOCIATION
		#ifdef ASSOCIATION_REPORTS_GROUP_NUM
	 	SendReport(ASSOCIATION_REPORTS_GROUP_NUM, COMMAND_CLASS_BATTERY,  BATTERY_LEVEL_GET);
	 	#endif
	#else
		SendReportToNode(dstBatteryReport, COMMAND_CLASS_BATTERY, BATTERY_LEVEL_GET);
	#endif	
}
#endif

// Callback function for sending wakeup notification

void SendWakeupNotificationCallback(BYTE txStatus) {
	if(txStatus == TRANSMIT_COMPLETE_OK) {
		ShortKickSleep(); // We got in contact with the Wakeup Node, expect no more information frame
		#if WITH_CC_BATTERY
		SendUnsolicitedBatteryReport();
		#endif
	}
}

// Send WAKE_UP_NOTIFICATION
void SendWakeupNotification(void) {
	if (VAR(WAKEUP_NODEID) != ZW_TEST_NOT_A_NODEID) {
		if ((txBuf = CreatePacket()) != NULL) {
			txBuf->ZW_WakeUpNotificationFrame.cmdClass = COMMAND_CLASS_WAKE_UP;
			txBuf->ZW_WakeUpNotificationFrame.cmd = WAKE_UP_NOTIFICATION;
			
			SetPacketCallbackID(SEND_CBK_WAKEUP_NOTIFICATION);
			EnqueuePacket(VAR(WAKEUP_NODEID), sizeof(ZW_WAKE_UP_NOTIFICATION_FRAME), VAR(WAKEUP_NODEID) != NODE_BROADCAST ? TRANSMIT_OPTION_DEFAULT_VALUE : 0);
		}
	} else {
		#if WITH_CC_BATTERY
		SendUnsolicitedBatteryReport();
		#endif
	}
}

void ReleaseeSleepLockWithTimer(void) {
	_ReleaseSleepLockTimer();
	ReleaseSleepLock();
}

#endif

void SendNIFCallback(BYTE txStatus) {
	if (txStatus == TRANSMIT_COMPLETE_OK) {
		LEDSet(LED_DONE);
	} else {
		LEDSet(LED_FAILED);
	}
	#if !PRODUCT_LISTENING_TYPE  || PRODUCT_DYNAMIC_TYPE
		#if PRODUCT_DYNAMIC_TYPE
		if (DYNAMIC_TYPE_CURRENT_VALUE != DYNAMIC_MODE_ALWAYS_LISTENING) {
		#endif
			LongKickSleep();
		#if PRODUCT_DYNAMIC_TYPE
		}
		#endif
	#endif
	#if WITH_CC_WAKE_UP
	SendWakeupNotification();
	#endif
}

void SendNIF(void) {
	if ((txBuf = CreatePacket()) != NULL) {
			SetPacketCallbackID(SEND_CBK_NIF);
			// size does not matter for SendNIF, but 0 is illegal to allocate!
			EnqueuePacketF(NODE_BROADCAST, 1, TRANSMIT_OPTION_ACK, SNDBUF_NIF);
	}
}

void SendCallback(BYTE cbk, BYTE txStatus) {
	switch (cbk) {
		case SEND_CBK_NIF:
			SendNIFCallback(txStatus);
			break;
		#if WITH_CC_WAKE_UP
		case SEND_CBK_WAKEUP_NOTIFICATION:
			SendWakeupNotificationCallback(txStatus);
			break;
		#endif
		#ifdef SendCallbackCustom
		default:
			SendCallbackCustom(txStatus);
			break;
		#endif
	}
}

#if !PRODUCT_LISTENING_TYPE  || PRODUCT_DYNAMIC_TYPE
void _ReleaseSleepLockTimer(void) {
	if (sleepTimerHandle != 0xFF)
		ZW_TIMER_CANCEL(sleepTimerHandle);
}

static BYTE sleepLockedCounter = 1; // the default value is just in case, not to go to deep wake up.
static BYTE sleepKickLong = FALSE;

CBK_DEFINE( , void, TickSleepLock, (void)) {
	if (--sleepLockedCounter) {
		// restart the timer
		sleepTimerHandle = ZW_TIMER_START(CBK_POINTER(TickSleepLock), WAIT_BEFORE_SLEEP_TIME, TIMER_ONE_TIME);
	} else
		ReleaseSleepLock();
}

void ReleaseSleepLock(void) {
	sleepLocked = FALSE;
	sleepTimerHandle = 0xFF; // to invalidate timer handler
	sleepKickLong = FALSE; // next kick will be short by default
}

void _KickSleep(void) {
	_ReleaseSleepLockTimer();
	sleepLocked = TRUE;
	sleepTimerHandle = ZW_TIMER_START(CBK_POINTER(TickSleepLock), WAIT_BEFORE_SLEEP_TIME, TIMER_ONE_TIME);
}

void LongKickSleep(void) {
	sleepKickLong = TRUE;
	ShortKickSleep();
}

void ShortKickSleep(void) {
	if (sleepKickLong) // this allows to run ShortKickSleep after LongKickSleep and still keep the long time.
		sleepLockedCounter = AFTER_INCLUSION_AWAKE_TIME;
	else
		sleepLockedCounter = 1;
	_KickSleep();
}

void GoSleep(void) {
	// NB! ZW_SetSleepMode MUST be called after ZW_SetWutTimeout() and ZW_SetExtIntLevel() calls!

	
	#ifndef ZW050x
	IT1 = WAKEUP_INT1_EDGE_OR_LEVEL; // wake up by edge or level
	#endif
	
	ZW_SetExtIntLevel(ZW_INT1, WAKEUP_INT1_LEVEL); // Set INT1 mode

	if (timer10msHandler != 0xFF)
		ZW_TIMER_CANCEL(timer10msHandler);
	
	// Turn off all h/w connected to pins in OUT mode
	// Turn unused pull-up resistors for pins in IN mode to save energy (if present)
	// Turn off ADC if enabled. Note that if some ADC pin is connected to INT1 via diode, keeping ADC on may lead to wrong wakeup reason on next wakeup by WUT

	LED_HW_OFF();
	
	#if NUM_BUTTONS > 0
	BUTTONS_HW_STOP();
	#endif
	
	SWITCH_OFF_HW();

	// NB! Enable interrupts after ZW_SetExtIntLevel() call
	EX1 = 1; // enable INT1
	
	#if WITH_CC_WAKE_UP
		#if FIRMWARE_UPGRADE_BATTERY_SAVING
		NZ(wakeupFWUpdate)	= isFWUInProgress(); 
		if(g_one_poll && isFWUInProgress())
			ZW_SetWutTimeout(FIRMWARE_UPGRADE_SLEEP_TIME-1);
		else
			ZW_SetWutTimeout(SLEEP_TIME-1); // -1 is needed to correct according to ZW_SetWutTimeout() description	
		#else
		ZW_SetWutTimeout(SLEEP_TIME-1); // -1 is needed to correct according to ZW_SetWutTimeout() description
		#endif
		//ZW_SetWutTimeout(1); // For debugging
		ZW_SetSleepMode((myNodeId && NZ(wakeupEnabled))? ZW_WUT_MODE : ZW_STOP_MODE, ZW_INT_MASK_EXT1, 0); // go into WUT sleep (if included) or deep sleep (if not in network).
	#elif PRODUCT_FLIRS_TYPE
		#warning "Door lock auto off!"
		ZW_SetSleepMode(myNodeId ? ZW_FREQUENTLY_LISTENING_MODE : ZW_STOP_MODE, ZW_INT_MASK_EXT1, FLIRS_BEAM_COUNT);
	#else																																														 	
		ZW_SetSleepMode(ZW_STOP_MODE, ZW_INT_MASK_EXT1, 0);
	#endif
}
#endif

void SendReportToNode(BYTE dstNode, BYTE classcmd, BYTE reportparam) {
	if ((txBuf = CreatePacket()) != NULL) {
		txBuf->ZW_BasicReportFrame.cmdClass = classcmd;
		txBuf->ZW_BasicReportFrame.cmd = BASIC_REPORT; // All CCs have same value for _REPORT constant
		txBuf->ZW_BasicReportFrame.value = reportparam;
	
		// Size of most Report frame are equal to BASIC's Report size
		EnqueuePacket(dstNode, sizeof(txBuf->ZW_BasicReportFrame), TRANSMIT_OPTION_USE_DEFAULT);
	}
}

// ! debug only
void SendDataReportToNode(BYTE dstNode, BYTE classcmd, BYTE cmd, BYTE * reportdata, BYTE numdata) {
	if ((txBuf = CreatePacket()) != NULL) {
		txBuf->ZW_BasicReportFrame.cmdClass = classcmd;
		txBuf->ZW_BasicReportFrame.cmd = cmd; // All CCs have same value for _REPORT constant
		memcpy(&(txBuf->ZW_BasicReportFrame.value), reportdata, numdata);
		//txBuf->ZW_BasicReportFrame.value = reportparam;
	
		// Size of most Report frame are equal to BASIC's Report size
		EnqueuePacket(dstNode, sizeof(txBuf->ZW_BasicReportFrame) + numdata - 1, TRANSMIT_OPTION_USE_DEFAULT);
	}
}

#if WITH_CC_DEVICE_RESET_LOCALLY
void deviceResetLocallyNotify(void) {

	 BYTE report_sended = FALSE;


	// 2 (?) Mirko 

	// Если slave никуда не включен - ничего никуда не посылаем
	// Это правильно? Или всетаки broadcast?
	// Наверное правильно - эту сеть увидит только ZSniffer
    // !? Для контроллера все равно будет не 0 => Broadcast - а это правильно?
	if (myNodeId == 0)
		return;
	
	if ((txBuf = CreatePacket()) == NULL)
		return;

	txBuf->ZW_DeviceResetLocallyNotificationFrame.cmdClass         = COMMAND_CLASS_DEVICE_RESET_LOCALLY;
	txBuf->ZW_DeviceResetLocallyNotificationFrame.cmd              = DEVICE_RESET_LOCALLY_NOTIFICATION; 
	
	#if WITH_CC_ASSOCIATION && defined(ASSOCIATION_REPORTS_GROUP_NUM)
	// Считаем количество ассоцированных с LifeLine узлов
	if(countAssociationGroupFullSize(ASSOCIATION_REPORTS_GROUP_NUM) != 0)
	{	// В LifeLine кто-то есть - посылаем им
		EnqueuePacketToGroup(ASSOCIATION_REPORTS_GROUP_NUM, sizeof(txBuf->ZW_DeviceResetLocallyNotificationFrame));
		// Отмечаем, что отчет послали 
		report_sended = TRUE;
	}	
	#endif

	// Только для этих случаев есть смысл спрашивать про SIS/SUC
	#if  (defined(ZW_CONTROLLER) ||  defined(ZW_SLAVE_ENHANCED_232))
	{
		BYTE SISNodeId = ZW_GetSUCNodeID(); // SIS_ID == SUC_ID по протоколу
		if(SISNodeId != 0) // Допустимый ID [1; 232], 0 - нет SIS
		{
			// Посылаем SIS ResetLocallyNotification	
			EnqueuePacket(SISNodeId, sizeof(txBuf->ZW_DeviceResetLocallyNotificationFrame), TRANSMIT_OPTION_USE_DEFAULT);		
			report_sended = TRUE;
		}
	}
	#endif

	// Нет ни LifeLine, ни SIS
	if(!report_sended)
	{
		// Остается одно - broadcast
		EnqueuePacket(255, sizeof(txBuf->ZW_DeviceResetLocallyNotificationFrame), TRANSMIT_OPTION_USE_DEFAULT);		
	}
}
#endif

#if WITH_CC_ASSOCIATION
#if CONTROL_CC_DOOR_LOCK
void SendDoorLockGroup(BYTE group, BYTE mode) {
	if ((txBuf = CreatePacketToGroup()) == NULL)
		return;

	txBuf->ZW_DoorLockOperationSetFrame.cmdClass         = COMMAND_CLASS_DOOR_LOCK;
	txBuf->ZW_DoorLockOperationSetFrame.cmd              = DOOR_LOCK_OPERATION_SET; 
	txBuf->ZW_DoorLockOperationSetFrame.doorLockMode     = mode;

	EnqueuePacketToGroup(group, sizeof(txBuf->ZW_DoorLockOperationSetFrame));
}
#endif

#if CONTROL_CC_BASIC && WITH_CC_ASSOCIATION
void SendSet(BYTE group, BYTE classcmd, BYTE value) {
	if ((txBuf = CreatePacketToGroup()) == NULL)
		return;

	txBuf->ZW_BasicSetFrame.cmdClass = classcmd;
	txBuf->ZW_BasicSetFrame.cmd = BASIC_SET; // All CCs have same value for _SET constant
	txBuf->ZW_BasicSetFrame.value = value;

	EnqueuePacketToGroup(group, sizeof(txBuf->ZW_BasicSetFrame));
}
#endif

void SendReport(BYTE group, BYTE classcmd, BYTE reportparam) {
	if ((txBuf = CreatePacketToGroup()) != NULL) {
		txBuf->ZW_BasicReportFrame.cmdClass = classcmd;
		txBuf->ZW_BasicReportFrame.cmd = BASIC_REPORT; // All CCs have same value for _REPORT constant
		txBuf->ZW_BasicReportFrame.value = reportparam;
	
		// Size of most Report frame are equal to BASIC's Report size
		EnqueuePacketToGroup(group, sizeof(txBuf->ZW_BasicReportFrame));
	}
}
#endif

#if CONTROL_CC_SWITCH_MULTILEVEL && WITH_CC_ASSOCIATION
void SendStartChange(BYTE group, BOOL up_down, BOOL doIgnoreStartLevel, BYTE startLevel) {
	if ((txBuf = CreatePacketToGroup()) == NULL)
		return;

	txBuf->ZW_SwitchMultilevelStartLevelChangeFrame.cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL;
	txBuf->ZW_SwitchMultilevelStartLevelChangeFrame.cmd = SWITCH_MULTILEVEL_START_LEVEL_CHANGE;
	txBuf->ZW_SwitchMultilevelStartLevelChangeFrame.level = (((BYTE)up_down) << 6) | (((BYTE)doIgnoreStartLevel) << 5);
	txBuf->ZW_SwitchMultilevelStartLevelChangeFrame.startLevel = startLevel;

	EnqueuePacketToGroup(group, sizeof(txBuf->ZW_SwitchMultilevelStartLevelChangeFrame));
}

void SendStopChange(BYTE group) {
	if ((txBuf = CreatePacketToGroup()) == NULL)
		return;

	txBuf->ZW_SwitchMultilevelStopLevelChangeFrame.cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL;
	txBuf->ZW_SwitchMultilevelStopLevelChangeFrame.cmd = SWITCH_MULTILEVEL_STOP_LEVEL_CHANGE;

	EnqueuePacketToGroup(group, sizeof(txBuf->ZW_SwitchMultilevelStopLevelChangeFrame));
}
#endif

#if CONTROL_CC_SCENE_ACTIVATION && WITH_CC_ASSOCIATION
void SendScene(BYTE group, BYTE sceneId, BYTE dimmingDuration)  {
	if ((txBuf = CreatePacketToGroup()) == NULL)
		return;

	txBuf->ZW_SceneActivationSetFrame.cmdClass = COMMAND_CLASS_SCENE_ACTIVATION;
	txBuf->ZW_SceneActivationSetFrame.cmd = SCENE_ACTIVATION_SET;
	txBuf->ZW_SceneActivationSetFrame.sceneId = sceneId;
	txBuf->ZW_SceneActivationSetFrame.dimmingDuration = dimmingDuration;

	EnqueuePacketToGroup(group, sizeof(txBuf->ZW_SceneActivationSetFrame));
}

#if WITH_CC_SCENE_CONTROLLER_CONF
void SendSceneToGroup(BYTE groupNum) {
	if (scenesCtrl[groupNum].sceneID)
		SendScene(groupNum, scenesCtrl[groupNum].sceneID, scenesCtrl[groupNum].dimmingDuration);
}
#endif
#endif

#if WITH_CC_ASSOCIATION
#if WITH_CC_SENSOR_MULTILEVEL
void SendSensorMultilevelReportToGroup(BYTE deviceType, BYTE group, WORD measurement) {
	if ((txBuf = CreatePacketToGroup()) != NULL) {
		txBuf->ZW_SensorMultilevelReport2byteFrame.cmdClass = COMMAND_CLASS_SENSOR_MULTILEVEL;
		txBuf->ZW_SensorMultilevelReport2byteFrame.cmd = SENSOR_MULTILEVEL_REPORT;
		txBuf->ZW_SensorMultilevelReport2byteFrame.sensorType = deviceType;
		
		switch(deviceType) {
			#if CC_SENSOR_MULTILEVEL_WITH_AIR_TEMPERATURE
			case SENSOR_MULTILEVEL_REPORT_TEMPERATURE_VERSION_1_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
			(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
			((CC_SENSOR_MULTILEVEL_AIR_TEMPERATURE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
			((CC_SENSOR_MULTILEVEL_AIR_TEMPERATURE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_GENERAL_PURPOSE
			case SENSOR_MULTILEVEL_REPORT_GENERAL_PURPOSE_VALUE_VERSION_1_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_GENERAL_PURPOSE_VALUE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_GENERAL_PURPOSE_VALUE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_LUMINANCE
			case SENSOR_MULTILEVEL_REPORT_LUMINANCE_VERSION_1_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_LUMINANCE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_LUMINANCE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_POWER
			case SENSOR_MULTILEVEL_REPORT_POWER_VERSION_2_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_POWER_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_POWER_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
#endif
#if CC_SENSOR_MULTILEVEL_WITH_HUMIDITY
			case SENSOR_MULTILEVEL_REPORT_RELATIVE_HUMIDITY_VERSION_2_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
			(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_RELATIVE_HUMIDITY_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_RELATIVE_HUMIDITY_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_VELOCITY
			case SENSOR_MULTILEVEL_REPORT_VELOCITY_VERSION_2_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_VELOCITY_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_VELOCITY_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_DIRECTION
			case SENSOR_MULTILEVEL_REPORT_DIRECTION_VERSION_2_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_DIRECTION_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_DIRECTION_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_ATHMOSPHERIC_PRESSURE
			case SENSOR_MULTILEVEL_REPORT_ATMOSPHERIC_PRESSURE_VERSION_2_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_ATMOSPHERIC_PRESSURE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_ATMOSPHERIC_PRESSURE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_BAROMETRIC_PRESSURE
			case SENSOR_MULTILEVEL_REPORT_BAROMETRIC_PRESSURE_VERSION_2_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_BAROMETRIC_PRESSURE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_BAROMETRIC_PRESSURE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_SOLAR_RADIATION
			case SENSOR_MULTILEVEL_REPORT_SOLAR_RADIATION_VERSION_2_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_SOLAR_RADIATION_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_SOLAR_RADIATION_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_DEW_POINT
			case SENSOR_MULTILEVEL_REPORT_DEW_POINT_VERSION_2_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_DEW_POINT_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_DEW_POINT_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_RAIN_RATE
			case SENSOR_MULTILEVEL_REPORT_RAIN_RATE_VERSION_2_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_RAIN_RATE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_RAIN_RATE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_TIDE_LEVEL
			case SENSOR_MULTILEVEL_REPORT_TIDE_LEVEL_VERSION_2_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_TIDE_LEVEL_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_TIDE_LEVEL_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_WEIGHT
			case SENSOR_MULTILEVEL_REPORT_WEIGHT_VERSION_3_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_WEIGHT_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_WEIGHT_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_VOLTAGE
			case SENSOR_MULTILEVEL_REPORT_VOLTAGE_VERSION_3_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_VOLTAGE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_VOLTAGE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_CURRENT
			case SENSOR_MULTILEVEL_REPORT_CURRENT_VERSION_3_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_CURRENT_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_CURRENT_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_CO2_LEVEL
			case SENSOR_MULTILEVEL_REPORT_CO2_LEVEL_VERSION_3_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_CO2_LEVEL_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_CO2_LEVEL_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_AIR_FLOW
			case SENSOR_MULTILEVEL_REPORT_AIR_FLOW_VERSION_3_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_AIR_FLOW_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_AIR_FLOW_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_TANK_CAPACITY
			case SENSOR_MULTILEVEL_REPORT_TANK_CAPACITY_VERSION_3_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_TANK_CAPACITY_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_TANK_CAPACITY_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_DISTANCE
			case SENSOR_MULTILEVEL_REPORT_DISTANCE_VERSION_3_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_DISTANCE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_DISTANCE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_ANGLE_POSITION
			case SENSOR_MULTILEVEL_REPORT_ANGLE_POSITION_VERSION_4_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_ANGLE_POSITION_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_ANGLE_POSITION_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_ROTATION
			case SENSOR_MULTILEVEL_REPORT_ROTATION_V5_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_ROTATION_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_ROTATION_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_WATER_TEMPERATURE
			case SENSOR_MULTILEVEL_REPORT_WATER_TEMPERATURE_V5_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_WATER_TEMPERATURE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_WATER_TEMPERATURE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_SOIL_TEMPERATURE
			case SENSOR_MULTILEVEL_REPORT_SOIL_TEMPERATURE_V5_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_SOIL_TEMPERATURE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_SOIL_TEMPERATURE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_SEISMIC_INTENSITY
			case SENSOR_MULTILEVEL_REPORT_SEISMIC_INTENSITY_V5_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_SEISMIC_INTENSITY_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_SEISMIC_INTENSITY_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_SEISMIC_MAGNITUDE
			case SENSOR_MULTILEVEL_REPORT_SEISMIC_MAGNITUDE_V5_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_SEISMIC_MAGNITUDE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_SEISMIC_MAGNITUDE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_ULTRAVIOLET
			case SENSOR_MULTILEVEL_REPORT_ULTRAVIOLET_V5_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_ULTRAVIOLET_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_ULTRAVIOLET_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_ELECTRICAL_RESISTIVITY
			case SENSOR_MULTILEVEL_REPORT_ELECTRICAL_RESISTIVITY_V5_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_ELECTRICAL_RESISTIVITY_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_ELECTRICAL_RESISTIVITY_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_ELECTRICAL_CONDUCTIVITY
			case SENSOR_MULTILEVEL_REPORT_ELECTRICAL_CONDUCTIVITY_V5_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_ELECTRICAL_CONDUCTIVITY_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_ELECTRICAL_CONDUCTIVITY_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_LOUDNESS
			case SENSOR_MULTILEVEL_REPORT_LOUDNESS_V5_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_LOUDNESS_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_LOUDNESS_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_MOISTURE
			case SENSOR_MULTILEVEL_REPORT_MOISTURE_V5_V5:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_MOISTURE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_MOISTURE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_FREQUENCY
			case SENSOR_MULTILEVEL_REPORT_FREQUENCY_:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_TIME
			case SENSOR_MULTILEVEL_REPORT_TIME_:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
			#if CC_SENSOR_MULTILEVEL_WITH_TARGET_TEMPERATURE
			case SENSOR_MULTILEVEL_REPORT_TARGET_TEMPERATURE:
				txBuf->ZW_SensorMultilevelReport2byteFrame.level = \
					(CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
					((CC_SENSOR_MULTILEVEL_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
					((CC_SENSOR_MULTILEVEL_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
				break;
			#endif
		}
			*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = measurement;
		EnqueuePacketToGroup(group, sizeof(txBuf->ZW_SensorMultilevelReport2byteFrame));
	}
}
#endif
#endif

// Binary sensor routine
#if WITH_CC_SENSOR_BINARY || WITH_CC_SENSOR_BINARY_V2
void sendBinarySensorReport(BYTE dstNode, BYTE Type) {
	if ((txBuf = CreatePacket()) != NULL) {
		#if WITH_CC_SENSOR_BINARY_V2
			txBuf->ZW_SensorBinaryReportV2Frame.cmdClass	=	COMMAND_CLASS_SENSOR_BINARY;
			txBuf->ZW_SensorBinaryReportV2Frame.cmd			=	SENSOR_BINARY_REPORT;
			txBuf->ZW_SensorBinaryReportV2Frame.sensorType =	Type;
			txBuf->ZW_SensorBinaryReportV2Frame.sensorValue	=	SENSOR_BINARY_VALUE(Type);
			EnqueuePacket(dstNode, sizeof(txBuf->ZW_SensorBinaryReportV2Frame), TRANSMIT_OPTION_USE_DEFAULT);
		#endif
		#if WITH_CC_SENSOR_BINARY
			txBuf->ZW_SensorBinaryReportFrame.cmdClass	=	COMMAND_CLASS_SENSOR_BINARY;
			txBuf->ZW_SensorBinaryReportFrame.cmd		=	SENSOR_BINARY_REPORT;
			txBuf->ZW_SensorBinaryReportFrame.sensorValue=	SENSOR_BINARY_VALUE(Type);
			EnqueuePacket(dstNode, sizeof(txBuf->ZW_SensorBinaryReportFrame), TRANSMIT_OPTION_USE_DEFAULT);
		#endif
	}
}
#if WITH_CC_ASSOCIATION
void sendBinarySensorReportToGroup(BYTE Group, BYTE Type) {
	if ((txBuf = CreatePacketToGroup()) != NULL) {
		#if WITH_CC_SENSOR_BINARY_V2
			txBuf->ZW_SensorBinaryReportV2Frame.cmdClass	=	COMMAND_CLASS_SENSOR_BINARY;
			txBuf->ZW_SensorBinaryReportV2Frame.cmd			=	SENSOR_BINARY_REPORT;
			txBuf->ZW_SensorBinaryReportV2Frame.sensorType		=	Type;
			txBuf->ZW_SensorBinaryReportV2Frame.sensorValue		=	SENSOR_BINARY_VALUE(Type);
			EnqueuePacketToGroup(Group, sizeof(txBuf->ZW_SensorBinaryReportV2Frame));
		#endif
		#if WITH_CC_SENSOR_BINARY
			txBuf->ZW_SensorBinaryReportFrame.cmdClass	=	COMMAND_CLASS_SENSOR_BINARY;
			txBuf->ZW_SensorBinaryReportFrame.cmd		=	SENSOR_BINARY_REPORT;
			txBuf->ZW_SensorBinaryReportFrame.sensorValue=	SENSOR_BINARY_VALUE(Type);
			EnqueuePacketToGroup(Group, sizeof(txBuf->ZW_SensorBinaryReportFrame));
		#endif
	}
}
#endif
#endif


//Notification routines
#if WITH_CC_NOTIFICATION
BYTE NotificationEventStatusGetter(BYTE type, BYTE event) {
	switch (type) {
		
		#if CC_NOTIFICATION_WITH_SMOKE_ALARM
		case NOTIFICATION_TYPE_SMOKE_ALARM:
			switch(event) {
				#if CC_EVENT_WITH_SMOKE
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_SMOKE_ALARM,NOTIFICATION_EVENT_SMOKE)
				#endif

				#if CC_EVENT_WITH_SMOKE_UL
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_SMOKE_ALARM,NOTIFICATION_EVENT_SMOKE_UL)
				#endif
				
				#if CC_EVENT_WITH_SMOKE_TEST
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_SMOKE_ALARM,NOTIFICATION_EVENT_SMOKE_TEST)
				#endif

				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_SMOKE_ALARM,NOTIFICATION_OFF_VALUE)
			}
			break;
		#endif
		
		#if CC_NOTIFICATION_WITH_CO_ALARM
		case NOTIFICATION_TYPE_CO_ALARM:
			switch(event) {
				#if CC_EVENT_WITH_CO
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_CO_ALARM,NOTIFICATION_EVENT_CO)
				#endif
				
				#if CC_EVENT_WITH_CO_UL
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_CO_ALARM,NOTIFICATION_EVENT_CO_UL)
				#endif

				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_CO_ALARM,NOTIFICATION_OFF_VALUE)
			}
			break;
		#endif
		
		#if CC_NOTIFICATION_WITH_CO2_ALARM
		case NOTIFICATION_TYPE_CO2_ALARM:
			switch(event) {
				#if CC_EVENT_WITH_CO2
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_CO2_ALARM,NOTIFICATION_EVENT_CO2)
				#endif
				
				#if CC_EVENT_WITH_CO2_UL
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_CO2_ALARM,NOTIFICATION_EVENT_CO2_UL)
				#endif

				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_CO2_ALARM,NOTIFICATION_OFF_VALUE)
			}
			break;
		#endif
		
		#if CC_NOTIFICATION_WITH_HEAT_ALARM
		case NOTIFICATION_TYPE_HEAT_ALARM:
			switch(event) {
				#if CC_EVENT_WITH_OVERHEAT
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_HEAT_ALARM,NOTIFICATION_EVENT_OVERHEAT)
				#endif
				
				#if CC_EVENT_WITH_OVERHEAT_UL
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_HEAT_ALARM,NOTIFICATION_EVENT_OVERHEAT_UL)
				#endif
				
				#if CC_EVENT_WITH_TEMP_RISE
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_HEAT_ALARM,NOTIFICATION_EVENT_TEMP_RISE)
				#endif
				
				#if CC_EVENT_WITH_TEMP_RISE_UL
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_HEAT_ALARM,NOTIFICATION_EVENT_TEMP_RISE_UL)
				#endif
				
				#if CC_EVENT_WITH_UNDERHEAT
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_HEAT_ALARM,NOTIFICATION_EVENT_UNDERHEAT)
				#endif
				
				#if CC_EVENT_WITH_UNDERHEAT_UL
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_HEAT_ALARM,NOTIFICATION_EVENT_UNDERHEAT_UL)
				#endif

				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_HEAT_ALARM,NOTIFICATION_OFF_VALUE)
			}
			break;
		#endif
		
		#if CC_NOTIFICATION_WITH_WATER_ALARM
		case NOTIFICATION_TYPE_WATER_ALARM:
			switch(event) {
				#if CC_EVENT_WITH_WATER_LEAK
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_WATER_ALARM,NOTIFICATION_EVENT_WATER_LEAK)
				#endif
				
				#if CC_EVENT_WITH_WATER_LEAK_UL
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_WATER_ALARM,NOTIFICATION_EVENT_WATER_LEAK_UL)
				#endif
				
				#if CC_EVENT_WITH_WATER_LEVEL_DROP
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_WATER_ALARM,NOTIFICATION_EVENT_WATER_LEVEL_DROP)
				#endif
				
				#if CC_EVENT_WITH_WATER_LEVEL_DROP_UL
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_WATER_ALARM,NOTIFICATION_EVENT_WATER_LEVEL_DROP_UL)
				#endif

				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_WATER_ALARM,NOTIFICATION_OFF_VALUE)
			}
			break;
		#endif
		
		#if CC_NOTIFICATION_WITH_ACCESS_CONTROL_ALARM
		case NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM:
			switch(event) {
				#if CC_EVENT_WITH_MANUAL_LOCK
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_MANUAL_LOCK)
				#endif
				#if CC_EVENT_WITH_MANUAL_UNLOCK
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_MANUAL_UNLOCK)
				#endif
				
				#if CC_EVENT_WITH_MANUAL_NOT_FULLY_LOCKED
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_MANUAL_NOT_FULLY_LOCKED)
				#endif
				
				#if CC_EVENT_WITH_RF_LOCK
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_RF_LOCK)
				#endif
				
				#if CC_EVENT_WITH_RF_UNLOCK
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_RF_UNLOCK)
				#endif
				
				#if CC_EVENT_WITH_RF_NOT_FULLY_LOCKED
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_RF_NOT_FULLY_LOCKED)
				#endif
				
				#if CC_EVENT_WITH_KEYPAD_LOCK
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_KEYPAD_LOCK)
				#endif
				
				#if CC_EVENT_WITH_KEYPAD_UNLOCK
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_KEYPAD_UNLOCK)
				#endif
				
				#if CC_EVENT_WITH_AUTO_LOCKED
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_AUTO_LOCKED)
				#endif
				
				#if CC_EVENT_WITH_AUTO_LOCK_NOT_FULLY_LOCKED
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_AUTO_LOCK_NOT_FULLY_LOCKED)
				#endif

				#if CC_EVENT_WITH_LOCK_JAMMED
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_LOCK_JAMMED)
				#endif
				
				#if CC_EVENT_WITH_ALL_USER_CODES_DELETED
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_ALL_USER_CODES_DELETED)
				#endif
				
				#if CC_EVENT_WITH_SINGLE_USER_CODE_DELETED
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_SINGLE_USER_CODE_DELETED)
				#endif
				
				#if CC_EVENT_WITH_NEW_USER_CODE_ADDED
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_NEW_USER_CODE_ADDED)
				#endif
				
				#if CC_EVENT_WITH_NEW_USER_CODE_NOT_ADDED
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_NEW_USER_CODE_NOT_ADDED)
				#endif
				
				#if CC_EVENT_WITH_KEYPAD_TEMPORARY_DISABLE
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_KEYPAD_TEMPORARY_DISABLE)
				#endif
				
				#if CC_EVENT_WITH_KEYPAD_BUSY
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_KEYPAD_BUSY)
				#endif
				
				#if CC_EVENT_WITH_NEW_PROGRAM_CODE_ENTERED
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_NEW_PROGRAM_CODE_ENTERED)
				#endif
				
				#if CC_EVENT_WITH_MANUAL_CODE_EXCEEDS_LIMITS
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_MANUAL_CODE_EXCEEDS_LIMITS)
				#endif
				#if CC_EVENT_WITH_RF_UNLOCK_INVALID_CODE
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_RF_UNLOCK_INVALID_CODE)
				#endif
				
				#if CC_EVENT_WITH_RF_LOCK_INVALID_CODE
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_RF_LOCK_INVALID_CODE)
				#endif
				
				#if CC_EVENT_WITH_WINDOW_DOOR_CLOSED
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_WINDOW_DOOR_CLOSED)
				#endif
				
				#if CC_EVENT_WITH_WINDOW_DOOR_OPENED
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_EVENT_WINDOW_DOOR_OPENED)
				#endif

				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM,NOTIFICATION_OFF_VALUE)
			}
			break;
		#endif
		
		#if CC_NOTIFICATION_WITH_BURGLAR_ALARM
		case NOTIFICATION_TYPE_BURGLAR_ALARM
			switch(event) {
				#if CC_EVENT_WITH_INTRUSION
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_BURGLAR_ALARM,NOTIFICATION_EVENT_INTRUSION)
				#endif
				
				#if CC_EVENT_WITH_INTRUSION_UL
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_BURGLAR_ALARM,NOTIFICATION_EVENT_INTRUSION_UL)
				#endif
				
				#if CC_EVENT_WITH_TAMPER_REMOVED
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_BURGLAR_ALARM,NOTIFICATION_EVENT_TAMPER_REMOVED)
				#endif
				
				#if CC_EVENT_WITH_TAMPER_INVALID_CODE
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_BURGLAR_ALARM,NOTIFICATION_EVENT_TAMPER_INVALID_CODE)
				#endif
				
				#if CC_EVENT_WITH_GLASS_BREAK
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_BURGLAR_ALARM,NOTIFICATION_EVENT_GLASS_BREAK)
				#endif
				#if CC_EVENT_WITH_GLASS_BREAK_UL
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_BURGLAR_ALARM,NOTIFICATION_EVENT_GLASS_BREAK_UL)
				#endif
				
				#if CC_EVENT_WITH_MOTION_DETECTION
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_BURGLAR_ALARM,NOTIFICATION_EVENT_MOTION_DETECTION)
				#endif

				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_BURGLAR_ALARM,NOTIFICATION_OFF_VALUE)
			}
			break;
		#endif
		
		#if CC_NOTIFICATION_WITH_POWER_MANAGEMENT_ALARM
		case NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM:
			switch(event) {
				#if CC_EVENT_WITH_POWER_APPLIED
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM,NOTIFICATION_EVENT_POWER_APPLIED)
				#endif
				
				#if CC_EVENT_WITH_AC_DISCONNECTED
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM,NOTIFICATION_EVENT_AC_DISCONNECTED)
				#endif
				
				#if CC_EVENT_WITH_AC_RECONNECTED
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM,NOTIFICATION_EVENT_AC_RECONNECTED)
				#endif
				
				#if CC_EVENT_WITH_SURGE
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM,NOTIFICATION_EVENT_SURGE)
				#endif
				
				#if CC_EVENT_WITH_VOLTAGE_DROP
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM,NOTIFICATION_EVENT_VOLTAGE_DROP)
				#endif
				
				#if CC_EVENT_WITH_OVER_CURRENT
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM,NOTIFICATION_EVENT_OVER_CURRENT)
				#endif
				
				#if CC_EVENT_WITH_OVER_VOLTAGE
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM,NOTIFICATION_EVENT_OVER_VOLTAGE)
				#endif
				#if CC_EVENT_WITH_OVER_LOAD
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM,NOTIFICATION_EVENT_OVER_LOAD)
				#endif
				
				#if CC_EVENT_WITH_LOAD_ERROR
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM,NOTIFICATION_EVENT_LOAD_ERROR)
				#endif
				
				#if CC_EVENT_WITH_REPLACE_BAT_SOON
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM,NOTIFICATION_EVENT_REPLACE_BAT_SOON)
				#endif
				
				#if CC_EVENT_WITH_REPLACE_BAT_NOW
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM,NOTIFICATION_EVENT_REPLACE_BAT_NOW)
				#endif

				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM,NOTIFICATION_OFF_VALUE)
			}
			break;
		#endif
		
		#if CC_NOTIFICATION_WITH_SYSTEM_ALARM
		case NOTIFICATION_TYPE_SYSTEM_ALARM:
			switch(event) {
				#if CC_EVENT_WITH_HW_FAIL
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_SYSTEM_ALARM,NOTIFICATION_EVENT_HW_FAIL)
				#endif
				
				#if CC_EVENT_WITH_SW_FAIL
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_SYSTEM_ALARM,NOTIFICATION_EVENT_SW_FAIL)
				#endif
				
				#if CC_EVENT_WITH_HW_FAIL_OEM
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_SYSTEM_ALARM,NOTIFICATION_EVENT_HW_FAIL_OEM)
				#endif
				
				#if CC_EVENT_WITH_SW_FAIL_OEM
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_SYSTEM_ALARM,NOTIFICATION_EVENT_SW_FAIL_OEM)
				#endif

				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_SYSTEM_ALARM,NOTIFICATION_OFF_VALUE)
			}
			break;
		#endif
		
		#if CC_NOTIFICATION_WITH_EMERGENCY_ALARM
		case NOTIFICATION_TYPE_EMERGENCY_ALARM:
			switch(event) {
				#if CC_EVENT_WITH_CONTACT_POLICE
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_EMERGENCY_ALARM,NOTIFICATION_EVENT_CONTACT_POLICE)
				#endif
				
				#if CC_EVENT_WITH_CONTACT_FIREMEN
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_EMERGENCY_ALARM,NOTIFICATION_EVENT_CONTACT_FIREMEN)
				#endif
				
				#if CC_EVENT_WITH_CONTACT_MEDIC
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_EMERGENCY_ALARM,NOTIFICATION_EVENT_CONTACT_MEDIC)
				#endif

				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_EMERGENCY_ALARM,NOTIFICATION_OFF_VALUE)
			}
			break;
		#endif
		
		#if CC_NOTIFICATION_WITH_CLOCK_ALARM
		case NOTIFICATION_TYPE_CLOCK_ALARM:
			switch(event) {
				#if CC_EVENT_WITH_WAKE_UP_ALERT
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_CLOCK_ALARM,NOTIFICATION_EVENT_WAKE_UP_ALERT)
				#endif
				
				#if CC_EVENT_WITH_TIMER_ENDED
				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_CLOCK_ALARM,NOTIFICATION_EVENT_TIMER_ENDED)
				#endif

				NOTIFICATION_EVENT_STATUS(NOTIFICATION_TYPE_CLOCK_ALARM,NOTIFICATION_OFF_VALUE)
			}
			break;
		#endif
			
		default:
			break;
	}
}

void sendNotificationReport(BYTE dstNode, BYTE Type, BYTE Event,BOOL alarm_version_flag) {
	if ((txBuf = CreatePacket()) != NULL) {
		txBuf->ZW_NotificationReport1byteV3Frame.cmdClass = COMMAND_CLASS_NOTIFICATION_V3;
		txBuf->ZW_NotificationReport1byteV3Frame.cmd = NOTIFICATION_REPORT_V3;
		txBuf->ZW_NotificationReport1byteV3Frame.v1AlarmType = 0x00; 
		txBuf->ZW_NotificationReport1byteV3Frame.v1AlarmLevel = 0x00; 
		txBuf->ZW_NotificationReport1byteV3Frame.zensorNetSourceNodeId = 0x00;
		if(Event==NOTIFICATION_EVENT_UNKNOWN) {txBuf->ZW_NotificationReport1byteV3Frame.notificationStatus = 0x00;}
		else {txBuf->ZW_NotificationReport1byteV3Frame.notificationStatus = NotificationEventStatusGetter(Type,Event);}		
		txBuf->ZW_NotificationReport1byteV3Frame.notificationType = Type; 
		txBuf->ZW_NotificationReport1byteV3Frame.mevent = Event;
		txBuf->ZW_NotificationReport1byteV3Frame.properties1 = 0;// Encapsulated commands not included! 
		txBuf->ZW_NotificationReport1byteV3Frame.eventParameter1 = 0; // Encapsulated commands not included!
		if(alarm_version_flag) {
			txBuf->ZW_NotificationReport1byteV4Frame.sequenceNumber = 0;// Encapsulated commands not included!
		}
	}
	if(alarm_version_flag) {
		EnqueuePacket(dstNode, sizeof(txBuf->ZW_NotificationReport1byteV4Frame), TRANSMIT_OPTION_USE_DEFAULT);
	} else {
		EnqueuePacket(dstNode, sizeof(txBuf->ZW_AlarmReport1byteV2Frame), TRANSMIT_OPTION_USE_DEFAULT);
	}
	
}

#if WITH_CC_ASSOCIATION

void sendNotificationReportToGroup(BYTE Group, BYTE Type, BYTE Event) {
	if ((txBuf = CreatePacketToGroup()) != NULL) {
		txBuf->ZW_NotificationReport1byteV3Frame.cmdClass = COMMAND_CLASS_NOTIFICATION_V3;
		txBuf->ZW_NotificationReport1byteV3Frame.cmd = NOTIFICATION_REPORT_V3;
		txBuf->ZW_NotificationReport1byteV3Frame.v1AlarmType = 0x00; 
		txBuf->ZW_NotificationReport1byteV3Frame.v1AlarmLevel = 0x00; 
		txBuf->ZW_NotificationReport1byteV3Frame.zensorNetSourceNodeId = 0x00;
		if(Event==NOTIFICATION_EVENT_UNKNOWN) {txBuf->ZW_NotificationReport1byteV3Frame.notificationStatus = 0x00;}
		else {txBuf->ZW_NotificationReport1byteV3Frame.notificationStatus = NotificationEventStatusGetter(Type,Event);}		
		txBuf->ZW_NotificationReport1byteV3Frame.notificationType = Type; 
		txBuf->ZW_NotificationReport1byteV3Frame.mevent = Event;
		txBuf->ZW_NotificationReport1byteV3Frame.properties1 = 0;// Encapsulated commands not included! 
		txBuf->ZW_NotificationReport1byteV3Frame.eventParameter1 = 0; // Encapsulated commands not included!
		txBuf->ZW_NotificationReport1byteV4Frame.sequenceNumber = 0;// Encapsulated commands not included!
	}
		EnqueuePacketToGroup(Group, sizeof(txBuf->ZW_NotificationReport1byteV4Frame));
}

#endif
#endif

// Meter
#if WITH_CC_METER

//#define METER_SIZE 4	

// #define METER_RATE_TYPE
// #define METER_RESETABLE
// #define METER_RESET_HANDLER
// #define METER_PREVIOUS_VALUE
// #define METER_DELTA

#define FILL_METER_REPORT_FRAME_qER(S)\
	txBuf->ZW_MeterReport##S##byteFrame.cmdClass 	= COMMAND_CLASS_METER;\
	txBuf->ZW_MeterReport##S##byteFrame.cmd			=	METER_REPORT;\
	txBuf->ZW_MeterReport##S##byteFrame.meterType	=	METER_TYPE; \
	txBuf->ZW_MeterReport##S##byteFrame.properties1	=	(METER_PRECISION << 5) | (METER_SCALE << 3) | S;
	
			
BYTE fillMeterReportPacket(BYTE scale)
{
	BYTE   	len 			= sizeof(txBuf->ZW_MeterReport1byteV4Frame)-5;
	BYTE  	*v_dst_addr   = &txBuf->ZW_MeterReport1byteV4Frame.meterValue1;
	
	txBuf->ZW_MeterReport1byteV4Frame.cmdClass 		= 	COMMAND_CLASS_METER;
	txBuf->ZW_MeterReport1byteV4Frame.cmd			=	METER_REPORT;
	txBuf->ZW_MeterReport1byteV4Frame.properties1	=	(((scale >>2) & 0x01) ? METER_REPORT_PROPERTIES1_SCALE2_BIT_MASK_V4:0) |
	    												((METER_RATE_TYPE << METER_REPORT_PROPERTIES1_RATE_TYPE_SHIFT_V4) & METER_REPORT_PROPERTIES1_RATE_TYPE_MASK_V4)|
	    												(METER_TYPE & METER_REPORT_PROPERTIES1_METER_TYPE_MASK_V4);
	txBuf->ZW_MeterReport1byteV4Frame.properties2	=	((METER_PRECISION(scale) << METER_REPORT_PROPERTIES2_PRECISION_SHIFT_V4) & METER_REPORT_PROPERTIES2_PRECISION_MASK_V4)| 
														((scale << METER_REPORT_PROPERTIES2_SCALE10_SHIFT_V4) & METER_REPORT_PROPERTIES2_SCALE10_MASK_V4) | 
														(METER_SIZE(scale) &  METER_REPORT_PROPERTIES2_SIZE_MASK_V4 );
	
	switch(METER_SIZE(scale))
	{
		case 1:
				*v_dst_addr = METER_VALUE(scale);	
				break;
		case 2:
				*((WORD*)v_dst_addr) = METER_VALUE(scale);
				break;
		case 4: 
				*((DWORD*)v_dst_addr) = METER_VALUE(scale);
				break;
			
	}

	v_dst_addr += METER_SIZE(scale); 
	len += METER_SIZE(scale) + 2;
	*((WORD*)(v_dst_addr)) = METER_DELTA(scale);

	
	if(METER_DELTA(scale) != 0)
	{
			v_dst_addr += 2;
			switch(METER_SIZE(scale))
			{
				case 1:
				*v_dst_addr = METER_PREVIOUS_VALUE(scale);	
				break;
				case 2:
				*((WORD*)v_dst_addr) = METER_PREVIOUS_VALUE(scale);
				break;
				case 4: 
				*((DWORD*)v_dst_addr) = METER_PREVIOUS_VALUE(scale);
				break;
			
			}
			len += METER_SIZE(scale); 
	} 

	return len;
}
void sendMeterReport(BYTE dstNode, BYTE scale) {
	if ((txBuf = CreatePacket()) != NULL) 
	{
		BYTE len = fillMeterReportPacket(scale);
		EnqueuePacket(dstNode, len, TRANSMIT_OPTION_USE_DEFAULT);
			
	}
}

#if WITH_CC_ASSOCIATION
void sendMeterReportToGroup(BYTE Group, BYTE scale) {
	if ((txBuf = CreatePacketToGroup()) != NULL) 
	{
		BYTE len = fillMeterReportPacket(scale);
		EnqueuePacketToGroup(Group, len);
		
	}
}
#endif
#endif


#if WITH_CC_METER_TABLE_MONITOR
void sendMeterTableDataReport(BYTE dstNodeOrGroup, 
							  BYTE reportsToFollow,
							  DWORD datasetn,
							  DWORD timestamp,
							  BYTE  precScale,
							  DWORD value) {
	WORD year=1970;
	//BYTE  month=1, day=1, hour=(datasetn/3600)%24, minute=(datasetn%3600)%60, second=datasetn%60;
	BYTE  month, day, hour, minute, second;	
	BYTE  structsize;

	if(datasetn & METER_TBL_ZWME_DATASET4_GROUP) {
		#if WITH_CC_ASSOCIATION
			if((txBuf = CreatePacketToGroup()) == NULL)
				return;
		#else
			return;	
		#endif
	} else {
		if ((txBuf = CreatePacket()) == NULL)
			return; 
	
	}
	// !MEMORY
	decodeTimeStamp(timestamp, &year, &month, &day, &hour, &minute, &second);
	
	if(datasetn & METER_TBL_ZWME_DATASET4_HISTORICAL) {
		txBuf->ZW_MeterTblHistoricalDataReport1byteFrame.cmdClass			=	COMMAND_CLASS_METER_TBL_MONITOR;
		txBuf->ZW_MeterTblHistoricalDataReport1byteFrame.cmd				=	METER_TBL_HISTORICAL_DATA_REPORT;
		txBuf->ZW_MeterTblHistoricalDataReport1byteFrame.reportsToFollow 	=	reportsToFollow;
		txBuf->ZW_MeterTblHistoricalDataReport1byteFrame.properties1		=	METER_TBL_CAP_RATE_TYPE;
		txBuf->ZW_MeterTblHistoricalDataReport1byteFrame.dataset3			=	datasetn & 0xFF;
		txBuf->ZW_MeterTblHistoricalDataReport1byteFrame.dataset2			=	(datasetn >> 8) & 0xFF;
		txBuf->ZW_MeterTblHistoricalDataReport1byteFrame.dataset1			=	(datasetn >> 16) & 0xFF;
		txBuf->ZW_MeterTblHistoricalDataReport1byteFrame.year1				=	(year >> 8) & 0xFF;
		txBuf->ZW_MeterTblHistoricalDataReport1byteFrame.year2				=	year & 0xFF;
		txBuf->ZW_MeterTblHistoricalDataReport1byteFrame.month				=	month;
		txBuf->ZW_MeterTblHistoricalDataReport1byteFrame.day				=	day;
		txBuf->ZW_MeterTblHistoricalDataReport1byteFrame.hourLocalTime		=	hour;
		txBuf->ZW_MeterTblHistoricalDataReport1byteFrame.minuteLocalTime	=	minute;
		txBuf->ZW_MeterTblHistoricalDataReport1byteFrame.secondLocalTime	=	second;
	
		txBuf->ZW_MeterTblHistoricalDataReport1byteFrame.variantgroup1.properties1		=   precScale;
		FILL_MULTIBYTE_PARAM4(txBuf->ZW_MeterTblHistoricalDataReport1byteFrame.variantgroup1.historicalValue, value);
		structsize = sizeof(txBuf->ZW_MeterTblHistoricalDataReport1byteFrame);
	} else {
		txBuf->ZW_MeterTblCurrentDataReport1byteFrame.cmdClass			=	COMMAND_CLASS_METER_TBL_MONITOR;
		txBuf->ZW_MeterTblCurrentDataReport1byteFrame.cmd				=	METER_TBL_CURRENT_DATA_REPORT;
		txBuf->ZW_MeterTblCurrentDataReport1byteFrame.reportsToFollow 	=	reportsToFollow;
		txBuf->ZW_MeterTblCurrentDataReport1byteFrame.properties1		=	METER_TBL_CAP_RATE_TYPE;
		txBuf->ZW_MeterTblCurrentDataReport1byteFrame.dataset3			=	datasetn & 0xFF;
		txBuf->ZW_MeterTblCurrentDataReport1byteFrame.dataset2			=	(datasetn >> 8) & 0xFF;
		txBuf->ZW_MeterTblCurrentDataReport1byteFrame.dataset1			=	(datasetn >> 16) & 0xFF;
		txBuf->ZW_MeterTblCurrentDataReport1byteFrame.year1				=	(year >> 8) & 0xFF;
		txBuf->ZW_MeterTblCurrentDataReport1byteFrame.year2				=	year & 0xFF;
		txBuf->ZW_MeterTblCurrentDataReport1byteFrame.month				=	month;
		txBuf->ZW_MeterTblCurrentDataReport1byteFrame.day				=	day;
		txBuf->ZW_MeterTblCurrentDataReport1byteFrame.hourLocalTime		=	hour;
		txBuf->ZW_MeterTblCurrentDataReport1byteFrame.minuteLocalTime	=	minute;
		txBuf->ZW_MeterTblCurrentDataReport1byteFrame.secondLocalTime	=	second;
	
		txBuf->ZW_MeterTblCurrentDataReport1byteFrame.variantgroup1.properties1		=   precScale;
		FILL_MULTIBYTE_PARAM4(txBuf->ZW_MeterTblCurrentDataReport1byteFrame.variantgroup1.currentValue, value);
		structsize = sizeof(txBuf->ZW_MeterTblCurrentDataReport1byteFrame);
	}
	
	if(datasetn & METER_TBL_ZWME_DATASET4_GROUP) {
		#if WITH_CC_ASSOCIATION
			EnqueuePacketToGroup(dstNodeOrGroup, structsize);		
		#endif
	} else {
		EnqueuePacket(dstNodeOrGroup, structsize, TRANSMIT_OPTION_USE_DEFAULT);
	}
}							  
#endif

#if WITH_CC_TIME_PARAMETERS

void sendTimeParametersReport(BYTE dstNode, TIME_PARAMETERS_DATA * pointer_TimeData) {
	if ((txBuf = CreatePacket()) == NULL)
			return; 
	
	txBuf->ZW_TimeParametersReportFrame.cmdClass 		=	COMMAND_CLASS_TIME_PARAMETERS;
	txBuf->ZW_TimeParametersReportFrame.cmd	  			=	TIME_PARAMETERS_REPORT;
	txBuf->ZW_TimeParametersReportFrame.year1			=	WORD_HI_BYTE(pointer_TimeData->year);
	txBuf->ZW_TimeParametersReportFrame.year2			=	WORD_LO_BYTE(pointer_TimeData->year);
	txBuf->ZW_TimeParametersReportFrame.month			=	pointer_TimeData->month;
	txBuf->ZW_TimeParametersReportFrame.day				=	pointer_TimeData->day;
	txBuf->ZW_TimeParametersReportFrame.hourUtc			=	pointer_TimeData->hours;
	txBuf->ZW_TimeParametersReportFrame.minuteUtc		=	pointer_TimeData->minutes;
	txBuf->ZW_TimeParametersReportFrame.secondUtc		=	pointer_TimeData->seconds;
		
	EnqueuePacket(dstNode, sizeof(txBuf->ZW_TimeParametersReportFrame), TRANSMIT_OPTION_USE_DEFAULT);
}
void sendTimeParametersGet(BYTE dstNode) {
	if ((txBuf = CreatePacket()) == NULL)
			return; 
					
			
	txBuf->ZW_TimeParametersGetFrame.cmdClass 		=	COMMAND_CLASS_TIME_PARAMETERS;
	txBuf->ZW_TimeParametersGetFrame.cmd	  		=	TIME_PARAMETERS_GET;
		
	EnqueuePacket(dstNode, sizeof(txBuf->ZW_TimeParametersGetFrame), TRANSMIT_OPTION_USE_DEFAULT);
}

#endif

#if WITH_CC_SWITCH_BINARY
void sendSwitchBinaryReport(BYTE dstNode) {
	if ((txBuf = CreatePacket()) == NULL)
			return; 
	
	txBuf->ZW_SwitchBinaryReportFrame.cmdClass 		=	COMMAND_CLASS_SWITCH_BINARY;
	txBuf->ZW_SwitchBinaryReportFrame.cmd	  		=	SWITCH_BINARY_REPORT;
	txBuf->ZW_SwitchBinaryReportFrame.value	  		=	SWITCH_BINARY_VALUE;
		
	EnqueuePacket(dstNode, sizeof(txBuf->ZW_SwitchBinaryReportFrame), TRANSMIT_OPTION_USE_DEFAULT);

}
#if WITH_CC_ASSOCIATION
void sendSwitchBinaryReportToGroup(BYTE dstGroup) {
	if ((txBuf = CreatePacketToGroup()) == NULL)
			return; 
	
	txBuf->ZW_SwitchBinaryReportFrame.cmdClass 		=	COMMAND_CLASS_SWITCH_BINARY;
	txBuf->ZW_SwitchBinaryReportFrame.cmd	  		=	SWITCH_BINARY_REPORT;
	txBuf->ZW_SwitchBinaryReportFrame.value	  		=	SWITCH_BINARY_VALUE;
		
	EnqueuePacketToGroup(dstGroup, sizeof(txBuf->ZW_SwitchBinaryReportFrame));

}
#endif

#endif

#if WITH_CC_CENTRAL_SCENE
void SendCentralScene(BYTE group, BYTE attribute, BYTE scene) {
	#if PRODUCT_LISTENING_TYPE
	static BYTE centralSceneSequence = 0;
	#endif
	if ((txBuf = CreatePacketToGroup()) == NULL)
			return; 
	
	txBuf->ZW_CentralSceneNotificationFrame.cmdClass 		=	COMMAND_CLASS_CENTRAL_SCENE;
	txBuf->ZW_CentralSceneNotificationFrame.cmd	  			=	CENTRAL_SCENE_NOTIFICATION;
	#if PRODUCT_LISTENING_TYPE
	txBuf->ZW_CentralSceneNotificationFrame.sequenceNumber	=	centralSceneSequence++;
	#else
	txBuf->ZW_CentralSceneNotificationFrame.sequenceNumber	=	NZ(centralSceneSequence)++;
	#endif
	txBuf->ZW_CentralSceneNotificationFrame.properties1	 	=	attribute & CENTRAL_SCENE_NOTIFICATION_PROPERTIES1_KEY_ATTRIBUTES_MASK;
	txBuf->ZW_CentralSceneNotificationFrame.sceneNumber		=	scene;
		
	EnqueuePacketToGroup(group, sizeof(txBuf->ZW_CentralSceneNotificationFrame));	
}

void SendCentralSceneClick(BYTE group, BYTE scene) {
	SendCentralScene(group, CENTRAL_SCENE_KEY_ATTRIBUTE_PRESSED, scene);
}
void SendCentralSceneHoldStart(BYTE group, BYTE scene) {
	SendCentralScene(group, CENTRAL_SCENE_KEY_ATTRIBUTE_HELD_DOWN, scene);
	centralSceneHeldDown[scene - 1] = group;
}
void SendCentralSceneHoldRelease(BYTE group, BYTE scene) {
	SendCentralScene(group, CENTRAL_SCENE_KEY_ATTRIBUTE_RELEASED, scene);
	centralSceneHeldDown[scene - 1] = CENTRAL_SCENE_REPEAT_EMPTY;
}

void CentralSceneInit(void) {
	BYTE n;

	for (n = 0; n < CENTRAL_SCENE_MAX_SCENES; n++)
		centralSceneHeldDown[n] = CENTRAL_SCENE_REPEAT_EMPTY;
}

void CentralSceneRepeat(void) {
	BYTE n;

	for (n = 0; n < CENTRAL_SCENE_MAX_SCENES; n++) {
		if (centralSceneHeldDown[n] != CENTRAL_SCENE_REPEAT_EMPTY) {
			SendCentralScene(centralSceneHeldDown[n], CENTRAL_SCENE_KEY_ATTRIBUTE_HELD_DOWN, n + 1);
		}
	}
}
void CentralSceneClearRepeats()
{
	BYTE n;

	for (n = 0; n < CENTRAL_SCENE_MAX_SCENES; n++) {
		centralSceneHeldDown[n] = CENTRAL_SCENE_REPEAT_EMPTY;
	}
}
#endif


#if WITH_CC_THERMOSTAT_SETPOINT
void SendThermostatSetPointReport(BYTE mode, WORD temp) {
	if ((txBuf = CreatePacketToGroup()) != NULL) {
		txBuf->ZW_ThermostatSetpointReport2byteFrame.cmdClass = COMMAND_CLASS_THERMOSTAT_SETPOINT;
		txBuf->ZW_ThermostatSetpointReport2byteFrame.cmd = THERMOSTAT_SETPOINT_REPORT;
		txBuf->ZW_ThermostatSetpointReport2byteFrame.level = mode;
		txBuf->ZW_ThermostatSetpointReport2byteFrame.level2 = 
				((THERMOSTAT_SETPOINT_PRECISION << THERMOSTAT_SETPOINT_SET_LEVEL2_PRECISION_SHIFT) & THERMOSTAT_SETPOINT_SET_LEVEL2_PRECISION_MASK) | \
				((THERMOSTAT_SETPOINT_SCALE_CELSIUS << THERMOSTAT_SETPOINT_SET_LEVEL2_SCALE_SHIFT) & THERMOSTAT_SETPOINT_SET_LEVEL2_SCALE_MASK) | \
				(THERMOSTAT_SETPOINT_TEMPERATURE_SIZE & THERMOSTAT_SETPOINT_SET_LEVEL2_SIZE_MASK),
		txBuf->ZW_ThermostatSetpointReport2byteFrame.value1 = WORD_HI_BYTE(temp);
		txBuf->ZW_ThermostatSetpointReport2byteFrame.value2 = WORD_LO_BYTE(temp);
		EnqueuePacketToGroup(ASSOCIATION_REPORTS_GROUP_NUM, sizeof(txBuf->ZW_ThermostatSetpointReport2byteFrame));
	}
}
#endif

#if CONTROL_CC_SWITCH_ALL
void SwitchAllSend(BYTE mode) {
	if ((txBuf = CreatePacket()) != NULL) {
		txBuf->ZW_SwitchAllOnFrame.cmdClass = COMMAND_CLASS_SWITCH_ALL; // On and Off frame have the same size
		txBuf->ZW_SwitchAllOnFrame.cmd = mode;
		EnqueuePacket(NODE_BROADCAST, sizeof(txBuf->ZW_SwitchAllOnFrame), TRANSMIT_OPTION_ACK);
	}
}
#endif
//#endif // WITH_CC_ASSOCIATION

#if CONTROL_CC_PROXIMITY_MULTILEVEL
void ProximitySendBasic(BYTE level) {
	if ((txBuf = CreatePacket()) != NULL) {
		txBuf->ZW_BasicSetFrame.cmdClass = COMMAND_CLASS_BASIC;
		txBuf->ZW_BasicSetFrame.cmd = BASIC_SET;
		txBuf->ZW_BasicSetFrame.value = level;
		EnqueuePacket(NODE_BROADCAST, sizeof(txBuf->ZW_BasicSetFrame), TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_LOW_POWER);
	}	
}

void ProximitySendStartChange(BOOL up_down) {
	if ((txBuf = CreatePacket()) != NULL) {
		txBuf->ZW_SwitchMultilevelStartLevelChangeFrame.cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL;
		txBuf->ZW_SwitchMultilevelStartLevelChangeFrame.cmd = SWITCH_MULTILEVEL_START_LEVEL_CHANGE;
		txBuf->ZW_SwitchMultilevelStartLevelChangeFrame.level = (((BYTE)up_down) << 6) | (((BYTE)TRUE) << 5);
		txBuf->ZW_SwitchMultilevelStartLevelChangeFrame.startLevel = 0;
		EnqueuePacket(NODE_BROADCAST, sizeof(txBuf->ZW_SwitchMultilevelStartLevelChangeFrame), TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_LOW_POWER);
	}	
}

void ProximitySendStopChange(void) {
	if ((txBuf = CreatePacket()) != NULL) {
		txBuf->ZW_SwitchMultilevelStartLevelChangeFrame.cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL;
		txBuf->ZW_SwitchMultilevelStartLevelChangeFrame.cmd = SWITCH_MULTILEVEL_STOP_LEVEL_CHANGE;
		EnqueuePacket(NODE_BROADCAST, sizeof(txBuf->ZW_SwitchMultilevelStartLevelChangeFrame), TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_LOW_POWER);
	}	
}
#endif

#if WITH_CC_DOOR_LOCK
void sendDoorLockOperationReport(BYTE dstNode) {
	if ((txBuf = CreatePacket()) == NULL)
			return; 
	
	txBuf->ZW_DoorLockOperationReportV2Frame.cmdClass = COMMAND_CLASS_DOOR_LOCK_V2;
	txBuf->ZW_DoorLockOperationReportV2Frame.cmd = DOOR_LOCK_OPERATION_REPORT_V2;
	txBuf->ZW_DoorLockOperationReportV2Frame.doorLockMode = pDoorLockData->mode; 
	txBuf->ZW_DoorLockOperationReportV2Frame.properties1 = pDoorLockData->doorHandleMode;
	//txBuf->ZW_DoorLockOperationReportV2Frame.doorCondition = DOOR_LOCK_CONDITION_BOLT_CLOSED_MASK;
	txBuf->ZW_DoorLockOperationReportV2Frame.doorCondition = pDoorLockData->condition;
	txBuf->ZW_DoorLockOperationReportV2Frame.lockTimeoutMinutes = pDoorLockData->lockTimeoutMin;
	txBuf->ZW_DoorLockOperationReportV2Frame.lockTimeoutSeconds = pDoorLockData->lockTimeoutSec;
		
	EnqueuePacket(dstNode, sizeof(txBuf->ZW_DoorLockOperationReportV2Frame), TRANSMIT_OPTION_USE_DEFAULT);
}


#if WITH_CC_ASSOCIATION
void sendDoorLockOperationReportToGroup(BYTE dstGroup) {
	if ((txBuf = CreatePacketToGroup()) == NULL)
			return; 
	
	txBuf->ZW_DoorLockOperationReportV2Frame.cmdClass = COMMAND_CLASS_DOOR_LOCK_V2;
	txBuf->ZW_DoorLockOperationReportV2Frame.cmd = DOOR_LOCK_OPERATION_REPORT_V2;
	txBuf->ZW_DoorLockOperationReportV2Frame.doorLockMode = pDoorLockData->mode; 
	txBuf->ZW_DoorLockOperationReportV2Frame.properties1 = pDoorLockData->doorHandleMode;	
	//txBuf->ZW_DoorLockOperationReportV2Frame.doorCondition = DOOR_LOCK_CONDITION_BOLT_CLOSED_MASK;
  txBuf->ZW_DoorLockOperationReportV2Frame.doorCondition = pDoorLockData->condition;
	txBuf->ZW_DoorLockOperationReportV2Frame.lockTimeoutMinutes = pDoorLockData->lockTimeoutMin;
	txBuf->ZW_DoorLockOperationReportV2Frame.lockTimeoutSeconds = pDoorLockData->lockTimeoutSec;
		
	EnqueuePacketToGroup(dstGroup, sizeof(txBuf->ZW_DoorLockOperationReportV2Frame));
}
#endif
#endif

#if WITH_CC_DOOR_LOCK_LOGGING
void createDoorLockLoggingReport(BYTE lock_event) {
	CMD_DOOR_LOCK_LOGGING_DATA dummyStruc;
	CMD_DOOR_LOCK_LOGGING_DATA * pointer_dummyStruc = &dummyStruc;


	VAR(DOOR_LOCK_LOGGING_BUF_INDEX)++;
	if (VAR(DOOR_LOCK_LOGGING_LAST_REPORT) < DOOR_LOCK_LOGGING_RECORDS_MAX_SUPPORTED_NUMBER) {
		VAR(DOOR_LOCK_LOGGING_LAST_REPORT)++;
		SAVE_EEPROM_BYTE(DOOR_LOCK_LOGGING_LAST_REPORT);
	} else {
		if (VAR(DOOR_LOCK_LOGGING_BUF_INDEX) == DOOR_LOCK_LOGGING_RECORDS_MAX_SUPPORTED_NUMBER) {
			VAR(DOOR_LOCK_LOGGING_BUF_INDEX) = FALSE;
		}
	}
	SAVE_EEPROM_BYTE(DOOR_LOCK_LOGGING_BUF_INDEX);
	

	// TODO We need to call here function to talk to RTC and get time back!
	//DOOR_LOCK_LOGGING_DATE_AND_TIME_RETURN(pointer_dummyStruc);
	dummyStruc.masked_legal_and_hours = (dummyStruc.masked_legal_and_hours & RECORD_REPORT_PROPERTIES1_HOUR_LOCAL_TIME_MASK) | DOOR_LOCK_LOGGING_VALID_DATA_MASK;
	dummyStruc.event = lock_event;
	dummyStruc.user = DEFAULT_USER_IDENTIFIER;
	dummyStruc.code_length = DOOR_LOCK_LOGGING_DEFAULT_CODE_LENGTH;
	dummyStruc.user_code = 0x00000042;

	EEPROM_WRITE_ARRAY(((WORD)&(EEOFFSET_DOOR_LOCK_LOGGING_DATA_far[VAR(DOOR_LOCK_LOGGING_BUF_INDEX)])),dummyStruc,sizeof(CMD_DOOR_LOCK_LOGGING_DATA));
	if (myNodeId) {
		sendDoorLockLoggingReportToGroup(0,ASSOCIATION_REPORTS_GROUP_NUM);
	}
}

void sendDoorLockLoggingReport(BYTE number,BYTE DstNode) {
	if ((txBuf = CreatePacket()) == NULL)
			return;

	if (number > VAR(DOOR_LOCK_LOGGING_LAST_REPORT))
			return;

	{
		BYTE dataPointer = 0;
		if (number == FALSE) {
			number = VAR(DOOR_LOCK_LOGGING_LAST_REPORT);
		}
		if (VAR(DOOR_LOCK_LOGGING_LAST_REPORT) == DOOR_LOCK_LOGGING_RECORDS_MAX_SUPPORTED_NUMBER) {
			if ((number+VAR(DOOR_LOCK_LOGGING_BUF_INDEX)) >= DOOR_LOCK_LOGGING_RECORDS_MAX_SUPPORTED_NUMBER) {
				dataPointer += (number + VAR(DOOR_LOCK_LOGGING_BUF_INDEX) - DOOR_LOCK_LOGGING_RECORDS_MAX_SUPPORTED_NUMBER);
			} else {
				dataPointer += (number + VAR(DOOR_LOCK_LOGGING_BUF_INDEX));
			}
		} else {
			dataPointer += (number-1);
		}
		txBuf->ZW_RecordReport4byteFrame.cmdClass = COMMAND_CLASS_DOOR_LOCK_LOGGING;
		txBuf->ZW_RecordReport4byteFrame.cmd = RECORD_REPORT;
		txBuf->ZW_RecordReport4byteFrame.recordNumber = number;
		EEPROM_READ_ARRAY(((WORD)&(EEOFFSET_DOOR_LOCK_LOGGING_DATA_far[dataPointer])),&(txBuf->ZW_RecordReport4byteFrame.year1),(WORD)sizeof(CMD_DOOR_LOCK_LOGGING_DATA));
			
		EnqueuePacket(DstNode, sizeof(txBuf->ZW_RecordReport4byteFrame), TRANSMIT_OPTION_USE_DEFAULT);
	}  
}

#if WITH_CC_ASSOCIATION
void sendDoorLockLoggingReportToGroup(BYTE number,BYTE dstGroup) {
	if ((txBuf = CreatePacketToGroup()) == NULL)
			return;

	if (number > VAR(DOOR_LOCK_LOGGING_LAST_REPORT))
			return;

	{
		BYTE dataPointer = 0;
		if (number == FALSE) {
			number = VAR(DOOR_LOCK_LOGGING_LAST_REPORT);
		}
		if (VAR(DOOR_LOCK_LOGGING_LAST_REPORT) == DOOR_LOCK_LOGGING_RECORDS_MAX_SUPPORTED_NUMBER) {
			if ((number+VAR(DOOR_LOCK_LOGGING_BUF_INDEX)) >= DOOR_LOCK_LOGGING_RECORDS_MAX_SUPPORTED_NUMBER) {
				dataPointer += (number + VAR(DOOR_LOCK_LOGGING_BUF_INDEX) - DOOR_LOCK_LOGGING_RECORDS_MAX_SUPPORTED_NUMBER);
			} else {
				dataPointer += (number + VAR(DOOR_LOCK_LOGGING_BUF_INDEX));
			}
		} else {
			dataPointer += (number-1);
		}
		txBuf->ZW_RecordReport4byteFrame.cmdClass = COMMAND_CLASS_DOOR_LOCK_LOGGING;
		txBuf->ZW_RecordReport4byteFrame.cmd = RECORD_REPORT;
		txBuf->ZW_RecordReport4byteFrame.recordNumber = number;
		EEPROM_READ_ARRAY(((WORD)&(EEOFFSET_DOOR_LOCK_LOGGING_DATA_far[dataPointer])),&(txBuf->ZW_RecordReport4byteFrame.year1),(WORD)sizeof(CMD_DOOR_LOCK_LOGGING_DATA));
			
		EnqueuePacketToGroup(dstGroup, sizeof(txBuf->ZW_RecordReport4byteFrame));
	}
}
#endif
#endif

#if WITH_PENDING_BATTERY
BYTE * pendingBatteryValue() { return &(NZ(pendingBatteryVal));}
BYTE * pendingBatteryCount() { return &(NZ(pendingBatteryCount));}

#endif

#if ERASE_NVM

#warning "ERASE NVM ENABLED!"

#if ERASE_NVM_ZERO
	#define EE_ERASE_CONTENT		0x00
#else
	#define EE_ERASE_CONTENT		0xff
#endif

/*
BOOL CheckNVM()
{
	WORD i = NVM_CHECK_SIZE;
	do {
		if(ZW_MEM_GET_BYTE(i) != EE_ERASE_CONTENT)
			return FALSE;
	} while (i-- != 0);

	return TRUE;
}*/
#define IOBUFF_SIZE 128
BYTE wr_buffer[IOBUFF_SIZE];
	
BOOL checkNVMValue(DWORD addr,  BYTE val)
{
	BYTE test_ok = TRUE;
	WORD i;

    NVM_ext_read_long_buffer(addr, wr_buffer, IOBUFF_SIZE);
    i = IOBUFF_SIZE;
    while(i)
    {
        i--;
        if(wr_buffer[i] != val)
        {
           test_ok = FALSE;
       	   break;
        }
    }	
            
    return test_ok;

}
void writeNVMValue(DWORD addr,  BYTE val)
{
	BYTE test_ok = TRUE;
	
	memset(wr_buffer, val, IOBUFF_SIZE);
    NVM_ext_write_long_buffer(addr, wr_buffer, IOBUFF_SIZE);
            
}          
void EraseNVMemory()
{ 	
       
     
    WORD    sizekb;
    DWORD   size;
    DWORD   index;

	WORD i = 0xffff;
	
	#ifdef NVM_CHECK_LEDpin
	BYTE pin_state = 0;
	WORD duration  = 0xAfff;
	#endif

	PIN_OUT(NVM_CHECK_LEDpin);
	PIN_ON(NVM_CHECK_LEDpin);

	
	#if !NVM_CHECK_ONLY
	{

        ZW_NVRGetValue(offsetof(NVR_FLASH_STRUCT, bNVMSize), 2, &sizekb);
        size  = ((DWORD)sizekb) << 10;
        index = ((DWORD)size)/IOBUFF_SIZE;
          

        while(index)
        {
            index--;
            writeNVMValue(((DWORD)index)*IOBUFF_SIZE, index%131);

        }

        index = ((DWORD)size)/IOBUFF_SIZE;
        while(index)
        {
            index--;
            if(!checkNVMValue(((DWORD)index)*IOBUFF_SIZE, index%131))
            {
            	duration = 0x8ff;	
            	break;
            }	

        }

        index = ((DWORD)size)/IOBUFF_SIZE;
        while(index)
        {
            index--;
            writeNVMValue(((DWORD)index)*IOBUFF_SIZE, 0xFF);
        }

	}
	
	#endif
	
	


	while(1)
	{
		#ifdef NVM_CHECK_LEDpin
		if((i % duration) == 0)
			pin_state = !pin_state;
		if(pin_state)
			PIN_ON(NVM_CHECK_LEDpin)
		else
			PIN_OFF(NVM_CHECK_LEDpin)
		#endif
		i++;
	}
}

#endif


BYTE ApplicationInitSW(void) {
	#if WITH_CC_WAKE_UP
	BOOL can_sleep = FALSE;
	#endif
	
	#if WITH_CC_SECURITY
	p_nodeInfoCCs_size = &nodeInfoCCsOutsideSecurity_size;
	#else
	p_nodeInfoCCs_size = &nodeInfoCCsNormal_size;
	#endif

	#if ERASE_NVM
	EraseNVMemory();
	#endif
	
	#if WITH_CC_WAKE_UP

	#if defined(CUSTOM_HALFSLEEP_HANDLER)
	can_sleep = TRUE;
	if (CUSTOM_HALFSLEEP_HANDLER())  // Если обработчик отработал нормально и нет нужды просыпаться...
		GoSleep();					 // спим дальше	
	else
		can_sleep = FALSE;
	#endif
		
	if ((wakeupReason == ZW_WAKEUP_WUT ) && (NZ(wakeupCounterMagic) == WAKE_UP_MAGIC_VALUE)) {
		// Updates the Wakeup count counter in SRAM.
		#if FIRMWARE_UPGRADE_BATTERY_SAVING
		if (NZ(wakeupFWUpdate))
		{
			can_sleep = FALSE;
		}
		else
		#endif
		
		if (NZ(wakeupCount) && --NZ(wakeupCount)) { // just in case Magic is accidentely correct and wakeupCount is 0 (not to go in a very deep sleep)
			#if BETWEEN_WUT_WAKEUP_ACTION_PRESENT
			BetweenWUTWakeupAction();
			#endif

			#if defined(CUSTOM_HALFSLEEP_HANDLER)
			if(can_sleep)
			#endif	
			{
				GoSleep();
				can_sleep = TRUE; //return TRUE; // don't read EEPROM to save time
			}
		}
	}
		
	if(can_sleep)
			return TRUE;

	// We are waking up due to interval reached or it is first wakeup after boot and SRAM is not initialized
	NZ(wakeupCounterMagic) = WAKE_UP_MAGIC_VALUE;
	#endif


	MemoryGetID(NULL, &myNodeId);
	
	// Get stored values
	if (ZME_GET_BYTE(EE(MAGIC)) == MAGIC_VALUE) {
		#if WITH_CC_ASSOCIATION
		AssociationInit();
		#endif
		#if WITH_CC_SCENE_CONTROLLER_CONF
		ScenesInit();
		#endif

		ZME_GET_BUFFER(EEOFFSET_START, eeParams, EEOFFSET_DELIMITER);

		#if WITH_CC_SCENE_ACTUATOR_CONF
	        scenesAct[0] =  ZME_GET_BYTE(EE(SCENE_ACTUATOR_CONF_LEVEL));
	        ZME_GET_BUFFER(EE(SCENE_ACTUATOR_CONF_LEVEL) + 1, &scenesAct[1], SCENE_ACTUATOR_EE_SIZE-1);
		#endif
	} else {
		RestoreToDefaultNonExcludableValues();
		RestoreToDefault(); // Reset configuration to default
	}

	#if WITH_CC_WAKE_UP
		NZ(wakeupEnabled) 	= (VAR(ENABLE_WAKEUPS) == ENABLE_WAKEUPS_YES);
	#endif
		
	#if WITH_TRIAC
	#ifdef TRIAC_ENABLE
	if(TRIAC_ENABLE)
	#endif
	{
		TRIAC_Init();
	}
	#endif
	
	
	#if WITH_NWI
	// Set Network wide inclusion active if we dont have a node ID
	bNWIStartup = (myNodeId == 0);
	#endif

	if (myNodeId == 0) {
		// If with WakeUp and not included, the device should go int deep sleep instead of WUT sleep. So the only wakeup reason here can be ZW_WAKEUP_RESET.
		LEDSet(LED_FAILED); // show that we are not in network
	}
	
	
	#if WITH_FSM
		ZMEFSM_Init();
	#endif
	

	#if WITH_CC_FIRMWARE_UPGRADE
		FWUpdate_Init();
	#endif	

	#if defined(INIT_CUSTOM_SW)
	 	INIT_CUSTOM_SW();
	#endif

	#if WITH_CC_CENTRAL_SCENE
	 	CentralSceneInit();
	#endif

	#if WITH_CC_POWERLEVEL
	PowerLevelInit();
	#endif

	#if WITH_CC_DOOR_LOCK
	VAR(DOOR_LOCK_TIMEOUT_FLAG) = 0;
	#endif


	#if WITH_CC_BATTERY && defined(ZW050x)
		#if PRODUCT_DYNAMIC_TYPE
		if (DYNAMIC_TYPE_CURRENT_VALUE != DYNAMIC_MODE_ALWAYS_LISTENING) {
		#endif
		 	if(wakeupReason == ZW_WAKEUP_POR && myNodeId != 0) {
				if (VAR(BATTERY_REPORT_ON_POR_SENT)) {
					// this is to send only on every second POR boot not to kill battery in a loop
					VAR(BATTERY_REPORT_ON_POR_SENT) = FALSE;
				} else {
					#if WITH_CC_ASSOCIATION
						#ifdef ASSOCIATION_REPORTS_GROUP_NUM
						SendReport(ASSOCIATION_REPORTS_GROUP_NUM, COMMAND_CLASS_BATTERY, 0xff);
						#endif
					#else	
						SendReportToNode(NODE_BROADCAST, COMMAND_CLASS_BATTERY, 0xff);
					#endif
					VAR(BATTERY_REPORT_ON_POR_SENT) = TRUE;
				}
				SAVE_EEPROM_BYTE(BATTERY_REPORT_ON_POR_SENT);
			}
		#if PRODUCT_DYNAMIC_TYPE
		}
		#endif
	#endif

	#if WITH_CC_SECURITY
		SecurityInit();
	#endif

	#ifdef ZW_CONTROLLER
		controllerUpdateState();
	#endif	

	#if PRODUCT_FLIRS_TYPE && defined(ZW050x)
		if((wakeupReason == ZW_WAKEUP_SENSOR) && (myNodeId != 0)) {
			ShortKickSleep();
		}
	#endif

	return TRUE;
}

void ApplicationPoll(void) {
	
	g_one_poll = TRUE;
	#if WITH_CC_WAKE_UP
	if (wakeupReason == ZW_WAKEUP_WUT) {
		wakeupReason = NOT_WAKEUP_REASON; // to run this code only once after wakeup by WUT

		// If the counter reaches zero, a wakeup information frame is sent.
		//// This was already checked in InitSW, so wakeupCount is 0 here
		SendWakeupNotification();
		SetWakeupPeriod();
		#if WITH_CC_FIRMWARE_UPGRADE
		#if FIRMWARE_UPGRADE_BATTERY_SAVING    
		fwuWakeup();
		NZ(wakeupFWUpdate)	= isFWUInProgress();
		#endif
		#endif
		return; // Let's now give control to the lib to send the Wakeup Notification
	}
	#endif

	
	#if FIRST_RUN_ACTION_PRESENT
	if (firstRun) {
		firstRun = FALSE;
		FirstRunAction();
	}
	#endif
	#if (!PRODUCT_LISTENING_TYPE || PRODUCT_DYNAMIC_TYPE) && FIRST_RUN_BATTERY_INSERT_ACTION_PRESENT
		#if PRODUCT_DYNAMIC_TYPE
		if (DYNAMIC_TYPE_CURRENT_VALUE != DYNAMIC_MODE_ALWAYS_LISTENING) {
		#endif
		if (NZ(firstRunBatteryInsert) != NON_ZERO_MAGIC_VALUE) {
			NZ(firstRunBatteryInsert) = NON_ZERO_MAGIC_VALUE;			
			#if WITH_CC_WAKE_UP
			// Устанавливаем правильный WakeUp-период после замены батареек
			SetWakeupPeriod();
			#endif

			#if FIRST_RUN_BATTERY_INSERT_ACTION_PRESENT
			FirstRunBatteryInsertAction();
			#endif
		}
		#if PRODUCT_DYNAMIC_TYPE
		}
		#endif
	#endif

	#if WITH_TRIAC
	TRIAC_10MSTimer();	
	#endif

	ApplicationPollFunction();
	
	#if NUM_BUTTONS > 0
	ClearButtonEvents();
	#endif

	SendQueue();

	#if !PRODUCT_LISTENING_TYPE  || PRODUCT_DYNAMIC_TYPE
	#if PRODUCT_DYNAMIC_TYPE
	if (DYNAMIC_TYPE_CURRENT_VALUE != DYNAMIC_MODE_ALWAYS_LISTENING) {
	#endif
		// Sleep is allowed only if we are idle not sending a packet
		if (
			mode == MODE_IDLE && // we are not doing something
			LED_IS_OFF() && // LED is not blinking/lighting
			#if FSM_WAIT_BEFORE_SLEEP
			!ZMEFSM_HaveActiveFSM() && // Нет активных КА
			#endif
			!sleepLocked && // application does not want us to wait for something
			!queueSending && // no packet is sending
			#if NUM_BUTTONS > 0
			ButtonsAreIdle() && // buttons are not waiting for events
			#endif
			#if WITH_CC_PROTECTION
			(VAR(PROTECTION) != PROTECTION_SET_PROTECTION_BY_SEQUENCE || !protectedUnlocked) &&
			#endif
			#if CUSTOM_SLEEP_LOCK
			CustomSleepLock() &&
			#endif
			!hardBlock &&
			1 
			) {
				GoSleep();
				// After leaving this function we will not get back in ApplicationPoll untill next wake up
		}
		
		#if PRODUCT_FLIRS_TYPE==0
		// not sleeping, but may be turn off RX?
		if (
			mode == MODE_IDLE && // we are not doing something
			#if FSM_WAIT_BEFORE_SLEEP
			!ZMEFSM_HaveActiveFSM() && // Нет активных КА
			#endif
			!sleepLocked && // application does not want us to wait for something - not waiting for a packet
			!queueSending && // no packet is sending
			1
			) {
				(void) ZW_SetRFReceiveMode(FALSE);
		}
		#endif
	#if PRODUCT_DYNAMIC_TYPE
	}
	#endif
	#endif

	#if WITH_NWI
	if (bNWIStartup) {
		StartLearnModeNow(ZW_SET_LEARN_MODE_NWI);
		mode = MODE_LEARN;
		LEDSet(LED_NWI);
		bNWIStartup = FALSE;
	}
	#endif
}

#if WITH_CC_MULTICHANNEL || WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
void MultichannelBegin()
{
	MULTICHANNEL_ENCAP_ENABLE;
	
}
void MultichannelSet(BYTE src, BYTE dst)
//SOURCE is Incoming to the commandhandler from controller , so it's the destinations endpoint *MAD SMILE*
//DESTINATION is Device's endpoint, so it's the source endpoint *MAD SMILE*
{
	MULTICHANNEL_SRC_END_POINT = src;
	MULTICHANNEL_DST_END_POINT = dst;
}
void MultichannelEnd()
{
	MULTICHANNEL_ENCAP_DISABLE;
}
#endif

void MultichannelCommandHandler(BYTE sourceNode, ZW_APPLICATION_TX_BUFFER *pCmd, BYTE cmdLength) reentrant {

	BYTE classcmd 	= 	pCmd->ZW_Common.cmdClass;
	BYTE cmd 		= 	pCmd->ZW_Common.cmd;
	
	BYTE param1 = *((BYTE*)pCmd + OFFSET_PARAM_1);
	BYTE param2 = *((BYTE*)pCmd + OFFSET_PARAM_2);
	BYTE param3 = *((BYTE*)pCmd + OFFSET_PARAM_3);
	BYTE param4 = *((BYTE*)pCmd + OFFSET_PARAM_4);

	MULTICHANNEL_ENCAP_ENABLE;
	
	switch(classcmd) {

		#if WITH_CC_BASIC
		case COMMAND_CLASS_BASIC:
			switch(cmd) {
				case BASIC_SET:
					BASIC_SET_ACTION(param1);
					break;

				case BASIC_GET:
					SendReportToNode(sourceNode, COMMAND_CLASS_BASIC, BASIC_REPORT_VALUE);
					break;
			}
			break;
		#endif

		#if WITH_CC_SENSOR_BINARY
			case  COMMAND_CLASS_SENSOR_BINARY:
			 	if(cmd == SENSOR_BINARY_GET)
					sendBinarySensorReport(sourceNode,
											SENSOR_BINARY_REPORT_TYPE_DEFAULT);
			break; 
		#endif
				
		#if WITH_CC_SENSOR_BINARY_V2
			case COMMAND_CLASS_SENSOR_BINARY_V2:
				switch(cmd) {
					
					case SENSOR_BINARY_SUPPORTED_GET_SENSOR_V2:
						if ((txBuf = CreatePacket()) != NULL) {
							BYTE sz;
							txBuf->ZW_SensorBinarySupportedSensorReport1byteV2Frame.cmdClass = COMMAND_CLASS_SENSOR_BINARY_V2;
							txBuf->ZW_SensorBinarySupportedSensorReport1byteV2Frame.cmd = SENSOR_BINARY_SUPPORTED_SENSOR_REPORT_V2;
							
							#if CC_SENSOR_BINARY_DYNAMIC_TYPES
								sz = sizeof(txBuf->ZW_SensorBinarySupportedSensorReport1byteV2Frame) - 1  
								+ CC_SENSOR_BINARY_DYNAMIC_TYPE_GETTER(&(txBuf->ZW_SensorBinarySupportedSensorReport1byteV2Frame.bitMask1));
							#else
								txBuf->ZW_SensorBinarySupportedSensorReport1byteV2Frame.bitMask1 =
																	#if CC_SENSOR_BINARY_V2_WITH_GENERAL_PURPOSE
																	(1<<SENSOR_BINARY_REPORT_TYPE_GENERAL_PURPOSE) |
																	#endif
																	#if CC_SENSOR_BINARY_V2_WITH_SMOKE
																	(1<<SENSOR_BINARY_REPORT_TYPE_SMOKE) |
																	#endif
																	#if CC_SENSOR_BINARY_V2_WITH_CO
																	(1<<SENSOR_BINARY_REPORT_TYPE_CO) |
																	#endif
																	#if CC_SENSOR_BINARY_V2_WITH_CO2
																	(1<<SENSOR_BINARY_REPORT_TYPE_CO2) |
																	#endif
																	#if CC_SENSOR_BINARY_V2_WITH_HEAT
																	(1<<SENSOR_BINARY_REPORT_TYPE_HEAT) |
																	#endif
																	#if CC_SENSOR_BINARY_V2_WITH_WATER
																	(1<<SENSOR_BINARY_REPORT_TYPE_WATER) |
																	#endif
																	#if CC_SENSOR_BINARY_V2_WITH_FREEZE
																	(1<<SENSOR_BINARY_REPORT_TYPE_FREEZE) |
																	#endif
																	0;
								sz = sizeof(
									#if CC_SENSOR_BINARY_V2_WITH_TAMPER || \
									CC_SENSOR_BINARY_V2_WITH_AUX || \
									CC_SENSOR_BINARY_V2_WITH_DOOR_WINDOW || \
									CC_SENSOR_BINARY_V2_WITH_TILT || \
									CC_SENSOR_BINARY_V2_WITH_MOTION || \
									CC_SENSOR_BINARY_V2_WITH_GLASSBREAK || \
									0
									txBuf->ZW_SensorBinarySupportedSensorReport2byteV2Frame.bitMask2 =
																		#if CC_SENSOR_BINARY_V2_WITH_TAMPER
																		(1<<(SENSOR_BINARY_REPORT_TYPE_TAMPER - 8)) |
																		#endif
																		#if CC_SENSOR_BINARY_V2_WITH_AUX
																		(1<<(SENSOR_BINARY_REPORT_TYPE_AUX - 8)) |
																		#endif
																		#if CC_SENSOR_BINARY_V2_WITH_DOOR_WINDOW
																		(1<<(SENSOR_BINARY_REPORT_TYPE_DOOR_WINDOW - 8)) |
																		#endif
																		#if CC_SENSOR_BINARY_V2_WITH_TILT
																		(1<<(SENSOR_BINARY_REPORT_TYPE_TILT - 8)) |
																		#endif
																		#if CC_SENSOR_BINARY_V2_WITH_MOTION
																		(1<<(SENSOR_BINARY_REPORT_TYPE_MOTION - 8)) |
																		#endif
																		#if CC_SENSOR_BINARY_V2_WITH_GLASSBREAK
																		(1<<(SENSOR_BINARY_REPORT_TYPE_GLASSBREAK - 8)) |
																		#endif
																		0
									txBuf->ZW_SensorBinarySupportedSensorReport2byteV2Frame
									#else
									txBuf->ZW_SensorBinarySupportedSensorReport1byteV2Frame
									#endif
								);
							#endif
							EnqueuePacket(sourceNode, sz, txOption);
						}

					break;

					case SENSOR_BINARY_GET_V2:
						switch(param1) {
							case 0xff:
								sendBinarySensorReport(sourceNode, CC_SENSOR_BINARY_DEFAULT_SENSOR_TYPE);
								break;
							
							#if CC_SENSOR_BINARY_V2_WITH_GENERAL_PURPOSE
							SENSOR_BINARY_V2_TYPE_ENABLED(SENSOR_BINARY_REPORT_TYPE_GENERAL_PURPOSE)
							#endif
							
							#if CC_SENSOR_BINARY_V2_WITH_SMOKE
							SENSOR_BINARY_V2_TYPE_ENABLED(SENSOR_BINARY_REPORT_TYPE_SMOKE)
							#endif
							
							#if CC_SENSOR_BINARY_V2_WITH_CO
							SENSOR_BINARY_V2_TYPE_ENABLED(SENSOR_BINARY_REPORT_TYPE_CO)
							#endif
							
							#if CC_SENSOR_BINARY_V2_WITH_CO2
							SENSOR_BINARY_V2_TYPE_ENABLED(SENSOR_BINARY_REPORT_TYPE_CO2)
							#endif
							
							#if CC_SENSOR_BINARY_V2_WITH_HEAT
							SENSOR_BINARY_V2_TYPE_ENABLED(SENSOR_BINARY_REPORT_TYPE_HEAT)
							#endif
							
							#if CC_SENSOR_BINARY_V2_WITH_WATER
							SENSOR_BINARY_V2_TYPE_ENABLED(SENSOR_BINARY_REPORT_TYPE_WATER)
							#endif
							
							#if CC_SENSOR_BINARY_V2_WITH_FREEZE
							SENSOR_BINARY_V2_TYPE_ENABLED(SENSOR_BINARY_REPORT_TYPE_FREEZE)
							#endif
							
							#if CC_SENSOR_BINARY_V2_WITH_TAMPER
							SENSOR_BINARY_V2_TYPE_ENABLED(SENSOR_BINARY_REPORT_TYPE_TAMPER)
							#endif
							
							#if CC_SENSOR_BINARY_V2_WITH_AUX
							SENSOR_BINARY_V2_TYPE_ENABLED(SENSOR_BINARY_REPORT_TYPE_AUX)
							#endif
							
							#if CC_SENSOR_BINARY_V2_WITH_DOOR_WINDOW
							SENSOR_BINARY_V2_TYPE_ENABLED(SENSOR_BINARY_REPORT_TYPE_DOOR_WINDOW)
							#endif
							
							#if CC_SENSOR_BINARY_V2_WITH_TILT
							SENSOR_BINARY_V2_TYPE_ENABLED(SENSOR_BINARY_REPORT_TYPE_TILT)
							#endif
							
							#if CC_SENSOR_BINARY_V2_WITH_MOTION
							SENSOR_BINARY_V2_TYPE_ENABLED(SENSOR_BINARY_REPORT_TYPE_MOTION)
							#endif
							
							#if CC_SENSOR_BINARY_V2_WITH_GLASSBREAK
							SENSOR_BINARY_V2_TYPE_ENABLED(SENSOR_BINARY_REPORT_TYPE_GLASSBREAK)
							#endif
							//SENSOR_BINARY_V2_TYPE_VALIDATOR
						}
					break;
				}
			break;	
			
		#endif

		#if WITH_CC_INDICATOR

		 	case COMMAND_CLASS_INDICATOR:
		 		switch(cmd){
		 			case INDICATOR_SET:
		 				INDICATOR_ACTION(param1);
		 				break;
		 			case INDICATOR_GET:
		 				SendReportToNode(sourceNode, COMMAND_CLASS_INDICATOR, INDICATOR_VALUE);
		 				break;

		 		}
		 		break;

		#endif

		#if WITH_CC_SENSOR_MULTILEVEL_V4
		case COMMAND_CLASS_SENSOR_MULTILEVEL:
			if (cmd == SENSOR_MULTILEVEL_GET) {
				if ((txBuf = CreatePacket()) != NULL) {
					txBuf->ZW_SensorMultilevelReport2byteFrame.cmdClass = COMMAND_CLASS_SENSOR_MULTILEVEL;
					txBuf->ZW_SensorMultilevelReport2byteFrame.cmd = SENSOR_MULTILEVEL_REPORT;
					txBuf->ZW_SensorMultilevelReport2byteFrame.sensorType = CC_SENSOR_MULTILEVEL_V4_SENSOR_TYPE;
					txBuf->ZW_SensorMultilevelReport2byteFrame.level =
							(CC_SENSOR_MULTILEVEL_V4_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK) | \
							((CC_SENSOR_MULTILEVEL_V4_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
							((CC_SENSOR_MULTILEVEL_V4_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
					*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_AIR_TEMPERATURE_GETTER;
					EnqueuePacket(sourceNode, sizeof(txBuf->ZW_SensorMultilevelReport2byteFrame), txOption);
				}
			}
			break;
		#endif

		#if WITH_CC_SENSOR_MULTILEVEL
		case COMMAND_CLASS_SENSOR_MULTILEVEL:
			switch (cmd) {
				case SENSOR_MULTILEVEL_SUPPORTED_GET_SENSOR_V5:
					if ((txBuf = CreatePacket()) != NULL) {
						BYTE sz;
						txBuf->ZW_SensorMultilevelSupportedSensorReport1byteV5Frame.cmdClass = COMMAND_CLASS_SENSOR_MULTILEVEL;
						txBuf->ZW_SensorMultilevelSupportedSensorReport1byteV5Frame.cmd = SENSOR_MULTILEVEL_SUPPORTED_SENSOR_REPORT_V5;
						#if CC_SENSOR_MULTILEVEL_DYNAMIC_TYPES
							sz = sizeof(txBuf->ZW_SensorMultilevelSupportedSensorReport1byteV5Frame) - 1  
								+ CC_SENSOR_MULTILEVEL_DYNAMIC_TYPE_GETTER(&(txBuf->ZW_SensorMultilevelSupportedSensorReport1byteV5Frame.bitMask1));
						#else
						txBuf->ZW_SensorMultilevelSupportedSensorReport1byteV5Frame.bitMask1 = // always present
										#if CC_SENSOR_MULTILEVEL_WITH_AIR_TEMPERATURE
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_AIR_TEMPERATURE - 0)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_GENERAL_PURPOSE
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_GENERAL_PURPOSE - 0)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_LUMINANCE
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_LUMINANCE - 0)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_POWER
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_POWER - 0)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_HUMIDITY
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_HUMIDITY - 0)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_VELOCITY
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_VELOCITY - 0)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_DIRECTION
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_DIRECTION - 0)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_ATHMOSPHERIC_PRESSURE
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_ATHMOSPHERIC_PRESSURE - 0)) | 
										#endif
										0;
						#if \
							CC_SENSOR_MULTILEVEL_WITH_BAROMETRIC_PRESSURE || \
							CC_SENSOR_MULTILEVEL_WITH_SOLAR_RADIATION || \
							CC_SENSOR_MULTILEVEL_WITH_DEW_POINT || \
							CC_SENSOR_MULTILEVEL_WITH_RAIN_RATE || \
							CC_SENSOR_MULTILEVEL_WITH_TIDE_LEVEL || \
							CC_SENSOR_MULTILEVEL_WITH_WEIGHT || \
							CC_SENSOR_MULTILEVEL_WITH_VOLTAGE || \
							CC_SENSOR_MULTILEVEL_WITH_CURRENT || \
							CC_SENSOR_MULTILEVEL_WITH_CO2_LEVEL || \
							CC_SENSOR_MULTILEVEL_WITH_AIR_FLOW || \
							CC_SENSOR_MULTILEVEL_WITH_TANK_CAPACITY || \
							CC_SENSOR_MULTILEVEL_WITH_DISTANCE || \
							CC_SENSOR_MULTILEVEL_WITH_ANGLE_POSITION || \
							CC_SENSOR_MULTILEVEL_WITH_ROTATION || \
							CC_SENSOR_MULTILEVEL_WITH_WATER_TEMPERATURE || \
							CC_SENSOR_MULTILEVEL_WITH_SOIL_TEMPERATURE || \
							CC_SENSOR_MULTILEVEL_WITH_SEISMIC_INTENSITY || \
							CC_SENSOR_MULTILEVEL_WITH_SEISMIC_MAGNITUDE || \
							CC_SENSOR_MULTILEVEL_WITH_ULTRAVIOLET || \
							CC_SENSOR_MULTILEVEL_WITH_ELECTRICAL_RESISTIVITY || \
							CC_SENSOR_MULTILEVEL_WITH_ELECTRICAL_CONDUCTIVITY || \
							CC_SENSOR_MULTILEVEL_WITH_LOUDNESS || \
							CC_SENSOR_MULTILEVEL_WITH_MOISTURE || \
							CC_SENSOR_MULTILEVEL_WITH_FREQUENCY || \
							CC_SENSOR_MULTILEVEL_WITH_TIME || \
							CC_SENSOR_MULTILEVEL_WITH_TARGET_TEMPERATURE || \
							0
						txBuf->ZW_SensorMultilevelSupportedSensorReport1byteV5Frame.bitMask2 =
										#if CC_SENSOR_MULTILEVEL_WITH_BAROMETRIC_PRESSURE
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_BAROMETRIC_PRESSURE - 8)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_SOLAR_RADIATION
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_SOLAR_RADIATION - 8)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_DEW_POINT
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_DEW_POINT - 8)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_RAIN_RATE
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_RAIN_RATE - 8)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_TIDE_LEVEL
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_TIDE_LEVEL - 8)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_WEIGHT_
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_WEIGHT - 8)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_VOLTAGE_
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_VOLTAGE - 8)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_CURRENT_
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_CURRENT - 8)) | 
										#endif
										0;
						#if \
							CC_SENSOR_MULTILEVEL_WITH_CO2_LEVEL || \
							CC_SENSOR_MULTILEVEL_WITH_AIR_FLOW || \
							CC_SENSOR_MULTILEVEL_WITH_TANK_CAPACITY || \
							CC_SENSOR_MULTILEVEL_WITH_DISTANCE || \
							CC_SENSOR_MULTILEVEL_WITH_ANGLE_POSITION || \
							CC_SENSOR_MULTILEVEL_WITH_ROTATION || \
							CC_SENSOR_MULTILEVEL_WITH_WATER_TEMPERATURE || \
							CC_SENSOR_MULTILEVEL_WITH_SOIL_TEMPERATURE || \
							CC_SENSOR_MULTILEVEL_WITH_SEISMIC_INTENSITY || \
							CC_SENSOR_MULTILEVEL_WITH_SEISMIC_MAGNITUDE || \
							CC_SENSOR_MULTILEVEL_WITH_ULTRAVIOLET || \
							CC_SENSOR_MULTILEVEL_WITH_ELECTRICAL_RESISTIVITY || \
							CC_SENSOR_MULTILEVEL_WITH_ELECTRICAL_CONDUCTIVITY || \
							CC_SENSOR_MULTILEVEL_WITH_LOUDNESS || \
							CC_SENSOR_MULTILEVEL_WITH_MOISTURE || \
							CC_SENSOR_MULTILEVEL_WITH_FREQUENCY || \
							CC_SENSOR_MULTILEVEL_WITH_TIME || \
							CC_SENSOR_MULTILEVEL_WITH_TARGET_TEMPERATURE || \
							0
						txBuf->ZW_SensorMultilevelSupportedSensorReport1byteV5Frame.bitMask3 =
										#if CC_SENSOR_MULTILEVEL_WITH_CO2_LEVEL
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_CO2_LEVEL - 16)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_AIR_FLOW
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_AIR_FLOW - 16)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_TANK_CAPACITY
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_TANK_CAPACITY - 16)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_DISTANCE_
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_DISTANCE - 16)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_ANGLE_POSITION
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_ANGLE_POSITION - 16)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_ROTATION_
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_ROTATION - 16)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_WATER_TEMPERATURE
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_WATER_TEMPERATURE - 16)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_SOIL_TEMPERATURE
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_SOIL_TEMPERATURE - 16)) | 
										#endif
										0;
						#if \
							CC_SENSOR_MULTILEVEL_WITH_SEISMIC_INTENSITY || \
							CC_SENSOR_MULTILEVEL_WITH_SEISMIC_MAGNITUDE || \
							CC_SENSOR_MULTILEVEL_WITH_ULTRAVIOLET || \
							CC_SENSOR_MULTILEVEL_WITH_ELECTRICAL_RESISTIVITY || \
							CC_SENSOR_MULTILEVEL_WITH_ELECTRICAL_CONDUCTIVITY || \
							CC_SENSOR_MULTILEVEL_WITH_LOUDNESS || \
							CC_SENSOR_MULTILEVEL_WITH_MOISTURE || \
							CC_SENSOR_MULTILEVEL_WITH_FREQUENCY || \
							CC_SENSOR_MULTILEVEL_WITH_TIME || \
							CC_SENSOR_MULTILEVEL_WITH_TARGET_TEMPERATURE || \
							0
						txBuf->ZW_SensorMultilevelSupportedSensorReport1byteV5Frame.bitMask4 =
										#if CC_SENSOR_MULTILEVEL_WITH_SEISMIC_INTENSITY
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_SEISMIC_INTENSITY - 24)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_SEISMIC_MAGNITUDE
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_SEISMIC_MAGNITUDE - 24)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_ULTRAVIOLET_
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_ULTRAVIOLET - 24)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_ELECTRICAL_RESISTIVITY
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_ELECTRICAL_RESISTIVITY - 24)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_ELECTRICAL_CONDUCTIVITY
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_ELECTRICAL_CONDUCTIVITY - 24)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_LOUDNESS_
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_LOUDNESS - 24)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_MOISTURE_
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_MOISTURE - 24)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_FREQUENCY_
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_FREQUENCY - 24)) | 
										#endif
						#if \
							CC_SENSOR_MULTILEVEL_WITH_TIME || \
							CC_SENSOR_MULTILEVEL_WITH_TARGET_TEMPERATURE || \
							0
						txBuf->ZW_SensorMultilevelSupportedSensorReport1byteV5Frame.bitMask5 =
										#if CC_SENSOR_MULTILEVEL_WITH_TIME_
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_TIME - 32)) | 
										#endif
										#if CC_SENSOR_MULTILEVEL_WITH_TARGET_TEMPERATURE
										(1 << (SENSOR_MULTILEVEL_REPORT_BIT_TARGET_TEMPERATURE - 32)) | 
										#endif
										0;
						#endif
						#endif
						#endif
						#endif

							sz = sizeof(
							#if \
								CC_SENSOR_MULTILEVEL_WITH_TIME || \
								CC_SENSOR_MULTILEVEL_WITH_TARGET_TEMPERATURE || \
								0
							txBuf->ZW_SensorMultilevelSupportedSensorReport5byteV5Frame
							#else
							#if \
								CC_SENSOR_MULTILEVEL_WITH_SEISMIC_INTENSITY || \
								CC_SENSOR_MULTILEVEL_WITH_SEISMIC_MAGNITUDE || \
								CC_SENSOR_MULTILEVEL_WITH_ULTRAVIOLET || \
								CC_SENSOR_MULTILEVEL_WITH_ELECTRICAL_RESISTIVITY || \
								CC_SENSOR_MULTILEVEL_WITH_ELECTRICAL_CONDUCTIVITY || \
								CC_SENSOR_MULTILEVEL_WITH_LOUDNESS || \
								CC_SENSOR_MULTILEVEL_WITH_MOISTURE || \
								CC_SENSOR_MULTILEVEL_WITH_FREQUENCY || \
								0
							txBuf->ZW_SensorMultilevelSupportedSensorReport4byteV5Frame
							#else
							#if \
								CC_SENSOR_MULTILEVEL_WITH_CO2_LEVEL || \
								CC_SENSOR_MULTILEVEL_WITH_AIR_FLOW || \
								CC_SENSOR_MULTILEVEL_WITH_TANK_CAPACITY || \
								CC_SENSOR_MULTILEVEL_WITH_DISTANCE || \
								CC_SENSOR_MULTILEVEL_WITH_ANGLE_POSITION || \
								CC_SENSOR_MULTILEVEL_WITH_ROTATION || \
								CC_SENSOR_MULTILEVEL_WITH_WATER_TEMPERATURE || \
								CC_SENSOR_MULTILEVEL_WITH_SOIL_TEMPERATURE || \
								0
							txBuf->ZW_SensorMultilevelSupportedSensorReport3byteV5Frame
							#else
							#if \
								CC_SENSOR_MULTILEVEL_WITH_BAROMETRIC_PRESSURE || \
								CC_SENSOR_MULTILEVEL_WITH_SOLAR_RADIATION || \
								CC_SENSOR_MULTILEVEL_WITH_DEW_POINT || \
								CC_SENSOR_MULTILEVEL_WITH_RAIN_RATE || \
								CC_SENSOR_MULTILEVEL_WITH_TIDE_LEVEL || \
								CC_SENSOR_MULTILEVEL_WITH_WEIGHT || \
								CC_SENSOR_MULTILEVEL_WITH_VOLTAGE || \
								CC_SENSOR_MULTILEVEL_WITH_CURRENT || \
								0
							txBuf->ZW_SensorMultilevelSupportedSensorReport2byteV5Frame
							#else
							txBuf->ZW_SensorMultilevelSupportedSensorReport1byteV5Frame
							#endif
							#endif
							#endif
							#endif
							);
						#endif //	SENSOR_MULTILEVEL_DYNAMIC_TYPES
						EnqueuePacket(sourceNode, sz, txOption);
					}
					break;

				case SENSOR_MULTILEVEL_SUPPORTED_GET_SCALE_V5:
					if ((txBuf = CreatePacket()) != NULL) {
						txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.cmdClass = COMMAND_CLASS_SENSOR_MULTILEVEL;
						txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.cmd = SENSOR_MULTILEVEL_SUPPORTED_SCALE_REPORT_V5;
						txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.sensorType = param1;
						switch (param1) {
							#if CC_SENSOR_MULTILEVEL_WITH_AIR_TEMPERATURE
							case SENSOR_MULTILEVEL_REPORT_TEMPERATURE_VERSION_1_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_AIR_TEMPERATURE_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_GENERAL_PURPOSE
							case SENSOR_MULTILEVEL_REPORT_GENERAL_PURPOSE_VALUE_VERSION_1_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_GENERAL_PURPOSE_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_LUMINANCE
							case SENSOR_MULTILEVEL_REPORT_LUMINANCE_VERSION_1_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_LUMINANCE_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_POWER
							case SENSOR_MULTILEVEL_REPORT_POWER_VERSION_2_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_POWER_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_HUMIDITY
							case SENSOR_MULTILEVEL_REPORT_RELATIVE_HUMIDITY_VERSION_2_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_HUMIDITY_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_VELOCITY
							case SENSOR_MULTILEVEL_REPORT_VELOCITY_VERSION_2_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_VELOCITY_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_DIRECTION
							case SENSOR_MULTILEVEL_REPORT_DIRECTION_VERSION_2_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_DIRECTION_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_ATHMOSPHERIC_PRESSURE
							case SENSOR_MULTILEVEL_REPORT_ATMOSPHERIC_PRESSURE_VERSION_2_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_ATHMOSPHERIC_PRESSURE_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_BAROMETRIC_PRESSURE
							case SENSOR_MULTILEVEL_REPORT_BAROMETRIC_PRESSURE_VERSION_2_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_BAROMETRIC_PRESSURE_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_SOLAR_RADIATION
							case SENSOR_MULTILEVEL_REPORT_SOLAR_RADIATION_VERSION_2_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_SOLAR_RADIATION_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_DEW_POINT
							case SENSOR_MULTILEVEL_REPORT_DEW_POINT_VERSION_2_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_DEW_POINT_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_RAIN_RATE
							case SENSOR_MULTILEVEL_REPORT_RAIN_RATE_VERSION_2_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_RAIN_RATE_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_TIDE_LEVEL
							case SENSOR_MULTILEVEL_REPORT_TIDE_LEVEL_VERSION_2_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_TIDE_LEVEL_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_WEIGHT
							case SENSOR_MULTILEVEL_REPORT_WEIGHT_VERSION_3_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_WEIGHT_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_VOLTAGE
							case SENSOR_MULTILEVEL_REPORT_VOLTAGE_VERSION_3_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_VOLTAGE_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_CURRENT
							case SENSOR_MULTILEVEL_REPORT_CURRENT_VERSION_3_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_CURRENT_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_CO2_LEVEL
							case SENSOR_MULTILEVEL_REPORT_CO2_LEVEL_VERSION_3_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_CO2_LEVEL_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_AIR_FLOW
							case SENSOR_MULTILEVEL_REPORT_AIR_FLOW_VERSION_3_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_AIR_FLOW_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_TANK_CAPACITY
							case SENSOR_MULTILEVEL_REPORT_TANK_CAPACITY_VERSION_3_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_TANK_CAPACITY_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_DISTANCE
							case SENSOR_MULTILEVEL_REPORT_DISTANCE_VERSION_3_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_DISTANCE_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_ANGLE_POSITION
							case SENSOR_MULTILEVEL_REPORT_ANGLE_POSITION_VERSION_4_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_ANGLE_POSITION_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_ROTATION
							case SENSOR_MULTILEVEL_REPORT_ROTATION_V5_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_ROTATION_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_WATER_TEMPERATURE
							case SENSOR_MULTILEVEL_REPORT_WATER_TEMPERATURE_V5_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_WATER_TEMPERATURE_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_SOIL_TEMPERATURE
							case SENSOR_MULTILEVEL_REPORT_SOIL_TEMPERATURE_V5_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_SOIL_TEMPERATURE_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_SEISMIC_INTENSITY
							case SENSOR_MULTILEVEL_REPORT_SEISMIC_INTENSITY_V5_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_SEISMIC_INTENSITY_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_SEISMIC_MAGNITUDE
							case SENSOR_MULTILEVEL_REPORT_SEISMIC_MAGNITUDE_V5_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_SEISMIC_MAGNITUDE_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_ULTRAVIOLET
							case SENSOR_MULTILEVEL_REPORT_ULTRAVIOLET_V5_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_ULTRAVIOLET_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_ELECTRICAL_RESISTIVITY
							case SENSOR_MULTILEVEL_REPORT_ELECTRICAL_RESISTIVITY_V5_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_ELECTRICAL_RESISTIVITY_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_ELECTRICAL_CONDUCTIVITY
							case SENSOR_MULTILEVEL_REPORT_ELECTRICAL_CONDUCTIVITY_V5_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_ELECTRICAL_CONDUCTIVITY_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_LOUDNESS
							case SENSOR_MULTILEVEL_REPORT_LOUDNESS_V5_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_LOUDNESS_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_MOISTURE
							case SENSOR_MULTILEVEL_REPORT_MOISTURE_V5_V5:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_MOISTURE_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_FREQUENCY
							#error Finish V7 variables!
							case SENSOR_MULTILEVEL_REPORT_FREQUENCY_:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_FREQUENCY_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_TIME
							#error Finish V7 variables!
							case SENSOR_MULTILEVEL_REPORT_TIME_:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_TIME_SCALE;
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_TARGET_TEMPERATURE
							#error Finish V7 variables!
							case SENSOR_MULTILEVEL_REPORT_TARGET_TEMPERATURE:
								txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame.properties1 = 1 << CC_SENSOR_MULTILEVEL_TARGET_TEMPERATURE_SCALE;
								break;
							#endif
							default:
								break; // do not send the report
						}
						
						EnqueuePacket(sourceNode, sizeof(txBuf->ZW_SensorMultilevelSupportedScaleReportV5Frame), txOption);
					}
					break;

				case SENSOR_MULTILEVEL_GET:
					if ((txBuf = CreatePacket()) != NULL) {
						txBuf->ZW_SensorMultilevelReport2byteFrame.cmdClass = COMMAND_CLASS_SENSOR_MULTILEVEL;
						txBuf->ZW_SensorMultilevelReport2byteFrame.cmd = SENSOR_MULTILEVEL_REPORT;
						if (cmdLength < 3)
							param1 = CC_SENSOR_MULTILEVEL_DEFAULT_SENSOR_TYPE;
						txBuf->ZW_SensorMultilevelReport2byteFrame.sensorType = param1;
						switch (param1) {
							#if CC_SENSOR_MULTILEVEL_WITH_AIR_TEMPERATURE
							case SENSOR_MULTILEVEL_REPORT_TEMPERATURE_VERSION_1_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_AIR_TEMPERATURE_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_AIR_TEMPERATURE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_AIR_TEMPERATURE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_GENERAL_PURPOSE
							case SENSOR_MULTILEVEL_REPORT_GENERAL_PURPOSE_VALUE_VERSION_1_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_GENERAL_PURPOSE_GETTER;								
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_GENERAL_PURPOSE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_GENERAL_PURPOSE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_LUMINANCE
							case SENSOR_MULTILEVEL_REPORT_LUMINANCE_VERSION_1_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_LUMINANCE_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_LUMINANCE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_LUMINANCE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_POWER
							case SENSOR_MULTILEVEL_REPORT_POWER_VERSION_2_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_POWER_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_POWER_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_POWER_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_HUMIDITY
							case SENSOR_MULTILEVEL_REPORT_RELATIVE_HUMIDITY_VERSION_2_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_HUMIDITY_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_HUMIDITY_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_HUMIDITY_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_VELOCITY
							case SENSOR_MULTILEVEL_REPORT_VELOCITY_VERSION_2_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_VELOCITY_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_VELOCITY_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_VELOCITY_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_DIRECTION
							case SENSOR_MULTILEVEL_REPORT_DIRECTION_VERSION_2_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_DIRECTION_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_DIRECTION_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_DIRECTION_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_ATHMOSPHERIC_PRESSURE
							case SENSOR_MULTILEVEL_REPORT_ATMOSPHERIC_PRESSURE_VERSION_2_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_ATHMOSPHERIC_PRESSURE_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_ATHMOSPHERIC_PRESSURE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_ATHMOSPHERIC_PRESSURE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_BAROMETRIC_PRESSURE
							case SENSOR_MULTILEVEL_REPORT_BAROMETRIC_PRESSURE_VERSION_2_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_BAROMETRIC_PRESSURE_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_BAROMETRIC_PRESSURE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_BAROMETRIC_PRESSURE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_SOLAR_RADIATION
							case SENSOR_MULTILEVEL_REPORT_SOLAR_RADIATION_VERSION_2_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_SOLAR_RADIATION_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_SOLAR_RADIATION_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_SOLAR_RADIATION_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_DEW_POINT
							case SENSOR_MULTILEVEL_REPORT_DEW_POINT_VERSION_2_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_DEW_POINT_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_DEW_POINT_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_DEW_POINT_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_RAIN_RATE
							case SENSOR_MULTILEVEL_REPORT_RAIN_RATE_VERSION_2_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_RAIN_RATE_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_RAIN_RATE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_RAIN_RATE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_TIDE_LEVEL
							case SENSOR_MULTILEVEL_REPORT_TIDE_LEVEL_VERSION_2_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_TIDE_LEVEL_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_TIDE_LEVEL_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_TIDE_LEVEL_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_WEIGHT
							case SENSOR_MULTILEVEL_REPORT_WEIGHT_VERSION_3_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_WEIGHT_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_WEIGHT_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_WEIGHT_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_VOLTAGE
							case SENSOR_MULTILEVEL_REPORT_VOLTAGE_VERSION_3_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_VOLTAGE_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_VOLTAGE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_VOLTAGE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_CURRENT
							case SENSOR_MULTILEVEL_REPORT_CURRENT_VERSION_3_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_CURRENT_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_CURRENT_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_CURRENT_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_CO2_LEVEL
							case SENSOR_MULTILEVEL_REPORT_CO2_LEVEL_VERSION_3_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_CO2_LEVEL_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_CO2_LEVEL_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_CO2_LEVEL_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_AIR_FLOW
							case SENSOR_MULTILEVEL_REPORT_AIR_FLOW_VERSION_3_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_AIR_FLOW_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_AIR_FLOW_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_AIR_FLOW_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_TANK_CAPACITY
							case SENSOR_MULTILEVEL_REPORT_TANK_CAPACITY_VERSION_3_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_TANK_CAPACITY_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_TANK_CAPACITY_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_TANK_CAPACITY_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_DISTANCE
							case SENSOR_MULTILEVEL_REPORT_DISTANCE_VERSION_3_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_DISTANCE_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_DISTANCE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_DISTANCE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_ANGLE_POSITION
							case SENSOR_MULTILEVEL_REPORT_ANGLE_POSITION_VERSION_4_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_ANGLE_POSITION_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_ANGLE_POSITION_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_ANGLE_POSITION_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_ROTATION
							case SENSOR_MULTILEVEL_REPORT_ROTATION_V5_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_ROTATION_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_ROTATION_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_ROTATION_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_WATER_TEMPERATURE
							case SENSOR_MULTILEVEL_REPORT_WATER_TEMPERATURE_V5_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_WATER_TEMPERATURE_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_WATER_TEMPERATURE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_WATER_TEMPERATURE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_SOIL_TEMPERATURE
							case SENSOR_MULTILEVEL_REPORT_SOIL_TEMPERATURE_V5_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_SOIL_TEMPERATURE_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_SOIL_TEMPERATURE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_SOIL_TEMPERATURE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_SEISMIC_INTENSITY
							case SENSOR_MULTILEVEL_REPORT_SEISMIC_INTENSITY_V5_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_SEISMIC_INTENSITY_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_SEISMIC_INTENSITY_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_SEISMIC_INTENSITY_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_SEISMIC_MAGNITUDE
							case SENSOR_MULTILEVEL_REPORT_SEISMIC_MAGNITUDE_V5_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_SEISMIC_MAGNITUDE_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_SEISMIC_MAGNITUDE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_SEISMIC_MAGNITUDE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_ULTRAVIOLET
							case SENSOR_MULTILEVEL_REPORT_ULTRAVIOLET_V5_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_ULTRAVIOLET_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_ULTRAVIOLET_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_ULTRAVIOLET_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_ELECTRICAL_RESISTIVITY
							case SENSOR_MULTILEVEL_REPORT_ELECTRICAL_RESISTIVITY_V5_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_ELECTRICAL_RESISTIVITY_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_ELECTRICAL_RESISTIVITY_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_ELECTRICAL_RESISTIVITY_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_ELECTRICAL_CONDUCTIVITY
							case SENSOR_MULTILEVEL_REPORT_ELECTRICAL_CONDUCTIVITY_V5_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_ELECTRICAL_CONDUCTIVITY_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_ELECTRICAL_CONDUCTIVITY_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_ELECTRICAL_CONDUCTIVITY_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_LOUDNESS
							case SENSOR_MULTILEVEL_REPORT_LOUDNESS_V5_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_LOUDNESS_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_LOUDNESS_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_LOUDNESS_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_MOISTURE
							case SENSOR_MULTILEVEL_REPORT_MOISTURE_V5_V5:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_MOISTURE_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL_MOISTURE_SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL_MOISTURE_PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_FREQUENCY
							#error Finish V7 variables!
							case SENSOR_MULTILEVEL_REPORT_FREQUENCY_:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_FREQUENCY_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL__SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL__PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_TIME
							#error Finish V7 variables!
							case SENSOR_MULTILEVEL_REPORT_TIME_:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_TIME_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL__SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL__PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							#if CC_SENSOR_MULTILEVEL_WITH_TARGET_TEMPERATURE
							#error Finish V7 variables!
							case SENSOR_MULTILEVEL_REPORT_TARGET_TEMPERATURE_:
								*((WORD *)&txBuf->ZW_SensorMultilevelReport2byteFrame.sensorValue1) = CC_SENSOR_MULTILEVEL_TARGET_TEMPERATURE_GETTER;
								txBuf->ZW_SensorMultilevelReport2byteFrame.level =
									((CC_SENSOR_MULTILEVEL__SCALE << SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_SCALE_MASK) | \
									((CC_SENSOR_MULTILEVEL__PRECISION << SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_SHIFT) & SENSOR_MULTILEVEL_REPORT_LEVEL_PRECISION_MASK);
								break;
							#endif
							default:
								break; // do not send the report
						}

						txBuf->ZW_SensorMultilevelReport2byteFrame.level += (CC_SENSOR_MULTILEVEL_SIZE & SENSOR_MULTILEVEL_REPORT_LEVEL_SIZE_MASK);
						EnqueuePacket(sourceNode, sizeof(txBuf->ZW_SensorMultilevelReport2byteFrame), txOption);
					}
					break;
			}
			break;
		#endif
				
		#if WITH_CC_METER	
			case  COMMAND_CLASS_METER:
				switch(cmd)
				{
					case METER_GET:
						sendMeterReport(sourceNode, ((cmdLength > OFFSET_PARAM_1) ? ((param1 >> 3)&0x03) : METER_DEFAULT_SCALE));
						break;
					case METER_RESET_V4:
						METER_RESET_HANDLER;
						break;
					case METER_SUPPORTED_GET_V4:
						if ((txBuf = CreatePacket()) != NULL) {
						txBuf->ZW_MeterSupportedReport1byteV4Frame.cmdClass 	= COMMAND_CLASS_METER;
						txBuf->ZW_MeterSupportedReport1byteV4Frame.cmd 			= METER_SUPPORTED_REPORT_V4;
						txBuf->ZW_MeterSupportedReport1byteV4Frame.properties1 	= ((METER_RESETABLE&0x01) ? METER_SUPPORTED_REPORT_PROPERTIES1_METER_RESET_BIT_MASK_V4:0) | 
																				  ((METER_RATE_TYPE << METER_SUPPORTED_REPORT_PROPERTIES1_RATE_TYPE_SHIFT_V4) & METER_SUPPORTED_REPORT_PROPERTIES1_RATE_TYPE_MASK_V4) | 
																				  (METER_TYPE & METER_SUPPORTED_REPORT_PROPERTIES1_METER_TYPE_MASK_V4); 
						txBuf->ZW_MeterSupportedReport1byteV4Frame.properties2 	= METER_SCALE & METER_SUPPORTED_REPORT_PROPERTIES2_SCALE_SUPPORTED_0_MASK_V4; 
						EnqueuePacket(sourceNode, sizeof(txBuf->ZW_MeterSupportedReport1byteV4Frame)-2, txOption);
						}
						break; 
				}
			break; 
		#endif

		#ifdef ZW_CONTROLLER
		case COMMAND_CLASS_CONTROLLER_REPLICATION:
		{
			case CTRL_REPLICATION_TRANSFER_GROUP:
        	case CTRL_REPLICATION_TRANSFER_GROUP_NAME:
        	case CTRL_REPLICATION_TRANSFER_SCENE:
        	case CTRL_REPLICATION_TRANSFER_SCENE_NAME:
        		ZW_ReplicationReceiveComplete();
        		break;
		}
		break;
		#endif
		
		#if WITH_CC_SWITCH_BINARY
			#ifdef CC_SWITCH_BINARY_ON
			if(CC_SWITCH_BINARY_ON)
			#endif
			{
			case  COMMAND_CLASS_SWITCH_BINARY:
				switch(cmd) {
					case SWITCH_BINARY_SET:
						SWITCH_BINARY_ACTION(param1);
					break;
					case SWITCH_BINARY_GET:
						sendSwitchBinaryReport(sourceNode);
					break;
				}
				break;
			}
			
		#endif

		#if WITH_CC_SWITCH_MULTILEVEL
		case COMMAND_CLASS_SWITCH_MULTILEVEL:
			#ifdef CC_SWITCH_MULTILEVEL_ON
			if(CC_SWITCH_MULTILEVEL_ON)
			#endif
			
			{
			switch(cmd) {
				case SWITCH_MULTILEVEL_SET:
					if (param1 == SWITCHED_OFF || param1 == SWITCHED_ON || (param1 >= SWITCH_MULTILEVEL_LEVEL_MIN && param1 <= SWITCH_MULTILEVEL_LEVEL_MAX)) {
						SWITCH_MULTILEVEL_SET_ACTION(param1, ((cmdLength > OFFSET_PARAM_2) ? param2 : 0xFF));
					}
					break;

				case SWITCH_MULTILEVEL_GET:
					SendReportToNode(sourceNode, COMMAND_CLASS_SWITCH_MULTILEVEL, SWITCH_MULTILEVEL_VALUE());
					break;

				case SWITCH_MULTILEVEL_START_LEVEL_CHANGE:
					if (param1 & SWITCH_MULTILEVEL_START_LEVEL_CHANGE_PROPERTIES1_RESERVED2_BIT_MASK_V2)
						break; // ignore values SWITCH_MULTILEVEL_START_LEVEL_CHANGE_UP_DOWN_RESERVED_V3 and SWITCH_MULTILEVEL_START_LEVEL_CHANGE_UP_DOWN

					SWITCH_MULTILEVEL_START_CHANGE_ACTION(
						((param1 & SWITCH_MULTILEVEL_START_LEVEL_CHANGE_LEVEL_UP_DOWN_BIT_MASK) != 0),
						((param1 & SWITCH_MULTILEVEL_START_LEVEL_CHANGE_LEVEL_IGNORE_START_LEVEL_BIT_MASK) != 0),
						param2,
						((cmdLength > OFFSET_PARAM_3) ? param3 : 0xFF));
					break;

				case SWITCH_MULTILEVEL_STOP_LEVEL_CHANGE:
					SWITCH_MULTILEVEL_STOP_CHANGE_ACTION();
					break;

				case SWITCH_MULTILEVEL_SUPPORTED_GET_V3:
					if ((txBuf = CreatePacket()) != NULL) {
						txBuf->ZW_SwitchMultilevelSupportedReportV3Frame.cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL;
						txBuf->ZW_SwitchMultilevelSupportedReportV3Frame.cmd = SWITCH_MULTILEVEL_SUPPORTED_REPORT_V3;
						txBuf->ZW_SwitchMultilevelSupportedReportV3Frame.properties1 = 0x03; // Close/Open
						txBuf->ZW_SwitchMultilevelSupportedReportV3Frame.properties2 = 0x00; // Unsupported
						EnqueuePacket(sourceNode, sizeof(txBuf->ZW_SwitchMultilevelSupportedReportV3Frame), txOption);
					}
					break;
			}
			}
			break;
		#endif
			
		#if WITH_CC_NOTIFICATION
		case COMMAND_CLASS_NOTIFICATION_V3:
			switch(cmd) {
				
				case NOTIFICATION_SET_V3:
					switch(param1) {
						#if CC_NOTIFICATION_WITH_SMOKE_ALARM
						case NOTIFICATION_TYPE_SMOKE_ALARM:
							NOTIFICATION_TYPE_SMOKE_ALARM_SETTER
							break;
						#endif
							
						#if CC_NOTIFICATION_WITH_CO_ALARM
						case NOTIFICATION_TYPE_CO_ALARM:
							NOTIFICATION_TYPE_CO_ALARM_SETTER
							break;
						#endif
							
						#if CC_NOTIFICATION_WITH_CO2_ALARM
						case NOTIFICATION_TYPE_CO2_ALARM:
							NOTIFICATION_TYPE_CO2_ALARM_SETTER
							break;
						#endif
							
						#if CC_NOTIFICATION_WITH_HEAT_ALARM
						case NOTIFICATION_TYPE_HEAT_ALARM:
							NOTIFICATION_TYPE_HEAT_ALARM_SETTER
							break;
						#endif
							
						#if CC_NOTIFICATION_WITH_WATER_ALARM
						case NOTIFICATION_TYPE_WATER_ALARM:
							NOTIFICATION_TYPE_WATER_ALARM_SETTER
							break;
						#endif
							
						#if CC_NOTIFICATION_WITH_ACCESS_CONTROL_ALARM
						case NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM:
							NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM_SETTER
							break;
						#endif
							
						#if CC_NOTIFICATION_WITH_BURGLAR_ALARM
						case NOTIFICATION_TYPE_BURGLAR_ALARM:
							NOTIFICATION_TYPE_BURGLAR_ALARM_SETTER
							break;
						#endif
							
						#if CC_NOTIFICATION_WITH_POWER_MANAGEMENT_ALARM
						case NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM:
							NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM_SETTER
							break;
						#endif
							
						#if CC_NOTIFICATION_WITH_SYSTEM_ALARM
						case NOTIFICATION_TYPE_SYSTEM_ALARM:
							NOTIFICATION_TYPE_SYSTEM_ALARM_SETTER
							break;
						#endif
							
						#if CC_NOTIFICATION_WITH_EMERGENCY_ALARM
						case NOTIFICATION_TYPE_EMERGENCY_ALARM:
							NOTIFICATION_TYPE_EMERGENCY_ALARM_SETTER
							break;
						#endif
							
						#if CC_NOTIFICATION_WITH_CLOCK_ALARM
						case NOTIFICATION_TYPE_CLOCK_ALARM:
							NOTIFICATION_TYPE_CLOCK_ALARM_SETTER
							break;
						#endif
							
						#if CC_NOTIFICATION_WITH_APPLIANCE_ALARM
						case NOTIFICATION_TYPE_APPLIANCE_ALARM:
							NOTIFICATION_TYPE_APPLIANCE_ALARM_SETTER
							break;
						#endif
					}
					break;

				case NOTIFICATION_GET_V3:
				//ALARM_V1 is hardcoded for backward compatibility
			        if (cmdLength == sizeof(ZW_ALARM_GET_FRAME)) {
						if ((txBuf = CreatePacket()) != NULL) {
							txBuf->ZW_AlarmReportFrame.cmdClass = COMMAND_CLASS_ALARM;
							txBuf->ZW_AlarmReportFrame.cmd = ALARM_REPORT;
							txBuf->ZW_AlarmReportFrame.alarmType = 0x00; 
							txBuf->ZW_AlarmReportFrame.alarmLevel = 0x00; 
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_AlarmReportFrame), txOption);
						}
						break;
			        } else {//ALARM_V2
			        BYTE alarm_version;
			        if (cmdLength == sizeof(ZW_ALARM_GET_V2_FRAME)) {
			        	alarm_version=FALSE;
			        	switch(param2) {
									
							case CC_NOTIFICATION_RETURN_FIRST_DETECTED:
								if ((txBuf = CreatePacket()) != NULL) {
									txBuf->ZW_NotificationReport1byteV3Frame.cmdClass = COMMAND_CLASS_NOTIFICATION_V3;
									txBuf->ZW_NotificationReport1byteV3Frame.cmd = NOTIFICATION_REPORT_V3;
									txBuf->ZW_NotificationReport1byteV3Frame.v1AlarmType = 0x00; 
									txBuf->ZW_NotificationReport1byteV3Frame.v1AlarmLevel = 0x00; 
									txBuf->ZW_NotificationReport1byteV3Frame.zensorNetSourceNodeId = 0x00;
									txBuf->ZW_NotificationReport1byteV3Frame.notificationStatus = NOTIFICATION_NO_PENDING_STATUS;		
									txBuf->ZW_NotificationReport1byteV3Frame.notificationType = 0; 
									txBuf->ZW_NotificationReport1byteV3Frame.mevent = 0;
									txBuf->ZW_NotificationReport1byteV3Frame.properties1 = 0;		// Encapsulated commands not included! 
									txBuf->ZW_NotificationReport1byteV3Frame.eventParameter1 = 0; 	// Encapsulated commands not included!
									txBuf->ZW_NotificationReport1byteV4Frame.sequenceNumber = 0;	// Encapsulated commands not included!
								}
								EnqueuePacket(sourceNode, sizeof(txBuf->ZW_NotificationReport1byteV4Frame), TRANSMIT_OPTION_USE_DEFAULT);
								break;

							#if CC_NOTIFICATION_WITH_SMOKE_ALARM
							case NOTIFICATION_TYPE_SMOKE_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_SMOKE
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_SMOKE)
									#endif

									#if CC_EVENT_WITH_SMOKE_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_SMOKE_UL)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif

							#if CC_NOTIFICATION_WITH_CO_ALARM
							case NOTIFICATION_TYPE_CO_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_CO
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_CO)
									#endif

									#if CC_EVENT_WITH_CO_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_CO_UL)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif

							#if CC_NOTIFICATION_WITH_CO2_ALARM
							case NOTIFICATION_TYPE_CO2_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_CO2
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_CO2)
									#endif

									#if CC_EVENT_WITH_CO2_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_CO2_UL)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif

							#if CC_NOTIFICATION_WITH_HEAT_ALARM
							case NOTIFICATION_TYPE_HEAT_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_OVERHEAT
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_OVERHEAT)
									#endif

									#if CC_EVENT_WITH_OVERHEAT_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_OVERHEAT_UL)
									#endif

									#if CC_EVENT_WITH_TEMP_RISE
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_TEMP_RISE)
									#endif

									#if CC_EVENT_WITH_TEMP_RISE_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_TEMP_RISE_UL)
									#endif

									#if CC_EVENT_WITH_UNDERHEAT
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_UNDERHEAT)
									#endif

									#if CC_EVENT_WITH_UNDERHEAT_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_UNDERHEAT_UL)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif

							#if CC_NOTIFICATION_WITH_WATER_ALARM
							case NOTIFICATION_TYPE_WATER_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_WATER_LEAK
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_WATER_LEAK)
									#endif

									#if CC_EVENT_WITH_WATER_LEAK_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_WATER_LEAK_UL)
									#endif

									#if CC_EVENT_WITH_WATER_LEVEL_DROP
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_WATER_LEVEL_DROP)
									#endif

									#if CC_EVENT_WITH_WATER_LEVEL_DROP_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_WATER_LEVEL_DROP_UL)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif


							#if CC_NOTIFICATION_WITH_ACCESS_CONTROL_ALARM
							case NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_MANUAL_LOCK
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_MANUAL_LOCK)
									#endif

									#if CC_EVENT_WITH_MANUAL_UNLOCK
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_MANUAL_UNLOCK)
									#endif

									#if CC_EVENT_WITH_RF_LOCK
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_RF_LOCK)
									#endif

									#if CC_EVENT_WITH_RF_UNLOCK
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_RF_UNLOCK)
									#endif

									#if CC_EVENT_WITH_KEYPAD_LOCK
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_KEYPAD_LOCK)
									#endif

									#if CC_EVENT_WITH_KEYPAD_UNLOCK
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_KEYPAD_UNLOCK)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif

							#if CC_NOTIFICATION_WITH_BURGLAR_ALARM
							case NOTIFICATION_TYPE_BURGLAR_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_INTRUSION
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_INTRUSION)
									#endif

									#if CC_EVENT_WITH_INTRUSION_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_INTRUSION_UL)
									#endif

									#if CC_EVENT_WITH_TAMPER_REMOVED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_TAMPER_REMOVED)
									#endif

									#if CC_EVENT_WITH_TAMPER_INVALID_CODE
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_TAMPER_INVALID_CODE)
									#endif

									#if CC_EVENT_WITH_GLASS_BREAK
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_GLASS_BREAK)
									#endif

									#if CC_EVENT_WITH_GLASS_BREAK_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_GLASS_BREAK_UL)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif

							#if CC_NOTIFICATION_WITH_POWER_MANAGEMENT_ALARM
							case NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_POWER_APPLIED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_POWER_APPLIED)
									#endif

									#if CC_EVENT_WITH_AC_DISCONNECTED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_AC_DISCONNECTED)
									#endif

									#if CC_EVENT_WITH_AC_RECONNECTED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_AC_RECONNECTED)
									#endif

									#if CC_EVENT_WITH_SURGE
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_SURGE)
									#endif

									#if CC_EVENT_WITH_VOLTAGE_DROP
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_VOLTAGE_DROP)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif

							#if CC_NOTIFICATION_WITH_SYSTEM_ALARM
							case NOTIFICATION_TYPE_SYSTEM_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_HW_FAIL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_HW_FAIL)
									#endif

									#if CC_EVENT_WITH_SW_FAIL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_SW_FAIL)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif

							#if CC_NOTIFICATION_WITH_EMERGENCY_ALARM
							case NOTIFICATION_TYPE_EMERGENCY_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_CONTACT_POLICE
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_CONTACT_POLICE)
									#endif

									#if CC_EVENT_WITH_CONTACT_FIREMEN
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_CONTACT_FIREMEN)
									#endif

									#if CC_EVENT_WITH_CONTACT_MEDIC
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_CONTACT_MEDIC)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif

							#if CC_NOTIFICATION_WITH_CLOCK_ALARM
							case NOTIFICATION_TYPE_CLOCK_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_WAKE_UP_ALERT
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_WAKE_UP_ALERT)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif						
						}
						break;
			        } else {
			        if (cmdLength == sizeof(ZW_NOTIFICATION_GET_V3_FRAME)) {
			        	alarm_version=TRUE;
						switch(param2) {
							//NOTIFICATION_GET_HANDLER
							case CC_NOTIFICATION_RETURN_FIRST_DETECTED:
								if ((txBuf = CreatePacket()) != NULL) {
									txBuf->ZW_NotificationReport1byteV3Frame.cmdClass = COMMAND_CLASS_NOTIFICATION_V3;
									txBuf->ZW_NotificationReport1byteV3Frame.cmd = NOTIFICATION_REPORT_V3;
									txBuf->ZW_NotificationReport1byteV3Frame.v1AlarmType = 0x00; 
									txBuf->ZW_NotificationReport1byteV3Frame.v1AlarmLevel = 0x00; 
									txBuf->ZW_NotificationReport1byteV3Frame.zensorNetSourceNodeId = 0x00;
									txBuf->ZW_NotificationReport1byteV3Frame.notificationStatus = NOTIFICATION_NO_PENDING_STATUS;		
									txBuf->ZW_NotificationReport1byteV3Frame.notificationType = 0; 
									txBuf->ZW_NotificationReport1byteV3Frame.mevent = 0;
									txBuf->ZW_NotificationReport1byteV3Frame.properties1 = 0;		// Encapsulated commands not included! 
									txBuf->ZW_NotificationReport1byteV3Frame.eventParameter1 = 0; 	// Encapsulated commands not included!
									txBuf->ZW_NotificationReport1byteV4Frame.sequenceNumber = 0;	// Encapsulated commands not included!
								}
								EnqueuePacket(sourceNode, sizeof(txBuf->ZW_NotificationReport1byteV4Frame), TRANSMIT_OPTION_USE_DEFAULT);
								break;


							#if CC_NOTIFICATION_WITH_SMOKE_ALARM
							case NOTIFICATION_TYPE_SMOKE_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_SMOKE
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_SMOKE)
									#endif

									#if CC_EVENT_WITH_SMOKE_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_SMOKE_UL)
									#endif

									#if CC_EVENT_WITH_SMOKE_TEST
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_SMOKE_TEST)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif

							#if CC_NOTIFICATION_WITH_CO_ALARM
							case NOTIFICATION_TYPE_CO_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_CO
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_CO)
									#endif

									#if CC_EVENT_WITH_CO_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_CO_UL)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif

							#if CC_NOTIFICATION_WITH_CO2_ALARM
							case NOTIFICATION_TYPE_CO2_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_CO2
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_CO2)
									#endif

									#if CC_EVENT_WITH_CO2_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_CO2_UL)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif

							#if CC_NOTIFICATION_WITH_HEAT_ALARM
							case NOTIFICATION_TYPE_HEAT_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_OVERHEAT
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_OVERHEAT)
									#endif

									#if CC_EVENT_WITH_OVERHEAT_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_OVERHEAT_UL)
									#endif

									#if CC_EVENT_WITH_TEMP_RISE
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_TEMP_RISE)
									#endif

									#if CC_EVENT_WITH_TEMP_RISE_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_TEMP_RISE_UL)
									#endif

									#if CC_EVENT_WITH_UNDERHEAT
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_UNDERHEAT)
									#endif

									#if CC_EVENT_WITH_UNDERHEAT_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_UNDERHEAT_UL)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif

							#if CC_NOTIFICATION_WITH_WATER_ALARM
							case NOTIFICATION_TYPE_WATER_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_WATER_LEAK
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_WATER_LEAK)
									#endif

									#if CC_EVENT_WITH_WATER_LEAK_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_WATER_LEAK_UL)
									#endif

									#if CC_EVENT_WITH_WATER_LEVEL_DROP
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_WATER_LEVEL_DROP)
									#endif

									#if CC_EVENT_WITH_WATER_LEVEL_DROP_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_WATER_LEVEL_DROP_UL)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif


							#if CC_NOTIFICATION_WITH_ACCESS_CONTROL_ALARM
							case NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_MANUAL_LOCK
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_MANUAL_LOCK)
									#endif

									#if CC_EVENT_WITH_MANUAL_UNLOCK
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_MANUAL_UNLOCK)
									#endif

									#if CC_EVENT_WITH_MANUAL_NOT_FULLY_LOCKED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_MANUAL_NOT_FULLY_LOCKED)
									#endif

									#if CC_EVENT_WITH_RF_LOCK
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_RF_LOCK)
									#endif

									#if CC_EVENT_WITH_RF_UNLOCK
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_RF_UNLOCK)
									#endif

									#if CC_EVENT_WITH_RF_NOT_FULLY_LOCKED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_RF_NOT_FULLY_LOCKED)
									#endif

									#if CC_EVENT_WITH_KEYPAD_LOCK
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_KEYPAD_LOCK)
									#endif

									#if CC_EVENT_WITH_KEYPAD_UNLOCK
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_KEYPAD_UNLOCK)
									#endif

									#if CC_EVENT_WITH_AUTO_LOCKED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_AUTO_LOCKED)
									#endif

									#if CC_EVENT_WITH_AUTO_LOCK_NOT_FULLY_LOCKED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_AUTO_LOCK_NOT_FULLY_LOCKED)
									#endif

									#if CC_EVENT_WITH_LOCK_JAMMED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_LOCK_JAMMED)
									#endif

									#if CC_EVENT_WITH_ALL_USER_CODES_DELETED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_ALL_USER_CODES_DELETED)
									#endif

									#if CC_EVENT_WITH_SINGLE_USER_CODE_DELETED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_SINGLE_USER_CODE_DELETED)
									#endif

									#if CC_EVENT_WITH_NEW_USER_CODE_ADDED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_NEW_USER_CODE_ADDED)
									#endif

									#if CC_EVENT_WITH_NEW_USER_CODE_NOT_ADDED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_NEW_USER_CODE_NOT_ADDED)
									#endif

									#if CC_EVENT_WITH_KEYPAD_TEMPORARY_DISABLE
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_KEYPAD_TEMPORARY_DISABLE)
									#endif

									#if CC_EVENT_WITH_KEYPAD_BUSY
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_KEYPAD_BUSY)
									#endif

									#if CC_EVENT_WITH_NEW_PROGRAM_CODE_ENTERED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_NEW_PROGRAM_CODE_ENTERED)
									#endif

									#if CC_EVENT_WITH_MANUAL_CODE_EXCEEDS_LIMITS
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_MANUAL_CODE_EXCEEDS_LIMITS)
									#endif

									#if CC_EVENT_WITH_RF_UNLOCK_INVALID_CODE
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_RF_UNLOCK_INVALID_CODE)
									#endif

									#if CC_EVENT_WITH_RF_LOCK_INVALID_CODE
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_RF_LOCK_INVALID_CODE)
									#endif

									#if CC_EVENT_WITH_WINDOW_DOOR_OPENED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_WINDOW_DOOR_OPENED)
									#endif

									#if CC_EVENT_WITH_WINDOW_DOOR_CLOSED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_WINDOW_DOOR_CLOSED)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif

							#if CC_NOTIFICATION_WITH_BURGLAR_ALARM
							case NOTIFICATION_TYPE_BURGLAR_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_INTRUSION
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_INTRUSION)
									#endif

									#if CC_EVENT_WITH_INTRUSION_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_INTRUSION_UL)
									#endif

									#if CC_EVENT_WITH_TAMPER_REMOVED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_TAMPER_REMOVED)
									#endif

									#if CC_EVENT_WITH_TAMPER_INVALID_CODE
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_TAMPER_INVALID_CODE)
									#endif

									#if CC_EVENT_WITH_GLASS_BREAK
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_GLASS_BREAK)
									#endif

									#if CC_EVENT_WITH_GLASS_BREAK_UL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_GLASS_BREAK_UL)
									#endif

									#if CC_EVENT_WITH_MOTION_DETECTION
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_MOTION_DETECTION)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif

							#if CC_NOTIFICATION_WITH_POWER_MANAGEMENT_ALARM
							case NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_POWER_APPLIED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_POWER_APPLIED)
									#endif

									#if CC_EVENT_WITH_AC_DISCONNECTED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_AC_DISCONNECTED)
									#endif

									#if CC_EVENT_WITH_AC_RECONNECTED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_AC_RECONNECTED)
									#endif

									#if CC_EVENT_WITH_SURGE
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_SURGE)
									#endif

									#if CC_EVENT_WITH_VOLTAGE_DROP
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_VOLTAGE_DROP)
									#endif

									#if CC_EVENT_WITH_OVER_CURRENT
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_OVER_CURRENT)
									#endif

									#if CC_EVENT_WITH_OVER_VOLTAGE
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_OVER_VOLTAGE)
									#endif

									#if CC_EVENT_WITH_OVER_LOAD
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_OVER_LOAD)
									#endif

									#if CC_EVENT_WITH_LOAD_ERROR
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_LOAD_ERROR)
									#endif

									#if CC_EVENT_WITH_REPLACE_BAT_SOON
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_REPLACE_BAT_SOON)
									#endif

									#if CC_EVENT_WITH_REPLACE_BAT_NOW
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_REPLACE_BAT_NOW)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif

							#if CC_NOTIFICATION_WITH_SYSTEM_ALARM
							case NOTIFICATION_TYPE_SYSTEM_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_HW_FAIL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_HW_FAIL)
									#endif

									#if CC_EVENT_WITH_SW_FAIL
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_SW_FAIL)
									#endif

									#if CC_EVENT_WITH_HW_FAIL_OEM
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_HW_FAIL_OEM)
									#endif

									#if CC_EVENT_WITH_SW_FAIL_OEM
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_SW_FAIL_OEM)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif

							#if CC_NOTIFICATION_WITH_EMERGENCY_ALARM
							case NOTIFICATION_TYPE_EMERGENCY_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_CONTACT_POLICE
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_CONTACT_POLICE)
									#endif

									#if CC_EVENT_WITH_CONTACT_FIREMEN
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_CONTACT_FIREMEN)
									#endif

									#if CC_EVENT_WITH_CONTACT_MEDIC
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_CONTACT_MEDIC)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif

							#if CC_NOTIFICATION_WITH_CLOCK_ALARM
							case NOTIFICATION_TYPE_CLOCK_ALARM:
								switch(param3){

									#if CC_EVENT_WITH_WAKE_UP_ALERT
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_WAKE_UP_ALERT)
									#endif

									#if CC_EVENT_WITH_TIMER_ENDED
									MAKE_NOTIFICATION_EVENT_REPORT(NOTIFICATION_EVENT_TIMER_ENDED)
									#endif

									MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT
								}
							break;
							#endif						
						}
						break;
					}
					}
					}
					break;

				#if WITH_PARSING_NOTIFICATION_REPORTS
				case NOTIFICATION_REPORT_V3:
					CUSTOM_NOTIRICATION_REPORT_PARSER
				break;
				#endif

				
				case NOTIFICATION_SUPPORTED_GET_V3:
					if ((txBuf = CreatePacket()) != NULL) {
						txBuf->ZW_NotificationSupportedReport1byteV3Frame.cmdClass = COMMAND_CLASS_NOTIFICATION_V3;
						txBuf->ZW_NotificationSupportedReport1byteV3Frame.cmd = NOTIFICATION_SUPPORTED_REPORT_V3;
																	#if CC_NOTIFICATION_WITH_POWER_MANAGEMENT_ALARM || \
																	CC_NOTIFICATION_WITH_SYSTEM_ALARM || \
																	CC_NOTIFICATION_WITH_EMERGENCY_ALARM || \
																	CC_NOTIFICATION_WITH_CLOCK_ALARM || \
																	CC_NOTIFICATION_WITH_APPLIANCE_ALARM || \
																	0
						txBuf->ZW_NotificationSupportedReport1byteV3Frame.properties1 = 2;
																	#else
						txBuf->ZW_NotificationSupportedReport1byteV3Frame.properties1 = 1;
																	#endif
						txBuf->ZW_NotificationSupportedReport1byteV3Frame.bitMask1 = 
																#if CC_NOTIFICATION_WITH_SMOKE_ALARM
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_TYPE_SMOKE_ALARM)
																#endif
																#if  CC_NOTIFICATION_WITH_CO_ALARM
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_TYPE_CO_ALARM)
																#endif
																#if  CC_NOTIFICATION_WITH_CO2_ALARM
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_TYPE_CO2_ALARM)
																#endif
																#if  CC_NOTIFICATION_WITH_HEAT_ALARM
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_TYPE_HEAT_ALARM)
																#endif
																#if  CC_NOTIFICATION_WITH_WATER_ALARM
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_TYPE_WATER_ALARM)
																#endif
																#if  CC_NOTIFICATION_WITH_ACCESS_CONTROL_ALARM
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM)
																#endif
																#if  CC_NOTIFICATION_WITH_BURGLAR_ALARM
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_TYPE_BURGLAR_ALARM)
																#endif
																0;
						#if CC_NOTIFICATION_WITH_POWER_MANAGEMENT_ALARM || \
						CC_NOTIFICATION_WITH_SYSTEM_ALARM || \
						CC_NOTIFICATION_WITH_EMERGENCY_ALARM || \
						CC_NOTIFICATION_WITH_CLOCK_ALARM || \
						CC_NOTIFICATION_WITH_APPLIANCE_ALARM || \
						0
						txBuf->ZW_NotificationSupportedReport2byteV3Frame.bitMask2 = 
																#if CC_NOTIFICATION_WITH_POWER_MANAGEMENT_ALARM
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM - 8)
																#endif
																#if  CC_NOTIFICATION_WITH_SYSTEM_ALARM
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_TYPE_SYSTEM_ALARM - 8)
																#endif
																#if  CC_NOTIFICATION_WITH_EMERGENCY_ALARM
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_TYPE_EMERGENCY_ALARM - 8)
																#endif
																#if  CC_NOTIFICATION_WITH_CLOCK_ALARM
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_TYPE_CLOCK_ALARM - 8)
																#endif
																#if  CC_NOTIFICATION_WITH_APPLIANCE_ALARM
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_TYPE_APPLIANCE_ALARM - 8)
																#endif
																0;
						EnqueuePacket(sourceNode, sizeof(txBuf->ZW_NotificationSupportedReport2byteV3Frame), txOption);
						#else
						EnqueuePacket(sourceNode, sizeof(txBuf->ZW_NotificationSupportedReport1byteV3Frame), txOption);
						#endif
					}
					break;
				
				case EVENT_SUPPORTED_GET_V3:
					if ((txBuf = CreatePacket()) != NULL) {	
						txBuf->ZW_EventSupportedReport1byteV3Frame.cmdClass = COMMAND_CLASS_NOTIFICATION_V3;
						txBuf->ZW_EventSupportedReport1byteV3Frame.cmd = EVENT_SUPPORTED_REPORT_V3;
						txBuf->ZW_EventSupportedReport1byteV3Frame.notificationType = param1; 
						switch(param1) {
							#if CC_NOTIFICATION_WITH_SMOKE_ALARM
							case NOTIFICATION_TYPE_SMOKE_ALARM:
								txBuf->ZW_EventSupportedReport1byteV3Frame.properties1 = 0x01; \
								txBuf->ZW_EventSupportedReport1byteV3Frame.bitMask1 = 
																#if CC_EVENT_WITH_SMOKE
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_SMOKE)
																#endif
																#if CC_EVENT_WITH_SMOKE_UL
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_SMOKE_UL)
																#endif
																#if CC_EVENT_WITH_SMOKE_TEST
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_SMOKE_TEST)
																#endif
																0;
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_EventSupportedReport1byteV3Frame), txOption);
							break;
							#endif
							
							#if CC_NOTIFICATION_WITH_CO_ALARM
							case NOTIFICATION_TYPE_CO_ALARM:
								txBuf->ZW_EventSupportedReport1byteV3Frame.properties1 = 0x01; \
								txBuf->ZW_EventSupportedReport1byteV3Frame.bitMask1 = 
																#if CC_EVENT_WITH_CO
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_CO)
																#endif
																#if CC_EVENT_WITH_CO_UL
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_CO_UL)
																#endif
																0;
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_EventSupportedReport1byteV3Frame), txOption);
							break;
							#endif
							
							#if CC_NOTIFICATION_WITH_CO2_ALARM
							case NOTIFICATION_TYPE_CO2_ALARM:
								txBuf->ZW_EventSupportedReport1byteV3Frame.properties1 = 0x01; \
								txBuf->ZW_EventSupportedReport1byteV3Frame.bitMask1 = 
																#if CC_EVENT_WITH_CO2
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_CO2)
																#endif
																#if CC_EVENT_WITH_CO2_UL
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_CO2_UL)
																#endif
																0;
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_EventSupportedReport1byteV3Frame), txOption);
							break;
							#endif
							
							#if CC_NOTIFICATION_WITH_HEAT_ALARM
							case NOTIFICATION_TYPE_HEAT_ALARM:
								txBuf->ZW_EventSupportedReport1byteV3Frame.properties1 = 0x01; \
								txBuf->ZW_EventSupportedReport1byteV3Frame.bitMask1 = 
																#if CC_EVENT_WITH_OVERHEAT
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_OVERHEAT)
																#endif
																#if CC_EVENT_WITH_OVERHEAT_UL
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_OVERHEAT_UL)
																#endif
																#if CC_EVENT_WITH_TEMP_RISE
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_TEMP_RISE)
																#endif
																#if CC_EVENT_WITH_TEMP_RISE_UL
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_TEMP_RISE_UL)
																#endif
																#if CC_EVENT_WITH_UNDERHEAT
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_UNDERHEAT)
																#endif
																#if CC_EVENT_WITH_UNDERHEAT_UL
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_UNDERHEAT_UL)
																#endif
																0;
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_EventSupportedReport1byteV3Frame), txOption);
							break;
							#endif
							
							#if CC_NOTIFICATION_WITH_WATER_ALARM
							case NOTIFICATION_TYPE_WATER_ALARM:
								txBuf->ZW_EventSupportedReport1byteV3Frame.properties1 = 0x01; \
								txBuf->ZW_EventSupportedReport1byteV3Frame.bitMask1 = 
																#if CC_EVENT_WITH_WATER_LEAK
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_WATER_LEAK)
																#endif
																#if CC_EVENT_WITH_WATER_LEAK_UL
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_WATER_LEAK_UL)
																#endif
																#if CC_EVENT_WITH_WATER_LEVEL_DROP
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_WATER_LEVEL_DROP)
																#endif
																#if CC_EVENT_WITH_WATER_LEVEL_DROP_UL
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_WATER_LEVEL_DROP_UL)
																#endif
																0;
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_EventSupportedReport1byteV3Frame), txOption);
							break;
							#endif
							
							#if CC_NOTIFICATION_WITH_ACCESS_CONTROL_ALARM
							case NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM:
								txBuf->ZW_EventSupportedReport1byteV3Frame.properties1 = 0x01; \
								txBuf->ZW_EventSupportedReport1byteV3Frame.bitMask1 =  
																#if CC_EVENT_WITH_MANUAL_LOCK
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_MANUAL_LOCK)
																#endif
																#if CC_EVENT_WITH_MANUAL_UNLOCK
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_MANUAL_UNLOCK)
																#endif
																#if CC_EVENT_WITH_MANUAL_NOT_FULLY_LOCKED
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_)
																#endif
																#if CC_EVENT_WITH_RF_LOCK
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_RF_LOCK)
																#endif
																#if CC_EVENT_WITH_RF_UNLOCK
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_RF_UNLOCK)
																#endif
																#if CC_EVENT_WITH_KEYPAD_LOCK
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_KEYPAD_LOCK)
																#endif
																#if CC_EVENT_WITH_KEYPAD_UNLOCK
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_KEYPAD_UNLOCK)
																#endif
																0;
							#if CC_EVENT_WITH_RF_NOT_FULLY_LOCKED || \
							CC_EVENT_WITH_AUTO_LOCKED || \
							CC_EVENT_WITH_AUTO_LOCK_NOT_FULLY_LOCKED || \
							CC_EVENT_WITH_LOCK_JAMMED || \
							CC_EVENT_WITH_ALL_USER_CODES_DELETED || \
							CC_EVENT_WITH_SINGLE_USER_CODE_DELETED || \
							CC_EVENT_WITH_NEW_USER_CODE_ADDED || \
							CC_EVENT_WITH_NEW_USER_CODE_NOT_ADDED || \
							CC_EVENT_WITH_KEYPAD_TEMPORARY_DISABLE || \
							CC_EVENT_WITH_KEYPAD_BUSY || \
							CC_EVENT_WITH_NEW_PROGRAM_CODE_ENTERED || \
							CC_EVENT_WITH_MANUAL_CODE_EXCEEDS_LIMITS || \
							CC_EVENT_WITH_RF_UNLOCK_INVALID_CODE || \
							CC_EVENT_WITH_RF_LOCK_INVALID_CODE || \
							CC_EVENT_WITH_WINDOW_DOOR_OPENED || \
							CC_EVENT_WITH_WINDOW_DOOR_CLOSED || \
							0
							txBuf->ZW_EventSupportedReport2byteV3Frame.bitMask2 = 
																#if CC_EVENT_WITH_RF_NOT_FULLY_LOCKED
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_RF_NOT_FULLY_LOCKED - 8)
																#endif
																#if CC_EVENT_WITH_AUTO_LOCKED
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_AUTO_LOCKED - 8)
																#endif
																#if CC_EVENT_WITH_AUTO_LOCK_NOT_FULLY_LOCKED
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_AUTO_LOCK_NOT_FULLY_LOCKED - 8)
																#endif
																#if CC_EVENT_WITH_LOCK_JAMMED
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_LOCK_JAMMED - 8)
																#endif
																#if CC_EVENT_WITH_ALL_USER_CODES_DELETED
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_ALL_USER_CODES_DELETED - 8)
																#endif
																#if CC_EVENT_WITH_SINGLE_USER_CODE_DELETED
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_SINGLE_USER_CODE_DELETED - 8)
																#endif
																#if CC_EVENT_WITH_NEW_USER_CODE_ADDED
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_NEW_USER_CODE_ADDED - 8)
																#endif
																#if CC_EVENT_WITH_NEW_USER_CODE_NOT_ADDED
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_NEW_USER_CODE_NOT_ADDED - 8)
																#endif
																0;
							#endif
							#if CC_EVENT_WITH_KEYPAD_TEMPORARY_DISABLE || \
							CC_EVENT_WITH_KEYPAD_BUSY || \
							CC_EVENT_WITH_NEW_PROGRAM_CODE_ENTERED || \
							CC_EVENT_WITH_MANUAL_CODE_EXCEEDS_LIMITS || \
							CC_EVENT_WITH_RF_UNLOCK_INVALID_CODE || \
							CC_EVENT_WITH_RF_LOCK_INVALID_CODE || \
							CC_EVENT_WITH_WINDOW_DOOR_OPENED || \
							CC_EVENT_WITH_WINDOW_DOOR_CLOSED || \
							0
							txBuf->ZW_EventSupportedReport3byteV3Frame.bitMask3 = 
																#if CC_EVENT_WITH_KEYPAD_TEMPORARY_DISABLE
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_KEYPAD_TEMPORARY_DISABLE - 16)
																#endif
																#if CC_EVENT_WITH_KEYPAD_BUSY
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_KEYPAD_BUSY - 16)
																#endif
																#if CC_EVENT_WITH_NEW_PROGRAM_CODE_ENTERED
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_NEW_PROGRAM_CODE_ENTERED - 16)
																#endif
																#if CC_EVENT_WITH_MANUAL_CODE_EXCEEDS_LIMITS
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_MANUAL_CODE_EXCEEDS_LIMITS - 16)
																#endif
																#if CC_EVENT_WITH_RF_UNLOCK_INVALID_CODE
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_RF_UNLOCK_INVALID_CODE - 16)
																#endif
																#if CC_EVENT_WITH_RF_LOCK_INVALID_CODE
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_RF_LOCK_INVALID_CODE - 16)
																#endif
																#if CC_EVENT_WITH_WINDOW_DOOR_CLOSED
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_ - 16)
																#endif
																#if CC_EVENT_WITH_WINDOW_DOOR_OPENED
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_ - 16)
																#endif
																0;		
							#endif
							#if CC_EVENT_WITH_KEYPAD_TEMPORARY_DISABLE || \
							CC_EVENT_WITH_KEYPAD_BUSY || \
							CC_EVENT_WITH_NEW_PROGRAM_CODE_ENTERED || \
							CC_EVENT_WITH_MANUAL_CODE_EXCEEDS_LIMITS || \
							CC_EVENT_WITH_RF_UNLOCK_INVALID_CODE || \
							CC_EVENT_WITH_RF_LOCK_INVALID_CODE || \
							CC_EVENT_WITH_WINDOW_DOOR_OPENED || \
							CC_EVENT_WITH_WINDOW_DOOR_CLOSED || \
							0
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_EventSupportedReport3byteV3Frame), txOption);
							#elif CC_EVENT_WITH_RF_NOT_FULLY_LOCKED || \
							CC_EVENT_WITH_AUTO_LOCKED || \
							CC_EVENT_WITH_AUTO_LOCK_NOT_FULLY_LOCKED || \
							CC_EVENT_WITH_LOCK_JAMMED || \
							CC_EVENT_WITH_ALL_USER_CODES_DELETED || \
							CC_EVENT_WITH_SINGLE_USER_CODE_DELETED || \
							CC_EVENT_WITH_NEW_USER_CODE_ADDED || \
							CC_EVENT_WITH_NEW_USER_CODE_NOT_ADDED || \
							0
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_EventSupportedReport2byteV3Frame), txOption);
							#else
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_EventSupportedReport1byteV3Frame), txOption);
							#endif
							break;
							#endif

							#if CC_NOTIFICATION_WITH_BURGLAR_ALARM
							case NOTIFICATION_TYPE_BURGLAR_ALARM:
								txBuf->ZW_EventSupportedReport1byteV3Frame.properties1 = 0x01; \
								txBuf->ZW_EventSupportedReport1byteV3Frame.bitMask1 = 
																#if CC_EVENT_WITH_INTRUSION
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_INTRUSION)
																#endif
																#if CC_EVENT_WITH_INTRUSION_UL
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_INTRUSION_UL)
																#endif
																#if CC_EVENT_WITH_TAMPER_REMOVED
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_TAMPER_REMOVED)
																#endif
																#if CC_EVENT_WITH_TAMPER_INVALID_CODE
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_TAMPER_INVALID_CODE)
																#endif
																#if CC_EVENT_WITH_GLASS_BREAK
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_GLASS_BREAK)
																#endif
																#if CC_EVENT_WITH_GLASS_BREAK_UL
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_GLASS_BREAK_UL)
																#endif
																#if CC_EVENT_WITH_MOTION_DETECTION
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_MOTION_DETECTION)
																#endif
																0;
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_EventSupportedReport1byteV3Frame), txOption);
							break;
							#endif

							#if CC_NOTIFICATION_WITH_POWER_MANAGEMENT_ALARM
							case NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM:
								txBuf->ZW_EventSupportedReport1byteV3Frame.properties1 = 0x01; \
								txBuf->ZW_EventSupportedReport1byteV3Frame.bitMask1 = 
																#if CC_EVENT_WITH_POWER_APPLIED
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_POWER_APPLIED)
																#endif
																#if CC_EVENT_WITH_AC_DISCONNECTED
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_AC_DISCONNECTED)
																#endif
																#if CC_EVENT_WITH_AC_RECONNECTED
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_AC_RECONNECTED)
																#endif
																#if CC_EVENT_WITH_SURGE
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_SURGE)
																#endif
																#if CC_EVENT_WITH_VOLTAGE_DROP
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_VOLTAGE_DROP)
																#endif
																#if CC_EVENT_WITH_OVER_CURRENT
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_OVER_CURRENT)
																#endif
																#if CC_EVENT_WITH_OVER_VOLTAGE
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_OVER_VOLTAGE)
																#endif
																0;
							#if CC_EVENT_WITH_OVER_LOAD || \
							CC_EVENT_WITH_LOAD_ERROR || \
							CC_EVENT_WITH_REPLACE_BAT_SOON || \
							CC_EVENT_WITH_REPLACE_BAT_NOW || \
							0
							txBuf->ZW_EventSupportedReport2byteV3Frame.bitMask2 = 
																#if CC_EVENT_WITH_OVER_LOAD
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_OVER_LOAD - 8)
																#endif
																#if CC_EVENT_WITH_LOAD_ERROR
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_LOAD_ERROR - 8)
																#endif
																#if CC_EVENT_WITH_REPLACE_BAT_SOON
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_REPLACE_BAT_SOON - 8)
																#endif
																#if CC_EVENT_WITH_REPLACE_BAT_NOW
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_REPLACE_BAT_NOW - 8)
																#endif
																0;
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_EventSupportedReport2byteV3Frame), txOption);
							#else
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_EventSupportedReport1byteV3Frame), txOption);									
							#endif
							break;
							#endif

							#if CC_NOTIFICATION_WITH_SYSTEM_ALARM
							case NOTIFICATION_TYPE_SYSTEM_ALARM:
								txBuf->ZW_EventSupportedReport1byteV3Frame.properties1 = 0x01; \
								txBuf->ZW_EventSupportedReport1byteV3Frame.bitMask1 = 
																#if CC_EVENT_WITH_HW_FAIL
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_HW_FAIL)
																#endif
																#if CC_EVENT_WITH_SW_FAIL
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_SW_FAIL)
																#endif
																#if CC_EVENT_WITH_HW_FAIL_OEM
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_HW_FAIL_OEM)
																#endif
																#if CC_EVENT_WITH_SW_FAIL_OEM
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_SW_FAIL_OEM)
																#endif
																0;
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_EventSupportedReport1byteV3Frame), txOption);
							break;
							#endif

							#if CC_NOTIFICATION_WITH_EMERGENCY_ALARM
							case NOTIFICATION_TYPE_EMERGENCY_ALARM:
								txBuf->ZW_EventSupportedReport1byteV3Frame.properties1 = 0x01; \
								txBuf->ZW_EventSupportedReport1byteV3Frame.bitMask1 = 
																#if CC_EVENT_WITH_CONTACT_POLICE
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_CONTACT_POLICE)
																#endif
																#if CC_EVENT_WITH_CONTACT_FIREMEN
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_CONTACT_FIREMEN)
																#endif
																#if CC_EVENT_WITH_CONTACT_MEDIC
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_CONTACT_MEDIC)
																#endif
																0;
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_EventSupportedReport1byteV3Frame), txOption);
							break;
							#endif
							
							#if CC_NOTIFICATION_WITH_CLOCK_ALARM
							case NOTIFICATION_TYPE_CLOCK_ALARM:
								txBuf->ZW_EventSupportedReport1byteV3Frame.properties1 = 0x01; \
								txBuf->ZW_EventSupportedReport1byteV3Frame.bitMask1 = 
																#if CC_EVENT_WITH_WAKE_UP_ALERT
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_WAKE_UP_ALERT)
																#endif
																#if CC_EVENT_WITH_TIMER_ENDED
																MAKE_NOTIFICATION_REPORT_BITMASK(NOTIFICATION_EVENT_TIMER_ENDED)
																#endif
																0;
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_EventSupportedReport1byteV3Frame), txOption);
							break;
							#endif

							default:
								txBuf->ZW_EventSupportedReport1byteV3Frame.properties1 = 0;//EVENT_UNKNOWN_BIT_PROPERITES;
								txBuf->ZW_EventSupportedReport1byteV3Frame.bitMask1 = 0;//EVENT_UNKNOWN_BIT_PROPERITES;
							break;
						}
						EnqueuePacket(sourceNode, sizeof(txBuf->ZW_EventSupportedReport1byteV3Frame), txOption);
					}
					break;
			}
			break;
		#endif

		#if WITH_CC_DOOR_LOCK
		case COMMAND_CLASS_DOOR_LOCK_V2:
			switch(cmd) {
				case DOOR_LOCK_OPERATION_SET_V2:
					pDoorLockData->mode = param1;
					VAR(DOOR_LOCK_TIMEOUT_FLAG) = FALSE; // Cancel Timer if any
					if (param1 == DOOR_LOCK_OPERATION_SET_DOOR_SECURED_V2) {
						pDoorLockData->condition &= DOOR_LOCK_CONDITION_BOLT_CLOSED_MASK;
					} else {
						pDoorLockData->condition |= DOOR_LOCK_CONDITION_BOLT_OPENED_MASK;
					}
					if (pDoorLockData->type == DOOR_OPERATION_TIMED) {
						switch (param1) {
							case DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED_WITH_TIMEOUT_V2:
								VAR(DOOR_LOCK_TIMEOUT_FLAG) |= DOOR_LOCK_TIMEOUT_ENABLE;
							break;

							case DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED_FOR_INSIDE_DOOR_HANDLES_WITH_TIMEOUT_V2:
								VAR(DOOR_LOCK_TIMEOUT_FLAG) |= DOOR_LOCK_INSIDE_HANDLES_TIMEOUT_ENABLE;
							break;

							case DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED_FOR_OUTSIDE_DOOR_HANDLES_WITH_TIMEOUT_V2:
								VAR(DOOR_LOCK_TIMEOUT_FLAG) |= DOOR_LOCK_OUTSIDE_HANDLES_TIMEOUT_ENABLE;
							break;

							default:
							break;
						}
						VAR(DOOR_LOCK_TIMEOUT_MINUTES) = pDoorLockData->lockTimeoutMin;
						VAR(DOOR_LOCK_TIMEOUT_SECONDS) = pDoorLockData->lockTimeoutSec;
						//ZW_SerialPutByte(VAR(DOOR_LOCK_TIMEOUT_FLAG));
						//ZW_SerialPutByte(VAR(DOOR_LOCK_TIMEOUT_MINUTES));
						//ZW_SerialPutByte(VAR(DOOR_LOCK_TIMEOUT_SECONDS));
					}
					DOOR_LOCK_OPERATION_SET_ACTION(param1);
					SAVE_EEPROM_ARRAY(DOOR_LOCK_DATA,DOOR_LOCK_DATA_SIZE);
					SAVE_EEPROM_BYTE(DOOR_LOCK_TIMEOUT_SECONDS);
					SAVE_EEPROM_BYTE(DOOR_LOCK_TIMEOUT_MINUTES);
				break;

				case DOOR_LOCK_OPERATION_GET_V2:
					sendDoorLockOperationReport(sourceNode);
				break;

				case DOOR_LOCK_CONFIGURATION_SET_V2:
					if ((param1 == DOOR_LOCK_CONFIGURATION_SET_CONSTANT_OPERATION) && (param3 == CONSTANT_OPERATION_TIME_VALUE) && (param4 == CONSTANT_OPERATION_TIME_VALUE)) {
						VAR(DOOR_LOCK_TIMEOUT_FLAG) = FALSE; // Cancel Timer if any
						pDoorLockData->type = param1;
						pDoorLockData->doorHandleMode = param2;
						pDoorLockData->lockTimeoutMin = param3;
						pDoorLockData->lockTimeoutSec = param4;
						SAVE_EEPROM_ARRAY(DOOR_LOCK_DATA,DOOR_LOCK_DATA_SIZE);
					} else {
						if ((param1 == DOOR_LOCK_CONFIGURATION_SET_TIMED_OPERATION) 
							&& ((param3 >= 0) && (param3 <= TIMED_OPERATION_MINUTES_MAX_VALUE)) 
							&& ((param4 >= 0) && (param4 <= TIMED_OPERATION_SECONDS_MAX_VALUE)))
						{
							pDoorLockData->type = param1;
							pDoorLockData->doorHandleMode = param2;
							pDoorLockData->lockTimeoutMin = param3;
							pDoorLockData->lockTimeoutSec = param4;
							SAVE_EEPROM_ARRAY(DOOR_LOCK_DATA,DOOR_LOCK_DATA_SIZE);
						}
					}
				break;

				case DOOR_LOCK_CONFIGURATION_GET_V2:
					if ((txBuf = CreatePacket()) != NULL) {
						txBuf->ZW_DoorLockConfigurationReportV2Frame.cmdClass = COMMAND_CLASS_DOOR_LOCK_V2;
						txBuf->ZW_DoorLockConfigurationReportV2Frame.cmd = DOOR_LOCK_CONFIGURATION_REPORT_V2;
						txBuf->ZW_DoorLockConfigurationReportV2Frame.operationType = pDoorLockData->type; 
						txBuf->ZW_DoorLockConfigurationReportV2Frame.properties1 = pDoorLockData->doorHandleMode;
						txBuf->ZW_DoorLockConfigurationReportV2Frame.lockTimeoutMinutes = pDoorLockData->lockTimeoutMin;
						txBuf->ZW_DoorLockConfigurationReportV2Frame.lockTimeoutSeconds = pDoorLockData->lockTimeoutSec;
						EnqueuePacket(sourceNode, sizeof(txBuf->ZW_DoorLockConfigurationReportV2Frame), txOption);
					}
				break;
			}
			break;
		#endif
	}
	
	MULTICHANNEL_ENCAP_DISABLE;
}

#define RECEIVE_STATUS_TYPE_LOCAL	0x80 // for MultiCmd packets

#if SECURITY_DEBUG
BOOL dbgSecurityEchoFilter(BYTE source, BYTE * d, BYTE cmdlen);
#endif


#ifdef ZW050x
void __ApplicationCommandHandler(BYTE rxStatus, BYTE sourceNode,  ZW_APPLICATION_TX_BUFFER *pCmd, BYTE cmdLength) reentrant;
#endif

#ifdef ZW_CONTROLLER 
void ApplicationCommandHandler(BYTE rxStatus, BYTE destNode, BYTE sourceNode, ZW_APPLICATION_TX_BUFFER *pCmd, BYTE cmdLength) {
	if(destNode == myNodeId)
		__ApplicationCommandHandler(rxStatus, sourceNode, pCmd, cmdLength);
}
#else 
void ApplicationCommandHandler(BYTE rxStatus, BYTE sourceNode, ZW_APPLICATION_TX_BUFFER *pCmd, BYTE cmdLength)  {
	#if PRODUCT_FLIRS_TYPE
	ShortKickSleep();
	#endif
	#ifdef ZW050x
	__ApplicationCommandHandler(rxStatus, sourceNode, pCmd, cmdLength);
}
#endif
#endif

#ifdef ZW050x
void __ApplicationCommandHandler(BYTE rxStatus, BYTE sourceNode, ZW_APPLICATION_TX_BUFFER *pCmd, BYTE cmdLength) reentrant {
	#endif
	BYTE classcmd = pCmd->ZW_Common.cmdClass;
	BYTE cmd = pCmd->ZW_Common.cmd;
	BYTE param1 = *((BYTE*)pCmd + OFFSET_PARAM_1);
	BYTE param2 = *((BYTE*)pCmd + OFFSET_PARAM_2);
	BYTE param3 = *((BYTE*)pCmd + OFFSET_PARAM_3); // needed for FREQ_CHANGE
	BYTE param4 = *((BYTE*)pCmd + OFFSET_PARAM_4);

	BYTE config_param_to_default;

	BYTE config_param_size;
	BYTE report_value_b;
	WORD report_value_w;
	DWORD report_value_dw;
	
 	BYTE txOption = ((rxStatus & RECEIVE_STATUS_LOW_POWER) ? TRANSMIT_OPTION_LOW_POWER : 0) | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE;
	
	#if SECURITY_DEBUG
		if(dbgSecurityEchoFilter(sourceNode, (BYTE*)pCmd, cmdLength))
			return; // Сообщение было обработано как тестовый запрос
	#endif
		
	#if WITH_CC_MULTICHANNEL
		#warning Check that the CC is in NIF (for channel = 0) or is supported by the channel (channel >= 1) before passing it to this handler
		MULTICHANNEL_SRC_END_POINT = 0;
		MULTICHANNEL_DST_END_POINT = 0;				
	#endif

	#if WITH_CC_SECURITY
		//#ifdef ZW_CONTROLLER
		//if()
		//#endif
		if (((rxStatus & RECEIVE_STATUS_SECURE_PACKET) == 0) && (SecurityIncomingUnsecurePacketsPolicy(classcmd) == PACKET_POLICY_DROP))
			return;
	#endif
	
	switch(classcmd) {
		case COMMAND_CLASS_MANUFACTURER_SPECIFIC:
			if (cmd == MANUFACTURER_SPECIFIC_GET) {
				if ((txBuf = CreatePacket()) != NULL) {
					txBuf->ZW_ManufacturerSpecificReportFrame.cmdClass = COMMAND_CLASS_MANUFACTURER_SPECIFIC;
					txBuf->ZW_ManufacturerSpecificReportFrame.cmd = MANUFACTURER_SPECIFIC_REPORT;
					txBuf->ZW_ManufacturerSpecificReportFrame.manufacturerId1 = _MANUFACTURER_ID_MAJOR;
					txBuf->ZW_ManufacturerSpecificReportFrame.manufacturerId2 = _MANUFACTURER_ID_MINOR;
					txBuf->ZW_ManufacturerSpecificReportFrame.productTypeId1 = _PRODUCT_TYPE_ID_MAJOR;
					txBuf->ZW_ManufacturerSpecificReportFrame.productTypeId2 = _PRODUCT_TYPE_ID_MINOR;
					txBuf->ZW_ManufacturerSpecificReportFrame.productId1 = _APP_VERSION_MAJOR_;
					txBuf->ZW_ManufacturerSpecificReportFrame.productId2 = _APP_VERSION_MINOR_;
					EnqueuePacket(sourceNode, sizeof(txBuf->ZW_ManufacturerSpecificReportFrame), txOption);
				}
			}
			break;
	
		#if WITH_PROPRIETARY_EXTENTIONS
		case COMMAND_CLASS_MANUFACTURER_PROPRIETARY:
			if (cmd == _MANUFACTURER_ID_MAJOR && param1 == _MANUFACTURER_ID_MINOR) {
				if (param2 == FUNC_ID_ZME_SWITCH_FREQ && 
					(	param3 == FREQ_EU || 
						param3 == FREQ_RU || 
						param3 == FREQ_IN ||
						param3 == FREQ_US ||
						param3 == FREQ_ANZ ||
						param3 == FREQ_HK ||
						param3 == FREQ_CN ||
						param3 == FREQ_JP ||
						param3 == FREQ_KR ||
						param3 == FREQ_IL ||
						param3 == FREQ_MY							
						)) {
					FreqChange(param3);	
				}
				if (param2 == FUNC_ID_ZME_REBOOT) {
					RebootChip();
				}
			
				// 0     1     2     3     4     5     6     7
				// CC    cmd   par1  par2  par3  par4  par5  par6
				// CC    manf1 manf2 func  addr1 addr2 val1  val2
				// pCmd	            pcmd+4      pCmd+6
				
				if (param2 == FUNC_ID_ZME_EEPROM_WRITE && cmdLength > 6) {
					register BYTE k;
					DWORD homeId;
					MemoryGetID(&homeId, &k); // k will contain nodeId just to pass something (NULL is not allowed for nodeId)
					if (homeId == 0x77777777) { // Make sure we are inside special 77777777 homeId network!
						for (k = 0; k < cmdLength - 6; k++) {
							ZME_PUT_BYTE(*((WORD*)((BYTE*)pCmd + 4)) + k, *((BYTE*)pCmd + 6 + k));
						}
					}
				}
			}
			break;
		#endif
	
		case COMMAND_CLASS_VERSION:
			switch (cmd) {
				case VERSION_GET:
				{
					BYTE cmd_size = sizeof(txBuf->ZW_VersionReportFrame);
					if ((txBuf = CreatePacket()) != NULL) {

						txBuf->ZW_VersionReportFrame.cmdClass = COMMAND_CLASS_VERSION;
						#if WITH_CC_VERSION_2
							txBuf->ZW_VersionReportFrame.cmd 	  = VERSION_REPORT_V2;
						#else
							txBuf->ZW_VersionReportFrame.cmd 	  = VERSION_REPORT;
						#endif
						txBuf->ZW_VersionReportFrame.zWaveLibraryType = ZW_TYPE_LIBRARY();
						#ifdef ZW050x
						{
							PROTOCOL_VERSION protocolVersion;
							ZW_GetProtocolVersion(&protocolVersion);
							txBuf->ZW_VersionReportFrame.zWaveProtocolVersion = protocolVersion.protocolVersionMajor;
							txBuf->ZW_VersionReportFrame.zWaveProtocolSubVersion = protocolVersion.protocolVersionMinor;
						}
						#else
						txBuf->ZW_VersionReportFrame.zWaveProtocolVersion = ZW_VERSION_MAJOR;
						txBuf->ZW_VersionReportFrame.zWaveProtocolSubVersion = ZW_VERSION_MINOR;
						#endif
						txBuf->ZW_VersionReportFrame.applicationVersion = _PRODUCT_FIRMWARE_VERSION_MAJOR;
						txBuf->ZW_VersionReportFrame.applicationSubVersion = _PRODUCT_FIRMWARE_VERSION_MINOR;
						
						#if WITH_CC_VERSION_2
						{
							BYTE i;
							BYTE *pBuff = (BYTE*)(&(txBuf->ZW_VersionReportFrame.applicationSubVersion));
							pBuff++;
							pBuff[0] = VERSION2_HARDWARE_VERSION;
							pBuff[1] = VERSION2_FIRMWARE_COUNT;
							cmd_size += 2;

							for (i = 0; i < VERSION2_FIRMWARE_COUNT; i++, pBuff += 2, cmd_size += 2) {
								pBuff[0] = VERSION2_FIRMWARE_VERSION(i);
								pBuff[1] = VERSION2_FIRMWARE_SUBVERSION(i);
							}
						}	
						#endif
						EnqueuePacket(sourceNode, cmd_size, txOption);
					}
				}
				break;

				case VERSION_COMMAND_CLASS_GET:
					if ((txBuf = CreatePacket()) != NULL) {
						txBuf->ZW_VersionCommandClassReportFrame.cmdClass = COMMAND_CLASS_VERSION;
						txBuf->ZW_VersionCommandClassReportFrame.cmd = VERSION_COMMAND_CLASS_REPORT;
						txBuf->ZW_VersionCommandClassReportFrame.requestedCommandClass = param1;
						switch (param1) {
							case COMMAND_CLASS_MANUFACTURER_SPECIFIC:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = MANUFACTURER_SPECIFIC_VERSION;
								break;

							case COMMAND_CLASS_VERSION:
								#if WITH_CC_VERSION_2
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = VERSION_VERSION_V2;
								#else
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = VERSION_VERSION;			
								#endif
								break;

							#if WITH_CC_BASIC
							case COMMAND_CLASS_BASIC:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = BASIC_VERSION;
								break;
							#endif	

							#if WITH_CC_SWITCH_BINARY
							case COMMAND_CLASS_SWITCH_BINARY:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = SWITCH_BINARY_VERSION;
								break;
							#endif	

							#if WITH_CC_SWITCH_MULTILEVEL
							case COMMAND_CLASS_SWITCH_MULTILEVEL:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = SWITCH_MULTILEVEL_VERSION_V3;
								break;
							#endif	

							#if WITH_CC_SENSOR_BINARY
							case COMMAND_CLASS_SENSOR_BINARY:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = SENSOR_BINARY_VERSION;
								break;
							#endif
							
							#if WITH_CC_SENSOR_BINARY_V2
							case COMMAND_CLASS_SENSOR_BINARY_V2:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = SENSOR_BINARY_VERSION_V2;
								break;
							#endif

							#if WITH_CC_NOTIFICATION
							case COMMAND_CLASS_NOTIFICATION_V4:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = NOTIFICATION_VERSION_V4;
								break;
							#endif
							
							#if WITH_CC_METER
							case COMMAND_CLASS_METER:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = METER_VERSION_V4; //METER_VERSION;
								break;
							#endif	
							
							#if WITH_CC_METER_TABLE_MONITOR
							case COMMAND_CLASS_METER_TBL_MONITOR:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = METER_TBL_MONITOR_VERSION;
								break;	
							#endif
							
							#if WITH_CC_MULTICHANNEL
							case COMMAND_CLASS_MULTI_CHANNEL_V3:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = MULTI_CHANNEL_VERSION_V3;
								break;
							#endif
							
							#if WITH_CC_TIME_PARAMETERS
							case COMMAND_CLASS_TIME_PARAMETERS:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = TIME_PARAMETERS_VERSION;
								break;
							#endif
							
							#if WITH_CC_SENSOR_MULTILEVEL_V4
							case COMMAND_CLASS_SENSOR_MULTILEVEL:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = SENSOR_MULTILEVEL_VERSION_V4;
								break;
							#endif

							#if WITH_CC_SENSOR_MULTILEVEL
							case COMMAND_CLASS_SENSOR_MULTILEVEL:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = SENSOR_MULTILEVEL_VERSION_V5;
								break;
							#endif

							#if WITH_CC_PROTECTION
							case COMMAND_CLASS_PROTECTION:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = PROTECTION_VERSION;
								break;
							#endif
	
							#if WITH_CC_THERMOSTAT_MODE
							case COMMAND_CLASS_THERMOSTAT_MODE:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = THERMOSTAT_MODE_VERSION_V3;
								break;
							#endif
	
							#if WITH_CC_THERMOSTAT_SETPOINT
							case COMMAND_CLASS_THERMOSTAT_SETPOINT:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = THERMOSTAT_SETPOINT_VERSION_V3;
								break;
							#endif
	
							#if WITH_CC_CONFIGURATION
							case COMMAND_CLASS_CONFIGURATION:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = CONFIGURATION_VERSION;
								break;
							#endif
	
							#if WITH_CC_ASSOCIATION
							case COMMAND_CLASS_ASSOCIATION:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = ASSOCIATION_VERSION_V2;
								break;
							#endif

							#if WITH_CC_SCENE_CONTROLLER_CONF
							case COMMAND_CLASS_SCENE_CONTROLLER_CONF:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = SCENE_CONTROLLER_CONF_VERSION;
								break;
							#endif

							#if WITH_CC_SCENE_ACTUATOR_CONF
							case COMMAND_CLASS_SCENE_ACTUATOR_CONF:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = SCENE_ACTUATOR_CONF_VERSION;
								break;
							#endif

							#if WITH_CC_SCENE_ACTIVATION
							case COMMAND_CLASS_SCENE_ACTIVATION:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = SCENE_ACTIVATION_VERSION;
								break;
							#endif

							#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
							case COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = MULTI_CHANNEL_ASSOCIATION_VERSION_V2;
								break;
							#endif
	
							#if WITH_CC_NODE_NAMING
							case COMMAND_CLASS_NODE_NAMING:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = NODE_NAMING_VERSION;
								break;
							#endif

							#if WITH_CC_INDICATOR
							case COMMAND_CLASS_INDICATOR:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = INDICATOR_VERSION;
								break;
							#endif

							#if WITH_CC_BATTERY
							case COMMAND_CLASS_BATTERY:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = BATTERY_VERSION;
								break;
							#endif

							#if WITH_CC_WAKE_UP
							case COMMAND_CLASS_WAKE_UP:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = WAKE_UP_VERSION_V2;
								break;
							#endif

							#if WITH_CC_SECURITY
							case COMMAND_CLASS_SECURITY:
								if (VAR(SECURITY_MODE) == SECURITY_MODE_DISABLED) {
									txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = UNKNOWN_VERSION;
								} else {
									txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = SECURITY_VERSION;
								}
								break;
							#endif
							#if WITH_CC_MULTI_COMMAND
							case COMMAND_CLASS_MULTI_CMD:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = MULTI_CMD_VERSION;
								break;
							#endif
							#if WITH_CC_FIRMWARE_UPGRADE
  								case COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2:
									txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = FIRMWARE_UPDATE_MD_VERSION_V2;
									break;
							#endif
																						
  							//#endif
							#if WITH_CC_ZWPLUS_INFO
							case COMMAND_CLASS_ZWAVEPLUS_INFO_V2:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = ZWAVEPLUS_INFO_VERSION_V2;
								break;	
							#endif
							
							#if WITH_PROPRIETARY_EXTENTIONS_IN_NIF
							case COMMAND_CLASS_MANUFACTURER_PROPRIETARY:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = MANUFACTURER_PROPRIETARY_VERSION;
								break;
							#endif

							#if WITH_CC_DEVICE_RESET_LOCALLY
							case COMMAND_CLASS_DEVICE_RESET_LOCALLY:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = DEVICE_RESET_LOCALLY_VERSION;
								break;
							#endif	
							
							#if WITH_CC_ASSOCIATION_GROUP_INFORMATION
							case COMMAND_CLASS_ASSOCIATION_GRP_INFO:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = ASSOCIATION_GRP_INFO_VERSION;
								break;
							#endif	
							#if WITH_CC_CENTRAL_SCENE
							case COMMAND_CLASS_CENTRAL_SCENE:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = CENTRAL_SCENE_VERSION;
								break;
							#endif
							#if WITH_CC_POWERLEVEL
							case COMMAND_CLASS_POWERLEVEL:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = POWERLEVEL_VERSION;
								break;
							#endif
							#if WITH_CC_DOOR_LOCK
							case COMMAND_CLASS_DOOR_LOCK_V2:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = DOOR_LOCK_VERSION_V2;
								break;
							#endif
							#if WITH_CC_DOOR_LOCK_LOGGING
							case COMMAND_CLASS_DOOR_LOCK_LOGGING:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = DOOR_LOCK_LOGGING_VERSION;
								break;
							#endif

							#if WITH_CC_SWITCH_ALL
							case COMMAND_CLASS_SWITCH_ALL:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = SWITCH_ALL_VERSION;
								break;
							#endif
							/*
							#if WITH_CC_
							case COMMAND_CLASS_:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = _VERSION;
								break;
							#endif
							*/

							default:
								txBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = UNKNOWN_VERSION;
								break;
						}
						EnqueuePacket(sourceNode, sizeof(txBuf->ZW_VersionCommandClassReportFrame), txOption);
					}
					break;
			}
			break;

		//SWITCH_ALL
		#if WITH_CC_SWITCH_ALL
		case COMMAND_CLASS_SWITCH_ALL:
			switch(cmd) {
				case SWITCH_ALL_ON:
					if ((VAR(SWITCH_ALL_MODE) == SWITCH_ALL_REPORT_INCLUDED_IN_THE_ALL_ON_ALL_OFF_FUNCTIONALITY) || (VAR(SWITCH_ALL_MODE) == SWITCH_ALL_REPORT_EXCLUDED_FROM_THE_ALL_OFF_FUNCTIONALITY_BUT_NOT_ALL_ON))	{
						SWITCH_ALL_ON_ACTION()
					}
					break;

				case SWITCH_ALL_OFF:
					if ((VAR(SWITCH_ALL_MODE) == SWITCH_ALL_REPORT_INCLUDED_IN_THE_ALL_ON_ALL_OFF_FUNCTIONALITY) || (VAR(SWITCH_ALL_MODE) == SWITCH_ALL_REPORT_EXCLUDED_FROM_THE_ALL_ON_FUNCTIONALITY_BUT_NOT_ALL_OFF)) {
						SWITCH_ALL_OFF_ACTION()
					}						
					break;

				case SWITCH_ALL_SET:
					VAR(SWITCH_ALL_MODE) = param1;
					SAVE_EEPROM_BYTE(SWITCH_ALL_MODE);
					break;

				case SWITCH_ALL_GET:
					SendReportToNode(sourceNode, COMMAND_CLASS_SWITCH_ALL, VAR(SWITCH_ALL_MODE));
					break;
			}
			break;
		#endif
			
		#if WITH_CC_SCENE_ACTUATOR_CONF
		case COMMAND_CLASS_SCENE_ACTUATOR_CONF:
			switch (cmd) {
				case SCENE_ACTUATOR_CONF_GET:
					if (param1 == 0)
						param1 = scenesAct[0]; // return info about current scene
					if ((txBuf = CreatePacket()) != NULL) {
						txBuf->ZW_SceneActuatorConfReportFrame.cmdClass = COMMAND_CLASS_SCENE_ACTUATOR_CONF;
						txBuf->ZW_SceneActuatorConfReportFrame.cmd = SCENE_ACTUATOR_CONF_REPORT;
						txBuf->ZW_SceneActuatorConfReportFrame.sceneId = param1;
						txBuf->ZW_SceneActuatorConfReportFrame.level = scenesAct[param1]; // this is OK even if current scene == scene[0] == 0 (no active scene)
						txBuf->ZW_SceneActuatorConfReportFrame.dimmingDuration = 0xFF;
						EnqueuePacket(sourceNode, sizeof(txBuf->ZW_SceneActuatorConfReportFrame), txOption);
					}
					break;

				case SCENE_ACTUATOR_CONF_SET:
					// this device type should ignore Dimming Duration field
					if (param1 != 0) {
						register BYTE lvl = (param3 & SCENE_ACTUATOR_CONF_SET_LEVEL2_OVERRIDE_BIT_MASK) ? param4 : CURRENT_ACTUATOR_LEVEL();
						scenesAct[param1] = lvl;
						ZW_MEM_PUT_BYTE(EE(SCENE_ACTUATOR_CONF_LEVEL) + (WORD)param1, lvl);
					}
					break;
			}
			break;
		#endif

		#if WITH_CC_SCENE_ACTIVATION
		case COMMAND_CLASS_SCENE_ACTIVATION:
			if (cmd == SCENE_ACTIVATION_SET && param1 != 0) {
				#if WITH_CC_SCENE_ACTUATOR_CONF
				scenesAct[0] = param1;
				ZW_MEM_PUT_BYTE(EE(SCENE_ACTUATOR_CONF_LEVEL) + (WORD)0, param1);
				SCENE_ACTIVATION_ACTION(scenesAct[param1]);
				#else
				SCENE_ACTIVATION_RAW_ACTION(param1);
				#endif
			}
			break;
		#endif

		#if WITH_CC_SCENE_CONTROLLER_CONF
		case COMMAND_CLASS_SCENE_CONTROLLER_CONF:
			switch (cmd) {
				case SCENE_CONTROLLER_CONF_SET:
					if (param1 > 0 && param1 <= ASSOCIATION_NUM_GROUPS) {
						param1--; // group index starts from 0, while association group from 1
						scenesCtrl[param1].sceneID = param2;
						scenesCtrl[param1].dimmingDuration = param3;
						ZW_MEM_PUT_BYTE(EE(SCENE_CONTROLLER_CONF) + param1 * SCENE_HOLDER_SIZE    , scenesCtrl[param1].sceneID);
						ZW_MEM_PUT_BYTE(EE(SCENE_CONTROLLER_CONF) + param1 * SCENE_HOLDER_SIZE + 1, scenesCtrl[param1].dimmingDuration);
					}
					break;
				case SCENE_CONTROLLER_CONF_GET:
					if (param1 == 0 || param1 > ASSOCIATION_NUM_GROUPS)
						break;
					if ((txBuf = CreatePacket()) != NULL) {
						txBuf->ZW_SceneControllerConfReportFrame.cmdClass = COMMAND_CLASS_SCENE_CONTROLLER_CONF;
						txBuf->ZW_SceneControllerConfReportFrame.cmd = SCENE_CONTROLLER_CONF_REPORT;
						txBuf->ZW_SceneControllerConfReportFrame.groupId = param1;
						param1--; // group index starts from 0, while association group from 1
						txBuf->ZW_SceneControllerConfReportFrame.sceneId = scenesCtrl[param1].sceneID;
						txBuf->ZW_SceneControllerConfReportFrame.dimmingDuration = scenesCtrl[param1].dimmingDuration;
						EnqueuePacket(sourceNode, sizeof(txBuf->ZW_SceneControllerConfReportFrame), txOption);
					}
					break;
			}
			break;
		#endif

		#if WITH_CC_PROTECTION
		case COMMAND_CLASS_PROTECTION: // Version 1
			switch(cmd) {
				case PROTECTION_SET:
					if (param1 == PROTECTION_SET_UNPROTECTED || param1 == PROTECTION_SET_PROTECTION_BY_SEQUENCE || param1 == PROTECTION_SET_NO_OPERATION_POSSIBLE) {
						VAR(PROTECTION) = param1;
						SAVE_EEPROM_BYTE(PROTECTION);

						// stop timer if any
						protectionSecondsDoCount = FALSE;

						// lock immediatelly if protection is enabled
						if (VAR(PROTECTION) != PROTECTION_SET_UNPROTECTED) 
						{
							protectedUnlocked = FALSE;
						}
					}
					break;

				case PROTECTION_GET:
					SendReportToNode(sourceNode, COMMAND_CLASS_PROTECTION, VAR(PROTECTION));
					break;
			}
			break;
		#endif

	        #if WITH_CC_WAKE_UP
	        // WAKE_UP
	        case COMMAND_CLASS_WAKE_UP:
	        	switch (cmd) {
	        	        case WAKE_UP_INTERVAL_SET:
							VAR(WAKEUP_NODEID) = param4;
	        	        	VAR(SLEEP_PERIOD + 1) = param1;
	        	        	VAR(SLEEP_PERIOD + 2) = param2;
	        	        	VAR(SLEEP_PERIOD + 3) = param3;
	        	        	SetWakeupPeriod(); // this function can alter sleepSeconds*
	        		        break;
	        	
	        	        case WAKE_UP_INTERVAL_GET:
					if ((txBuf = CreatePacket()) != NULL) {
						txBuf->ZW_WakeUpIntervalReportFrame.cmdClass = COMMAND_CLASS_WAKE_UP;
	        	        		txBuf->ZW_WakeUpIntervalReportFrame.cmd = WAKE_UP_INTERVAL_REPORT;
	        	        		txBuf->ZW_WakeUpIntervalReportFrame.seconds1 = VAR(SLEEP_PERIOD + 1);
		        	        	txBuf->ZW_WakeUpIntervalReportFrame.seconds2 = VAR(SLEEP_PERIOD + 2);
		        	        	txBuf->ZW_WakeUpIntervalReportFrame.seconds3 = VAR(SLEEP_PERIOD + 3);
	        		        	txBuf->ZW_WakeUpIntervalReportFrame.nodeid = VAR(WAKEUP_NODEID);
	        	        		EnqueuePacket(sourceNode, sizeof(txBuf->ZW_WakeUpIntervalReportFrame), txOption);
					}
		        	        break;

	        	        case WAKE_UP_NO_MORE_INFORMATION:
	        	        	ReleaseeSleepLockWithTimer();
	        	        	// We don't want to run KickSleep after this packet, so we just return instead of break!
	        	        	return;
	        	        	// break;

	        	        case WAKE_UP_INTERVAL_CAPABILITIES_GET_V2:
					if ((txBuf = CreatePacket()) != NULL) {
		        	        	txBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.cmdClass = COMMAND_CLASS_WAKE_UP;
		        	        	txBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.cmd = WAKE_UP_INTERVAL_CAPABILITIES_REPORT_V2;
		        	        	txBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.minimumWakeUpIntervalSeconds1 = MINIMUM_SLEEP_SECONDS_MSB;
		        	        	txBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.minimumWakeUpIntervalSeconds2 = MINIMUM_SLEEP_SECONDS_M;  
		        	        	txBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.minimumWakeUpIntervalSeconds3 = MINIMUM_SLEEP_SECONDS_LSB;
		        	        	txBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.maximumWakeUpIntervalSeconds1 = MAXIMUM_SLEEP_SECONDS_MSB;
		        	        	txBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.maximumWakeUpIntervalSeconds2 = MAXIMUM_SLEEP_SECONDS_M;  
		        	        	txBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.maximumWakeUpIntervalSeconds3 = MAXIMUM_SLEEP_SECONDS_LSB;
		        	        	txBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.defaultWakeUpIntervalSeconds1 = DEFAULT_SLEEP_SECONDS_MSB;
		        	        	txBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.defaultWakeUpIntervalSeconds2 = DEFAULT_SLEEP_SECONDS_M;  
		        	        	txBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.defaultWakeUpIntervalSeconds3 = DEFAULT_SLEEP_SECONDS_LSB;
		        	        	txBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.wakeUpIntervalStepSeconds1 = STEP_SLEEP_SECONDS_MSB;
		        	        	txBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.wakeUpIntervalStepSeconds2 = STEP_SLEEP_SECONDS_M;  
		        	        	txBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.wakeUpIntervalStepSeconds3 = STEP_SLEEP_SECONDS_LSB;
		        	        	EnqueuePacket(sourceNode, sizeof(txBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame), txOption);
						}
		        	        break;
	        	}
	        	break;   
	        #endif   

		#if WITH_CC_BATTERY
		case COMMAND_CLASS_BATTERY:
			if (cmd == BATTERY_GET) {
				SendReportToNode(sourceNode, COMMAND_CLASS_BATTERY, BATTERY_LEVEL_GET);
			}
			break;
		#endif
				
		#if WITH_CC_METER_TABLE_MONITOR
			case COMMAND_CLASS_METER_TBL_MONITOR:
				{
					switch(cmd)
					{
					
						
						case METER_TBL_TABLE_POINT_ADM_NO_GET: 
						{
							if ((txBuf = CreatePacket()) == NULL)
								return;
							txBuf->ZW_MeterTblTablePointAdmNoReport1byteFrame.cmdClass 						= COMMAND_CLASS_METER_TBL_MONITOR;
							txBuf->ZW_MeterTblTablePointAdmNoReport1byteFrame.cmd	   						= METER_TBL_TABLE_POINT_ADM_NO_REPORT;
							txBuf->ZW_MeterTblTablePointAdmNoReport1byteFrame.properties1 					= 0x01; 
							txBuf->ZW_MeterTblTablePointAdmNoReport1byteFrame.meterPointAdmNumberCharacter1 	= '0';
	
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_MeterTblTablePointAdmNoReport1byteFrame), TRANSMIT_OPTION_USE_DEFAULT);	

						}
						break;
						case METER_TBL_TABLE_ID_GET:
						{
							if ((txBuf = CreatePacket()) == NULL)
								return;
							txBuf->ZW_MeterTblTableIdReport1byteFrame.cmdClass 			= COMMAND_CLASS_METER_TBL_MONITOR;
							txBuf->ZW_MeterTblTableIdReport1byteFrame.cmd	   			= METER_TBL_TABLE_ID_REPORT;
							txBuf->ZW_MeterTblTableIdReport1byteFrame.properties1 		= 0x01; 
							txBuf->ZW_MeterTblTableIdReport1byteFrame.meterIdCharacter1 = '0';
	
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_MeterTblTableIdReport1byteFrame), TRANSMIT_OPTION_USE_DEFAULT);	
						}
						
						break;
						case METER_TBL_TABLE_CAPABILITY_GET:
						{
							if ((txBuf = CreatePacket()) == NULL)
								return;
								 
							txBuf->ZW_MeterTblReportFrame.cmdClass 			=	COMMAND_CLASS_METER_TBL_MONITOR;
							txBuf->ZW_MeterTblReportFrame.cmd	  			=	METER_TBL_REPORT;
							txBuf->ZW_MeterTblReportFrame.properties1		=	(METER_TBL_CAP_RATE_TYPE << 6) | METER_TBL_CAP_METER_TYPE;
							txBuf->ZW_MeterTblReportFrame.properties2		=	METER_TBL_CAP_METER_PAY;
							txBuf->ZW_MeterTblReportFrame.datasetSupported1	=	METER_TBL_CAP_DATASET1;
							txBuf->ZW_MeterTblReportFrame.datasetSupported2	=	METER_TBL_CAP_DATASET2;
							txBuf->ZW_MeterTblReportFrame.datasetSupported3	=	METER_TBL_CAP_DATASET3;
	
							txBuf->ZW_MeterTblReportFrame.datasetHistorySupported1	=	METER_TBL_CAP_DATASET1;
							txBuf->ZW_MeterTblReportFrame.datasetHistorySupported2	=	METER_TBL_CAP_DATASET2;
							txBuf->ZW_MeterTblReportFrame.datasetHistorySupported3	=	METER_TBL_CAP_DATASET3;
	
							FILL_MULTIBYTE_PARAM3(txBuf->ZW_MeterTblReportFrame.dataHistorySupported, METER_TBL_CAP_HISTORY_SIZE);
		
	
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_MeterTblReportFrame), TRANSMIT_OPTION_USE_DEFAULT);
						}
						break;
						case METER_TBL_STATUS_SUPPORTED_GET:   // =0 dummy
						{
							if ((txBuf = CreatePacket()) == NULL)
								return; 
			
							txBuf->ZW_MeterTblStatusSupportedReportFrame.cmdClass = COMMAND_CLASS_METER_TBL_MONITOR;
							txBuf->ZW_MeterTblStatusSupportedReportFrame.cmd	  = METER_TBL_STATUS_SUPPORTED_REPORT;
	
							txBuf->ZW_MeterTblStatusSupportedReportFrame.supportedOperatingStatus1	  = 0;
							txBuf->ZW_MeterTblStatusSupportedReportFrame.supportedOperatingStatus2	  = 0;
							txBuf->ZW_MeterTblStatusSupportedReportFrame.supportedOperatingStatus3	  = 0;
	
							txBuf->ZW_MeterTblStatusSupportedReportFrame.statusEventLogDepth		  = 0;	
			
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_MeterTblStatusSupportedReportFrame), TRANSMIT_OPTION_USE_DEFAULT);
						}
						break;
						case METER_TBL_STATUS_DEPTH_GET:
						case METER_TBL_STATUS_DATE_GET:
						{
							if ((txBuf = CreatePacket()) == NULL)
								return; 
			
							txBuf->ZW_MeterTblStatusReport1byteFrame.cmdClass 			=	COMMAND_CLASS_METER_TBL_MONITOR;
							txBuf->ZW_MeterTblStatusReport1byteFrame.cmd	  			=	METER_TBL_STATUS_REPORT;
							txBuf->ZW_MeterTblStatusReport1byteFrame.reportsToFollow	=	0;
	
							txBuf->ZW_MeterTblStatusReport1byteFrame.currentOperatingStatus1	  = 0;
							txBuf->ZW_MeterTblStatusReport1byteFrame.currentOperatingStatus2	  = 0;
							txBuf->ZW_MeterTblStatusReport1byteFrame.currentOperatingStatus3	  = 0;
	
							txBuf->ZW_MeterTblStatusSupportedReportFrame.statusEventLogDepth		  = 0;	
			
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_MeterTblStatusReport1byteFrame), TRANSMIT_OPTION_USE_DEFAULT);
						}
						break;
						
						case METER_TBL_CURRENT_DATA_GET:
						{
							METER_TBL_DATA_HANDLER(sourceNode, ((DWORD)param1 << 16)|((DWORD)param2 << 8)|(param3));
						}
						break;
						case METER_TBL_HISTORICAL_DATA_GET:
						{
							
							
							METER_TBL_DATA_HISTORICAL_HANDLER( sourceNode,
															  param1, /* maximum reports */
															  ((DWORD)param2 << 16) | ((DWORD)param3 << 8) | (param4), /* dataset */
															  /* start */
															  makeTimeStamp(*((WORD*)((BYTE*)pCmd+ OFFSET_PARAM_4 + 1)), /* year*/
															  				*((BYTE*)pCmd + OFFSET_PARAM_4 + 3), /* month */
															  				*((BYTE*)pCmd + OFFSET_PARAM_4 + 4), /* day */
															  				*((BYTE*)pCmd + OFFSET_PARAM_4 + 5), /* hour */
															  				*((BYTE*)pCmd + OFFSET_PARAM_4 + 6), /* minute */
															  				*((BYTE*)pCmd + OFFSET_PARAM_4 + 7)), /* second */
															  /* stop */
															  makeTimeStamp(*((WORD*)((BYTE*)pCmd+ OFFSET_PARAM_4 + 8)), /* year*/
															  				*((BYTE*)pCmd + OFFSET_PARAM_4 + 10), /* month */
															  				*((BYTE*)pCmd + OFFSET_PARAM_4 + 11), /* day */
															  				*((BYTE*)pCmd + OFFSET_PARAM_4 + 12), /* hour */
															  				*((BYTE*)pCmd + OFFSET_PARAM_4 + 13), /* minute */
															  				*((BYTE*)pCmd + OFFSET_PARAM_4 + 14)) /* second */								
															  );
						}
						break;				
					}
				}
			break;
		#endif
		
		#if WITH_CC_MULTICHANNEL
			case COMMAND_CLASS_MULTI_CHANNEL_V3:
			{
					switch(cmd)
					{
						case MULTI_CHANNEL_END_POINT_GET_V3: 
						{
							if ((txBuf = CreatePacket()) == NULL)
								return;
							txBuf->ZW_MultiChannelEndPointReportV3Frame.cmdClass 						= COMMAND_CLASS_MULTI_CHANNEL_V3;
							txBuf->ZW_MultiChannelEndPointReportV3Frame.cmd	   							= MULTI_CHANNEL_END_POINT_REPORT_V3;
							txBuf->ZW_MultiChannelEndPointReportV3Frame.properties1 					= ((MULTICHANNEL_END_POINT_DYNAMIC & 0x01) << 7) | ((MULTICHANNEL_END_POINT_IDENTICAL & 0x01) << 6); 
							txBuf->ZW_MultiChannelEndPointReportV3Frame.properties2 					= MULTICHANNEL_END_POINT_COUNT & 0x7F;
	
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_MultiChannelEndPointReportV3Frame), TRANSMIT_OPTION_USE_DEFAULT);	
						}
						break;
						case MULTI_CHANNEL_CAPABILITY_GET_V3:
						{
							BYTE packet_size, i = 0;
							if ((txBuf = CreatePacket()) == NULL)
								return;
							txBuf->ZW_MultiChannelCapabilityReport1byteV3Frame.cmdClass 			= COMMAND_CLASS_MULTI_CHANNEL_V3;
							txBuf->ZW_MultiChannelCapabilityReport1byteV3Frame.cmd	   				= MULTI_CHANNEL_CAPABILITY_REPORT_V3;
							txBuf->ZW_MultiChannelCapabilityReport1byteV3Frame.properties1 			= param1; 
							txBuf->ZW_MultiChannelCapabilityReport1byteV3Frame.genericDeviceClass	= MULTICHANNEL_CAPS_GENERIC_CLASS(param1);
							txBuf->ZW_MultiChannelCapabilityReport1byteV3Frame.specificDeviceClass	= MULTICHANNEL_CAPS_SPECIFIC_CLASS(param1);
/*
							for (; i < MULTICHANNEL_CAPS_CC_COUNT(param1); i++)
								*(&(txBuf->ZW_MultiChannelCapabilityReport1byteV3Frame.commandClass1) + i)	= MULTICHANNEL_CAPS_CC(param1, i);
							packet_size = sizeof(txBuf->ZW_MultiChannelCapabilityReport1byteV3Frame) + i;
*/
							
							switch(MULTICHANNEL_CAPS_CC_COUNT(param1))
							{
								case 1:
								txBuf->ZW_MultiChannelCapabilityReport1byteV3Frame.commandClass1	= MULTICHANNEL_CAPS_CC(param1, 0);	
								packet_size = sizeof(txBuf->ZW_MultiChannelCapabilityReport1byteV3Frame);
								break;
								case 2:
								txBuf->ZW_MultiChannelCapabilityReport2byteV3Frame.commandClass1	= MULTICHANNEL_CAPS_CC(param1, 0);
								txBuf->ZW_MultiChannelCapabilityReport2byteV3Frame.commandClass2	= MULTICHANNEL_CAPS_CC(param1, 1);		
								packet_size = sizeof(txBuf->ZW_MultiChannelCapabilityReport2byteV3Frame);
								break;
								case 3:
								txBuf->ZW_MultiChannelCapabilityReport3byteV3Frame.commandClass1	= MULTICHANNEL_CAPS_CC(param1, 0);
								txBuf->ZW_MultiChannelCapabilityReport3byteV3Frame.commandClass2	= MULTICHANNEL_CAPS_CC(param1, 1);
								txBuf->ZW_MultiChannelCapabilityReport3byteV3Frame.commandClass3	= MULTICHANNEL_CAPS_CC(param1, 2);
								packet_size = sizeof(txBuf->ZW_MultiChannelCapabilityReport3byteV3Frame);
								break;
								case 4:
								txBuf->ZW_MultiChannelCapabilityReport4byteV3Frame.commandClass1	= MULTICHANNEL_CAPS_CC(param1, 0);
								txBuf->ZW_MultiChannelCapabilityReport4byteV3Frame.commandClass2	= MULTICHANNEL_CAPS_CC(param1, 1);
								txBuf->ZW_MultiChannelCapabilityReport4byteV3Frame.commandClass3	= MULTICHANNEL_CAPS_CC(param1, 2);
								txBuf->ZW_MultiChannelCapabilityReport4byteV3Frame.commandClass4	= MULTICHANNEL_CAPS_CC(param1, 3);
								packet_size = sizeof(txBuf->ZW_MultiChannelCapabilityReport4byteV3Frame);
								break;
							}
							
							EnqueuePacket(sourceNode, packet_size, TRANSMIT_OPTION_USE_DEFAULT);	
						}	
						break;
						case MULTI_CHANNEL_END_POINT_FIND_V3: {
							BYTE i, len;
							BYTE first=0, count=0; // Мы предполагаем, что конечные точки одного класса идут последовательно друг за другом
							
							for(i=0; i<MULTICHANNEL_END_POINT_COUNT; i++)
									if((param1  == MULTICHANNEL_CAPS_GENERIC_CLASS(i)) && (param2  == MULTICHANNEL_CAPS_SPECIFIC_CLASS(i)))
									count++;
							
							do {
							if ((txBuf = CreatePacket()) == NULL)
								return;
								 
							txBuf->ZW_MultiChannelEndPointFindReport1byteV3Frame.cmdClass 			=	COMMAND_CLASS_MULTI_CHANNEL_V3;
							txBuf->ZW_MultiChannelEndPointFindReport1byteV3Frame.cmd	  			=	MULTI_CHANNEL_END_POINT_FIND_REPORT_V3;
								txBuf->ZW_MultiChannelEndPointFindReport1byteV3Frame.reportsToFollow	= 	count / MULTI_CHANNEL_END_POINT_FIND_V3_REPORT_MAX_SIZE;
								
							txBuf->ZW_MultiChannelEndPointFindReport1byteV3Frame.genericDeviceClass		=	param1;
							txBuf->ZW_MultiChannelEndPointFindReport1byteV3Frame.specificDeviceClass	=	param2;
							
							// Такого класса устройство вообще не поддерживает
								txBuf->ZW_MultiChannelEndPointFindReport1byteV3Frame.variantgroup1.properties1  =	0;


								len = 0;
								for(i = first; i < MULTICHANNEL_END_POINT_COUNT; i++) {
									if(len + 2 > MULTI_CHANNEL_END_POINT_FIND_V3_REPORT_MAX_SIZE)
										break;

									if ((param1  == MULTICHANNEL_CAPS_GENERIC_CLASS(i)) && (param2  == MULTICHANNEL_CAPS_SPECIFIC_CLASS(i))) {
										*(&(txBuf->ZW_MultiChannelEndPointFindReport1byteV3Frame.variantgroup1.properties1) + len) = i;
										len++;
								count--;
									}
								}

								EnqueuePacket(sourceNode, sizeof(txBuf->ZW_MultiChannelEndPointFindReport1byteV3Frame) + len, TRANSMIT_OPTION_USE_DEFAULT);
							} while (count > 0);
						}
						break;
						case MULTI_CHANNEL_CMD_ENCAP_V3:   
						{
							BYTE * npCmd  = ((BYTE*)pCmd) + 4;
							
							MULTICHANNEL_SRC_END_POINT = param1 & MULTI_CHANNEL_CMD_ENCAP_PROPERTIES1_SOURCE_END_POINT_MASK_V2;
							MULTICHANNEL_DST_END_POINT = param2 & MULTI_CHANNEL_CMD_ENCAP_PROPERTIES2_DESTINATION_END_POINT_MASK_V2;
							
							if ((txBuf = CreatePacket()) == NULL)
								return; 
								
							if(param2 & MULTI_CHANNEL_CMD_ENCAP_PROPERTIES2_BIT_ADDRESS_BIT_MASK_V2) { // Битовая адресация
								BYTE i;
								for(i=0;i<7;i++) {
									if(param2 & (1<<i)) {
										MULTICHANNEL_DST_END_POINT = i + 1;
										MultichannelCommandHandler(sourceNode, (ZW_APPLICATION_TX_BUFFER *)npCmd, cmdLength-4);
									}
								}
							}
							else
								MultichannelCommandHandler(sourceNode, (ZW_APPLICATION_TX_BUFFER *)npCmd, cmdLength-4);
								
			
						}
						break;
						
					}
		}
		#endif
		
		#if WITH_CC_TIME_PARAMETERS
		case COMMAND_CLASS_TIME_PARAMETERS:
		{
			TIME_PARAMETERS_DATA * p_TimeData = (TIME_PARAMETERS_DATA *)((BYTE*)pCmd + OFFSET_PARAM_1);
			switch(cmd){
				case TIME_PARAMETERS_SET:
					//We give you the pointer to structure (defined in Common.h). Please use it to get the data.
					//Be in a hurry or coppy, because the data may vanish from there.
					TIMEPARAMETERS_SET(p_TimeData);
				break;

				case TIME_PARAMETERS_GET:
					//Please return us the pointer to structure (as the p_TimeData above) 
					sendTimeParametersReport(sourceNode, TIMEPARAMETERS_GET_DATA_POINTER);
				break;
				case TIME_PARAMETERS_REPORT:
					//We give you the pointer to structure (defined in Common.h). Please use it to get the data.
					//Be in a hurry or coppy, because the data may vanish from there.
					TIMEPARAMETERS_INCOMING_REPORT(p_TimeData);
				break;
				
			}
		}
		break;
		#endif
		
		#if WITH_CC_SECURITY
		case COMMAND_CLASS_SECURITY:
			SecurityCommandHandler(sourceNode, pCmd, cmdLength); 
			break;
		#endif

		#if WITH_CC_MULTI_COMMAND
		case COMMAND_CLASS_MULTI_CMD:

			if(!IsNodeSupportMultiCmd(sourceNode))
				MultiCmdListAdd(sourceNode);

			if (cmd == MULTI_CMD_ENCAP) {
				BYTE ii;
				BYTE len_cmd;
				BYTE data_size = *((BYTE*)pCmd + 2);
				BYTE * pCmdDta1 = ((BYTE*)pCmd + 3);
				
				for (ii=0; ii<data_size; ii++) {
					len_cmd = *pCmdDta1;
					pCmdDta1++;
					__ApplicationCommandHandler(RECEIVE_STATUS_TYPE_LOCAL | rxStatus, sourceNode, (ZW_APPLICATION_TX_BUFFER*)(pCmdDta1), len_cmd);
					pCmdDta1 += len_cmd;	
				}
			}
			break;
		#endif

		#if WITH_CC_ZWPLUS_INFO
		case COMMAND_CLASS_ZWAVEPLUS_INFO_V2:
			if (cmd == ZWAVEPLUS_INFO_GET) {

				if ((txBuf = CreatePacket()) == NULL)
					return;
				
				txBuf->ZW_ZwaveplusInfoReportV2Frame.cmdClass 				= COMMAND_CLASS_ZWAVEPLUS_INFO_V2;
				txBuf->ZW_ZwaveplusInfoReportV2Frame.cmd	   				= ZWAVEPLUS_INFO_REPORT;
				txBuf->ZW_ZwaveplusInfoReportV2Frame.zWaveVersion 			= 1; 
				txBuf->ZW_ZwaveplusInfoReportV2Frame.roleType				= ZWPLUS_INFO_ROLE_TYPE;
				txBuf->ZW_ZwaveplusInfoReportV2Frame.nodeType				= ZWPLUS_INFO_NODE_TYPE;
				txBuf->ZW_ZwaveplusInfoReportV2Frame.installerIconType1		= (BYTE) (ZWPLUS_INFO_INSTALLER_ICON >> 8);
				txBuf->ZW_ZwaveplusInfoReportV2Frame.installerIconType2		= (BYTE) (ZWPLUS_INFO_INSTALLER_ICON & 0xff);
				txBuf->ZW_ZwaveplusInfoReportV2Frame.userIconType1			= (BYTE) (ZWPLUS_INFO_CUSTOM_ICON >> 8);
				txBuf->ZW_ZwaveplusInfoReportV2Frame.userIconType2			= (BYTE) (ZWPLUS_INFO_CUSTOM_ICON & 0xff);
				
							
				EnqueuePacket(sourceNode, sizeof(txBuf->ZW_ZwaveplusInfoReportV2Frame), TRANSMIT_OPTION_USE_DEFAULT);

			}
			break;
		#endif	

		#if WITH_CC_ASSOCIATION_GROUP_INFORMATION

			case COMMAND_CLASS_ASSOCIATION_GRP_INFO:
				{
					switch(cmd){

						case ASSOCIATION_GROUP_NAME_GET:
							{
								//BYTE * ppName;
								BYTE 	len;
								// groupId = param1

								//len = AGI_NAME(param1-1, &ppName);
								//len = 4; //AGI_NAME(param1, &ppName);
								
								//if(len > MAX_GROUP_NAME)
								//	len = MAX_GROUP_NAME;

								if ((param1 > ASSOCIATION_NUM_GROUPS) || (param1 <= 0))
									return;

								if ((txBuf = CreatePacket()) == NULL)
									return;
								
								txBuf->ZW_AssociationGroupNameReport1byteFrame.cmdClass 			=	COMMAND_CLASS_ASSOCIATION_GRP_INFO;
								txBuf->ZW_AssociationGroupNameReport1byteFrame.cmd 					=	ASSOCIATION_GROUP_NAME_REPORT;
								txBuf->ZW_AssociationGroupNameReport1byteFrame.groupingIdentifier 	=	param1;
								
								len = AGI_NAME(param1-1, &(txBuf->ZW_AssociationGroupNameReport1byteFrame.name1));	
								
								txBuf->ZW_AssociationGroupNameReport1byteFrame.lengthOfName 		=   len;
								
								//memcpy(&(txBuf->ZW_AssociationGroupNameReport1byteFrame.name1), "TEST", len);
								//memcpy(&(txBuf->ZW_AssociationGroupNameReport1byteFrame.name1), ppName, len);
								EnqueuePacket(sourceNode, sizeof(txBuf->ZW_AssociationGroupNameReport1byteFrame)+len-1, TRANSMIT_OPTION_USE_DEFAULT);


							}	
							break;
						case ASSOCIATION_GROUP_INFO_GET:
							{
								BYTE * pBuf;
								BYTE size = sizeof(txBuf->ZW_AssociationGroupInfoReport1byteFrame);

			
								if (((param1 & ASSOCIATION_GROUP_INFO_GET_PROPERTIES1_LIST_MODE_BIT_MASK) == 0) && 
									((param2 > ASSOCIATION_NUM_GROUPS)|| (param2 <= 0)))
									return;
								
								if ((txBuf = CreatePacket()) == NULL)
									return;

								txBuf->ZW_AssociationGroupInfoReport1byteFrame.cmdClass 			=	COMMAND_CLASS_ASSOCIATION_GRP_INFO;
								txBuf->ZW_AssociationGroupInfoReport1byteFrame.cmd 					=	ASSOCIATION_GROUP_INFO_REPORT;
									
								pBuf = &(txBuf->ZW_AssociationGroupInfoReport1byteFrame.properties1);
								pBuf++;

								// Вернуть все списком?
								if(param1 & ASSOCIATION_GROUP_INFO_GET_PROPERTIES1_LIST_MODE_BIT_MASK) {
									BYTE i;
									
									txBuf->ZW_AssociationGroupInfoReport1byteFrame.properties1 			=	ASSOCIATION_GROUP_INFO_REPORT_PROPERTIES1_LIST_MODE_BIT_MASK | ASSOCIATION_NUM_GROUPS;
								
									memset(pBuf, 0, ASSOCIATION_NUM_GROUPS*sizeof(VG_ASSOCIATION_GROUP_INFO_REPORT_VG));

									// Нужно возвращать данные для всех групп
									// ! Считаем что у нас меньше 6-ти групп
									#if ASSOCIATION_NUM_GROUPS > 6
										#error "Number of groups can not be more than 6 in this code"
									#endif
									for(i=0; i<ASSOCIATION_NUM_GROUPS; i++, pBuf += sizeof(VG_ASSOCIATION_GROUP_INFO_REPORT_VG)){	
										if(i != 0)
											size += sizeof(VG_ASSOCIATION_GROUP_INFO_REPORT_VG);
										pBuf[0]				=	i+1;
										*(WORD*)(&pBuf[2])	=	AGI_PROFILE(i);
										/*pBuf[2]				=  	(AGI_PROFILE(i)) >> 8;
										pBuf[3]				=  	(AGI_PROFILE(i)) && 0xff;*/
											
									}


								}
								else
								{
									memset(pBuf, 0, sizeof(VG_ASSOCIATION_GROUP_INFO_REPORT_VG));

									// groupId = param2

									txBuf->ZW_AssociationGroupInfoReport1byteFrame.properties1 			=	1;
									pBuf[0]				=	param2;
									*(WORD*)(&pBuf[2])	=	AGI_PROFILE(param2-1);
									/*pBuf[2]				=  	(AGI_PROFILE(param2-1)) >> 8;
									pBuf[3]				=  	(AGI_PROFILE(param2-1)) && 0xff;*/
									
								}
								EnqueuePacket(sourceNode, size, TRANSMIT_OPTION_USE_DEFAULT);

							}
							break;
						case ASSOCIATION_GROUP_COMMAND_LIST_GET:
							{
								BYTE 	i;	
								BYTE * pBuf;
								BYTE 	size = sizeof(txBuf->ZW_AssociationGroupCommandListReport1byteFrame)-1;
								BYTE 	len, dl=0;
								DWORD 	ccs;

								#warning "VERA wants next 2 lines to be commented for Nevoton ??"
								if ((param2 > ASSOCIATION_NUM_GROUPS) || (param2 <= 0))
									return;	
								if ((txBuf = CreatePacket()) == NULL)
									return;

								txBuf->ZW_AssociationGroupCommandListReport1byteFrame.cmdClass 				=	COMMAND_CLASS_ASSOCIATION_GRP_INFO;
								txBuf->ZW_AssociationGroupCommandListReport1byteFrame.cmd 					=	ASSOCIATION_GROUP_COMMAND_LIST_REPORT;
								txBuf->ZW_AssociationGroupCommandListReport1byteFrame.groupingIdentifier	=	param2;		
								
								pBuf = &(txBuf->ZW_AssociationGroupCommandListReport1byteFrame.command1);
					

								for(i=0; i<AGI_CCS_COUNT(param2-1); i++, pBuf += len)
								{	
									ccs = AGI_CCS(param2-1, i);
									len = 2;

									if(ccs & 0xFF0000)
									{
										pBuf[0] = (WORD)((ccs >> 16) & 0xFF);
											
										*((WORD*) (pBuf+1)) = (WORD)(ccs & 0x00FFFF);
										len = 3;
									}
									else
									{
										*((WORD*) pBuf) = (WORD)(ccs & 0x00FFFF);

									}

									dl += len;
								}

								txBuf->ZW_AssociationGroupCommandListReport1byteFrame.listLength		=	dl;		
								
								size += dl;

								EnqueuePacket(sourceNode, size, TRANSMIT_OPTION_USE_DEFAULT);


							}
							break;


					}

				}
				break;
			// Необходимые макросы
			// #define AGI_NAME(G, NAME) 	// Возвращает имя группы (Записывает указатель на имя группы в NAME)
			// #define AGI_PROFILE(G) 		// Возвращает профиль группы
			// #define AGI_CCS_COUNT(G) 	// Возвращает число возможных комманд для группы 
			// #define AGI_CCS(G, I) 		// Возвращает соответствующую команду для группы


		#endif

		#if WITH_CC_CENTRAL_SCENE
		case COMMAND_CLASS_CENTRAL_SCENE:
			switch(cmd) {
				case CENTRAL_SCENE_SUPPORTED_GET:
					if ((txBuf = CreatePacket()) == NULL)
									return;

					txBuf->ZW_CentralSceneSupportedReportFrame.cmdClass 			=	COMMAND_CLASS_CENTRAL_SCENE;
					txBuf->ZW_CentralSceneSupportedReportFrame.cmd 					=	CENTRAL_SCENE_SUPPORTED_REPORT;
					txBuf->ZW_CentralSceneSupportedReportFrame.supportedScenes		=	CENTRAL_SCENE_MAX_SCENES;	

					EnqueuePacket(sourceNode, sizeof(txBuf->ZW_CentralSceneSupportedReportFrame), TRANSMIT_OPTION_USE_DEFAULT);
				break;
			}
		break;
		#endif

		#if WITH_CC_FIRMWARE_UPGRADE
  			case COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2:
  				FWUpdateCommandHandler(sourceNode, pCmd, cmdLength); 
				break;
		#endif				
		#if WITH_CC_POWERLEVEL
			case COMMAND_CLASS_POWERLEVEL:
				PowerLevelCommandHandler(sourceNode, pCmd, cmdLength);
				break;	
		#endif		

		
		#if WITH_CC_THERMOSTAT_MODE
		case COMMAND_CLASS_THERMOSTAT_MODE:
			switch(cmd) {
				case THERMOSTAT_MODE_SET:
					param1 = param1 & THERMOSTAT_MODE_SET_LEVEL_MODE_MASK_V2;
					if (
						#if CC_THERMOSTAT_MODE_WITH_MODE_OFF_V2
						param1 == THERMOSTAT_MODE_REPORT_MODE_OFF_V2 ||
						#endif
						#if CC_THERMOSTAT_MODE_WITH_MODE_HEAT_V2
						param1 == THERMOSTAT_MODE_REPORT_MODE_HEAT_V2 ||
						#endif
						#if CC_THERMOSTAT_MODE_WITH_MODE_COOL_V2
						param1 == THERMOSTAT_MODE_REPORT_MODE_COOL_V2 ||
						#endif
						#if CC_THERMOSTAT_MODE_WITH_MODE_AUTO_V2
						param1 == THERMOSTAT_MODE_REPORT_MODE_AUTO_V2 ||
						#endif
						#if CC_THERMOSTAT_MODE_WITH_MODE_AUXILIARY_HEAT_V2
						param1 == THERMOSTAT_MODE_REPORT_MODE_AUXILIARY_HEAT_V2 ||
						#endif
						#if CC_THERMOSTAT_MODE_WITH_MODE_RESUME_V2
						param1 == THERMOSTAT_MODE_REPORT_MODE_RESUME_V2 ||
						#endif
						#if CC_THERMOSTAT_MODE_WITH_MODE_FAN_ONLY_V2
						param1 == THERMOSTAT_MODE_REPORT_MODE_FAN_ONLY_V2 ||
						#endif
						#if CC_THERMOSTAT_MODE_WITH_MODE_FURNACE_V2
						param1 == THERMOSTAT_MODE_REPORT_MODE_FURNACE_V2 ||
						#endif
						#if CC_THERMOSTAT_MODE_WITH_MODE_DRY_AIR_V2
						param1 == THERMOSTAT_MODE_REPORT_MODE_DRY_AIR_V2 ||
						#endif
						#if CC_THERMOSTAT_MODE_WITH_MODE_MOIST_AIR_V2
						param1 == THERMOSTAT_MODE_REPORT_MODE_MOIST_AIR_V2 ||
						#endif
						#if CC_THERMOSTAT_MODE_WITH_MODE_AUTO_CHANGEOVER_V2
						param1 == THERMOSTAT_MODE_REPORT_MODE_AUTO_CHANGEOVER_V2 ||
						#endif
						#if CC_THERMOSTAT_MODE_WITH_MODE_ENERGY_SAVE_HEAT_V2
						param1 == THERMOSTAT_MODE_REPORT_MODE_ENERGY_SAVE_HEAT_V2 ||
						#endif
						#if CC_THERMOSTAT_MODE_WITH_MODE_ENERGY_SAVE_COOL_V2
						param1 == THERMOSTAT_MODE_REPORT_MODE_ENERGY_SAVE_COOL_V2 ||
						#endif
						#if CC_THERMOSTAT_MODE_WITH_MODE_AWAY_V2
						param1 == THERMOSTAT_MODE_REPORT_MODE_AWAY_V2 ||
						#endif
						#if CC_THERMOSTAT_MODE_WITH_MODE_FULL_POWER_V3
						param1 == THERMOSTAT_MODE_REPORT_MODE_FULL_POWER_V3 ||
						#endif
						#if CC_THERMOSTAT_MODE_WITH_MODE_MANUFACTURER_SPECIFIC_V3
						param1 == THERMOSTAT_MODE_REPORT_MODE_MANUFACTURER_SPECIFIC_V3 ||
						#endif
						0 // just to end the list
					) {
						VAR(THERMOSTAT_MODE) = param1;

						THERMOSTAT_MODE_SET_ACTION();

						SAVE_EEPROM_BYTE(THERMOSTAT_MODE);
						#if WITH_CC_ASSOCIATION
						SendThermostatModeReport();
						#endif
					}
					break;
		
				case THERMOSTAT_MODE_GET:
					SendReportToNode(sourceNode, COMMAND_CLASS_THERMOSTAT_MODE, VAR(THERMOSTAT_MODE));
					break;
			
				case THERMOSTAT_MODE_SUPPORTED_GET:
					if ((txBuf = CreatePacket()) != NULL) {
						txBuf->ZW_ThermostatModeSupportedReport4byteFrame.cmdClass = COMMAND_CLASS_THERMOSTAT_MODE;
						txBuf->ZW_ThermostatModeSupportedReport4byteFrame.cmd = THERMOSTAT_MODE_SUPPORTED_REPORT;
						txBuf->ZW_ThermostatModeSupportedReport4byteFrame.bitMask1 = // always present
										#if CC_THERMOSTAT_MODE_WITH_MODE_OFF_V2
										(1 << (THERMOSTAT_MODE_REPORT_MODE_OFF_V2 - 0)) |
										#endif
										#if CC_THERMOSTAT_MODE_WITH_MODE_HEAT_V2
										(1 << (THERMOSTAT_MODE_REPORT_MODE_HEAT_V2 - 0)) |
										#endif
										#if CC_THERMOSTAT_MODE_WITH_MODE_COOL_V2
										(1 << (THERMOSTAT_MODE_REPORT_MODE_COOL_V2 - 0)) |
										#endif
										#if CC_THERMOSTAT_MODE_WITH_MODE_AUTO_V2
										(1 << (THERMOSTAT_MODE_REPORT_MODE_AUTO_V2 - 0)) |
										#endif
										#if CC_THERMOSTAT_MODE_WITH_MODE_AUXILIARY_HEAT_V2
										(1 << (THERMOSTAT_MODE_REPORT_MODE_AUXILIARY_HEAT_V2 - 0)) |
										#endif
										#if CC_THERMOSTAT_MODE_WITH_MODE_RESUME_V2
										(1 << (THERMOSTAT_MODE_REPORT_MODE_RESUME_V2 - 0)) |
										#endif
										#if CC_THERMOSTAT_MODE_WITH_MODE_FAN_ONLY_V2
										(1 << (THERMOSTAT_MODE_REPORT_MODE_FAN_ONLY_V2 - 0)) |
										#endif
										#if CC_THERMOSTAT_MODE_WITH_MODE_FURNACE_V2
										(1 << (THERMOSTAT_MODE_REPORT_MODE_FURNACE_V2 - 0)) |
										#endif
										0;
						#if \
							CC_THERMOSTAT_MODE_WITH_MODE_DRY_AIR_V2 || \
							CC_THERMOSTAT_MODE_WITH_MODE_MOIST_AIR_V2 || \
							CC_THERMOSTAT_MODE_WITH_MODE_AUTO_CHANGEOVER_V2 || \
							CC_THERMOSTAT_MODE_WITH_MODE_ENERGY_SAVE_HEAT_V2 || \
							CC_THERMOSTAT_MODE_WITH_MODE_ENERGY_SAVE_COOL_V2 || \
							CC_THERMOSTAT_MODE_WITH_MODE_AWAY_V2 || \
							CC_THERMOSTAT_MODE_WITH_MODE_FULL_POWER_V3 || \
							CC_THERMOSTAT_MODE_WITH_MODE_MANUFACTURER_SPECIFIC_V3 || \
							0
						txBuf->ZW_ThermostatModeSupportedReport4byteFrame.bitMask2 =
										#if CC_THERMOSTAT_MODE_WITH_MODE_DRY_AIR_V2
										(1 << (THERMOSTAT_MODE_REPORT_MODE_DRY_AIR_V2 - 8)) |
										#endif
										#if CC_THERMOSTAT_MODE_WITH_MODE_MOIST_AIR_V2
										(1 << (THERMOSTAT_MODE_REPORT_MODE_MOIST_AIR_V2 - 8)) |
										#endif
										#if CC_THERMOSTAT_MODE_WITH_MODE_AUTO_CHANGEOVER_V2
										(1 << (THERMOSTAT_MODE_REPORT_MODE_AUTO_CHANGEOVER_V2 - 8)) |
										#endif
										#if CC_THERMOSTAT_MODE_WITH_MODE_ENERGY_SAVE_HEAT_V2
										(1 << (THERMOSTAT_MODE_REPORT_MODE_ENERGY_SAVE_HEAT_V2 - 8)) |
										#endif
										#if CC_THERMOSTAT_MODE_WITH_MODE_ENERGY_SAVE_COOL_V2
										(1 << (THERMOSTAT_MODE_REPORT_MODE_ENERGY_SAVE_COOL_V2 - 8)) |
										#endif
										#if CC_THERMOSTAT_MODE_WITH_MODE_AWAY_V2
										(1 << (THERMOSTAT_MODE_REPORT_MODE_AWAY_V2 - 8)) |
										#endif
										#if CC_THERMOSTAT_MODE_WITH_MODE_FULL_POWER_V3
										(1 << (THERMOSTAT_MODE_REPORT_MODE_FULL_POWER_V3 - 8)) |
										#endif
										0;
						#if \
							CC_THERMOSTAT_MODE_WITH_MODE_MANUFACTURER_SPECIFIC_V3 || \
							0
						txBuf->ZW_ThermostatModeSupportedReport4byteFrame.bitMask3 =
										0;
						#if \
							CC_THERMOSTAT_MODE_WITH_MODE_MANUFACTURER_SPECIFIC_V3 || \
							0
						txBuf->ZW_ThermostatModeSupportedReport4byteFrame.bitMask4 =
										#if CC_THERMOSTAT_MODE_WITH_MODE_MANUFACTURER_SPECIFIC_V3
										(1 << (THERMOSTAT_MODE_REPORT_MODE_MANUFACTURER_SPECIFIC_V3 - 24)) |
										#endif
										0;
						#endif
						#endif
						#endif
						EnqueuePacket(sourceNode, sizeof(
							#if \
								CC_THERMOSTAT_MODE_WITH_MODE_MANUFACTURER_SPECIFIC_V3 || \
								0
							txBuf->ZW_ThermostatModeSupportedReport4byteFrame
							#else
							#if \
								CC_THERMOSTAT_MODE_WITH_MODE_MANUFACTURER_SPECIFIC_V3 || \
								0
							txBuf->ZW_ThermostatModeSupportedReport3byteFrame
							#else
							#if \
								CC_THERMOSTAT_MODE_WITH_MODE_DRY_AIR_V2 || \
								CC_THERMOSTAT_MODE_WITH_MODE_MOIST_AIR_V2 || \
								CC_THERMOSTAT_MODE_WITH_MODE_AUTO_CHANGEOVER_V2 || \
								CC_THERMOSTAT_MODE_WITH_MODE_ENERGY_SAVE_HEAT_V2 || \
								CC_THERMOSTAT_MODE_WITH_MODE_ENERGY_SAVE_COOL_V2 || \
								CC_THERMOSTAT_MODE_WITH_MODE_AWAY_V2 || \
								CC_THERMOSTAT_MODE_WITH_MODE_FULL_POWER_V3 || \
								0
							txBuf->ZW_ThermostatModeSupportedReport2byteFrame
							#else
							txBuf->ZW_ThermostatModeSupportedReport1byteFrame
							#endif
							#endif
							#endif
						), txOption);
					}
					break;
			}
			break;
		#endif

		#if WITH_CC_THERMOSTAT_SETPOINT
		case COMMAND_CLASS_THERMOSTAT_SETPOINT:
			switch(cmd) {
				case THERMOSTAT_SETPOINT_SET:
					param1 = param1 & THERMOSTAT_SETPOINT_SET_LEVEL_SETPOINT_TYPE_MASK_V2;
					
					// hack for Vera
					if(
						((param2 & THERMOSTAT_SETPOINT_SET_LEVEL2_SIZE_MASK_V2) == 1) &&
						(((param2 & THERMOSTAT_SETPOINT_SET_LEVEL2_PRECISION_MASK_V2) >> THERMOSTAT_SETPOINT_SET_LEVEL2_PRECISION_SHIFT_V2) == 0)
						) {
						register WORD tmp;

						if (param3 >= 0x80) // this is same as parame3 < 0, but in unsigned world
							break; // accept positive only
						#message ThermostatSetPoint accepts positive values only

						tmp = (WORD)param3 * 10;
						param3 = (BYTE) (tmp >> 8);
						param4 = (BYTE) (tmp);
						param2 &= ~(THERMOSTAT_SETPOINT_SET_LEVEL2_SIZE_MASK_V2 | THERMOSTAT_SETPOINT_SET_LEVEL2_PRECISION_MASK_V2);
						param2 |= (THERMOSTAT_SETPOINT_PRECISION << THERMOSTAT_SETPOINT_SET_LEVEL2_PRECISION_SHIFT_V2) | THERMOSTAT_SETPOINT_TEMPERATURE_SIZE;
					}

					if(((param2 & THERMOSTAT_SETPOINT_SET_LEVEL2_SCALE_MASK_V2) >> THERMOSTAT_SETPOINT_SET_LEVEL2_SCALE_SHIFT_V2) == THERMOSTAT_SETPOINT_SCALE_FARENHEIT) {
						register WORD tmp;

						tmp = ((WORD)param3<<8) + (WORD)param4;
						//C = (F-32)*5/9
						tmp -= 32*10; // tmp i in 0.1C units
						tmp *= 5;
						tmp /= 9;
						param3 = (BYTE) (tmp >> 8);
						param4 = (BYTE) (tmp);
						param2 &= ~THERMOSTAT_SETPOINT_SET_LEVEL2_SCALE_MASK_V2;
						param2 |= THERMOSTAT_SETPOINT_SCALE_CELSIUS << THERMOSTAT_SETPOINT_SET_LEVEL2_SCALE_SHIFT_V2;
					}

					if(
						((param2 & THERMOSTAT_SETPOINT_SET_LEVEL2_SIZE_MASK_V2) == THERMOSTAT_SETPOINT_TEMPERATURE_SIZE) &&
						(((param2 & THERMOSTAT_SETPOINT_SET_LEVEL2_SCALE_MASK_V2) >> THERMOSTAT_SETPOINT_SET_LEVEL2_SCALE_SHIFT_V2) == THERMOSTAT_SETPOINT_SCALE_CELSIUS) &&
						(((param2 & THERMOSTAT_SETPOINT_SET_LEVEL2_PRECISION_MASK_V2) >> THERMOSTAT_SETPOINT_SET_LEVEL2_PRECISION_SHIFT_V2) == THERMOSTAT_SETPOINT_PRECISION)
						) {
						switch(param1) {
							#if CC_THERMOSTAT_SETPOINT_WITH_HEATING_1_V2
							case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_HEATING_1_V2:
								WORD_HI_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_HEAT)) = param3;
								WORD_LO_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_HEAT)) = param4;
								SAVE_EEPROM_WORD(THERMOSTAT_SETPOINT_HEAT);
								#if WITH_CC_ASSOCIATION
								SendThermostatSetPointReport(THERMOSTAT_MODE_REPORT_MODE_HEAT_V2, WORD_VAR(THERMOSTAT_SETPOINT_HEAT));
								#endif
								THERMOSTAT_SETPOINT_SET_ACTION();
								break;
							#endif
							#if CC_THERMOSTAT_SETPOINT_WITH_COOLING_1_V2
							case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_COOLING_1_V2:
								WORD_HI_BYTE(WORD_VAR(THERMOSTAT_MODE_REPORT_MODE_COOL_V2)) = param3;
								WORD_LO_BYTE(WORD_VAR(THERMOSTAT_MODE_REPORT_MODE_COOL_V2)) = param4;
								SAVE_EEPROM_WORD(THERMOSTAT_SETPOINT_COOL);
								#if WITH_CC_ASSOCIATION
								SendThermostatSetPointReport(THERMOSTAT_MODE_REPORT_MODE_COOL_V2, WORD_VAR(THERMOSTAT_SETPOINT_COOL));
								#endif
								THERMOSTAT_SETPOINT_SET_ACTION();
								break;
							#endif
							#if CC_THERMOSTAT_SETPOINT_WITH_FURNACE_V2
							case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_FURNACE_V2:
								WORD_HI_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_FURNACE)) = param3;
								WORD_LO_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_FURNACE)) = param4;
								SAVE_EEPROM_WORD(THERMOSTAT_SETPOINT_FURNACE);
								#if WITH_CC_ASSOCIATION
								SendThermostatSetPointReport(THERMOSTAT_MODE_REPORT_MODE_FURNACE_V2, WORD_VAR(THERMOSTAT_SETPOINT_FURNACE));
								#endif
								THERMOSTAT_SETPOINT_SET_ACTION();
								break;
							#endif
							#if CC_THERMOSTAT_SETPOINT_WITH_DRY_AIR_V2
							case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_DRY_AIR_V2:
								WORD_HI_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_DRY_AIR)) = param3;
								WORD_LO_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_DRY_AIR)) = param4;
								SAVE_EEPROM_WORD(THERMOSTAT_SETPOINT_DRY_AIR);
								#if WITH_CC_ASSOCIATION
								SendThermostatSetPointReport(THERMOSTAT_MODE_REPORT_MODE_DRY_AIR_V2, WORD_VAR(THERMOSTAT_SETPOINT_DRY_AIR));
								#endif
								THERMOSTAT_SETPOINT_SET_ACTION();
								break;
							#endif
							#if CC_THERMOSTAT_SETPOINT_WITH_MOIST_AIR_V2
							case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_MOIST_AIR_V2:
								WORD_HI_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_MOIST_AIR)) = param3;
								WORD_LO_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_MOIST_AIR)) = param4;
								SAVE_EEPROM_WORD(THERMOSTAT_SETPOINT_MOIST_AIR);
								#if WITH_CC_ASSOCIATION
								SendThermostatSetPointReport(THERMOSTAT_MODE_REPORT_MODE_MOIST_AIR_V2, WORD_VAR(THERMOSTAT_SETPOINT_MOIST_AIR));
								#endif
								THERMOSTAT_SETPOINT_SET_ACTION();
								break;
							#endif
							#if CC_THERMOSTAT_SETPOINT_WITH_AUTO_CHANGEOVER_V2
							case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_AUTO_CHANGEOVER_V2:
								WORD_HI_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_AUTO_CHANGEOVER)) = param3;
								WORD_LO_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_AUTO_CHANGEOVER)) = param4;
								SAVE_EEPROM_WORD(THERMOSTAT_SETPOINT_AUTO_CHANGEOVER);
								#if WITH_CC_ASSOCIATION
								SendThermostatSetPointReport(THERMOSTAT_MODE_REPORT_MODE_AUTO_CHANGEOVER_V2, WORD_VAR(THERMOSTAT_SETPOINT_AUTO_CHANGEOVER));
								#endif
								THERMOSTAT_SETPOINT_SET_ACTION();
								break;
							#endif
							#if CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_HEATING_V2
							case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_ENERGY_SAVE_HEATING_V2:
								WORD_HI_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_ENERGY_SAVE_HEAT)) = param3;
								WORD_LO_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_ENERGY_SAVE_HEAT)) = param4;
								SAVE_EEPROM_WORD(THERMOSTAT_SETPOINT_ENERGY_SAVE_HEAT);
								#if WITH_CC_ASSOCIATION
								SendThermostatSetPointReport(THERMOSTAT_MODE_REPORT_MODE_ENERGY_SAVE_HEAT_V2, WORD_VAR(THERMOSTAT_SETPOINT_ENERGY_SAVE_HEAT));
								#endif
								THERMOSTAT_SETPOINT_SET_ACTION();
								break;
							#endif
							#if CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_COOLING_V2
							case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_ENERGY_SAVE_COOLING_V2:
								WORD_HI_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_ENERGY_SAVE_COOL)) = param3;
								WORD_LO_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_ENERGY_SAVE_COOL)) = param4;
								SAVE_EEPROM_WORD(THERMOSTAT_SETPOINT_ENERGY_SAVE_COOL);
								#if WITH_CC_ASSOCIATION
								SendThermostatSetPointReport(THERMOSTAT_MODE_REPORT_MODE_ENERGY_SAVE_COOL_V2, WORD_VAR(THERMOSTAT_SETPOINT_ENERGY_SAVE_COOL));
								#endif
								THERMOSTAT_SETPOINT_SET_ACTION();
								break;
							#endif
							#if CC_THERMOSTAT_SETPOINT_WITH_AWAY_HEATING_V2
							case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_AWAY_HEATING_V2:
								WORD_HI_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_AWAY_HEAT)) = param3;
								WORD_LO_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_AWAY_HEAT)) = param4;
								SAVE_EEPROM_WORD(THERMOSTAT_SETPOINT_AWAY_HEAT);
								#if WITH_CC_ASSOCIATION
								SendThermostatSetPointReport(THERMOSTAT_MODE_REPORT_MODE_AWAY_COOL_V3, WORD_VAR(THERMOSTAT_SETPOINT_AWAY_HEAT));
								#endif
								THERMOSTAT_SETPOINT_SET_ACTION();
								break;
							#endif
							#if CC_THERMOSTAT_SETPOINT_WITH_AWAY_COOLING_V3
							case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_AWAY_COOLING_V3:
								WORD_HI_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_AWAY_COOL)) = param3;
								WORD_LO_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_AWAY_COOL)) = param4;
								SAVE_EEPROM_WORD(THERMOSTAT_SETPOINT_AWAY_COOL);
								#if WITH_CC_ASSOCIATION
								SendThermostatSetPointReport(THERMOSTAT_MODE_REPORT_MODE_AWAY_COOL_V3, WORD_VAR(THERMOSTAT_SETPOINT_AWAY_COOL));
								#endif
								THERMOSTAT_SETPOINT_SET_ACTION();
								break;
							#endif
							#if CC_THERMOSTAT_SETPOINT_WITH_FULL_POWER_V3
							case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_FULL_POWER_V3:
								WORD_HI_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_FULL_POWER)) = param3;
								WORD_LO_BYTE(WORD_VAR(THERMOSTAT_SETPOINT_FULL_POWER)) = param4;
								SAVE_EEPROM_WORD(THERMOSTAT_SETPOINT_FULL_POWER);
								#if WITH_CC_ASSOCIATION
								SendThermostatSetPointReport(THERMOSTAT_MODE_REPORT_MODE_FULL_POWER_V3, WORD_VAR(THERMOSTAT_SETPOINT_FULL_POWER));
								#endif
								THERMOSTAT_SETPOINT_SET_ACTION();
								break;
							#endif
						}
					}
					break;
			
				case THERMOSTAT_SETPOINT_GET:
					param1 = param1 & THERMOSTAT_SETPOINT_SET_LEVEL_SETPOINT_TYPE_MASK;
					if (
						#if CC_THERMOSTAT_SETPOINT_WITH_HEATING_1_V2
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_HEATING_1_V2 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_COOLING_1_V2
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_COOLING_1_V2 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_FURNACE_V2
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_FURNACE_V2 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_DRY_AIR_V2
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_DRY_AIR_V2 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_MOIST_AIR_V2
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_MOIST_AIR_V2 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_AUTO_CHANGEOVER_V2
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_AUTO_CHANGEOVER_V2 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_HEATING_V2
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_ENERGY_SAVE_HEATING_V2 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_COOLING_V2
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_ENERGY_SAVE_COOLING_V2 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_AWAY_HEATING_V2
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_AWAY_HEATING_V2 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_AWAY_COOLING_V3
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_AWAY_COOLING_V3 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_FULL_POWER_V3
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_FULL_POWER_V3 ||
						#endif
						0 // just to terminate the list
						) {
						if ((txBuf = CreatePacket()) != NULL) {
							WORD reportTemp;
						
							switch(param1) {
								#if CC_THERMOSTAT_SETPOINT_WITH_HEATING_1_V2
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_HEATING_1_V2:
									reportTemp = WORD_VAR(THERMOSTAT_SETPOINT_HEAT);
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_COOLING_1_V2
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_COOLING_1_V2:
									reportTemp = WORD_VAR(THERMOSTAT_SETPOINT_COOL);
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_FURNACE_V2
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_FURNACE_V2:
									reportTemp = WORD_VAR(THERMOSTAT_SETPOINT_FURNACE);
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_DRY_AIR_V2
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_DRY_AIR_V2:
									reportTemp = WORD_VAR(THERMOSTAT_SETPOINT_DRY_AIR);
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_MOIST_AIR_V2
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_MOIST_AIR_V2:
									reportTemp = WORD_VAR(THERMOSTAT_SETPOINT_MOIST_AIR);
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_AUTO_CHANGEOVER_V2
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_AUTO_CHANGEOVER_V2:
									reportTemp = WORD_VAR(THERMOSTAT_SETPOINT_AUTO_CHANGEOVER);
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_HEATING_V2
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_ENERGY_SAVE_HEATING_V2:
									reportTemp = WORD_VAR(THERMOSTAT_SETPOINT_ENERGY_SAVE_HEAT);
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_COOLING_V2
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_ENERGY_SAVE_COOLING_V2:
									reportTemp = WORD_VAR(THERMOSTAT_SETPOINT_ENERGY_SAVE_COOL);
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_AWAY_HEATING_V2
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_AWAY_HEATING_V2:
									reportTemp = WORD_VAR(THERMOSTAT_SETPOINT_AWAY_HEAT);
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_AWAY_COOLING_V3
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_AWAY_COOLING_V3:
									reportTemp = WORD_VAR(THERMOSTAT_SETPOINT_AWAY_COOL);
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_FULL_POWER_V3
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_FULL_POWER_V3:
									reportTemp = WORD_VAR(THERMOSTAT_SETPOINT_FULL_POWER);
									break;
								#endif
							}
							txBuf->ZW_ThermostatSetpointReport2byteFrame.cmdClass = COMMAND_CLASS_THERMOSTAT_SETPOINT;
							txBuf->ZW_ThermostatSetpointReport2byteFrame.cmd = THERMOSTAT_SETPOINT_REPORT;
							txBuf->ZW_ThermostatSetpointReport2byteFrame.level = param1;
							txBuf->ZW_ThermostatSetpointReport2byteFrame.level2 =
								((THERMOSTAT_SETPOINT_PRECISION << THERMOSTAT_SETPOINT_SET_LEVEL2_PRECISION_SHIFT) & THERMOSTAT_SETPOINT_SET_LEVEL2_PRECISION_MASK) | \
								((THERMOSTAT_SETPOINT_SCALE_CELSIUS << THERMOSTAT_SETPOINT_SET_LEVEL2_SCALE_SHIFT) & THERMOSTAT_SETPOINT_SET_LEVEL2_SCALE_MASK) | \
								(THERMOSTAT_SETPOINT_TEMPERATURE_SIZE & THERMOSTAT_SETPOINT_SET_LEVEL2_SIZE_MASK);						
							txBuf->ZW_ThermostatSetpointReport2byteFrame.value1 = (BYTE)(reportTemp >> 8);
							txBuf->ZW_ThermostatSetpointReport2byteFrame.value2 = (BYTE)(reportTemp);
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_ThermostatSetpointReport2byteFrame), txOption);
						}
					}
					break;

				case THERMOSTAT_SETPOINT_SUPPORTED_GET:
					if ((txBuf = CreatePacket()) != NULL) {
						txBuf->ZW_ThermostatSetpointSupportedReport3byteFrame.cmdClass = COMMAND_CLASS_THERMOSTAT_SETPOINT;
						txBuf->ZW_ThermostatSetpointSupportedReport3byteFrame.cmd = THERMOSTAT_SETPOINT_SUPPORTED_REPORT;
						txBuf->ZW_ThermostatSetpointSupportedReport3byteFrame.bitMask1 =
										#if CC_THERMOSTAT_SETPOINT_WITH_HEATING_1_V2
										(1 << THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_HEATING_1_V2) |
										#endif
										#if CC_THERMOSTAT_SETPOINT_WITH_COOLING_1_V2
										(1 << THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_COOLING_1_V2) |
										#endif
										#if CC_THERMOSTAT_SETPOINT_WITH_FURNACE_V2
										(1 << THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_FURNACE_V2) |
										#endif
										#if CC_THERMOSTAT_SETPOINT_WITH_DRY_AIR_V2
										(1 << THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_DRY_AIR_V2) |
										#endif
										#if CC_THERMOSTAT_SETPOINT_WITH_MOIST_AIR_V2
										(1 << THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_MOIST_AIR_V2) |
										#endif
										#if CC_THERMOSTAT_SETPOINT_WITH_AUTO_CHANGEOVER_V2
										(1 << THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_AUTO_CHANGEOVER_V2) |
										#endif
										#if CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_HEATING_V2
										(1 << THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_ENERGY_SAVE_HEATING_V2) |
										#endif
										0;
						txBuf->ZW_ThermostatSetpointSupportedReport3byteFrame.bitMask2 =
										#if CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_COOLING_V2
										(1 << THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_ENERGY_SAVE_COOLING_V2) |
										#endif
										#if CC_THERMOSTAT_SETPOINT_WITH_AWAY_HEATING_V2
										(1 << THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_AWAY_HEATING_V2) |
										#endif
										#if CC_THERMOSTAT_SETPOINT_WITH_AWAY_COOLING_V3
										(1 << THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_AWAY_COOLING_V3) |
										#endif
										#if CC_THERMOSTAT_SETPOINT_WITH_FULL_POWER_V3
										(1 << THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_FULL_POWER_V3) |
										#endif
										0;
						txBuf->ZW_ThermostatSetpointSupportedReport3byteFrame.bitMask3 =
										0;
						EnqueuePacket(sourceNode, sizeof(
							#if \
								0
							txBuf->ZW_ThermostatSetpointSupportedReport3byteFrame
							#else
							#if \
								CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_COOLING_V2 || \
								CC_THERMOSTAT_SETPOINT_WITH_AWAY_HEATING_V2 || \
								0
							txBuf->ZW_ThermostatSetpointSupportedReport2byteFrame
							#else
							txBuf->ZW_ThermostatSetpointSupportedReport1byteFrame
							#endif
							#endif
						), txOption);
					}
					break;

				case THERMOSTAT_SETPOINT_CAPABILITIES_GET:
					param1 = param1 & THERMOSTAT_SETPOINT_SET_LEVEL_SETPOINT_TYPE_MASK;
					if (
						#if CC_THERMOSTAT_SETPOINT_WITH_HEATING_1_V2
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_HEATING_1_V2 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_COOLING_1_V2
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_COOLING_1_V2 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_FURNACE_V2
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_FURNACE_V2 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_DRY_AIR_V2
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_DRY_AIR_V2 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_MOIST_AIR_V2
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_MOIST_AIR_V2 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_AUTO_CHANGEOVER_V2
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_AUTO_CHANGEOVER_V2 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_HEATING_V2
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_ENERGY_SAVE_HEATING_V2 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_COOLING_V2
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_ENERGY_SAVE_COOLING_V2 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_AWAY_HEATING_V2
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_AWAY_HEATING_V2 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_AWAY_COOLING_V3
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_AWAY_COOLING_V3 ||
						#endif
						#if CC_THERMOSTAT_SETPOINT_WITH_FULL_POWER_V3
						param1 == THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_FULL_POWER_V3 ||
						#endif
						0 // just to terminate the list
						) {
						if ((txBuf = CreatePacket()) != NULL) {
							WORD minTemp, maxTemp;
						
							switch(param1) {
								#if CC_THERMOSTAT_SETPOINT_WITH_HEATING_1_V2
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_HEATING_1_V2:
									minTemp = CC_THERMOSTAT_SETPOINT_HEATING_1_MIN;
									maxTemp = CC_THERMOSTAT_SETPOINT_HEATING_1_MAX;
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_COOLING_1_V2
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_COOLING_1_V2:
									minTemp = CC_THERMOSTAT_SETPOINT_COOLING_1_MIN;
									maxTemp = CC_THERMOSTAT_SETPOINT_COOLING_1_MAX;
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_FURNACE_V2
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_FURNACE_V2:
									minTemp = CC_THERMOSTAT_SETPOINT_FURNACE_MIN;
									maxTemp = CC_THERMOSTAT_SETPOINT_FURNACE_MAX;
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_DRY_AIR_V2
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_DRY_AIR_V2:
									minTemp = CC_THERMOSTAT_SETPOINT_DRY_AIR_MIN;
									maxTemp = CC_THERMOSTAT_SETPOINT_DRY_AIR_MAX;
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_MOIST_AIR_V2
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_MOIST_AIR_V2:
									minTemp = CC_THERMOSTAT_SETPOINT_MOIST_AIR_MIN;
									maxTemp = CC_THERMOSTAT_SETPOINT_MOIST_AIR_MAX;
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_AUTO_CHANGEOVER_V2
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_AUTO_CHANGEOVER_V2:
									minTemp = CC_THERMOSTAT_SETPOINT_AUTO_CHANGEOVER_MIN;
									maxTemp = CC_THERMOSTAT_SETPOINT_AUTO_CHANGEOVER_MAX;
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_HEATING_V2
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_ENERGY_SAVE_HEATING_V2:
									minTemp = CC_THERMOSTAT_SETPOINT_ENERGY_SAVE_HEATING_MIN;
									maxTemp = CC_THERMOSTAT_SETPOINT_ENERGY_SAVE_HEATING_MAX;
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_COOLING_V2
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_ENERGY_SAVE_COOLING_V2:
									minTemp = CC_THERMOSTAT_SETPOINT_ENERGY_SAVE_COOLING_MIN;
									maxTemp = CC_THERMOSTAT_SETPOINT_ENERGY_SAVE_COOLING_MAX;
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_AWAY_HEATING_V2
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_AWAY_HEATING_V2:
									minTemp = CC_THERMOSTAT_SETPOINT_AWAY_HEATING_MIN;
									maxTemp = CC_THERMOSTAT_SETPOINT_AWAY_HEATING_MAX;
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_AWAY_COOLING_V3
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_AWAY_COOLING_V3:
									minTemp = CC_THERMOSTAT_SETPOINT_AWAY_COOLING_MIN;
									maxTemp = CC_THERMOSTAT_SETPOINT_AWAY_COOLING_MAX;
									break;
								#endif
								#if CC_THERMOSTAT_SETPOINT_WITH_FULL_POWER_V3
								case THERMOSTAT_SETPOINT_REPORT_SETPOINT_TYPE_FULL_POWER_V3:
									minTemp = CC_THERMOSTAT_SETPOINT_FULL_POWER_MIN;
									maxTemp = CC_THERMOSTAT_SETPOINT_FULL_POWER_MAX;
									break;
								#endif
							}
							txBuf->ZW_ThermostatSetpointReport2byteFrame.cmdClass = COMMAND_CLASS_THERMOSTAT_SETPOINT;
							txBuf->ZW_ThermostatSetpointReport2byteFrame.cmd = THERMOSTAT_SETPOINT_CAPABILITIES_REPORT;
							txBuf->ZW_ThermostatSetpointReport2byteFrame.level = param1;
							txBuf->ZW_ThermostatSetpointReport2byteFrame.level2 =
								((THERMOSTAT_SETPOINT_PRECISION << THERMOSTAT_SETPOINT_SET_LEVEL2_PRECISION_SHIFT) & THERMOSTAT_SETPOINT_SET_LEVEL2_PRECISION_MASK) | \
								((THERMOSTAT_SETPOINT_SCALE_CELSIUS << THERMOSTAT_SETPOINT_SET_LEVEL2_SCALE_SHIFT) & THERMOSTAT_SETPOINT_SET_LEVEL2_SCALE_MASK) | \
								(THERMOSTAT_SETPOINT_TEMPERATURE_SIZE & THERMOSTAT_SETPOINT_SET_LEVEL2_SIZE_MASK);						
							txBuf->ZW_ThermostatSetpointReport2byteFrame.value1 = (BYTE)(minTemp >> 8);
							txBuf->ZW_ThermostatSetpointReport2byteFrame.value2 = (BYTE)(minTemp);
							#message Fix these fields in future
							*(&(txBuf->ZW_ThermostatSetpointReport2byteFrame.value2) + 1) =
								((THERMOSTAT_SETPOINT_PRECISION << THERMOSTAT_SETPOINT_SET_LEVEL2_PRECISION_SHIFT) & THERMOSTAT_SETPOINT_SET_LEVEL2_PRECISION_MASK) | \
								((THERMOSTAT_SETPOINT_SCALE_CELSIUS << THERMOSTAT_SETPOINT_SET_LEVEL2_SCALE_SHIFT) & THERMOSTAT_SETPOINT_SET_LEVEL2_SCALE_MASK) | \
								(THERMOSTAT_SETPOINT_TEMPERATURE_SIZE & THERMOSTAT_SETPOINT_SET_LEVEL2_SIZE_MASK);						
							*(&(txBuf->ZW_ThermostatSetpointReport2byteFrame.value2) + 2) = (BYTE)(maxTemp >> 8);
							*(&(txBuf->ZW_ThermostatSetpointReport2byteFrame.value2) + 3) = (BYTE)(maxTemp);
							EnqueuePacket(sourceNode, sizeof(txBuf->ZW_ThermostatSetpointReport2byteFrame) + 1 + 2 /*fix this too! */, txOption);
						}
					}
					break;
			}	
			break;
		#endif
		
		#if WITH_CC_CONFIGURATION
		case COMMAND_CLASS_CONFIGURATION:
			switch (cmd) {
				case CONFIGURATION_SET:
					config_param_to_default = param2 & CONFIGURATION_SET_LEVEL_DEFAULT_BIT_MASK;
					config_param_size = param2 & CONFIGURATION_SET_LEVEL_SIZE_MASK;
					report_value_b = param3;
					report_value_w = *((WORD*)((BYTE*)pCmd + OFFSET_PARAM_3));
					report_value_dw = *((DWORD*)((BYTE*)pCmd + OFFSET_PARAM_3));
					switch (param1) {
						CONFIG_PARAMETER_SET
					}
					break;
				case CONFIGURATION_GET:
					report_value_b = 0;
					report_value_w = 0;
					report_value_dw = 0;
					config_param_size = 1;
					switch (param1) {
						CONFIG_PARAMETER_GET

						case 230: //constant production Data
							ZW_NVRGetValue(0x70,4,&report_value_dw);
							config_param_size = 4;
						break;

						case 231: //constant production Data
							ZW_NVRGetValue(0x74,4,&report_value_dw);
							config_param_size = 4;
						break;
					}
					if ((txBuf = CreatePacket()) != NULL) {
						txBuf->ZW_ConfigurationReport1byteFrame.cmdClass = COMMAND_CLASS_CONFIGURATION;
						txBuf->ZW_ConfigurationReport1byteFrame.cmd = CONFIGURATION_REPORT;
						txBuf->ZW_ConfigurationReport1byteFrame.parameterNumber = param1;
						txBuf->ZW_ConfigurationReport1byteFrame.level = config_param_size & 0x07;
						switch (config_param_size) {
							case 1:
								txBuf->ZW_ConfigurationReport1byteFrame.configurationValue1 = report_value_b;
								break;
							case 2:
								FILL_MULTIBYTE_PARAM2(txBuf->ZW_ConfigurationReport2byteFrame.configurationValue, report_value_w);
								break;
							case 4:
								FILL_MULTIBYTE_PARAM4(txBuf->ZW_ConfigurationReport4byteFrame.configurationValue, report_value_dw);
								break;
						}
						EnqueuePacket(sourceNode, sizeof(txBuf->ZW_ConfigurationReport1byteFrame) + config_param_size - 1 , txOption);
					}
					break;
				}
			break;
		#endif

		#if WITH_CC_ASSOCIATION
		case COMMAND_CLASS_ASSOCIATION:
		#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		case COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2:
		#endif
		switch (cmd) {
				case ASSOCIATION_GET:
					AssociationSendGetReport(
						#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
						(BOOL)(classcmd == COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2),
						#endif
						param1, sourceNode);
					break;

				case ASSOCIATION_GROUPINGS_GET:
					AssociationSendGroupings(
						#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
						(BOOL)(classcmd == COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2),
						#endif
						sourceNode);
					break;

				case ASSOCIATION_SET:
					AssociationAdd(
						#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
						(BOOL)(classcmd == COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2),
						#endif
						param1, (BYTE*)pCmd + OFFSET_PARAM_2, cmdLength - OFFSET_PARAM_2);
					break;

				case ASSOCIATION_REMOVE:
					if (cmdLength == OFFSET_PARAM_1) {
						#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
						if (classcmd == COMMAND_CLASS_ASSOCIATION)
						#endif
							return; // to short command - not to remove everything in all groups.
						#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
						else {
							param1 = 0; // to remove everything in all groups for MIA // param1 is not a pointer to pCmd memory, so it is safe to assign it
							cmdLength++; // to make the size passed to AssociationRemove = 0
						}
						#endif
					}
					AssociationRemove(
						#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
						(BOOL)(classcmd == COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2),
						#endif
						param1, (BYTE*)pCmd + OFFSET_PARAM_2, cmdLength - OFFSET_PARAM_2);
					break;

				case ASSOCIATION_SPECIFIC_GROUP_GET_V2:
					AssociationSendSpecificGroupReport(/* only applies to ASSOCIATION CC :: (BOOL)(classcmd == COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2), */ sourceNode);
					break;
			}
			break;
		#endif

		#if WITH_CC_NODE_NAMING
		case COMMAND_CLASS_NODE_NAMING:
			switch (cmd) {
				case NODE_NAMING_NODE_NAME_GET:
				case NODE_NAMING_NODE_LOCATION_GET:
					if ((txBuf = CreatePacket()) != NULL) {
						txBuf->ZW_NodeNamingNodeNameReportFrame.cmdClass = COMMAND_CLASS_NODE_NAMING;
						txBuf->ZW_NodeNamingNodeNameReportFrame.cmd = (cmd == NODE_NAMING_NODE_NAME_GET) ? NODE_NAMING_NODE_NAME_REPORT : NODE_NAMING_NODE_LOCATION_REPORT;
						report_value_b =  ZME_GET_BYTE((cmd == NODE_NAMING_NODE_NAME_GET) ? EE(NODE_NAME_ENCODING) : EE(NODE_LOCATION_ENCODING));
						config_param_size = report_value_b >> NODE_NAMING_NODE_NAME_REPORT_LEVEL_RESERVED_SHIFT;
						txBuf->ZW_NodeNamingNodeNameReportFrame.level = report_value_b & NODE_NAMING_NODE_LOCATION_REPORT_LEVEL_CHAR_PRESENTATION_MASK;
						ZME_GET_BUFFER((cmd == NODE_NAMING_NODE_NAME_GET) ? EE(NODE_NAME) : EE(NODE_LOCATION), &txBuf->ZW_NodeNamingNodeNameReportFrame.nodeNameChar1, config_param_size);
						EnqueuePacket(sourceNode, sizeof(txBuf->ZW_NodeNamingNodeNameReportFrame) - 16 + config_param_size, txOption);
					}
					break;

				case NODE_NAMING_NODE_NAME_SET:
				case NODE_NAMING_NODE_LOCATION_SET:
					report_value_b = ((cmdLength - OFFSET_PARAM_2) > 16) ? 16 : (cmdLength - OFFSET_PARAM_2);
					ZME_PUT_BYTE((cmd == NODE_NAMING_NODE_NAME_SET) ? EE(NODE_NAME_ENCODING) : EE(NODE_LOCATION_ENCODING), (param1 & NODE_NAMING_NODE_LOCATION_REPORT_LEVEL_CHAR_PRESENTATION_MASK) | (report_value_b << NODE_NAMING_NODE_NAME_REPORT_LEVEL_RESERVED_SHIFT));
					ZME_PUT_BUFFER((cmd == NODE_NAMING_NODE_NAME_SET) ? EE(NODE_NAME) : EE(NODE_LOCATION), (BYTE*)pCmd + OFFSET_PARAM_2, report_value_b, NULL);
					break;
			}
			break;
		#endif

		#if WITH_CC_DOOR_LOCK_LOGGING
		case COMMAND_CLASS_DOOR_LOCK_LOGGING:
			switch (cmd) {
				case DOOR_LOCK_LOGGING_RECORDS_SUPPORTED_GET:
					if ((txBuf = CreatePacket()) != NULL) {
						txBuf->ZW_DoorLockLoggingRecordsSupportedReportFrame.cmdClass = COMMAND_CLASS_DOOR_LOCK_LOGGING;
						txBuf->ZW_DoorLockLoggingRecordsSupportedReportFrame.cmd = DOOR_LOCK_LOGGING_RECORDS_SUPPORTED_REPORT;
						txBuf->ZW_DoorLockLoggingRecordsSupportedReportFrame.maxRecordsStored = DOOR_LOCK_LOGGING_RECORDS_MAX_SUPPORTED_NUMBER;
						EnqueuePacket(sourceNode, sizeof(txBuf->ZW_DoorLockLoggingRecordsSupportedReportFrame), txOption);
					}
				break;

				case RECORD_GET:
					sendDoorLockLoggingReport(param1,sourceNode);
				break;
			}
		break;
		#endif

		// handle other classes in universal function used for channels 1-N and for channel 0
		default:
			MultichannelCommandHandler(sourceNode, pCmd, cmdLength);
	}
	
	#if WITH_CC_MULTICHANNEL 
		MULTICHANNEL_SRC_END_POINT = 0;
		MULTICHANNEL_DST_END_POINT = 0;
	#endif

	#if !PRODUCT_LISTENING_TYPE  || PRODUCT_DYNAMIC_TYPE
		#if PRODUCT_DYNAMIC_TYPE
		if (DYNAMIC_TYPE_CURRENT_VALUE != DYNAMIC_MODE_ALWAYS_LISTENING) {
		#endif
			#if WITH_CC_SECURITY
			if (classcmd != COMMAND_CLASS_SECURITY || cmd != SECURITY_NONCE_REPORT) 
			#endif
				ShortKickSleep(); // packet received - kick sleep timer
		#if PRODUCT_DYNAMIC_TYPE
		}
		#endif
	#endif
}

void ApplicationNodeinformation(BYTE *deviceOptionsMask, APPL_NODE_TYPE *nodeType, BYTE **nodeParm, BYTE *parmLength) {
	#if !PRODUCT_DYNAMIC_TYPE
	*deviceOptionsMask = (
		#if PRODUCT_LISTENING_TYPE
			APPLICATION_NODEINFO_LISTENING |
		#else 
			APPLICATION_NODEINFO_NOT_LISTENING |
			#if PRODUCT_FLIRS_TYPE == FLIRS_1000
				APPLICATION_FREQ_LISTENING_MODE_1000ms |
			#elif PRODUCT_FLIRS_TYPE == FLIRS_250
				APPLICATION_FREQ_LISTENING_MODE_250ms |
			#endif
		#endif
		APPLICATION_NODEINFO_OPTIONAL_FUNCTIONALITY
		);
	#else
	switch(DYNAMIC_TYPE_CURRENT_VALUE) {
		case DYNAMIC_MODE_ALWAYS_LISTENING:
			*deviceOptionsMask = (APPLICATION_NODEINFO_LISTENING | APPLICATION_NODEINFO_OPTIONAL_FUNCTIONALITY);
		break;
		
		case DYNAMIC_MODE_WAKE_UP:
			*deviceOptionsMask = (APPLICATION_NODEINFO_NOT_LISTENING | APPLICATION_NODEINFO_OPTIONAL_FUNCTIONALITY);
		break;
		
		case DYNAMIC_MODE_FLIRS:
			*deviceOptionsMask = (APPLICATION_NODEINFO_NOT_LISTENING | 
				#if PRODUCT_FLIRS_TYPE == FLIRS_1000
					APPLICATION_FREQ_LISTENING_MODE_1000ms |
				#elif PRODUCT_FLIRS_TYPE == FLIRS_250
					APPLICATION_FREQ_LISTENING_MODE_250ms |
				#endif
				APPLICATION_NODEINFO_OPTIONAL_FUNCTIONALITY);
		break;
	}
	#endif
	nodeType->generic = _PRODUCT_GENERIC_TYPE;
	nodeType->specific = _PRODUCT_SPECIFIC_TYPE;
	
	*nodeParm = nodeInfoCCs;
	*parmLength = nodeInfoCCs_size;
}

void ApplicationRfNotify(BYTE rfState) {}

//========================== ApplictionControllerUpdate =======================
// Inform a controller application that a node information is received.
// Called from the controller command handler when a node information frame
// is received and the Z-Wave protocol is not in a state where it is needed.
//
// IN Status event
// IN Node id of the node that send node info
// IN Pointer to Application Node information
// IN Node info length
//--------------------------------------------------------------------------*/
#ifdef ZW_CONTROLLER
void ApplicationControllerUpdate(BYTE bStatus, BYTE bNodeID, BYTE* pCmd, BYTE bLen) {
	CONTROLLER_UPDATE_ACTION(bStatus, bNodeID, pCmd, bLen);
	switch(bStatus)
	{
		case UPDATE_STATE_SUC_ID:
			sucNodeId = bNodeID;
			break;

		#ifdef WITH_SMART_REMOVE_EXCLUDED_FROM_GROUPS
		case UPDATE_STATE_DELETE_DONE:
			RemoveFromAllGroups(bNodeID);
			break;
		#endif
	}
}
#endif

#ifdef ZW_SLAVE
void ApplicationSlaveUpdate (BYTE bStatus, BYTE bNodeID, BYTE* pCmd, BYTE bLen) {
	SLAVE_UPDATE_ACTION(bStatus, bNodeID, pCmd, bLen);
}
#endif

void ApplicationTestPoll(void) {}
