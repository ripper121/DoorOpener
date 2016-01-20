///////////////////////////////////////////////////////////////
// Common helpers



#define VAR(name) eeParams[EEmemory(name)]
#define WORD_VAR(name) (*((WORD *)(&(VAR(name)))))
#define DWORD_VAR(name) (*((DWORD *)(&(VAR(name)))))

#define WORD_HI_BYTE(variable) (*((BYTE *)(&(variable))))
#define WORD_LO_BYTE(variable) (*((BYTE *)(&(variable)) + 1))

#define Nth_BYTE(variable, n) (*((BYTE *)(&(variable)) + n))

#define GET_HI_4BITS(B) ((B&0xF0) >> 4)
#define GET_LOW_4BITS(B) (B&0x0F)
#define SET_HI_4BITS(B, S) ((B&0x0F) | ((S&0x0F) << 4))
#define SET_LOW_4BITS(B, S)((B&0xF0) | (S&0x0F) ) 

#define GET_HI_8BITS(B)		((B&0xFF00) >> 8)
#define GET_LOW_8BITS(B)	(B&0x00FF)
#define SET_HI_8BITS(B, S)	((B&0x00FF) | (((WORD)(S&0x0FF))<< 8))
#define SET_LOW_8BITS(B, S) ((B&0xFF00) | (S&0xFF) ) 

#define GET_CHN_4BITS(B, N)				(N ? GET_HI_4BITS(B) : GET_LOW_4BITS(B))	
#define SET_CHN_4BITS(B, S, N)			(N ? SET_HI_4BITS(B, S) : SET_LOW_4BITS(B, S))	


#define EEPROM_WRITE_HALF_BYTE(addr, n, value) { value |= GET_CHN_4BITS(ZME_GET_BYTE(addr), 1-n); \
									     	     ZME_PUT_BYTE(addr, value); }

#define EEPROM_WRITE_WORD(addr, value) { \
											ZME_PUT_BYTE(addr    ,	WORD_HI_BYTE(value)); \
											ZME_PUT_BYTE(addr + 1, 	WORD_LO_BYTE(value)); }
#define EEPROM_WRITE_DWORD(addr, value) { \
											ZME_PUT_BYTE(addr    , Nth_BYTE(value, 0)); \
								 		  ZME_PUT_BYTE(addr + 1, Nth_BYTE(value, 1)); \
								 		  ZME_PUT_BYTE(addr + 2, Nth_BYTE(value, 2)); \
								 		  ZME_PUT_BYTE(addr + 3, Nth_BYTE(value, 3)); \
								 		}
#define EEPROM_WRITE_ARRAY(addr, value, length) { \
											BYTE i; \
											for (i = 0; i < length; i++) { \
												ZME_PUT_BYTE(addr + i, Nth_BYTE(value, i)); \
											} \
								 		}

#define EEPROM_READ_WORD(addr, buffer) { ZME_GET_BUFFER(addr, buffer, 2);}
#define EEPROM_READ_DWORD(addr, buffer) { ZME_GET_BUFFER(addr, buffer, 4);}
#define EEPROM_READ_ARRAY(addr, buffer, length) { ZME_GET_BUFFER(addr, buffer, length);}

#define SAVE_EEPROM_BYTE(name) { ZME_PUT_BYTE(EE(name), VAR(name)); }
#define SAVE_EEPROM_WORD(name) { EEPROM_WRITE_WORD(EE(name), WORD_VAR(name)); }
#define SAVE_EEPROM_DWORD(name){ EEPROM_WRITE_DWORD(EE(name), DWORD_VAR(name));}
#define SAVE_EEPROM_ARRAY(name, length){ EEPROM_WRITE_ARRAY(EE(name), VAR(name), length);}

#define SAVE_EEPROM_BYTE_DEFAULT(name)  {       VAR(name) = DEFAULT_ ## name; SAVE_EEPROM_BYTE(name); }
#define SAVE_EEPROM_WORD_DEFAULT(name)  {  WORD_VAR(name) = DEFAULT_ ## name; SAVE_EEPROM_WORD(name); }
#define SAVE_EEPROM_DWORD_DEFAULT(name) { DWORD_VAR(name) = DEFAULT_ ## name; SAVE_EEPROM_DWORD(name);}
#define SAVE_EEPROM_ARRAY_DEFAULT(name, length) { \
											BYTE i; \
											for (i = 0; i < length; i++) { \
												Nth_BYTE(VAR(name), i) = (DEFAULT_ ## name)[i]; \
												ZME_PUT_BYTE(EE(name) + i, Nth_BYTE(VAR(name), i)); \
											} \
								 		}

// Offsets in received frame
#define OFFSET_CLASSCMD			0x00
#define OFFSET_CMD				0x01
#define OFFSET_PARAM_1			0x02
#define OFFSET_PARAM_2			0x03
#define OFFSET_PARAM_3			0x04
#define OFFSET_PARAM_4			0x05

#define FILL_MULTIBYTE_PARAM4(N, V)\
	N##1 = 	V >> 24;\
	N##2 = 	(V >> 16) & 0xff;\
	N##3 = 	(V >> 8) & 0xff;\
	N##4 = 	V  & 0xff;
#define FILL_MULTIBYTE_PARAM3(N, V)\
	N##1 = 	(V >> 16) & 0xff;\
	N##2 = 	(V >> 8) & 0xff;\
	N##3 = 	V  & 0xff;
#define FILL_MULTIBYTE_PARAM2(N, V)\
	N##1 = 	(V >> 8) & 0xff;\
	N##2 = 	V  & 0xff;

#define NSUFFIXMACRO(N, S, E) N##S##E


///////////////////////////////////////////////////////////////
// Forward declarations

static void RestoreToDefault(void);
void SendReportToNode(BYTE dstNode, BYTE classcmd, BYTE reportparam);
void TickSleepLock(void);
void SendNIF(void);
static void Timer10ms(void);
void SendNIFCallback(BYTE txStatus);

#if WITH_CC_ASSOCIATION
#if CONTROL_CC_DOOR_LOCK
void SendDoorLockGroup(BYTE group, BYTE mode);
#endif
#if CONTROL_CC_BASIC
void SendSet(BYTE group, BYTE classcmd, BYTE value);
#endif
void SendReport(BYTE group, BYTE classcmd, BYTE reportparam);
#if CONTROL_CC_SWITCH_MULTILEVEL
void SendStartChange(BYTE group, BOOL up_down, BOOL doIgnoreStartLevel, BYTE startLevel);
void SendStopChange(BYTE group);
#endif
#if CONTROL_CC_SCENE_ACTIVATION
void SendScene(BYTE group, BYTE sceneId, BYTE dimmingDuration);
#define SendSceneNoDimming(group, sceneId) SendScene(group, sceneId, 0xFF /* Device Specific */)
#if WITH_CC_SCENE_CONTROLLER_CONF
void SendSceneToGroup(BYTE groupNum);
#endif
#endif
#endif

#if WITH_CC_DEVICE_RESET_LOCALLY
void deviceResetLocallyNotify(void);
void ResetDeviceLocally(void);
#endif

#if WITH_CC_ASSOCIATION
#if WITH_CC_SENSOR_MULTILEVEL
void SendSensorMultilevelReportToGroup(BYTE deviceType, BYTE group, WORD measurement);
#endif
#endif

// exported from other files
void ApplicationPollFunction(void);

///////////////////////////////////////////////////////////////
// Device features definitions

BOOL hardBlock = FALSE;

#if !PRODUCT_LISTENING_TYPE || PRODUCT_DYNAMIC_TYPE
BYTE sleepTimerHandle = 0xFF;
#define WAIT_BEFORE_SLEEP_TIME                          250 // 2.5 seconds

BOOL sleepLocked = FALSE;

static void GoSleep(void);
static void LongKickSleep(void);
static void ShortKickSleep(void);
static void ReleaseSleepLock(void);
#endif

#if WITH_NWI
BOOL bNWIStartup;
#define DISABLE_NWI() { bNWIStartup = FALSE; }
#else
#define DISABLE_NWI() {} // not implemented if no NWI mode
#endif

#define LEARN_MODE_START() { \
			StartLearnModeNow(ZW_SET_LEARN_MODE_CLASSIC); \
			mode = MODE_LEARN; \
			LEDSet(LED_NWI); \
			DISABLE_NWI(); \
		}

#define LEARN_MODE_STOP() { \
			StartLearnModeNow(ZW_SET_LEARN_MODE_DISABLE); \
			mode = MODE_IDLE; \
			LEDSet(LED_NO); \
			DISABLE_NWI(); \
		}

///////////////////////////////////////////////////////////////
// EEPROM holders for CCs
#if WITH_CC_DOOR_LOCK
#define DOOR_LOCK_DATA_SIZE		6
#endif

#if WITH_CC_WAKE_UP || !PRODUCT_LISTENING_TYPE || defined(CUSTOM_NON_ZERO_VARIABLES)  || PRODUCT_DYNAMIC_TYPE
#include <ZW_non_zero.h>
typedef struct _NON_ZERO_ADDR {
	#if !PRODUCT_LISTENING_TYPE  || PRODUCT_DYNAMIC_TYPE
	BYTE myNodeId; // This value is also stored in SRAM not to read EEPROM each time on boot (needed to guess how to sleep)
	#endif
	#if WITH_CC_WAKE_UP
	BYTE wakeupCounterMagic; // To be sure that SRAM contains correct values
	WORD wakeupCount; // Wakeup timeout in SLEEP_TIME units
	BYTE wakeupEnabled;
	#if FIRMWARE_UPGRADE_BATTERY_SAVING
	BYTE wakeupFWUpdate;
	#endif
	
	#endif
	#if (!PRODUCT_LISTENING_TYPE  || PRODUCT_DYNAMIC_TYPE) && FIRST_RUN_BATTERY_INSERT_ACTION_PRESENT 
	BYTE firstRunBatteryInsert;
	#endif
	#if defined(CUSTOM_NON_ZERO_VARIABLES)
	CUSTOM_NON_ZERO_VARIABLES
	#endif
	#if WITH_CC_CENTRAL_SCENE
	BYTE centralSceneSequence;
	#endif

	#if WITH_PENDING_BATTERY
	BYTE pendingBatteryVal;
	BYTE pendingBatteryCount;
	#endif
} NON_ZERO_ADDR;
NON_ZERO_ADDR non_zero_variables _at_ (NON_ZERO_START_ADDR);
#define NON_ZERO_MAGIC_VALUE	0x42

#define NZ(name) non_zero_variables.##name

// to make life easier
#if !PRODUCT_LISTENING_TYPE  || PRODUCT_DYNAMIC_TYPE
#define myNodeId		non_zero_variables.myNodeId
#endif

#endif

///////////////////////////////////////////////////////////////
// CCs defines
#if WITH_CC_ASSOCIATION
#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
#define ASSOCIATION_EE_SIZE (ASSOCIATION_NUM_GROUPS * (1+2)*ASSOCIATION_GROUP_SIZE)
#else
#define ASSOCIATION_EE_SIZE (ASSOCIATION_NUM_GROUPS * ASSOCIATION_GROUP_SIZE)
#endif
#endif

#if WITH_CC_SCENE_CONTROLLER_CONF
#define SCENE_HOLDER_SIZE                       2 // Scene Number + Dimming Duration
#define SCENE_CONTROLLER_EE_SIZE		(ASSOCIATION_NUM_GROUPS * SCENE_HOLDER_SIZE)
#endif

#if WITH_CC_SCENE_ACTUATOR_CONF
#define SCENE_ACTUATOR_EE_SIZE			256 // 0: current Scene, 1-255 - levels for scenes.
#if SCENE_ACTUATOR_EE_SIZE-1 > 255
	#error attempt to read more than 255 bytes - split in several reads
#endif
XBYTE scenesAct[SCENE_ACTUATOR_EE_SIZE];
void ScenesActuatorClearAll(void);
#endif

///////////////////////////////////////////////////////////////
// EEPROM holders for CCs

#if defined(ZW050x)
#define EE(name) ((WORD)&(eeParamsEEPROM[EEOFFSET_ ## name]))
#else
#define EE(name) EEOFFSET_ ## name
#endif

#define EEmemory(name) EEOFFSET_ ## name

#define EE_VAR(name, size) \
	EEmemory(name), \
	___EE ## name = EEmemory(name) + size - 1, // just to allocate these numbers in enum to force compiler to use next available number

#define EEOFFSET_0 0
#define EEOFFSET_START	EE(0)

#define FWUPDATE_VECTOR_SIZE 8
enum {
	#if WITH_CC_PROTECTION
	EE_VAR(PROTECTION, 1)
	#endif

	#if WITH_CC_DOOR_LOCK
	EE_VAR(DOOR_LOCK_DATA, DOOR_LOCK_DATA_SIZE)
	EE_VAR(DOOR_LOCK_TIMEOUT_FLAG,1)
	EE_VAR(DOOR_LOCK_TIMEOUT_MINUTES,1)
	EE_VAR(DOOR_LOCK_TIMEOUT_SECONDS,1)
	#endif

	#if WITH_CC_DOOR_LOCK_LOGGING
	EE_VAR(DOOR_LOCK_LOGGING_BUF_INDEX, 1)
	EE_VAR(DOOR_LOCK_LOGGING_LAST_REPORT, 1)
	#endif

	#if WITH_CC_SWITCH_ALL
	EE_VAR(SWITCH_ALL_MODE, 1)
	#endif

	#if CONTROL_CC_SWITCH_ALL
	EE_VAR(CONTROL_SWITCH_ALL_MODE, 1)
	#endif

	#if WITH_CC_WAKE_UP && WITH_CC_BATTERY
	EE_VAR(UNSOLICITED_BATTERY_REPORT, 1)
	EE_VAR(ENABLE_WAKEUPS, 1)
	#endif

	#if WITH_CC_BATTERY && defined(ZW050x)
	EE_VAR(BATTERY_REPORT_ON_POR_SENT, 1)
	#endif
	
	#if WITH_CC_WAKE_UP
	EE_VAR(WAKEUP_NODEID, 1)
	EE_VAR(SLEEP_PERIOD, 4) // fisrt byte is used only to map value to DWORD
	#endif

	#if WITH_CC_SECURITY
	EE_VAR(SECURITY_MODE, 1)
	EE_VAR(NETWORK_KEY, 16)
	EE_VAR(SECURE_NODE_LIST, NODE_MASK_LENGTH)
	EE_VAR(SECURE_NODE_LIST_TEST, NODE_MASK_LENGTH)
	#endif
	
	#if WITH_CC_THERMOSTAT_MODE
	EE_VAR(THERMOSTAT_MODE, 1)
	#endif

	#if WITH_CC_THERMOSTAT_SETPOINT
	#if CC_THERMOSTAT_SETPOINT_WITH_HEATING_1_V2
	EE_VAR(THERMOSTAT_SETPOINT_HEAT, 2)
	#endif
	#if CC_THERMOSTAT_SETPOINT_WITH_COOLING_1_V2
	EE_VAR(THERMOSTAT_SETPOINT_COOL, 2)
	#endif
	#if CC_THERMOSTAT_SETPOINT_WITH_FURNACE_V2
	EE_VAR(THERMOSTAT_SETPOINT_FURNACE, 2)
	#endif
	#if CC_THERMOSTAT_SETPOINT_WITH_DRY_AIR_V2
	EE_VAR(THERMOSTAT_SETPOINT_DRY_AIR, 2)
	#endif
	#if CC_THERMOSTAT_SETPOINT_WITH_MOIST_AIR_V2
	EE_VAR(THERMOSTAT_SETPOINT_MOIST_AIR, 2)
	#endif
	#if CC_THERMOSTAT_SETPOINT_WITH_AUTO_CHANGEOVER_V2
	EE_VAR(THERMOSTAT_SETPOINT_AUTO_CHANGEOVER, 2)
	#endif
	#if CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_HEATING_V2
	EE_VAR(THERMOSTAT_SETPOINT_ENERGY_SAVE_HEAT, 2)
	#endif
	#if CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_COOLING_V2
	EE_VAR(THERMOSTAT_SETPOINT_ENERGY_SAVE_COOL, 2)
	#endif
	#if CC_THERMOSTAT_SETPOINT_WITH_AWAY_HEATING_V2
	EE_VAR(THERMOSTAT_SETPOINT_AWAY_HEAT, 2)
	#endif
	#if CC_THERMOSTAT_SETPOINT_WITH_AWAY_COOLING_V3
	EE_VAR(THERMOSTAT_SETPOINT_AWAY_COOL, 2)
	#endif
	#if CC_THERMOSTAT_SETPOINT_WITH_FULL_POWER_V3
	EE_VAR(THERMOSTAT_SETPOINT_FULL_POWER, 2)
	#endif
	#endif

	#if WITH_TRIAC
	EE_VAR(TRIAC_T_BEFORE_PULSE, 	1)
	EE_VAR(TRIAC_T_AFTER_PULSE, 	1)
	EE_VAR(TRIAC_T_PULSE_WIDTH, 	1)
	EE_VAR(TRIAC_PULSE_TYPE, 		1)
	#endif
	
	#if WITH_CC_POWERLEVEL
	EE_VAR(POWERLEVEL_TEST_NODE, 		1)
	EE_VAR(POWERLEVEL_TEST_LEVEL, 		1)
	EE_VAR(POWERLEVEL_TEST_FULL_COUNT, 	2)
	EE_VAR(POWERLEVEL_TEST_ACK_COUNT, 	2)
	#endif
	
	#if WITH_CC_FIRMWARE_UPGRADE
	#if FIRMWARE_UPGRADE_BATTERY_SAVING
	EE_VAR(FWUPGRADE_STATEVECTOR, 		FWUPDATE_VECTOR_SIZE)
	#endif
	#endif


	/*
	#if WITH_CC_METER_TABLE_MONITOR
	EE_VAR(METER_TABLE_ADM_NUMBER, 16)
	EE_VAR(METER_TABLE_ID_NUMBER, 16)
	#endif
	*/

	#if WITH_CC_MULTI_COMMAND
	EE_VAR(MULTICMD_NODE_LIST, NODE_MASK_LENGTH)
	#endif
	
	CUSTOM_EE_VARS

	EE_VAR(DELIMITER, 1) // this is to separate eeParams from other holders. It wastes one byte :(

	#if WITH_CC_NODE_NAMING
	EE_VAR(NODE_LOCATION_ENCODING, 1) // We suppose nodenaming and encodings to be one after other // just to minimize the code in NodeNamin
	EE_VAR(NODE_LOCATION, 16)
	EE_VAR(NODE_NAME_ENCODING, 1)
	EE_VAR(NODE_NAME, 16)
	#endif

	#if WITH_CC_ASSOCIATION
	EE_VAR(ASSOCIATIONS, ASSOCIATION_EE_SIZE)
	#endif

	#if WITH_CC_SCENE_CONTROLLER_CONF
	EE_VAR(SCENE_CONTROLLER_CONF, SCENE_CONTROLLER_EE_SIZE)
	#endif

	#if WITH_CC_SCENE_ACTUATOR_CONF
	EE_VAR(SCENE_ACTUATOR_CONF_LEVEL, SCENE_ACTUATOR_EE_SIZE)
	#endif

	EE_VAR(MAGIC, 1)
};

#define MAGIC_VALUE	0x42

XBYTE eeParams[EEmemory(DELIMITER) - EEOFFSET_0]; // This array holds all parameters that can be saved into EEPROM. They are ordered in the same way to restore it fastly.

#if defined(ZW050x)
BYTE far eeParamsEEPROM[EEmemory(MAGIC) - EEOFFSET_0 + 1];  //_at_ 0x00800;
#ifdef ZW_SLAVE_32
BYTE far eeConstantProductionParameters[8] _at_ 0x13000;
/*********************************************************************************************************************
-------------------------------------------------NVR------------------------------------------------------------------
0x80			0x81			0x82			0x83			0x84			0x85			0x86			0x87
-------------------------------------------------NVM------------------------------------------------------------------
0x3000 			0x3001 			0x3002 			0x3003 			0x3004 			0x3005 			0x3006 			0x3007
----------------------------------------------------------------------------------------------------------------------
WEEK 			YEAR 			CHIP_REEL 		CHIP_REEL 		INCR_1 			INCR_2 			WRK_STN_ID 		CRC8
*********************************************************************************************************************/
#endif
#endif

enum {
	SEND_CBK_NIF = 1, // !!!
	#if WITH_CC_WAKE_UP
	SEND_CBK_WAKEUP_NOTIFICATION,
	#endif
	#ifdef SendCallbackCustomID
	SendCallbackCustomID
	#endif
};

////////////////////////////////////////////////////////////////
// Дополнительное "железо"

#if WITH_TRIAC

#define TRIAC_PULSE_LONG		FALSE
#define TRIAC_PULSE_SHORT		TRUE
#define TRIAC_OUSIDE_PULSE_MIN	5
#define TRIAC_OUSIDE_PULSE_MAX	60
#define TRIAC_PULSE_WIDTH_MIN	3
#define TRIAC_PULSE_WIDTH_MAX	20

void TRIAC_Init(void);
void TRIAC_Set(BYTE level);

#define DEFAULT_TRIAC_T_BEFORE_PULSE	28
#define DEFAULT_TRIAC_T_AFTER_PULSE		28
#define DEFAULT_TRIAC_T_PULSE_WIDTH		10
#define DEFAULT_TRIAC_PULSE_TYPE		TRIAC_PULSE_LONG

#endif	

///////////////////////////////////////////////////////////////
// Supported CCs definitions

#if WITH_CC_ASSOCIATION
void AssociationStoreAll(void);
void AssociationClearAll(void);
void AssociationInit(void);
#endif

#if WITH_CC_SCENE_CONTROLLER_CONF
void ScenesClearAll(void);
#endif

#if WITH_CC_PROTECTION
#define DEFAULT_PROTECTION	PROTECTION_REPORT_UNPROTECTED

BOOL protectedUnlocked = FALSE;

// Tick counters in seconds
BYTE protectionSecondsCounter = 0;
BOOL protectionSecondsDoCount = FALSE;

#define PROTECTION_UNLOCK() { if (VAR(PROTECTION) == PROTECTION_SET_PROTECTION_BY_SEQUENCE) { ProtectionCounterStart(); protectedUnlocked = TRUE; LEDSet(LED_DONE); }}
#define PROTECTION_KICK() { if (VAR(PROTECTION) == PROTECTION_SET_PROTECTION_BY_SEQUENCE) ProtectionCounterStart(); }
#define PROTECTION_UNLOCKED() (VAR(PROTECTION) == PROTECTION_REPORT_UNPROTECTED || (protectedUnlocked && VAR(PROTECTION) == PROTECTION_REPORT_PROTECTION_BY_SEQUENCE))

static void ProtectionCounterStart(void);
#endif


#if WITH_CC_DOOR_LOCK
/**
 * Door Lock Mode (8 bit) will set the door lock device in unsecured or 
 * secured mode as well as other peripheral settings.
 *
 * 1) Constant mode. Door will be unsecured until set back to secured mode by Command.
 * 2) Timeout mode. Fallback to secured mode after timeout has expired (set by Door Lock Configuration Set).
 * 3) This is Read Only State, i.e. Bolt is not fully retracted/engaged
 */
typedef enum
{
  DOOR_MODE_UNSEC = DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED_V2,	/**< Door Unsecured 1)*/
  DOOR_MODE_UNSEC_TIMEOUT = DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED_WITH_TIMEOUT_V2,	/**< Door Unsecured with timeout 2)*/
  DOOR_MODE_UNSEC_INSIDE = DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED_FOR_INSIDE_DOOR_HANDLES_V2,	/**< Door Unsecured for inside Door Handles 1)*/
  DOOR_MODE_UNSEC_INSIDE_TIMEOUT = DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED_FOR_INSIDE_DOOR_HANDLES_WITH_TIMEOUT_V2,	/**< Door Unsecured for inside Door Handles with timeout 2)*/
  DOOR_MODE_UNSEC_OUTSIDE = DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED_FOR_OUTSIDE_DOOR_HANDLES_V2,	/**< Door Unsecured for outside Door Handles 1)*/
  DOOR_MODE_UNSEC_OUTSIDE_TIMEOUT = DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED_FOR_OUTSIDE_DOOR_HANDLES_WITH_TIMEOUT_V2,	/**< Door Unsecured for outside Door Handles with timeout 2)*/
  DOOR_MODE_UNKNOWN = DOOR_LOCK_OPERATION_SET_DOOR_LOCK_STATE_UNKNOWN_V2, /**<	Door/Lock State Unknown 3). (Version 2)*/
  DOOR_MODE_SECURED = DOOR_LOCK_OPERATION_SET_DOOR_SECURED_V2	/**< Door Secured*/
} DOOR_MODE;


/**
 * Operation mode (1byte).
 * The Operation Type field can be set to either constant or timed operation. When 
 * timed operation is set, the Lock Timer Minutes and Lock Timer Seconds fields 
 * MUST be set to valid values.
 */
typedef enum
{
  DOOR_OPERATION_CONST = 0x01,   /**< Constant operation*/
  DOOR_OPERATION_TIMED = 0x02,   /**< Timed operation*/
  DOOR_OPERATION_RESERVED = 0x03 /**< 0X03– 0XFF  Reserved*/
} DOOR_OPERATION;



typedef struct _CMD_CLASS_DOOR_LOCK_DATA
{
  DOOR_MODE mode;
  DOOR_OPERATION type;
  BYTE doorHandleMode; /* BIT-field Inside/Outside Door Handles Mode */
  BYTE condition; /**< Door condition (8 bits)*/
  BYTE lockTimeoutMin; /**< Lock Timeout Minutes, valid values 1-254 decimal*/
  BYTE lockTimeoutSec; /**< Lock Timeout Seconds, valid 1-59 decimal*/
} CMD_CLASS_DOOR_LOCK_DATA;

CMD_CLASS_DOOR_LOCK_DATA * pDoorLockData = (CMD_CLASS_DOOR_LOCK_DATA*) &VAR(DOOR_LOCK_DATA);

#define CONSTANT_OPERATION_TIME_VALUE					0xFE
#define TIMED_OPERATION_MINUTES_MAX_VALUE				0XFD
#define TIMED_OPERATION_SECONDS_MAX_VALUE				0X3B

#define DOOR_LOCK_CONDITION_BOLT_OPENED_MASK			0X02
#define DOOR_LOCK_CONDITION_BOLT_CLOSED_MASK			0XFD

#define DOOR_LOCK_TIMEOUT_ENABLE						0x01
#define DOOR_LOCK_TIMEOUT_DISABLE						0xFE
#define DOOR_LOCK_INSIDE_HANDLES_TIMEOUT_ENABLE			0x02
#define DOOR_LOCK_INSIDE_HANDLES_TIMEOUT_DISABLE		0XFD
#define DOOR_LOCK_OUTSIDE_HANDLES_TIMEOUT_ENABLE		0x04
#define DOOR_LOCK_OUTSIDE_HANDLES_TIMEOUT_DISABLE		0xFB


void sendDoorLockOperationReport(BYTE dstNode);
void sendDoorLockOperationReportToGroup(BYTE dstGroup);

BYTE code doorLockDefaultArray[] = { DEFAULT_DOOR_LOCK_MODE, DEFAULT_DOOR_LOCK_TYPE, DEFAULT_DOOR_LOCK_DOOR_HANDLE_MODE, DEFAULT_DOOR_LOCK_CONDITION, DEFAULT_DOOR_LOCK_TIMEOUT_MIN, DEFAULT_DOOR_LOCK_TIMEOUT_SEC };
#define DEFAULT_DOOR_LOCK_DATA		doorLockDefaultArray
#endif

//Door Lock Logging
#if WITH_CC_DOOR_LOCK_LOGGING

#define DOOR_LOCK_LOGGING_DEFAULT_CODE_LENGTH	4 // may be optimized
#define DOOR_LOCK_LOGGING_VALID_DATA_MASK		0x20
#define DEFAULT_USER_IDENTIFIER					0


#define DOOR_LOCK_LOGGING_EVENT_KEYPAD_ACCESS_CODE_VERIFIED_LOCK 			1
#define DOOR_LOCK_LOGGING_EVENT_KEYPAD_ACCESS_CODE_VERIFIED_UNLOCK 			2
#define DOOR_LOCK_LOGGING_EVENT_KEYPAD_LOCK_BUTTON_PRESSED 		 			3
#define DOOR_LOCK_LOGGING_EVENT_KEYPAD_UNLOCK_BUTTON_PRESSED 				4
#define DOOR_LOCK_LOGGING_EVENT_KEYPAD_ACCESS_LOCK_CODE_OUT_OF_SCHEDULE 	5
#define DOOR_LOCK_LOGGING_EVENT_KEYPAD_ACCESS_UNLOCK_CODE_OUT_OF_SCHEDULE 	6
#define DOOR_LOCK_LOGGING_EVENT_KEYPAD_ACCESS_CODE_ILLEGAL 		 			7
#define DOOR_LOCK_LOGGING_EVENT_KEY_LATCH_MANUAL_LOCKED			 			8
#define DOOR_LOCK_LOGGING_EVENT_KEY_LATCH_MANUAL_UNLOCKED		 			9
#define DOOR_LOCK_LOGGING_EVENT_AUTO_LOCK 						 			10
#define DOOR_LOCK_LOGGING_EVENT_AUTO_UNLOCK   					 			11
#define DOOR_LOCK_LOGGING_EVENT_Z_WAVE_ACCESS_CODE_VERIFIED_LOCK   			12
#define DOOR_LOCK_LOGGING_EVENT_Z_WAVE_ACCESS_CODE_VERIFIED_UNLOCK   		13
#define DOOR_LOCK_LOGGING_EVENT_Z_WAVE_NO_CODE_LOCK 	  					14
#define DOOR_LOCK_LOGGING_EVENT_Z_WAVE_NO_CODE_UNLOCK   					15
#define DOOR_LOCK_LOGGING_EVENT_Z_WAVE_ACCESS_LOCK_CODE_OUT_OF_SCHEDULE		16
#define DOOR_LOCK_LOGGING_EVENT_Z_WAVE_ACCESS_UNLOCK_CODE_OUT_OF_SCHEDULE	17
#define DOOR_LOCK_LOGGING_EVENT_Z_WAVE_ACCESS_CODE_ILLEGAL 					18
#define DOOR_LOCK_LOGGING_EVENT_KEY_LATCH_Z_WAVE_LOCKED						19
#define DOOR_LOCK_LOGGING_EVENT_KEY_LATCH_Z_WAVE_UNLOCKED					20
#define DOOR_LOCK_LOGGING_EVENT_LOCK_SECURED								21
#define DOOR_LOCK_LOGGING_EVENT_LOCK_UNSECURED								22
#define DOOR_LOCK_LOGGING_EVENT_USER_CODE_ADDED								23
#define DOOR_LOCK_LOGGING_EVENT_USER_CODE_DELETED							24
#define DOOR_LOCK_LOGGING_EVENT_ALL_USER_CODES_DELETED						25
#define DOOR_LOCK_LOGGING_EVENT_MASTER_CODE_CHANGED							26
#define DOOR_LOCK_LOGGING_EVENT_USER_CODE_CHANGED							27
#define DOOR_LOCK_LOGGING_EVENT_LOCK_RESET									28
#define DOOR_LOCK_LOGGING_EVENT_CONFIGURATION								29
#define DOOR_LOCK_LOGGING_EVENT_LOW_BATTERY									30
#define DOOR_LOCK_LOGGING_EVENT_NEW_BATTERY_INSTALLED						31


void sendDoorLockLoggingReport(BYTE number,BYTE DstNode);
void sendDoorLockLoggingReportToGroup(BYTE number, BYTE DstGroup);
void createDoorLockLoggingReport(BYTE event);

typedef struct _CMD_DOOR_LOCK_LOGGING_DATA {
	WORD year;
	BYTE month;
	BYTE day;
	BYTE masked_legal_and_hours;
	BYTE minutes;
	BYTE seconds;
	BYTE event;
	BYTE user;
	BYTE code_length;
	DWORD user_code; //may be optimized for various code lengths. Default 4.
} CMD_DOOR_LOCK_LOGGING_DATA;


#if defined(ZW050x)
CMD_DOOR_LOCK_LOGGING_DATA far EEOFFSET_DOOR_LOCK_LOGGING_DATA_far[DOOR_LOCK_LOGGING_RECORDS_MAX_SUPPORTED_NUMBER]; 
#else 
#error Not ready for 3rd gen, please complete the eeprom storage of the data
#endif

#endif


// Binary sensor specific routine...
#if WITH_CC_SENSOR_BINARY_V2 || WITH_CC_SENSOR_BINARY

#define SENSOR_BINARY_REPORT_TYPE_GENERAL_PURPOSE 		0x01
#define SENSOR_BINARY_REPORT_TYPE_SMOKE 				0x02
#define SENSOR_BINARY_REPORT_TYPE_CO 					0x03
#define SENSOR_BINARY_REPORT_TYPE_CO2 					0x04
#define SENSOR_BINARY_REPORT_TYPE_HEAT 					0x05
#define SENSOR_BINARY_REPORT_TYPE_WATER 				0x06
#define SENSOR_BINARY_REPORT_TYPE_FREEZE 				0x07
#define SENSOR_BINARY_REPORT_TYPE_TAMPER 				0x08
#define SENSOR_BINARY_REPORT_TYPE_AUX 					0x09
#define SENSOR_BINARY_REPORT_TYPE_DOOR_WINDOW 			0x0a
#define SENSOR_BINARY_REPORT_TYPE_TILT 					0x0b
#define SENSOR_BINARY_REPORT_TYPE_MOTION 				0x0c
#define SENSOR_BINARY_REPORT_TYPE_GLASSBREAK 			0x0d
#define SENSOR_BINARY_REPORT_TYPE_DEFAULT 				0xff	

#define BINARY_SENSOR_ON_VALUE 							0xff
#define BINARY_SENSOR_OFF_VALUE 						0x00



#define MAKE_SENSOR_BINARY_REPORT_BITMASK(NUM) | (1<<NUM) 
									
#define SENSOR_BINARY_V2_TYPE_ENABLED(ENABLED_TYPE) \
																	case ENABLED_TYPE:  \
																		sendBinarySensorReport(sourceNode, ENABLED_TYPE); \
																		break;

// You have to define macro to specify sensor value
//#define SENSOR_BINARY_VALUE(TYPE) ...

void sendBinarySensorReport(BYTE dstNode, BYTE Type);

#if WITH_CC_ASSOCIATION
void sendBinarySensorReportToGroup(BYTE Group, BYTE Type);
#endif
	
#endif

#if WITH_CC_NOTIFICATION
//Notification types
#define NOTIFICATION_TYPE_SMOKE_ALARM					0x01
#define NOTIFICATION_TYPE_CO_ALARM 						0x02
#define NOTIFICATION_TYPE_CO2_ALARM						0x03
#define NOTIFICATION_TYPE_HEAT_ALARM					0x04
#define NOTIFICATION_TYPE_WATER_ALARM 					0x05
#define NOTIFICATION_TYPE_ACCESS_CONTROL_ALARM			0x06
#define NOTIFICATION_TYPE_BURGLAR_ALARM					0x07
#define NOTIFICATION_TYPE_POWER_MANAGEMENT_ALARM		0x08
#define NOTIFICATION_TYPE_SYSTEM_ALARM 					0x09
#define NOTIFICATION_TYPE_EMERGENCY_ALARM				0x0a
#define NOTIFICATION_TYPE_CLOCK_ALARM					0x0b
#define NOTIFICATION_TYPE_APPLIANCE_ALARM				0x0c

//Notification events (UL-Unknown Location)

#define NOTIFICATION_ON_VALUE							0xff
#define NOTIFICATION_OFF_VALUE							0x00

#define NOTIFICATION_UNKNOWN_EVENT						0xfe
//Smoke
#define NOTIFICATION_EVENT_SMOKE						0x01
#define NOTIFICATION_EVENT_SMOKE_UL						0x02
#define NOTIFICATION_EVENT_SMOKE_TEST					0x03

//CO
#define NOTIFICATION_EVENT_CO							0x01
#define NOTIFICATION_EVENT_CO_UL						0x02

//CO2
#define NOTIFICATION_EVENT_CO2							0x01
#define NOTIFICATION_EVENT_CO2_UL						0x02

//HEAT
#define NOTIFICATION_EVENT_OVERHEAT						0x01
#define NOTIFICATION_EVENT_OVERHEAT_UL					0x02
#define NOTIFICATION_EVENT_TEMP_RISE					0x03
#define NOTIFICATION_EVENT_TEMP_RISE_UL					0x04
#define NOTIFICATION_EVENT_UNDERHEAT					0x05
#define NOTIFICATION_EVENT_UNDERHEAT_UL					0x06

//WATER
#define NOTIFICATION_EVENT_WATER_LEAK					0x01
#define NOTIFICATION_EVENT_WATER_LEAK_UL				0x02
#define NOTIFICATION_EVENT_WATER_LEVEL_DROP				0x03
#define NOTIFICATION_EVENT_WATER_LEVEL_DROP_UL			0x04

//ACCESS
#define NOTIFICATION_EVENT_MANUAL_LOCK					0x01
#define NOTIFICATION_EVENT_MANUAL_UNLOCK				0x02
#define NOTIFICATION_EVENT_MANUAL_NOT_FULLY_LOCKED		0x07
#define NOTIFICATION_EVENT_RF_LOCK						0x03
#define NOTIFICATION_EVENT_RF_UNLOCK					0x04
#define NOTIFICATION_EVENT_RF_NOT_FULLY_LOCKED			0x08
#define NOTIFICATION_EVENT_KEYPAD_LOCK					0x05
#define NOTIFICATION_EVENT_KEYPAD_UNLOCK				0x06
#define NOTIFICATION_EVENT_AUTO_LOCKED					0x09
#define NOTIFICATION_EVENT_AUTO_LOCK_NOT_FULLY_LOCKED	0x0a
#define NOTIFICATION_EVENT_LOCK_JAMMED					0x0b
#define NOTIFICATION_EVENT_ALL_USER_CODES_DELETED		0x0c
#define NOTIFICATION_EVENT_SINGLE_USER_CODE_DELETED		0x0d
#define NOTIFICATION_EVENT_NEW_USER_CODE_ADDED			0X0e
#define NOTIFICATION_EVENT_NEW_USER_CODE_NOT_ADDED		0x0f
#define NOTIFICATION_EVENT_KEYPAD_TEMPORARY_DISABLE		0x10
#define NOTIFICATION_EVENT_KEYPAD_BUSY					0x11
#define NOTIFICATION_EVENT_NEW_PROGRAM_CODE_ENTERED		0x12
#define NOTIFICATION_EVENT_MANUAL_CODE_EXCEEDS_LIMITS	0x13
#define NOTIFICATION_EVENT_RF_UNLOCK_INVALID_CODE		0x14
#define NOTIFICATION_EVENT_RF_LOCK_INVALID_CODE			0x15
#define NOTIFICATION_EVENT_WINDOW_DOOR_OPENED			0x16
#define NOTIFICATION_EVENT_WINDOW_DOOR_CLOSED			0x17

//BURGLAR
#define NOTIFICATION_EVENT_INTRUSION					0x01
#define NOTIFICATION_EVENT_INTRUSION_UL					0x02
#define NOTIFICATION_EVENT_TAMPER_REMOVED				0x03
#define NOTIFICATION_EVENT_TAMPER_INVALID_CODE			0x04
#define NOTIFICATION_EVENT_GLASS_BREAK					0x05
#define NOTIFICATION_EVENT_GLASS_BREAK_UL				0x06
#define NOTIFICATION_EVENT_MOTION_DETECTION				0x07

//POWER MANAGEMENT
#define NOTIFICATION_EVENT_POWER_APPLIED				0x01
#define NOTIFICATION_EVENT_AC_DISCONNECTED				0x02
#define NOTIFICATION_EVENT_AC_RECONNECTED				0x03
#define NOTIFICATION_EVENT_SURGE						0x04
#define NOTIFICATION_EVENT_VOLTAGE_DROP					0x05
#define NOTIFICATION_EVENT_OVER_CURRENT					0x06
#define NOTIFICATION_EVENT_OVER_VOLTAGE					0x07
#define NOTIFICATION_EVENT_OVER_LOAD					0x08
#define NOTIFICATION_EVENT_LOAD_ERROR					0x09
#define NOTIFICATION_EVENT_REPLACE_BAT_SOON				0x0a
#define NOTIFICATION_EVENT_REPLACE_BAT_NOW				0x0b


//SYSTEM
#define NOTIFICATION_EVENT_HW_FAIL						0x01
#define NOTIFICATION_EVENT_SW_FAIL						0x02
#define NOTIFICATION_EVENT_HW_FAIL_OEM					0x03
#define NOTIFICATION_EVENT_SW_FAIL_OEM					0x04

//EMERGENCY_ALARM
#define NOTIFICATION_EVENT_CONTACT_POLICE				0x01
#define NOTIFICATION_EVENT_CONTACT_FIREMEN				0x02
#define NOTIFICATION_EVENT_CONTACT_MEDIC				0x03

//CLOCK
#define NOTIFICATION_EVENT_WAKE_UP_ALERT				0x01
#define NOTIFICATION_EVENT_TIMER_ENDED					0x02

//Uknown event
#define NOTIFICATION_EVENT_UNKNOWN						0xfe
#define NOTIFICATION_NO_PENDING_STATUS					0xfe
#define CC_NOTIFICATION_RETURN_FIRST_DETECTED			0xff

#define NOTIFICATION_EVENT_STATUS(TYPE,EVENT) \
							case EVENT: \
								return (TYPE##_GETTER); \
								break;

//NOTIFICATION_SUPPORTED_GET_V3
#define NOTIFICATION_SUPPORTED_REPORT_BITMASK_NUMBER	0x02
#define MAKE_NOTIFICATION_REPORT_BITMASK(NUM) 	(1<<(NUM)) |

#define MAKE_NOTIFICATION_EVENT_REPORT(ENABLED_EVENT) \
											case ENABLED_EVENT: \
												sendNotificationReport(sourceNode,param2,param3,alarm_version); \
											break;

#define MAKE_NOTIFICATION_UNKNOWN_EVENT_REPORT  \
									default: \
										sendNotificationReport(sourceNode,param2,NOTIFICATION_EVENT_UNKNOWN,alarm_version); \ 
										break;

void sendNotificationReport(BYTE dstNode, BYTE Type, BYTE Event,BOOL alarm_version_flag);
#if WITH_CC_ASSOCIATION
void sendNotificationReportToGroup(BYTE Group, BYTE Type, BYTE Event);
#endif									
#endif

// Meter
#if WITH_CC_METER

// You have to define macro to specify meter value (in physical units)
// #define METER_VALUE(S)
// You have to define macro to specify meter type
// #define METER_TYPE
// You have to define macro to specify meter precision
// #define METER_PRECISION(S)
// You have to define macro to specify meter scale
// #define METER_SCALE
// You have to define macro to specify meter size
// #define METER_SIZE(S)

// #define METER_RATE_TYPE
// #define METER_RESETABLE
// #define METER_RESET_HANDLER
// #define METER_PREVIOUS_VALUE(S)
// #define METER_DELTA(S)
// #define METER_DEFAULT_SCALE


void sendMeterReport(BYTE dstNode, BYTE scale);

#if WITH_CC_ASSOCIATION
void sendMeterReportToGroup(BYTE Group, BYTE scale);
#endif

#endif

// Meter table monitor
#if WITH_CC_METER_TABLE_MONITOR

// You have to define macro to specify rate type
// #define METER_TBL_CAP_RATE_TYPE 

#define METER_TBL_RATE_TYPE_IMPORT							0x01
#define METER_TBL_RATE_TYPE_EXPORT							0x02

// You have to define macro to specify meter type
// #define METER_TBL_CAP_METER_TYPE
#define METER_TBL_METER_TYPE_SINGLE_E						0x01
#define METER_TBL_METER_TYPE_GAS							0x02
#define METER_TBL_METER_TYPE_WATER							0x03
#define METER_TBL_METER_TYPE_TWIN_E							0x04
#define METER_TBL_METER_TYPE_3P_SINGLE_DIRECT				0x05
#define METER_TBL_METER_TYPE_3P_SINGLE_ECT					0x06
#define METER_TBL_METER_TYPE_1P_DIRECT						0x07
#define METER_TBL_METER_TYPE_HEATING						0x08
#define METER_TBL_METER_TYPE_COOLING						0x09
#define METER_TBL_METER_TYPE_COMB_HEATING_COOLING			0x0A
#define METER_TBL_METER_TYPE_ELECTRIC_SUB					0x0B

 
// You have to define macro to specify payment
// #define METER_TBL_CAP_METER_PAY
#define METER_TBL_PAY_METER_CREDIT 							0x01
#define METER_TBL_PAY_METER_PREPAYMENT 						0x02
#define METER_TBL_PAY_METER_PREPAYMENT_W_DEBIT_RECOVERY 	0x03

// You have to define macro to specify dataset1
// #define METER_TBL_CAP_DATASET1

#define METER_TBL_WATER_GAS_DATASET1_ACC_VOLUME				0x01
#define METER_TBL_WATER_GAS_DATASET1_CURRENT_FLOW			0x02
#define METER_TBL_WATER_GAS_DATASET1_CURRENT_PRESSURE		0x04
#define METER_TBL_WATER_GAS_DATASET1_PEAK_FLOW				0x08
#define METER_TBL_WATER_GAS_DATASET1_HOUR_COUNTER			0x10
#define METER_TBL_WATER_GAS_DATASET1_INPUT_A				0x20
#define METER_TBL_WATER_GAS_DATASET1_INPUT_B				0x40

#define METER_TBL_ZWME_DATASET4_HISTORICAL					0x01000000
#define METER_TBL_ZWME_DATASET4_GROUP						0x02000000


// You have to define macro to specify dataset2
// #define METER_TBL_CAP_DATASET2

// You have to define macro to specify dataset3
// #define METER_TBL_CAP_DATASET3

// You have to define macro to specify history size
// #define METER_TBL_CAP_HISTORY_SIZE

// You have to define macro to handle data request
// #define METER_TBL_DATA_HANDLER(SRC_NODE, DATASET)

// You have to define macro to handle historical data request
// #define METER_TBL_DATA_HISTORICAL_HANDLER(SRC_NODE, MAX_REPORTS, DATASET, START_DS, STOP_DS)

#define METER_TBL_GAS_WATER_SCALE_M3						0x00
#define METER_TBL_GAS_WATER_SCALE_F3						0x01
#define METER_TBL_GAS_WATER_SCALE_USGALLON					0x02
#define METER_TBL_GAS_WATER_SCALE_PULSES					0x03
#define METER_TBL_GAS_WATER_SCALE_IMPGALLON					0x04
#define METER_TBL_GAS_WATER_SCALE_LITER						0x05
#define METER_TBL_GAS_WATER_SCALE_KPA						0x06
#define METER_TBL_GAS_WATER_SCALE_F3DIV100					0x07
#define METER_TBL_GAS_WATER_SCALE_M3_PH						0x08
#define METER_TBL_GAS_WATER_SCALE_LITER_PH					0x09
#define METER_TBL_GAS_WATER_SCALE_KWH						0x0A
#define METER_TBL_GAS_WATER_SCALE_MWH						0x0B
#define METER_TBL_GAS_WATER_SCALE_KW						0x0C
#define METER_TBL_GAS_WATER_SCALE_HOURS						0x0D


void sendMeterTableDataReport(BYTE dstNodeOrGroup, 
							  BYTE reportsToFollow,
							  DWORD datasetn,
							  DWORD timestamp,
							  BYTE  precScale,
							  DWORD value);
							  


 

#endif

#if WITH_CC_MULTICHANNEL

void MultichannelBegin();
void MultichannelSet(BYTE src, BYTE dst);
void MultichannelEnd();
// Необходимые макросы
// #define MULTICHANNEL_END_POINT_DYNAMIC
// #define MULTICHANNEL_END_POINT_IDENTICAL
// #define MULTICHANNEL_END_POINT_COUNT
// #define MULTICHANNEL_CAPS_GENERIC_CLASS(EP_NUM)
// #define MULTICHANNEL_CAPS_SPECIFIC_CLASS(EP_NUM)
// #define MULTICHANNEL_CAPS_CC_COUNT(EP_NUM)
// #define MULTICHANNEL_CAPS_CC(EP_NUM, CC_NUM)
	
#endif
#if WITH_CC_TIME_PARAMETERS
// Необходимые макросы
// #define TIMEPARAMETERS_GET
// #define TIMEPARAMETERS_SET(TS)
// #define TIMEPARAMETERS_INCOMING_REPORT(TS)

void sendTimeParametersReport(BYTE dstNode, DWORD ts);
void sendTimeParametersGet(BYTE dstNode);

typedef struct _TIME_PARAMETERS_DATA
{
	WORD year;
	BYTE month;
	BYTE day;
	BYTE hours;
	BYTE minutes;
	BYTE seconds;
} TIME_PARAMETERS_DATA;

#endif

#if WITH_CC_SWITCH_BINARY

// Необходимые макросы
// #define SWITCH_BINARY_ACTION(VALUE)
// #define SWITCH_BINARY_VALUE

void sendSwitchBinaryReport(BYTE dstNode);
#if WITH_CC_ASSOCIATION
void sendSwitchBinaryReportToGroup(BYTE dstGroup);
#endif		


#endif	

// --- Config
#if WITH_CC_CONFIGURATION

// Необходимы макросы
// #define CONFIG_PARAMETER_GET
// #define CONFIG_PARAMETER_SET

// Оборачивающие макросы

#define MAKE_CONFIG_PARAMETER_GETTER_1B(NUM, EEVAR)\
								case NUM:\
								  report_value_b = VAR(EEVAR);\
									break;
									
#define MAKE_CONFIG_PARAMETER_GETTER_2B(NUM, EEVAR)\
								case NUM:\
								  report_value_w = WORD_VAR(EEVAR);\
									config_param_size = 2;\
									break;
									
#define MAKE_CONFIG_PARAMETER_GETTER_4B(NUM, EEVAR)\
								case NUM:\
								  report_value_dw = DWORD_VAR(EEVAR);\
									config_param_size = 4;\
									break;
									
#define MAKE_CONFIG_PARAMETER_SETTER_1B(NUM, EEVAR, VALID, ACTION)\
								case NUM:\
									if (config_param_to_default)\
										VAR(EEVAR) = DEFAULT_##EEVAR;\
									else \
										if (VALID(report_value_b)) {\
											ACTION(VAR(EEVAR), report_value_b); \
											VAR(EEVAR) = report_value_b;\
										}\
									SAVE_EEPROM_BYTE(EEVAR);\
								break;

#define MAKE_CONFIG_PARAMETER_SETTER_2B(NUM, EEVAR, VALID, ACTION)\
								case NUM:\
									if (config_param_to_default)\
										WORD_VAR(EEVAR) = DEFAULT_##EEVAR;\
									else \
										if (VALID(report_value_w)){\
											ACTION(WORD_VAR(EEVAR), report_value_w); \
											WORD_VAR(EEVAR) = report_value_w;\
										}\
									SAVE_EEPROM_WORD(EEVAR);\
								break;
								
#define MAKE_CONFIG_PARAMETER_SETTER_4B(NUM, EEVAR, VALID, ACTION)\
								case NUM:\
									if (config_param_to_default)\
										DWORD_VAR(EEVAR) = DEFAULT_##EEVAR;\
									else \
										if (VALID(report_value_dw)) {\
											ACTION(DWORD_VAR(EEVAR), report_value_dw); \
											DWORD_VAR(EEVAR) = report_value_dw;\
										}\
									SAVE_EEPROM_DWORD(EEVAR);\
								break;
#endif


#if WITH_CC_SENSOR_MULTILEVEL
#define SENSOR_MULTILEVEL_REPORT_BIT_AIR_TEMPERATURE		0
#define SENSOR_MULTILEVEL_REPORT_BIT_GENERAL_PURPOSE		1
#define SENSOR_MULTILEVEL_REPORT_BIT_LUMINANCE				2
#define SENSOR_MULTILEVEL_REPORT_BIT_POWER					3
#define SENSOR_MULTILEVEL_REPORT_BIT_HUMIDITY				4
#define SENSOR_MULTILEVEL_REPORT_BIT_VELOCITY				5
#define SENSOR_MULTILEVEL_REPORT_BIT_DIRECTION				6
#define SENSOR_MULTILEVEL_REPORT_BIT_ATHMOSPHERIC_PRESSURE	7
#define SENSOR_MULTILEVEL_REPORT_BIT_BAROMETRIC_PRESSURE	8
#define SENSOR_MULTILEVEL_REPORT_BIT_SOLAR_RADIATION		9
#define SENSOR_MULTILEVEL_REPORT_BIT_DEW_POINT				10
#define SENSOR_MULTILEVEL_REPORT_BIT_RAIN_RATE				11
#define SENSOR_MULTILEVEL_REPORT_BIT_TIDE_LEVEL				12
#define SENSOR_MULTILEVEL_REPORT_BIT_WEIGHT					13
#define SENSOR_MULTILEVEL_REPORT_BIT_VOLTAGE				14
#define SENSOR_MULTILEVEL_REPORT_BIT_CURRENT				15
#define SENSOR_MULTILEVEL_REPORT_BIT_CO2_LEVEL				16
#define SENSOR_MULTILEVEL_REPORT_BIT_AIR_FLOW				17
#define SENSOR_MULTILEVEL_REPORT_BIT_TANK_CAPACITY			18
#define SENSOR_MULTILEVEL_REPORT_BIT_DISTANCE				19
#define SENSOR_MULTILEVEL_REPORT_BIT_ANGLE_POSITION			20
#define SENSOR_MULTILEVEL_REPORT_BIT_ROTATION				21
#define SENSOR_MULTILEVEL_REPORT_BIT_WATER_TEMPERATURE		22
#define SENSOR_MULTILEVEL_REPORT_BIT_SOIL_TEMPERATURE		23
#define SENSOR_MULTILEVEL_REPORT_BIT_SEISMIC_INTENSITY		24
#define SENSOR_MULTILEVEL_REPORT_BIT_SEISMIC_MAGNITUDE		25
#define SENSOR_MULTILEVEL_REPORT_BIT_ULTRAVIOLET			26
#define SENSOR_MULTILEVEL_REPORT_BIT_ELECTRICAL_RESISTIVITY	27
#define SENSOR_MULTILEVEL_REPORT_BIT_ELECTRICAL_CONDUCTIVITY	28
#define SENSOR_MULTILEVEL_REPORT_BIT_LOUDNESS				29
#define SENSOR_MULTILEVEL_REPORT_BIT_MOISTURE				30
#define SENSOR_MULTILEVEL_REPORT_BIT_FREQUENCY				31
#define SENSOR_MULTILEVEL_REPORT_BIT_TIME					32
#define SENSOR_MULTILEVEL_REPORT_BIT_TARGET_TEMPERATURE		33
#endif

#if WITH_CC_BATTERY && defined(ZW050x)
#define DEFAULT_BATTERY_REPORT_ON_POR_SENT	0
#endif

#if WITH_CC_NODE_NAMING
// just a string easy to locate in the .hex file to substitute with a serial of the device
static BYTE NODE_NAMING_DEFAULT_NAME[17] = {0 << NODE_NAMING_NODE_NAME_REPORT_LEVEL_RESERVED_SHIFT, 0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x01, 0x03, 0x05, 0x07, 0x09, 0x0b, 0x0d, 0x0f};
void NodeNamingClear(void);
#endif

#if WITH_CC_WAKE_UP

#define WAKE_UP_MAGIC_VALUE	0x43

#define MINIMUM_SLEEP_SECONDS_MSB                       0x00 // 4 minutes = 240 seconds = 0xf0
#define MINIMUM_SLEEP_SECONDS_M                         0x00
#define MINIMUM_SLEEP_SECONDS_LSB                       0xf0
#define MAXIMUM_SLEEP_SECONDS_MSB                       0xef // 182 days = 15728400 seconds = 0xefff10
#define MAXIMUM_SLEEP_SECONDS_M                         0xff // This limits SleepSeconds/StepSleepSeconds = 0xffff (full DWORD)
#define MAXIMUM_SLEEP_SECONDS_LSB                       0x10
#define DEFAULT_SLEEP_SECONDS_MSB                       (DEFAULT_WAKE_UP_INTERVAL / 256 / 256) // 1 week = 604800 seconds = 0x093a80
#define DEFAULT_SLEEP_SECONDS_M                         ((DEFAULT_WAKE_UP_INTERVAL - 256 * 256 * (DEFAULT_WAKE_UP_INTERVAL / 256 / 256)) / 256)
#define DEFAULT_SLEEP_SECONDS_LSB                       (DEFAULT_WAKE_UP_INTERVAL - 256 * 256 * (DEFAULT_WAKE_UP_INTERVAL / 256 / 256) - 256 * (DEFAULT_WAKE_UP_INTERVAL - 256 * 256 * (DEFAULT_WAKE_UP_INTERVAL / 256 / 256)) / 256)
#define STEP_SLEEP_SECONDS_MSB                          0x00 // 4 minutes = 240 seconds = 0xf0
#define STEP_SLEEP_SECONDS_M                            0x00
#define STEP_SLEEP_SECONDS_LSB                          0xf0
#define DEFAULT_WAKEUP_NODE				ZW_TEST_NOT_A_NODEID

static void StoreWakeupConfiguration(void);
static void SetWakeupPeriod(void);
static void SetDefaultWakeupConfiguration(void);

#if WITH_CC_BATTERY
#define DEFAULT_UNSOLICITED_BATTERY_REPORT		UNSOLICITED_BATTERY_REPORT_WAKEUP_NODE

enum {   
        UNSOLICITED_BATTERY_REPORT_NO = 0,
        UNSOLICITED_BATTERY_REPORT_WAKEUP_NODE,
        UNSOLICITED_BATTERY_REPORT_BROADCAST,  
};

extern BYTE const code _DEFAULT_ENABLE_WAKEUPS;
#define DEFAULT_ENABLE_WAKEUPS						ENABLE_WAKEUPS_YES //_DEFAULT_ENABLE_WAKEUPS // defined in ZME_App_Version

#ifndef BATTERY_LEVEL_GET
BYTE BatteryLevel(void);
#endif
static void SendUnsolicitedBatteryReport(void);
#endif
static void SendWakeupNotificationCallback(BYTE txStatus);
static void SendWakeupNotification(void);
static void _ReleaseSleepLockTimer(void);

#if !defined(SLEEP_TIME)
#define SLEEP_TIME      0xf0 // sleep maximum factor of minutes times (4 minutes) to preserve battery
#endif

// simple check
// original code: #if STEP_SLEEP_SECONDS_LSB != SLEEP_TIME || STEP_SLEEP_SECONDS_M != 0 || STEP_SLEEP_SECONDS_MSB != 0
#if STEP_SLEEP_SECONDS_LSB != SLEEP_TIME * (STEP_SLEEP_SECONDS_LSB/SLEEP_TIME) || STEP_SLEEP_SECONDS_M != 0 || STEP_SLEEP_SECONDS_MSB != 0
        #error Wrong STEP_SLEEP_SECONDS_* configuration
#endif
#if STEP_SLEEP_SECONDS_LSB != MINIMUM_SLEEP_SECONDS_LSB || STEP_SLEEP_SECONDS_M != MINIMUM_SLEEP_SECONDS_M || STEP_SLEEP_SECONDS_MSB != MINIMUM_SLEEP_SECONDS_MSB
        #error Wrong STEP_SLEEP_SECONDS_* and MINIMUM_SLEEP_SECONDS_* configuration
#endif
#endif

#if !PRODUCT_LISTENING_TYPE  || PRODUCT_DYNAMIC_TYPE
BYTE wakeupReason; // reason of WakeUp - set up in ApplicationInitHW
#define NOT_WAKEUP_REASON       0xff
#endif

#if WITH_CC_SECURITY
#define DEFAULT_SECURITY_MODE					SECURITY_MODE_NOT_YET
#endif

#if WITH_CC_VERSION_2
// Необходимые макросы
// #define VERSION2_HARDWARE_VERSION		//	Версия "железа" 
// #define VERSION2_FIRMWARE_COUNT			//	Количество дополнительных прошивок 
// #define VERSION2_FIRMWARE_VERSION(N)		//  Версия N-ой прошивки (Целая часть)
// #define VERSION2_FIRMWARE_SUBVERSION(N)	//  Версия N-ой прошивки (Дробная часть)
#endif

#if WITH_CC_ZWPLUS_INFO
// Необходимые макросы
// #define ZWPLUS_INFO_ROLE_TYPE
// #define ZWPLUS_INFO_NODE_TYPE
// #define ZWPLUS_INFO_INSTALLER_ICON
// #define ZWPLUS_INFO_CUSTOM_ICON
#endif


#if WITH_CC_MULTICHANNEL
#define MULTI_CHANNEL_END_POINT_FIND_V3_REPORT_MAX_SIZE  30
// #warning "Change /4 to /30"
#endif

#if WITH_CC_ASSOCIATION_GROUP_INFORMATION

#define MAX_GROUP_NAME (40-4)
// Необходимые макросы
// #define AGI_NAME(G, NAME) 	// Возвращает имя группы (Записывает указатель на имя группы в NAME)
// #define AGI_PROFILE(G) 		// Возвращает профиль группы
// #define AGI_CCS_COUNT(G) 	// Возвращает число возможных комманд для группы 
// #define AGI_CCS(G, I) 		// Возвращает соответствующую команду для группы


#endif

#if WITH_CC_INDICATOR

// Необходимые макросы 
// #define INDICATOR_VALUE
// #define INDICATOR_ACTION(V)

#endif

#if WITH_CC_CENTRAL_SCENE
// Необходимые макросы 
// #define CENTRAL_SCENE_MAX_SCENES

#define CENTRAL_SCENE_KEY_ATTRIBUTE_PRESSED 	0
#define CENTRAL_SCENE_KEY_ATTRIBUTE_RELEASED	1
#define CENTRAL_SCENE_KEY_ATTRIBUTE_HELD_DOWN	2

#define CENTRAL_SCENE_REPEAT_EMPTY				0xFF

BYTE centralSceneHeldDown[CENTRAL_SCENE_MAX_SCENES];
void SendCentralScene(BYTE group, BYTE attribute, BYTE scene);
void CentralSceneInit(void);
void CentralSceneRepeat(void);
void CentralSceneClearRepeats(void);
void SendCentralSceneClick(BYTE group, BYTE scene); 
void SendCentralSceneHoldStart(BYTE group, BYTE scene); 
void SendCentralSceneHoldRelease(BYTE group, BYTE scene);

#endif

#if WITH_CC_THERMOSTAT_MODE
#message Remove these defines in future
#define THERMOSTAT_MODE_REPORT_MODE_FULL_POWER_V3							0x0F
#define THERMOSTAT_MODE_REPORT_MODE_MANUFACTURER_SPECIFIC_V3				0x1F
#define THERMOSTAT_MODE_REPORT_MODE_AWAY_COOL_V3							0x0E
#define THERMOSTAT_MODE_REPORT_MODE_FULL_POWER_V3							0x0F
#define THERMOSTAT_MODE_VERSION_V3											3
#define THERMOSTAT_SETPOINT_VERSION_V3										3
#define THERMOSTAT_SETPOINT_CAPABILITIES_GET								0x09
#define THERMOSTAT_SETPOINT_CAPABILITIES_REPORT								0x0A
#warning Check these THERMOSTAT_SETPOINT_CAPABILITIES_* with Chong!

#if WITH_CC_ASSOCIATION
#define SendThermostatModeReport() SendReport(ASSOCIATION_REPORTS_GROUP_NUM, COMMAND_CLASS_THERMOSTAT_MODE, VAR(THERMOSTAT_MODE))
#endif
#endif

#if WITH_CC_THERMOSTAT_SETPOINT
#define THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_HEATING_1_V2				0x01
#define THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_COOLING_1_V2				0x02
#define THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_FURNACE_V2					0x03
#define THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_DRY_AIR_V2					0x04
#define THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_MOIST_AIR_V2				0x05
#define THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_AUTO_CHANGEOVER_V2			0x06
#define THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_ENERGY_SAVE_HEATING_V2		0x07
#define THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_ENERGY_SAVE_COOLING_V2		0x08
#define THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_AWAY_HEATING_V2			0x09
#define THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_AWAY_COOLING_V3			0x0A
#define THERMOSTAT_SETPOINT_REPORT_BIT_SETPOINT_TYPE_FULL_POWER_V3				0x0B

#define THERMOSTAT_SETPOINT_PRECISION			1
#define THERMOSTAT_SETPOINT_TEMPERATURE_SIZE	2
#define THERMOSTAT_SETPOINT_SCALE_CELSIUS		0x00
#define THERMOSTAT_SETPOINT_SCALE_FARENHEIT		0x01

#if WITH_CC_ASSOCIATION
void SendThermostatSetPointReport(BYTE mode, WORD temp);
#endif
#endif

#if WITH_PROPRIETARY_EXTENTIONS
static IBYTE freqChangeCode;
#if !defined(ZW050x)
sfr FLCON = 0x94;
sfr FLADR = 0x95;
#define FLCON_FLPRGS			0x02
#define FLCON_FLPRGI			0x01
#else
// Сегмент для смены частоты (0x2800-0x29FF)
#define REFLASH_PAGE_ADDR		0xA00  // Адрес страницы, используемой для перезаписи Flash
#define REFLASH_SEG_ADDR    	0x2000 // Смотри файл проекта = адрес Сегмент_4k(?CO?ZW_RF_FTBL)
#define REFLASH_CODE_SHIFT  	0xE0   // Смотри файл проекта = адрес ?CO?ZW_RF_FTBL

XBYTE reflashPage[256] _at_ REFLASH_PAGE_ADDR; // just make sure to be in lower 4k

#endif

#define FUNC_ID_ZME_SWITCH_FREQ	0xf2

#define FREQ_EU         0x00
#define FREQ_RU         0x01
#define FREQ_IN         0x02
#define FREQ_US         0x03
#define FREQ_ANZ        0x04
#define FREQ_HK         0x05
#define FREQ_CN         0x06
#define FREQ_JP         0x07
#define FREQ_KR         0x08
#define FREQ_IL         0x09
#define FREQ_MY         0x0A
#define MAX_FREQ_NUM    FREQ_MY


#define FUNC_ID_ZME_REBOOT		0xfa

#define FUNC_ID_ZME_EEPROM_WRITE	0xfb

void FreqChange(BYTE freq);
#endif

// universal for all CCs
enum {
	SWITCHED_OFF = 0,
	SWITCHED_ON = 255
};

#if WITH_CC_SWITCH_MULTILEVEL
#define SWITCH_MULTILEVEL_LEVEL_MIN	0
#define SWITCH_MULTILEVEL_LEVEL_MAX	99
#endif


#if WITH_CC_SWITCH_ALL
#define DEFAULT_SWITCH_ALL_MODE		SWITCH_ALL_REPORT_EXCLUDED_FROM_THE_ALL_ON_FUNCTIONALITY_BUT_NOT_ALL_OFF
void SwitchAllSend(BYTE mode);
#endif
///////////////////////////////////////////////////////////////
// Controlled CCs definitions

#if CONTROL_CC_SWITCH_ALL
#define DEFAULT_CONTROL_SWITCH_ALL_MODE		SWITCH_ALL_REPORT_EXCLUDED_FROM_THE_ALL_ON_FUNCTIONALITY_BUT_NOT_ALL_OFF
void SwitchAllSend(BYTE mode);
#endif

#if CONTROL_CC_PROXIMITY_MULTILEVEL
void ProximitySendBasic(BYTE level);
void ProximitySendStartChange(BOOL up_down);
void ProximitySendStopChange(void);
#endif

///////////////////////////////////////////////////////////////

// Normal NIF for simple device and NIF if Secure inclusion has failed
// Can not include Security, DoorLock and DoorLockLogging
BYTE nodeInfoCCsNormal[] = {
	#if WITH_CC_ZWPLUS_INFO
	COMMAND_CLASS_ZWAVEPLUS_INFO_V2,
	#endif
	#if WITH_CC_BASIC
	COMMAND_CLASS_BASIC,
	#endif	
	#if WITH_CC_SWITCH_BINARY
	COMMAND_CLASS_SWITCH_BINARY,
	#endif	
	#if WITH_CC_SWITCH_MULTILEVEL
	COMMAND_CLASS_SWITCH_MULTILEVEL,
	#endif	
	#if WITH_CC_SENSOR_BINARY || WITH_CC_SENSOR_BINARY_V2
	COMMAND_CLASS_SENSOR_BINARY,
	#endif	
	#if WITH_CC_NOTIFICATION
	COMMAND_CLASS_NOTIFICATION_V4,
	#endif	
	#if WITH_CC_SENSOR_MULTILEVEL || WITH_CC_SENSOR_MULTILEVEL_V4
	COMMAND_CLASS_SENSOR_MULTILEVEL,
	#endif
	#if WITH_CC_PROTECTION
	COMMAND_CLASS_PROTECTION,
	#endif
	#if WITH_CC_THERMOSTAT_MODE
	COMMAND_CLASS_THERMOSTAT_MODE,
	#endif
	#if WITH_CC_THERMOSTAT_SETPOINT
	COMMAND_CLASS_THERMOSTAT_SETPOINT,
	#endif
	#if WITH_CC_CONFIGURATION
	COMMAND_CLASS_CONFIGURATION,
	#endif
	#if WITH_CC_ASSOCIATION
	COMMAND_CLASS_ASSOCIATION,
	#endif
	#if WITH_CC_SCENE_CONTROLLER_CONF
	COMMAND_CLASS_SCENE_CONTROLLER_CONF,
	#endif
	#if WITH_CC_SCENE_ACTUATOR_CONF
	COMMAND_CLASS_SCENE_ACTUATOR_CONF,
	#endif
	#if WITH_CC_SCENE_ACTIVATION
	COMMAND_CLASS_SCENE_ACTIVATION,
	#endif
	#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
	COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
	#endif
	#if WITH_CC_NODE_NAMING
	COMMAND_CLASS_NODE_NAMING,
	#endif
	#if WITH_CC_INDICATOR
	COMMAND_CLASS_INDICATOR,
	#endif
	#if WITH_CC_BATTERY
	COMMAND_CLASS_BATTERY,
	#endif
	#if WITH_CC_WAKE_UP
	COMMAND_CLASS_WAKE_UP,
	#endif
	#if WITH_PROPRIETARY_EXTENTIONS_IN_NIF
	COMMAND_CLASS_MANUFACTURER_PROPRIETARY,
	#endif
	#if WITH_CC_METER
	COMMAND_CLASS_METER,
	#endif
	#if WITH_CC_METER_TABLE_MONITOR
	COMMAND_CLASS_METER_TBL_MONITOR,
	#endif
	#if WITH_CC_MULTICHANNEL
	COMMAND_CLASS_MULTI_CHANNEL_V3,
	#endif
	#if WITH_CC_MULTI_COMMAND
	COMMAND_CLASS_MULTI_CMD,
	#endif
	#if WITH_CC_FIRMWARE_UPGRADE
  	COMMAND_CLASS_FIRMWARE_UPDATE_MD,															
  	#endif															
	#if WITH_CC_TIME_PARAMETERS
	COMMAND_CLASS_TIME_PARAMETERS,
	#endif	
	#if WITH_CC_DEVICE_RESET_LOCALLY
	COMMAND_CLASS_DEVICE_RESET_LOCALLY,
	#endif
	#if WITH_CC_ASSOCIATION_GROUP_INFORMATION
	COMMAND_CLASS_ASSOCIATION_GRP_INFO,
	#endif
	#if WITH_CC_CENTRAL_SCENE
	COMMAND_CLASS_CENTRAL_SCENE,
	#endif
	#if WITH_CC_POWERLEVEL
	COMMAND_CLASS_POWERLEVEL,
	#endif
	#if WITH_CC_SWITCH_ALL
	COMMAND_CLASS_SWITCH_ALL,
	#endif
	
	COMMAND_CLASS_VERSION,
	COMMAND_CLASS_MANUFACTURER_SPECIFIC,
	COMMAND_CLASS_MARK,

	#if CONTROL_CC_BASIC
	COMMAND_CLASS_BASIC,
	#endif
	#if WITH_CC_CENTRAL_SCENE
	COMMAND_CLASS_CENTRAL_SCENE,
	#endif
	#if CONTROL_CC_SWITCH_MULTILEVEL
	COMMAND_CLASS_SWITCH_MULTILEVEL,
	#endif
	#if CONTROL_CC_SWITCH_ALL
	COMMAND_CLASS_SWITCH_ALL,
	#endif
	#if CONTROL_CC_SCENE_ACTIVATION
	COMMAND_CLASS_SCENE_ACTIVATION,
	#endif
	#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
	COMMAND_CLASS_MULTI_CHANNEL_V3, // we can control MC if MCA is present
	#endif
};
BYTE  nodeInfoCCsNormal_size = sizeof(nodeInfoCCsNormal);


#if WITH_CC_SECURITY
// NIF before Secure inclusion
// Can not include Security, DoorLock and DoorLockLogging
BYTE nodeInfoCCsOutsideSecurity[] = {
	#if WITH_CC_ZWPLUS_INFO
	COMMAND_CLASS_ZWAVEPLUS_INFO_V2,
	#endif
    #if WITH_PROPRIETARY_EXTENTIONS_IN_NIF
    COMMAND_CLASS_MANUFACTURER_PROPRIETARY,
    #endif
    #if WITH_CC_MULTI_COMMAND
    COMMAND_CLASS_MULTI_CMD,
    #endif	
    #if WITH_CC_FIRMWARE_UPGRADE
 	COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2,															
  	#endif
  	#if WITH_CC_POWERLEVEL
	COMMAND_CLASS_POWERLEVEL,
	#endif															

  	COMMAND_CLASS_SECURITY,
	COMMAND_CLASS_VERSION,
	COMMAND_CLASS_MANUFACTURER_SPECIFIC,

	#if ! SECURE_ALL_CC
		#if WITH_CC_BASIC
		COMMAND_CLASS_BASIC,
		#endif
		#if WITH_CC_SWITCH_BINARY
		COMMAND_CLASS_SWITCH_BINARY,
		#endif	
		#if WITH_CC_SWITCH_MULTILEVEL
		COMMAND_CLASS_SWITCH_MULTILEVEL,
		#endif	
		#if WITH_CC_SENSOR_BINARY || WITH_CC_SENSOR_BINARY_V2
		COMMAND_CLASS_SENSOR_BINARY,
		#endif	
		#if WITH_CC_NOTIFICATION
		COMMAND_CLASS_NOTIFICATION_V4,
		#endif	
		#if WITH_CC_SENSOR_MULTILEVEL || WITH_CC_SENSOR_MULTILEVEL_V4
		COMMAND_CLASS_SENSOR_MULTILEVEL,
		#endif
		#if WITH_CC_PROTECTION
		COMMAND_CLASS_PROTECTION,
		#endif
		#if WITH_CC_THERMOSTAT_MODE
		COMMAND_CLASS_THERMOSTAT_MODE,
		#endif
		#if WITH_CC_THERMOSTAT_SETPOINT
		COMMAND_CLASS_THERMOSTAT_SETPOINT,
		#endif
		#if WITH_CC_CONFIGURATION
		COMMAND_CLASS_CONFIGURATION,
		#endif
		#if WITH_CC_ASSOCIATION
		COMMAND_CLASS_ASSOCIATION,
		#endif
		#if WITH_CC_SCENE_CONTROLLER_CONF
		COMMAND_CLASS_SCENE_CONTROLLER_CONF,
		#endif
		#if WITH_CC_SCENE_ACTUATOR_CONF
		COMMAND_CLASS_SCENE_ACTUATOR_CONF,
		#endif
		#if WITH_CC_SCENE_ACTIVATION
		COMMAND_CLASS_SCENE_ACTIVATION,
		#endif
		#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
		#endif
		#if WITH_CC_NODE_NAMING
		COMMAND_CLASS_NODE_NAMING,
		#endif
		#if WITH_CC_INDICATOR
		COMMAND_CLASS_INDICATOR,
		#endif
		#if WITH_CC_BATTERY
		COMMAND_CLASS_BATTERY,
		#endif
		#if WITH_CC_WAKE_UP
		COMMAND_CLASS_WAKE_UP,
		#endif
		#if WITH_CC_METER
		COMMAND_CLASS_METER,
		#endif
		#if WITH_CC_METER_TABLE_MONITOR
		COMMAND_CLASS_METER_TBL_MONITOR,
		#endif
		#if WITH_CC_MULTICHANNEL
		COMMAND_CLASS_MULTI_CHANNEL_V3,
		#endif
		#if WITH_CC_TIME_PARAMETERS
		COMMAND_CLASS_TIME_PARAMETERS,
		#endif											
		#if WITH_CC_DEVICE_RESET_LOCALLY
		COMMAND_CLASS_DEVICE_RESET_LOCALLY,
		#endif
		#if WITH_CC_ASSOCIATION_GROUP_INFORMATION
		COMMAND_CLASS_ASSOCIATION_GRP_INFO,
		#endif
		#if WITH_CC_CENTRAL_SCENE
		COMMAND_CLASS_CENTRAL_SCENE,
		#endif
		#if WITH_CC_SWITCH_ALL
		COMMAND_CLASS_SWITCH_ALL,
		#endif
	#endif
	
	COMMAND_CLASS_MARK,

	#if CONTROL_CC_BASIC
	COMMAND_CLASS_BASIC,
	#endif
	#if WITH_CC_CENTRAL_SCENE
	COMMAND_CLASS_CENTRAL_SCENE,
	#endif
	#if CONTROL_CC_SWITCH_MULTILEVEL
	COMMAND_CLASS_SWITCH_MULTILEVEL,
	#endif
	#if CONTROL_CC_SWITCH_ALL
	COMMAND_CLASS_SWITCH_ALL,
	#endif
	#if CONTROL_CC_SCENE_ACTIVATION
	COMMAND_CLASS_SCENE_ACTIVATION,
	#endif
	#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
	COMMAND_CLASS_MULTI_CHANNEL_V3, // we can control MC if MCA is present
	#endif
};
BYTE nodeInfoCCsOutsideSecurity_size = sizeof(nodeInfoCCsOutsideSecurity); 
// SecureNIF - supported CCs under Security
// Can not include Security
BYTE nodeInfoCCsInsideSecurity[] = {
	
	#if SECURE_ALL_CC
		#if WITH_CC_ZWPLUS_INFO
		COMMAND_CLASS_ZWAVEPLUS_INFO_V2,
		#endif
		#if WITH_CC_BASIC
		COMMAND_CLASS_BASIC,
		#endif	
		#if WITH_CC_SWITCH_BINARY
		COMMAND_CLASS_SWITCH_BINARY,
		#endif	
		#if WITH_CC_SWITCH_MULTILEVEL
		COMMAND_CLASS_SWITCH_MULTILEVEL,
		#endif	
		#if WITH_CC_SENSOR_BINARY || WITH_CC_SENSOR_BINARY_V2
		COMMAND_CLASS_SENSOR_BINARY,
		#endif	
		#if WITH_CC_NOTIFICATION
		COMMAND_CLASS_NOTIFICATION_V4,
		#endif	
		#if WITH_CC_SENSOR_MULTILEVEL || WITH_CC_SENSOR_MULTILEVEL_V4
		COMMAND_CLASS_SENSOR_MULTILEVEL,
		#endif
		#if WITH_CC_PROTECTION
		COMMAND_CLASS_PROTECTION,
		#endif
		#if WITH_CC_THERMOSTAT_MODE
		COMMAND_CLASS_THERMOSTAT_MODE,
		#endif
		#if WITH_CC_THERMOSTAT_SETPOINT
		COMMAND_CLASS_THERMOSTAT_SETPOINT,
		#endif
		#if WITH_CC_CONFIGURATION
		COMMAND_CLASS_CONFIGURATION,
		#endif
		#if WITH_CC_ASSOCIATION
		COMMAND_CLASS_ASSOCIATION,
		#endif
		#if WITH_CC_SCENE_CONTROLLER_CONF
		COMMAND_CLASS_SCENE_CONTROLLER_CONF,
		#endif
		#if WITH_CC_SCENE_ACTUATOR_CONF
		COMMAND_CLASS_SCENE_ACTUATOR_CONF,
		#endif
		#if WITH_CC_SCENE_ACTIVATION
		COMMAND_CLASS_SCENE_ACTIVATION,
		#endif
		#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
		COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
		#endif
		#if WITH_CC_NODE_NAMING
		COMMAND_CLASS_NODE_NAMING,
		#endif
		#if WITH_CC_INDICATOR
		COMMAND_CLASS_INDICATOR,
		#endif
		#if WITH_CC_BATTERY
		COMMAND_CLASS_BATTERY,
		#endif
		#if WITH_CC_WAKE_UP
		COMMAND_CLASS_WAKE_UP,
		#endif
		#if WITH_PROPRIETARY_EXTENTIONS_IN_NIF
		COMMAND_CLASS_MANUFACTURER_PROPRIETARY,
		#endif
		#if WITH_CC_METER
		COMMAND_CLASS_METER,
		#endif
		#if WITH_CC_METER_TABLE_MONITOR
		COMMAND_CLASS_METER_TBL_MONITOR,
		#endif
		#if WITH_CC_TIME_PARAMETERS
		COMMAND_CLASS_TIME_PARAMETERS,
		#endif											
		#if WITH_CC_DEVICE_RESET_LOCALLY
		COMMAND_CLASS_DEVICE_RESET_LOCALLY,
		#endif
		#if WITH_CC_ASSOCIATION_GROUP_INFORMATION
		COMMAND_CLASS_ASSOCIATION_GRP_INFO,
		#endif
		#if WITH_CC_CENTRAL_SCENE
		COMMAND_CLASS_CENTRAL_SCENE,
		#endif
		#if WITH_CC_SWITCH_ALL
		COMMAND_CLASS_SWITCH_ALL,
		#endif
		#if WITH_CC_DOOR_LOCK
		COMMAND_CLASS_DOOR_LOCK_V2,
		#endif
		#if WITH_CC_DOOR_LOCK_LOGGING
		COMMAND_CLASS_DOOR_LOCK_LOGGING,
		#endif
		COMMAND_CLASS_VERSION,
		COMMAND_CLASS_MANUFACTURER_SPECIFIC,
	#endif
	
	#if WITH_CC_MULTICHANNEL
	COMMAND_CLASS_MULTI_CHANNEL_V3,
	#endif
	#if WITH_CC_MULTI_COMMAND
	COMMAND_CLASS_MULTI_CMD,
	#endif																	

	COMMAND_CLASS_MARK,

	#if CONTROL_CC_BASIC
	COMMAND_CLASS_BASIC,
	#endif
	#if WITH_CC_CENTRAL_SCENE
	COMMAND_CLASS_CENTRAL_SCENE,
	#endif
	#if CONTROL_CC_SWITCH_MULTILEVEL
	COMMAND_CLASS_SWITCH_MULTILEVEL,
	#endif
	#if CONTROL_CC_SWITCH_ALL
	COMMAND_CLASS_SWITCH_ALL,
	#endif
	#if CONTROL_CC_SCENE_ACTIVATION
	COMMAND_CLASS_SCENE_ACTIVATION,
	#endif
	#if CONTROL_CC_DOOR_LOCK
	COMMAND_CLASS_DOOR_LOCK,
	#endif
	#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2
	COMMAND_CLASS_MULTI_CHANNEL_V3, // we can control MC if MCA is present
	#endif
};
BYTE nodeInfoCCsInsideSecurity_size = sizeof(nodeInfoCCsInsideSecurity); 

#endif	

enum {
	CC_IS_UNSECURE,
	CC_IS_SECURE,
	CC_IS_UNSECURE_AND_SECURE,
};
#if WITH_CC_SECURITY
BYTE *nodeInfoCCs = nodeInfoCCsOutsideSecurity;
//BYTE nodeInfoCCs_size = nodeInfoCCsOutsideSecurity_size;
#else
BYTE *nodeInfoCCs = nodeInfoCCsNormal;
//BYTE nodeInfoCCs_size = nodeInfoCCsNormal_size;
#endif

BYTE *  p_nodeInfoCCs_size;
#define nodeInfoCCs_size (*p_nodeInfoCCs_size)


///////////////////////////////////////////////////////////////
// Variables

#ifdef ZW_SLAVE
enum {
	MODE_IDLE = 0,
	MODE_LEARN,
	MODE_LEARN_IN_PROGRESS,
	CUSTOM_MODES
};
#else
// ZW_CONTROLLER
enum {
	MODE_IDLE = 0,
	MODE_LEARN,
	MODE_LEARN_IN_PROGRESS,
	MODE_CONTROLLER_INCLUSION,
	MODE_CONTROLLER_EXCLUSION,
	MODE_CONTROLLER_SHIFT,
	CUSTOM_MODES
};
#endif

BYTE mode = MODE_IDLE;

#if PRODUCT_LISTENING_TYPE && !PRODUCT_DYNAMIC_TYPE
BYTE myNodeId; // this declaration is duplicated in !PRODUCT_LISTENING_TYPE definitions to be placed in SRAM
#endif

#if FIRST_RUN_ACTION_PRESENT
BOOL firstRun = TRUE;
#endif

extern BYTE txOption; // to minimize parameters list for some functions

////////////////////////////////////////////////////////////
// Some checks

#if WITH_CC_BATTERY && PRODUCT_LISTENING_TYPE && !PRODUCT_DYNAMIC_TYPE
#error Battery CC is not allowed in listening devices
#endif

#if WITH_CC_WAKE_UP && PRODUCT_LISTENING_TYPE && !PRODUCT_DYNAMIC_TYPE
#error WakeUp CC is not allowed in listening devices
#endif

#if WITH_CC_SECURITY && !WITH_FSM
#error Security requires FSM
#endif

#if WITH_CC_POWERLEVEL && !WITH_FSM
#error PowerLevel requires FSM
#endif

#if WITH_CC_FIRMWARE_UPGRADE && !WITH_FSM
#error FirmwareUpgrade requires FSM
#endif

#if WITH_CC_ZWPLUS_INFO && !defined(ASSOCIATION_REPORTS_GROUP_NUM)
#error Z-Wave Plus requires the lifeline group. Please define ASSOCIATION_REPORTS_GROUP_NUM
#endif

#if WITH_CC_ZWPLUS_INFO && (! WITH_CC_ASSOCIATION_GROUP_INFORMATION || ! WITH_CC_DEVICE_RESET_LOCALLY)
#error Z-Wave Plus requires the AGI and DeviceResetLocally
#endif

#if WITH_CC_SCENE_CONTROLLER_CONF && !WITH_CC_ASSOCIATION
#error SceneControllerConf requires Association
#endif

#if WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2 && !WITH_CC_ASSOCIATION
#error MultiChannelAssociation requires Association
#endif

#if WITH_CC_ASSOCIATION_GROUP_INFORMATION && !WITH_CC_ASSOCIATION
#error AGI requires Association
#endif

#if (defined(SendCallbackCustomID) && !defined(SendCallbackCustom)) || (!defined(SendCallbackCustomID) && defined(SendCallbackCustom))
#error Both should be defined: the list of callback IDs SendCallbackCustomID and callback function SendCallbackCustom
#endif

#if WITH_CC_DOOR_LOCK && !WITH_CC_SECURITY
#error Doorlock CC can be only Secure
#endif

#if WITH_CC_DOOR_LOCK_LOGGING && !WITH_CC_DOOR_LOCK
#error Doorlock LOGGING can be only WITH DOOR_LOCK_CC and Security
#endif

#ifdef ZW_CONTROLLER
void controllerStartInclusion(BYTE bSecure);
void controllerStartExclusion();
void controllerStopInclusion();
void controllerStopExclusion();
void controllerStartShift();
void controllerStopShift();
BOOL controllerCanLearn(void);
void controllerUpdateState();

extern BOOL realPrimaryController;
extern BYTE g_ControllerCanLearn;
#endif
