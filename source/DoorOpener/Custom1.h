// Devolo Key Fob

// Device tech data
#define MANUFACTURER_ID_MAJOR               0x01
#define MANUFACTURER_ID_MINOR               0x15
#define PRODUCT_TYPE_ID_MAJOR               0x01
#define PRODUCT_TYPE_ID_MINOR               0x00
#define PRODUCT_ID_MAJOR                    0x01
#define PRODUCT_ID_MINOR                    0x01
#define PRODUCT_FIRMWARE_VERSION_MAJOR      0x01
#define PRODUCT_FIRMWARE_VERSION_MINOR      0x00
#define PRODUCT_GENERIC_TYPE                GENERIC_TYPE_WALL_CONTROLLER
#define PRODUCT_SPECIFIC_TYPE               SPECIFIC_TYPE_BASIC_WALL_CONTROLLER


//device alwayse on
#define PRODUCT_LISTENING_TYPE              1 //1 -alwayse on 0 - battery
//dynamic device type
#define PRODUCT_DYNAMIC_TYPE 								0

//frequently listening device
#define PRODUCT_FLIRS_TYPE                  0 // 0, FLIRS_250, FLIRS_1000

//routinf stuff
#define PRODUCT_NO_ROUTES                   1 // If we don't want to use routing in this device - only direct + Explorer Frames

// Z-Wave Plus Info
#define ZWPLUS_INFO_ROLE_TYPE               ZWAVEPLUS_INFO_REPORT_ROLE_TYPE_SLAVE_PORTABLE_V2
#define ZWPLUS_INFO_NODE_TYPE               ZWAVEPLUS_INFO_REPORT_NODE_TYPE_ZWAVEPLUS_NODE_V2
#define ZWPLUS_INFO_INSTALLER_ICON          ICON_TYPE_GENERIC_REMOTE_CONTROL_SIMPLE
#define ZWPLUS_INFO_CUSTOM_ICON             ICON_TYPE_SPECIFIC_REMOTE_CONTROL_SIMPLE_KEYFOB

// Check Project config
// Slave Enhanced: project should contain: ZW_SLAVE, ZW_SLAVE_32, ZW_SLAVE_ENHANCED_232, ZW_SLAVE_ROUTING, ZW050x, BANKING, EU
// Routing Slave: project should contain: ZW_SLAVE, ZW_SLAVE_ROUTING, ZW050x, BANKING, EU
// Controller: project should contain: ....... TODO ...... , ZW050x, BANKING, EU

// Packet queue
#define QUEUE_SIZE                              50
#define EXPLORER_FRAME_TIMOUT								100 // in 10 ms, 0 for no timeout

#define AFTER_INCLUSION_AWAKE_TIME							4 // time to wait after inclusion and NIF send, in 2.5 seconds

// Device features
#define WITH_NWI                                1 // Только для не спящих устройств!
#define WITH_CC_BASIC                           1
#define WITH_CC_SWITCH_ALL                      0
#define WITH_CC_SWITCH_BINARY                   0
#define WITH_CC_SWITCH_MULTILEVEL               0
#define WITH_CC_SENSOR_BINARY                   0
#define WITH_CC_SENSOR_BINARY_V2                0
#define WITH_CC_SENSOR_MULTILEVEL_V4            0
#define WITH_CC_SENSOR_MULTILEVEL               0
#define WITH_CC_METER                           0
#define WITH_CC_METER_TABLE_MONITOR             0
#define WITH_CC_MULTICHANNEL                    0
#define WITH_CC_MULTI_COMMAND                   0
#define WITH_CC_SECURITY                        0
#define WITH_CC_ASSOCIATION                     0
#define WITH_CC_MULTI_CHANNEL_ASSOCIATION_V2    0
#define WITH_CC_SCENE_CONTROLLER_CONF           0
#define WITH_CC_SCENE_ACTUATOR_CONF             0
#define WITH_CC_SCENE_ACTIVATION                0
#define WITH_CC_CONFIGURATION                   0
#define WITH_CC_NODE_NAMING                     0
#define WITH_CC_PROTECTION                      0
#define WITH_CC_TIME_PARAMETERS                 0   
#define WITH_CC_BATTERY                         0
#define WITH_CC_WAKE_UP                         0
#define WITH_CC_INDICATOR                       0 // not finished yet
#define WITH_CC_THERMOSTAT_MODE                 0
#define WITH_CC_THERMOSTAT_SETPOINT             0
#define WITH_CC_FIRMWARE_UPGRADE                0
#define WITH_PROPRIETARY_EXTENTIONS             0
#define WITH_PROPRIETARY_EXTENTIONS_IN_NIF      0
#define WITH_CC_VERSION_2                       0
#define WITH_CC_ZWPLUS_INFO                     0
#define WITH_CC_ASSOCIATION_GROUP_INFORMATION   0
#define WITH_CC_DEVICE_RESET_LOCALLY            0   
#define WITH_CC_CENTRAL_SCENE                   0
#define WITH_CC_POWERLEVEL                      0
#define WITH_CC_DOOR_LOCK												0
#define WITH_CC_DOOR_LOCK_LOGGING 							0
#define WITH_CC_NOTIFICATION 										0


#define CONTROL_CC_BASIC                        0
#define CONTROL_CC_SWITCH_ALL                   0
#define CONTROL_CC_SWITCH_MULTILEVEL            0
#define CONTROL_CC_PROXIMITY_MULTILEVEL         0
#define CONTROL_CC_SCENE_ACTIVATION             0
#define CONTROL_CC_DOOR_LOCK                    0

#define WITH_FSM                                0 // Autoincluded by Security module

// i2c шина 
#define WITH_I2C_INTERFACE                      0
//#define I2C_DEBUG_INTERFACE

// FSM are also used by Security CC, so we need some for the interview to complete
// each FSM leaves for 100 ms after transmission complete, but each transmission is about 200ms, so we just need few FSMs for each packet send and receive
#define MAX_FSM                                 20
#define FSM_WAIT_BEFORE_SLEEP                   1


#define SECURE_ALL_CC                           0
// Отладка некоторых модулей
// Security
#define SECURITY_DEBUG                          0
#define SECURITY_DEBUG_MULTICOMMAND             0

#define SECURITY_DEBUG_DUMP_STATE               1
#define SECURITY_DEBUG_CC                       COMMAND_CLASS_SENSOR_BINARY
#define SECURITY_DEBUG_CC_ECHO                  COMMAND_CLASS_BASIC
#define SECURITY_DEBUG_CC_ECHO_SIZE             3


#define WITH_PENDING_BATTERY                    0

// Erase EEPROM content to 0x00 or 0xff
#define ERASE_NVM                               0
#define ERASE_NVM_ZERO                          0
#define NVM_CHECK_ONLY                          0
#define NVM_CHECK_SIZE                          0x01
#define NVM_CHECK_LEDpinPort                    RedLEDpinPort         
#define NVM_CHECK_LEDpinSHADOW                  RedLEDpinSHADOW
#define NVM_CHECK_LEDpinDIR                     RedLEDpinDIR         
#define NVM_CHECK_LEDpinSHADOWDIR               RedLEDpinSHADOWDIR
#define NVM_CHECK_LEDpinDIR_PAGE                RedLEDpinDIR_PAGE
#define NVM_CHECK_LEDpin                        RedLEDpin

#define WITH_TRIAC                              0

/////////////////////////////////////////////////////////////////////

#define FIRST_RUN_ACTION_PRESENT                0
#define FIRST_RUN_BATTERY_INSERT_ACTION_PRESENT 0
#define BETWEEN_WUT_WAKEUP_ACTION_PRESENT       0
//#define CUSTOM_SLEEP_LOCK                       0
//#define CustomSleepLock()         
// !!              
#define CUSTOM_SLEEP_LOCK                       1
#define CustomSleepLock()                       0
// Allocate persistent variables to be stored in EEPROM
// No comma is needed at the end of the line
#define CUSTOM_EE_VARS \
        EE_VAR(CURRENT_BUTTONS_PARAM_13, 1) \
        EE_VAR(CURRENT_BUTTONS_PARAM_24, 1) \
        EE_VAR(CLICK_ACTION_1, 1) \
        EE_VAR(CLICK_ACTION_2, 1) \
        EE_VAR(CLICK_ACTION_3, 1) \
        EE_VAR(CLICK_ACTION_4, 1) \
        EE_VAR(SEND_SWITCH_ALL, 1) \
        EE_VAR(INVERT_SWITCH, 1) \

// Define defaults
#define DEFAULT_CURRENT_BUTTONS_PARAM_13    _DEFAULT_GROUP_BUTTONS_13
#define DEFAULT_CURRENT_BUTTONS_PARAM_24    _DEFAULT_GROUP_BUTTONS_24
#define DEFAULT_CLICK_ACTION_1                      _DEFAULT_CLICK_ACTION_1
#define DEFAULT_CLICK_ACTION_2                      _DEFAULT_CLICK_ACTION_2
#define DEFAULT_CLICK_ACTION_3                      _DEFAULT_CLICK_ACTION_3
#define DEFAULT_CLICK_ACTION_4                      _DEFAULT_CLICK_ACTION_4
#define DEFAULT_SEND_SWITCH_ALL                     SWITCH_ALL_REPORT_EXCLUDED_FROM_THE_ALL_ON_FUNCTIONALITY_BUT_NOT_ALL_OFF
#define DEFAULT_INVERT_SWITCH                           INVERT_SWITCH_NO

// And add ResotreToDefault code
// keep same order as in CUSTOM_EE_VARS
#define CUSTOM_RESET_EEPROM_TO_DEFAULT()    { \
}

//#define CUSTOM_NON_ZERO_VARIABLES 


//2-button group states
enum {
	SEPARATE_BUTTONS = 0,
	UNITED_BUTTONS_NO_D_CLICK,
	UNITED_BUTTONS_WITH_D_CLICK
};

enum {
	ACTION_DISABLED = 0,
	SEND_BASIC_SET_ML,  	// 1
	SEND_BASIC_SET,     	// 2  
	SEND_SWITCH_ALL,    	// 3 
	SEND_SCENE,         	// 4 
	SEND_SCENE_ACTIVATION, 	// 5
	SEND_PROXIMITY,        	// 6  
	SEND_DOOR_LOCK,         // 7
	SEND_CENTRAL_SCENE		// 8 
};

// These constants are placed in fixed position in the final hex file
#define CUSTOM_CONST_CODE_PARAMETERS \
						CONST_CODE(_DEFAULT_CLICK_ACTION_1, SEND_CENTRAL_SCENE); \
						CONST_CODE(_DEFAULT_CLICK_ACTION_2, SEND_CENTRAL_SCENE); \
						CONST_CODE(_DEFAULT_CLICK_ACTION_3, SEND_CENTRAL_SCENE); \
						CONST_CODE(_DEFAULT_CLICK_ACTION_4, SEND_CENTRAL_SCENE); \
						CONST_CODE(_DEFAULT_GROUP_BUTTONS_13, UNITED_BUTTONS_NO_D_CLICK); \
						CONST_CODE(_DEFAULT_GROUP_BUTTONS_24, UNITED_BUTTONS_NO_D_CLICK);


// Slave
#define SLAVE_UPDATE_ACTION(S, N, C, L)     
#define LEARN_COMPLETE_ACTION(nodeId)   { }

/////////////////////////////////////////////////////////////////////
// Hardware

#define INIT_CUSTOM_HW()        {\	
																	PIN_OUT(RedLEDpin);PIN_ON(RedLEDpin);\
																	PIN_OUT(Relay0pin);PIN_OFF(Relay0pin);\
																	PIN_OUT(Relay1pin);PIN_OFF(Relay1pin);\
                                }

#define INIT_CUSTOM_SW()        {  } 

#define SWITCH_OFF_HW()         {\
																	PIN_OFF(RedLEDpin);\
																}

// Device modes
// MODE_IDLE, MODE_LEARN and MODE_LEARN_IN_PROGRESS are defined in the template. Device goes into sleep only in MODE_IDLE
#define CUSTOM_MODES \
	MODE_MANAGEMENT_SELECT, \
	MODE_MANAGEMENT_ADVANCED_SELECT, \
	MODE_MANAGEMENT_FREQ_SELECT, \
	MODE_ASSOCIATION_ASSIGN_SELF, \
	MODE_ASSOCIATION_ADD_SELF, \
	MODE_ASSOCIATION_REMOVE_SELF


// LED
// !!
/*    
#define LED_HW_INIT()           { PIN_OUT(GreenLEDpin); PIN_OUT(RedLEDpin); }
#define LED_HW_OFF()            { PIN_OFF(GreenLEDpin); PIN_OFF(RedLEDpin); }
#define LED_IS_OFF()            (LEDIsOff())
#define LED_COLOR_CODES_ARRAY \
                    LED_COLOR_NO, \
                    LED_COLOR_GREEN, \
                    LED_COLOR_RED

#define LED_HW_SET_COLOR(color) { \
                switch(color) { \
                    case LED_COLOR_NO: \
                            PIN_OFF(GreenLEDpin); PIN_OFF(RedLEDpin); \
                            break; \
                    case LED_COLOR_GREEN: \
                            PIN_ON(GreenLEDpin); PIN_OFF(RedLEDpin); \
                            break; \
                    case LED_COLOR_RED: \
                            PIN_OFF(GreenLEDpin); PIN_ON(RedLEDpin); \
                            break; \
                } \
    }
*/
#define LED_HW_INIT()           {  }
#define LED_HW_OFF()            {  }
#define LED_IS_OFF()            ()
#define LED_COLOR_CODES_ARRAY \
                    LED_COLOR_NO, \
                    LED_COLOR_GREEN, \
                    LED_COLOR_RED

#define LED_HW_SET_COLOR(color) { \
                switch(color) { \
                    case LED_COLOR_NO: \
                             \
                            break; \
                    case LED_COLOR_GREEN: \
                             \
                            break; \
                    case LED_COLOR_RED: \
                             \
                            break; \
                } \
    }

#define LED_TICK_RATE           8 // in 100 ms ticks
// Adjust the size of LED codes !

//// LED_NO, LED_DONE, LED_FAILED, LED_NWI
#define BASIC_LED_CODES_ARRAY \
                    {0, LED_COLOR_NO, LED_COLOR_NO, LED_COLOR_NO, LED_COLOR_NO, LED_COLOR_NO, LED_COLOR_NO, LED_COLOR_NO, LED_COLOR_NO }, \
                    {20, LED_COLOR_GREEN, LED_COLOR_NO, LED_COLOR_GREEN, LED_COLOR_NO, LED_COLOR_GREEN, LED_COLOR_NO, LED_COLOR_GREEN, LED_COLOR_NO }, \
                    {5, LED_COLOR_RED, LED_COLOR_RED, LED_COLOR_RED, LED_COLOR_RED, LED_COLOR_RED, LED_COLOR_RED, LED_COLOR_RED, LED_COLOR_RED }, \
                    {0, LED_COLOR_RED, LED_COLOR_GREEN, LED_COLOR_RED, LED_COLOR_GREEN, LED_COLOR_RED, LED_COLOR_GREEN, LED_COLOR_RED, LED_COLOR_GREEN },
#define CUSTOM_LED_CODES \
                    LED_WAITING_INPUT, \
                    LED_WAITING_INPUT2, \
                    LED_WAITING_INPUT3, \
                    LED_WAITING_NIF, \
                    LED_RESET_CONFIRM
#define CUSTOM_LED_CODES_ARRAY \
                    {0, LED_COLOR_GREEN, LED_COLOR_GREEN, LED_COLOR_GREEN, LED_COLOR_GREEN, LED_COLOR_NO, LED_COLOR_NO, LED_COLOR_NO, LED_COLOR_NO }, \
                    {0, LED_COLOR_GREEN, LED_COLOR_GREEN, LED_COLOR_RED, LED_COLOR_RED, LED_COLOR_GREEN, LED_COLOR_GREEN, LED_COLOR_RED, LED_COLOR_RED }, \
                    {0, LED_COLOR_GREEN, LED_COLOR_GREEN, LED_COLOR_NO, LED_COLOR_NO, LED_COLOR_GREEN, LED_COLOR_GREEN, LED_COLOR_NO, LED_COLOR_NO }, \
                    {0, LED_COLOR_GREEN, LED_COLOR_NO, LED_COLOR_RED, LED_COLOR_NO, LED_COLOR_GREEN, LED_COLOR_NO, LED_COLOR_RED, LED_COLOR_NO }, \
                    {20, LED_COLOR_RED, LED_COLOR_GREEN, LED_COLOR_RED, LED_COLOR_GREEN, LED_COLOR_RED, LED_COLOR_GREEN, LED_COLOR_RED, LED_COLOR_GREEN }
          
// Buttons
#define NUM_BUTTONS         2
#define BUTTONS_WITH_DOUBLE_CLICKS  BUTTONS_CONFIG_YES  // BUTTONS_CONFIG_NO | BUTTONS_CONFIG_YES | BUTTONS_CONFIG_CONFIGURABLE
//#define BUTTONS_DOUBLE_CLICKS_ENABLED waitForDoubleClicks[i]  // used if BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE. i is the button index defined in ButtonsPoll
#define BUTTONS_WITH_TRIPPLE_CLICKS BUTTONS_CONFIG_NO   // BUTTONS_CONFIG_NO | BUTTONS_CONFIG_YES | BUTTONS_CONFIG_CONFIGURABLE
//#define BUTTONS_TRIPPLE_CLICKS_ENABLED    waitForTrippleClicks[i] // used if BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE. i is the button index defined in ButtonsPoll
#define BUTTONS_WITH_HOLDS      BUTTONS_CONFIG_YES  // BUTTONS_CONFIG_NO | BUTTONS_CONFIG_YES | BUTTONS_CONFIG_CONFIGURABLE
//#define BUTTONS_HOLDS_ENABLED     waitForHoldsClicks[i]   // used if BUTTONS_WITH_HOLDS == BUTTONS_CONFIG_CONFIGURABLE. i is the button index defined in ButtonsPoll
#define BUTTONS_WITH_LONG_HOLDS     BUTTONS_CONFIG_YES  // BUTTONS_CONFIG_NO | BUTTONS_CONFIG_YES // BUTTONS_CONFIG_CONFIGURABLE is not available
#define BUTTONS_HW_INIT()       {   \
																		PIN_IN(Button0pin, 1);\
																		PIN_IN(Button1pin, 1);\
																}
                                                       

#define BUTTONS_HW_STOP()       { }
#define buttonClickTimeout      30 // 300 ms
// #define ButtonPressed(buttonIndex)   

// Clock
#define ONE_SECOND_TICK()       {\																	
																}
#define T10MS_TICK()       		{\	
																	Tick();\	
																}
#define QUARTER_SECOND_TICK()   {\	
																}

// Wakeup condition
#define WAKEUP_INT1_LEVEL       0 // can be 0, 1 or !PIN_GET(INT1pin)
#define WAKEUP_INT1_EDGE_OR_LEVEL   0 // level

/////////////////////////////////////////////////////////////////////
// Command Class specific

//  VERSION V2
#define VERSION2_HARDWARE_VERSION               1   //  Версия "железа" 
#define VERSION2_FIRMWARE_COUNT                 0   //  Количество дополнительных прошивок 
#define VERSION2_FIRMWARE_VERSION(N)            0   //  Версия N-ой прошивки (Целая часть)
#define VERSION2_FIRMWARE_SUBVERSION(N)         0   //  Версия N-ой прошивки (Дробная часть)

 // ASSOCIATION_GROUP_INFORMATION CC

// Необходимые макросы
#define AGI_NAME(G, NAME)    groupName(G, NAME)// Возвращает имя группы (Записывает указатель на имя группы в NAME)
#define AGI_PROFILE(G)       ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_NA//(0xaab0 | G)//ASSOCIATION_GROUP_INFO_REPORT_PROFILE_NOTIFICATION//ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_NA// Возвращает профиль группы
#define AGI_CCS_COUNT(G)     groupCommandCount(G)// 1// Возвращает число возможных комманд для группы 
#define AGI_CCS(G, I)        groupCommand(G, I)//(((WORD)COMMAND_CLASS_BASIC<<8)|BASIC_SET)// Возвращает соответствующую команду для группы
                                                      

// Basic CC
#define BASIC_SET_ACTION(V)          BasicAction(V)

#define BASIC_REPORT_VALUE           BasicGet()

// WakeUp CC
#define DEFAULT_WAKE_UP_INTERVAL    604800 // 1 week
#define SLEEP_TIME                  240 // override default WUT period

// Association CC
#define ASSOCIATION_NUM_GROUPS          5
#define ASSOCIATION_GROUP_SIZE          10
// NB! groups numbering in the code should start from 0
#define ASSOCIATION_REPORTS_GROUP_NUM   0 // group to send unsolicited reports

// Central Scene CC
#define CENTRAL_SCENE_MAX_SCENES        8
                                                        

// Configuration CC

#define CONFIG_PARAMETER_GET\
        MAKE_CONFIG_PARAMETER_GETTER_1B(1,  CURRENT_BUTTONS_PARAM_13)\
        MAKE_CONFIG_PARAMETER_GETTER_1B(2,  CURRENT_BUTTONS_PARAM_24)\
        MAKE_CONFIG_PARAMETER_GETTER_1B(11, CLICK_ACTION_1)\
        MAKE_CONFIG_PARAMETER_GETTER_1B(12, CLICK_ACTION_2)\
        MAKE_CONFIG_PARAMETER_GETTER_1B(13, CLICK_ACTION_3)\
        MAKE_CONFIG_PARAMETER_GETTER_1B(14, CLICK_ACTION_4)\
        MAKE_CONFIG_PARAMETER_GETTER_1B(21, SEND_SWITCH_ALL)\
        MAKE_CONFIG_PARAMETER_GETTER_1B(22, INVERT_SWITCH)//\
        //MAKE_CONFIG_PARAMETER_GETTER_1B(25, ENABLE_WAKEUPS) \
        //MAKE_CONFIG_PARAMETER_GETTER_1B(30, UNSOLICITED_BATTERY_REPORT)

#define VALID_BUTTONS_PARAM(V) (    V == SEPARATE_BUTTONS || \
                                    V == UNITED_BUTTONS_NO_D_CLICK || \
                                    V == UNITED_BUTTONS_WITH_D_CLICK)

#define VALID_ACTION_PARAM(V)  (V == ACTION_DISABLED || \
                                    V == SEND_BASIC_SET_ML || \
                                    V == SEND_BASIC_SET || \
                                    V == SEND_DOOR_LOCK || \
                                    V == SEND_SWITCH_ALL || \
                                    V == SEND_SCENE || \ 
                                    V == SEND_SCENE_ACTIVATION || \
                                    V == SEND_PROXIMITY || \
                                    V == SEND_CENTRAL_SCENE)

#define VALID_SWITCH_ALL_PARAM(V) (V == SWITCH_ALL_REPORT_EXCLUDED_FROM_THE_ALL_ON_FUNCTIONALITY_BUT_NOT_ALL_OFF || \ 
                                    V == SWITCH_ALL_REPORT_EXCLUDED_FROM_THE_ALL_OFF_FUNCTIONALITY_BUT_NOT_ALL_ON || \
                                     V == SWITCH_ALL_REPORT_INCLUDED_IN_THE_ALL_ON_ALL_OFF_FUNCTIONALITY)

#define VALID_INVERT_SWITCH_PARAM(V) (V == INVERT_SWITCH_NO || V == INVERT_SWITCH_YES)                                  

#define VALID_ENABLE_WAKEUPS_PARAM(V)   (V == ENABLE_WAKEUPS_YES || V == ENABLE_WAKEUPS_NO)

#define VALID_UNSOLICITED_BATTERY_REPORT_PARAM(V)   (V == UNSOLICITED_BATTERY_REPORT_NO || V == UNSOLICITED_BATTERY_REPORT_WAKEUP_NODE || V == UNSOLICITED_BATTERY_REPORT_BROADCAST)

#define DUMMY_ACTION(OLD, NEW)

#define CONFIG_PARAMETER_SET\
        MAKE_CONFIG_PARAMETER_SETTER_1B(1,  CURRENT_BUTTONS_PARAM_13,   VALID_BUTTONS_PARAM,                    DUMMY_ACTION)\
        MAKE_CONFIG_PARAMETER_SETTER_1B(2,  CURRENT_BUTTONS_PARAM_24,   VALID_BUTTONS_PARAM,                    DUMMY_ACTION)\
        MAKE_CONFIG_PARAMETER_SETTER_1B(11, CLICK_ACTION_1,             VALID_ACTION_PARAM,                     DUMMY_ACTION)\
        MAKE_CONFIG_PARAMETER_SETTER_1B(12, CLICK_ACTION_2,             VALID_ACTION_PARAM,                     DUMMY_ACTION)\
        MAKE_CONFIG_PARAMETER_SETTER_1B(13, CLICK_ACTION_3,             VALID_ACTION_PARAM,                     DUMMY_ACTION)\
        MAKE_CONFIG_PARAMETER_SETTER_1B(14, CLICK_ACTION_4,             VALID_ACTION_PARAM,                     DUMMY_ACTION)\
        MAKE_CONFIG_PARAMETER_SETTER_1B(21, SEND_SWITCH_ALL,            VALID_SWITCH_ALL_PARAM,                 DUMMY_ACTION)\
        MAKE_CONFIG_PARAMETER_SETTER_1B(22, INVERT_SWITCH,              VALID_INVERT_SWITCH_PARAM,              DUMMY_ACTION)//\
        //MAKE_CONFIG_PARAMETER_SETTER_1B(25, ENABLE_WAKEUPS,             VALID_ENABLE_WAKEUPS_PARAM,             DUMMY_ACTION)\
        //MAKE_CONFIG_PARAMETER_SETTER_1B(30, UNSOLICITED_BATTERY_REPORT, VALID_UNSOLICITED_BATTERY_REPORT_PARAM, DUMMY_ACTION)

// Secure CC
#define MAX_DATA_SENDERS                10  // number of packets sent simultaneously to different nodes
#define MAX_DATA_RECEIVERS          10  // number of packets received at one time from different nodes
// number of nonces to keep in the memory (recommended value is above 30), but if intensive data flow is expected (during interview), use bigger value
// NB! It is important that each nonce will live for 40 seconds after it was used, so to complete Security interview in less than 40 seconds we need to have enough nonces!
#define MAX_NONCES                   250


// FWUpdate CC
// Чтобы не срабатывала автоматическая перезагрузка после обновления - обьявляем пустой обработчик
//#define FWUPDATE_DONE_HANDLER

// Protection CC
#define PROTECTION_LOCK_TIMEOUT         5 // seconds

// ThermostatMode CC
#define CC_THERMOSTAT_MODE_WITH_MODE_OFF_V2             0
#define CC_THERMOSTAT_MODE_WITH_MODE_HEAT_V2            0
#define CC_THERMOSTAT_MODE_WITH_MODE_COOL_V2            0
#define CC_THERMOSTAT_MODE_WITH_MODE_AUTO_V2            0
#define CC_THERMOSTAT_MODE_WITH_MODE_AUXILIARY_HEAT_V2  0
#define CC_THERMOSTAT_MODE_WITH_MODE_RESUME_V2          0
#define CC_THERMOSTAT_MODE_WITH_MODE_FAN_ONLY_V2        0
#define CC_THERMOSTAT_MODE_WITH_MODE_FURNACE_V2         0
#define CC_THERMOSTAT_MODE_WITH_MODE_DRY_AIR_V2         0
#define CC_THERMOSTAT_MODE_WITH_MODE_MOIST_AIR_V2       0
#define CC_THERMOSTAT_MODE_WITH_MODE_AUTO_CHANGEOVER_V2     0
#define CC_THERMOSTAT_MODE_WITH_MODE_ENERGY_SAVE_HEAT_V2    0
#define CC_THERMOSTAT_MODE_WITH_MODE_ENERGY_SAVE_COOL_V2    0
#define CC_THERMOSTAT_MODE_WITH_MODE_AWAY_V2            0
#define CC_THERMOSTAT_MODE_WITH_MODE_FULL_POWER_V3      0
#define CC_THERMOSTAT_MODE_WITH_MODE_MANUFACTURER_SPECIFIC_V3   0

#define DEFAULT_THERMOSTAT_MODE                 THERMOSTAT_MODE_REPORT_MODE_HEAT_V2

#define THERMOSTAT_MODE_SET_ACTION()                UpdateStellaFThermostatModeTemp()

// ThermostatSetPoint CC
#define CC_THERMOSTAT_SETPOINT_WITH_HEATING_1_V2        0
#define CC_THERMOSTAT_SETPOINT_WITH_COOLING_1_V2        0
#define CC_THERMOSTAT_SETPOINT_WITH_FURNACE_V2          0
#define CC_THERMOSTAT_SETPOINT_WITH_DRY_AIR_V2          0
#define CC_THERMOSTAT_SETPOINT_WITH_MOIST_AIR_V2        0
#define CC_THERMOSTAT_SETPOINT_WITH_AUTO_CHANGEOVER_V2      0
#define CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_HEATING_V2  0
#define CC_THERMOSTAT_SETPOINT_WITH_ENERGY_SAVE_COOLING_V2  0
#define CC_THERMOSTAT_SETPOINT_WITH_AWAY_HEATING_V2     0
#define CC_THERMOSTAT_SETPOINT_WITH_AWAY_COOLING_V3     0
#define CC_THERMOSTAT_SETPOINT_WITH_FULL_POWER_V3       0

// min/max temperatures in precision units (for precision 1 temperature 26 degrees is 260)
#define CC_THERMOSTAT_SETPOINT_HEATING_1_MIN                
#define CC_THERMOSTAT_SETPOINT_HEATING_1_MAX                
#define CC_THERMOSTAT_SETPOINT_COOLING_1_MIN
#define CC_THERMOSTAT_SETPOINT_COOLING_1_MAX
#define CC_THERMOSTAT_SETPOINT_FURNACE_MIN
#define CC_THERMOSTAT_SETPOINT_FURNACE_MAX
#define CC_THERMOSTAT_SETPOINT_DRY_AIR_MIN
#define CC_THERMOSTAT_SETPOINT_DRY_AIR_MAX
#define CC_THERMOSTAT_SETPOINT_MOIST_AIR_MIN
#define CC_THERMOSTAT_SETPOINT_MOIST_AIR_MAX
#define CC_THERMOSTAT_SETPOINT_AUTO_CHANGEOVER_MIN
#define CC_THERMOSTAT_SETPOINT_AUTO_CHANGEOVER_MAX
#define CC_THERMOSTAT_SETPOINT_ENERGY_SAVE_HEATING_MIN  
#define CC_THERMOSTAT_SETPOINT_ENERGY_SAVE_HEATING_MAX  
#define CC_THERMOSTAT_SETPOINT_ENERGY_SAVE_COOLING_MIN
#define CC_THERMOSTAT_SETPOINT_ENERGY_SAVE_COOLING_MAX
#define CC_THERMOSTAT_SETPOINT_AWAY_HEATING_MIN
#define CC_THERMOSTAT_SETPOINT_AWAY_HEATING_MAX
#define CC_THERMOSTAT_SETPOINT_AWAY_COOLING_MIN
#define CC_THERMOSTAT_SETPOINT_AWAY_COOLING_MAX
#define CC_THERMOSTAT_SETPOINT_FULL_POWER_MIN
#define CC_THERMOSTAT_SETPOINT_FULL_POWER_MAX

// defaults are in precision 0.1 C, so multily by 10
//#define DEFAULT_THERMOSTAT_SETPOINT_HEAT          (22*10)
//#define DEFAULT_THERMOSTAT_SETPOINT_ENERGY_SAVE_HEAT      (18*10)

//#define THERMOSTAT_SETPOINT_SET_ACTION()          

// SensorMultilevel V4
#define CC_SENSOR_MULTILEVEL_V4_SENSOR_TYPE         SENSOR_MULTILEVEL_REPORT_TEMPERATURE_VERSION_1_V4
#define CC_SENSOR_MULTILEVEL_V4_SIZE                2
#define CC_SENSOR_MULTILEVEL_V4_SCALE               0 // Celsius
#define CC_SENSOR_MULTILEVEL_V4_PRECISION           1

// SensorMultilevel CC
#define CC_SENSOR_MULTILEVEL_WITH_AIR_TEMPERATURE       0
#define CC_SENSOR_MULTILEVEL_WITH_GENERAL_PURPOSE       0
#define CC_SENSOR_MULTILEVEL_WITH_LUMINANCE         0
#define CC_SENSOR_MULTILEVEL_WITH_POWER             0
#define CC_SENSOR_MULTILEVEL_WITH_HUMIDITY          0
#define CC_SENSOR_MULTILEVEL_WITH_VELOCITY          0
#define CC_SENSOR_MULTILEVEL_WITH_DIRECTION         0
#define CC_SENSOR_MULTILEVEL_WITH_ATHMOSPHERIC_PRESSURE     0
#define CC_SENSOR_MULTILEVEL_WITH_BAROMETRIC_PRESSURE       0
#define CC_SENSOR_MULTILEVEL_WITH_SOLAR_RADIATION       0
#define CC_SENSOR_MULTILEVEL_WITH_DEW_POINT         0
#define CC_SENSOR_MULTILEVEL_WITH_RAIN_RATE         0
#define CC_SENSOR_MULTILEVEL_WITH_TIDE_LEVEL            0
#define CC_SENSOR_MULTILEVEL_WITH_WEIGHT            0
#define CC_SENSOR_MULTILEVEL_WITH_VOLTAGE           0
#define CC_SENSOR_MULTILEVEL_WITH_CURRENT           0
#define CC_SENSOR_MULTILEVEL_WITH_CO2_LEVEL         0
#define CC_SENSOR_MULTILEVEL_WITH_AIR_FLOW          0
#define CC_SENSOR_MULTILEVEL_WITH_TANK_CAPACITY         0
#define CC_SENSOR_MULTILEVEL_WITH_DISTANCE          0
#define CC_SENSOR_MULTILEVEL_WITH_ANGLE_POSITION        0
#define CC_SENSOR_MULTILEVEL_WITH_ROTATION          0
#define CC_SENSOR_MULTILEVEL_WITH_WATER_TEMPERATURE     0
#define CC_SENSOR_MULTILEVEL_WITH_SOIL_TEMPERATURE      0
#define CC_SENSOR_MULTILEVEL_WITH_SEISMIC_INTENSITY     0
#define CC_SENSOR_MULTILEVEL_WITH_SEISMIC_MAGNITUDE     0
#define CC_SENSOR_MULTILEVEL_WITH_ULTRAVIOLET           0
#define CC_SENSOR_MULTILEVEL_WITH_ELECTRICAL_RESISTIVITY    0
#define CC_SENSOR_MULTILEVEL_WITH_ELECTRICAL_CONDUCTIVITY   0
#define CC_SENSOR_MULTILEVEL_WITH_LOUDNESS          0
#define CC_SENSOR_MULTILEVEL_WITH_MOISTURE          0
#define CC_SENSOR_MULTILEVEL_WITH_FREQUENCY         0
#define CC_SENSOR_MULTILEVEL_WITH_TIME              0
#define CC_SENSOR_MULTILEVEL_WITH_TARGET_TEMPERATURE        0

#define CC_SENSOR_MULTILEVEL_DEFAULT_SENSOR_TYPE        SENSOR_MULTILEVEL_REPORT_TEMPERATURE_VERSION_1_V5
#define CC_SENSOR_MULTILEVEL_SIZE               2

#define CC_SENSOR_MULTILEVEL_AIR_TEMPERATURE_GETTER     
#define CC_SENSOR_MULTILEVEL_GENERAL_PURPOSE_GETTER
#define CC_SENSOR_MULTILEVEL_LUMINANCE_GETTER
#define CC_SENSOR_MULTILEVEL_POWER_GETTER
#define CC_SENSOR_MULTILEVEL_HUMIDITY_GETTER
#define CC_SENSOR_MULTILEVEL_VELOCITY_GETTER
#define CC_SENSOR_MULTILEVEL_DIRECTION_GETTER
#define CC_SENSOR_MULTILEVEL_ATHMOSPHERIC_PRESSURE_GETTER
#define CC_SENSOR_MULTILEVEL_BAROMETRIC_PRESSURE_GETTER
#define CC_SENSOR_MULTILEVEL_SOLAR_RADIATION_GETTER
#define CC_SENSOR_MULTILEVEL_DEW_POINT_GETTER
#define CC_SENSOR_MULTILEVEL_RAIN_RATE_GETTER
#define CC_SENSOR_MULTILEVEL_TIDE_LEVEL_GETTER
#define CC_SENSOR_MULTILEVEL_WEIGHT_GETTER
#define CC_SENSOR_MULTILEVEL_VOLTAGE_GETTER
#define CC_SENSOR_MULTILEVEL_CURRENT_GETTER
#define CC_SENSOR_MULTILEVEL_CO2_LEVEL_GETTER
#define CC_SENSOR_MULTILEVEL_AIR_FLOW_GETTER
#define CC_SENSOR_MULTILEVEL_TANK_CAPACITY_GETTER
#define CC_SENSOR_MULTILEVEL_DISTANCE_GETTER
#define CC_SENSOR_MULTILEVEL_ANGLE_POSITION_GETTER
#define CC_SENSOR_MULTILEVEL_ROTATION_GETTER
#define CC_SENSOR_MULTILEVEL_WATER_TEMPERATURE_GETTER
#define CC_SENSOR_MULTILEVEL_SOIL_TEMPERATURE_GETTER
#define CC_SENSOR_MULTILEVEL_SEISMIC_INTENSITY_GETTER
#define CC_SENSOR_MULTILEVEL_SEISMIC_MAGNITUDE_GETTER
#define CC_SENSOR_MULTILEVEL_ULTRAVIOLET_GETTER
#define CC_SENSOR_MULTILEVEL_ELECTRICAL_RESISTIVITY_GETTER
#define CC_SENSOR_MULTILEVEL_ELECTRICAL_CONDUCTIVITY_GETTER
#define CC_SENSOR_MULTILEVEL_LOUDNESS_GETTER
#define CC_SENSOR_MULTILEVEL_MOISTURE_GETTER
#define CC_SENSOR_MULTILEVEL_FREQUENCY_GETTER
#define CC_SENSOR_MULTILEVEL_TIME_GETTER
#define CC_SENSOR_MULTILEVEL_TARGET_TEMPERATURE_GETTER

#define CC_SENSOR_MULTILEVEL_AIR_TEMPERATURE_PRECISION      
#define CC_SENSOR_MULTILEVEL_GENERAL_PURPOSE_PRECISION
#define CC_SENSOR_MULTILEVEL_LUMINANCE_PRECISION
#define CC_SENSOR_MULTILEVEL_POWER_PRECISION
#define CC_SENSOR_MULTILEVEL_HUMIDITY_PRECISION
#define CC_SENSOR_MULTILEVEL_VELOCITY_PRECISION
#define CC_SENSOR_MULTILEVEL_DIRECTION_PRECISION
#define CC_SENSOR_MULTILEVEL_ATHMOSPHERIC_PRESSURE_PRECISION
#define CC_SENSOR_MULTILEVEL_BAROMETRIC_PRESSURE_PRECISION
#define CC_SENSOR_MULTILEVEL_SOLAR_RADIATION_PRECISION
#define CC_SENSOR_MULTILEVEL_DEW_POINT_PRECISION
#define CC_SENSOR_MULTILEVEL_RAIN_RATE_PRECISION
#define CC_SENSOR_MULTILEVEL_TIDE_LEVEL_PRECISION
#define CC_SENSOR_MULTILEVEL_WEIGHT_PRECISION
#define CC_SENSOR_MULTILEVEL_VOLTAGE_PRECISION
#define CC_SENSOR_MULTILEVEL_CURRENT_PRECISION
#define CC_SENSOR_MULTILEVEL_CO2_LEVEL_PRECISION
#define CC_SENSOR_MULTILEVEL_AIR_FLOW_PRECISION
#define CC_SENSOR_MULTILEVEL_TANK_CAPACITY_PRECISION
#define CC_SENSOR_MULTILEVEL_DISTANCE_PRECISION
#define CC_SENSOR_MULTILEVEL_ANGLE_POSITION_PRECISION
#define CC_SENSOR_MULTILEVEL_ROTATION_PRECISION
#define CC_SENSOR_MULTILEVEL_WATER_TEMPERATURE_PRECISION
#define CC_SENSOR_MULTILEVEL_SOIL_TEMPERATURE_PRECISION
#define CC_SENSOR_MULTILEVEL_SEISMIC_INTENSITY_PRECISION
#define CC_SENSOR_MULTILEVEL_SEISMIC_MAGNITUDE_PRECISION
#define CC_SENSOR_MULTILEVEL_ULTRAVIOLET_PRECISION
#define CC_SENSOR_MULTILEVEL_ELECTRICAL_RESISTIVITY_PRECISION
#define CC_SENSOR_MULTILEVEL_ELECTRICAL_CONDUCTIVITY_PRECISION
#define CC_SENSOR_MULTILEVEL_LOUDNESS_PRECISION
#define CC_SENSOR_MULTILEVEL_MOISTURE_PRECISION
#define CC_SENSOR_MULTILEVEL_FREQUENCY_PRECISION
#define CC_SENSOR_MULTILEVEL_TIME_PRECISION
#define CC_SENSOR_MULTILEVEL_TARGET_TEMPERATURE_PRECISION

#define CC_SENSOR_MULTILEVEL_AIR_TEMPERATURE_SCALE      0 // Celsius
#define CC_SENSOR_MULTILEVEL_GENERAL_PURPOSE_SCALE      
#define CC_SENSOR_MULTILEVEL_LUMINANCE_SCALE        
#define CC_SENSOR_MULTILEVEL_POWER_SCALE            
#define CC_SENSOR_MULTILEVEL_HUMIDITY_SCALE     
#define CC_SENSOR_MULTILEVEL_VELOCITY_SCALE     
#define CC_SENSOR_MULTILEVEL_DIRECTION_SCALE        
#define CC_SENSOR_MULTILEVEL_ATHMOSPHERIC_PRESSURE_SCALE    
#define CC_SENSOR_MULTILEVEL_BAROMETRIC_PRESSURE_SCALE  
#define CC_SENSOR_MULTILEVEL_SOLAR_RADIATION_SCALE      
#define CC_SENSOR_MULTILEVEL_DEW_POINT_SCALE        
#define CC_SENSOR_MULTILEVEL_RAIN_RATE_SCALE        
#define CC_SENSOR_MULTILEVEL_TIDE_LEVEL_SCALE       
#define CC_SENSOR_MULTILEVEL_WEIGHT_SCALE           
#define CC_SENSOR_MULTILEVEL_VOLTAGE_SCALE      
#define CC_SENSOR_MULTILEVEL_CURRENT_SCALE      
#define CC_SENSOR_MULTILEVEL_CO2_LEVEL_SCALE        
#define CC_SENSOR_MULTILEVEL_AIR_FLOW_SCALE     
#define CC_SENSOR_MULTILEVEL_TANK_CAPACITY_SCALE        
#define CC_SENSOR_MULTILEVEL_DISTANCE_SCALE     
#define CC_SENSOR_MULTILEVEL_ANGLE_POSITION_SCALE       
#define CC_SENSOR_MULTILEVEL_ROTATION_SCALE     
#define CC_SENSOR_MULTILEVEL_WATER_TEMPERATURE_SCALE    
#define CC_SENSOR_MULTILEVEL_SOIL_TEMPERATURE_SCALE 
#define CC_SENSOR_MULTILEVEL_SEISMIC_INTENSITY_SCALE    
#define CC_SENSOR_MULTILEVEL_SEISMIC_MAGNITUDE_SCALE    
#define CC_SENSOR_MULTILEVEL_ULTRAVIOLET_SCALE      
#define CC_SENSOR_MULTILEVEL_ELECTRICAL_RESISTIVITY_SCALE   
#define CC_SENSOR_MULTILEVEL_ELECTRICAL_CONDUCTIVITY_SCALE  
#define CC_SENSOR_MULTILEVEL_LOUDNESS_SCALE     
#define CC_SENSOR_MULTILEVEL_MOISTURE_SCALE     
#define CC_SENSOR_MULTILEVEL_FREQUENCY_SCALE        
#define CC_SENSOR_MULTILEVEL_TIME_SCALE         
#define CC_SENSOR_MULTILEVEL_TARGET_TEMPERATURE_SCALE   

// Battery CC
// comment this if you want to use battery monitor by Z-Wave chip
//#define BATTERY_LEVEL_GET()         100 //BatteryLevel()
#define BATTERY_TYPE	BATTERY_TYPE_CR2032


