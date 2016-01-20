#ifndef __ZME_BATTERY
#define __ZME_BATTERY

#define BATTERY_WARNING_LEVEL	0x0f // if battery level is small
#define BATTERY_WARNING_REPORT	0xff // report 'replace battery'

#define BATTERY_TYPE_2AA		0
#define BATTERY_TYPE_CR123A		1
#define BATTERY_TYPE_CR2032		2
#define BATTERY_TYPE_MINMAX		3
#define BATTERY_TYPE_CUSTOM		255


#if BATTERY_TYPE == BATTERY_TYPE_2AA
        
                // AA/AAA/AAAA battaries:
                // Battery level = 1.21 V * 255 / ADCval
                // 3.00 V = 100 %
                // 2.4 V =   0 %
                // % = 100 * (Battery level - 2.4)/(3 - 2.4) 
                
        #define BATTERY_RANGE_HIGH 30
        #define BATTERY_RANGE_LOW  24

        
#elif BATTERY_TYPE == BATTERY_TYPE_CR123A
       #define BATTERY_RANGE_HIGH 31
       #define BATTERY_RANGE_LOW  24

#elif BATTERY_TYPE == BATTERY_TYPE_CR2032
        
               // CR2032 battery
               // Battery level = 1.21 V * 255 / ADCval
               // 2.6 V = 100 %
               // 2.1 V =   0 %
               // % = 100 * (Battery level - 2.1)/(2.6 - 2.1) 
        
        #define BATTERY_RANGE_HIGH 26
        #define BATTERY_RANGE_LOW  21
        // #warning "BATTERY CR2032"

#endif

#if (BATTERY_TYPE != BATTERY_TYPE_CUSTOM)

#ifdef BATTERY_LEVEL_GET
#undef BATTERY_LEVEL_GET
#endif
/*
#ifdef BATTERY_LEVEL_GET
#undef BATTERY_LEVEL_GET
#endif
*/

#define BATTERY_LEVEL_GET BatteryLevel()
//static 
BYTE BatteryLevel(void);
#define USE_DEFAULT_BATTERY_STATUS

#endif // BATTERY_TYPE != BATTERY_TYPE_CUSTOM
#endif // ZME_BATTERY
