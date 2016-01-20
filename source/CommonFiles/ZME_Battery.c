#include "ZME_Includes.h"

#if WITH_CC_BATTERY
#include "ZME_Battery.h"


// #include <ZW_non_zero.h> 
// #include <battery_monitor.h>
// #include <association_plus.h>
#include <ZW_uart_api.h>
#include <ZW_adcdriv_api.h>
// #include <CommandClassBattery.h>
// #include "misc.h"

// void ZCB_BattReportSentDone(BYTE txStatus);

// extern XBYTE lowBattReportAcked; /*the i var is defined in battery_non_zero_vars.c*/

#define ADC_RES      256
#define ADC_REF_MV   1230
// #define BATT_LOW_MV  2500

#ifndef BATTERY_RANGE_LOW 
#define BATTERY_RANGE_LOW 25
#endif
#define BATT_LOW_ADC_THRES  ((ADC_REF_MV * ADC_RES)/(BATTERY_RANGE_LOW*100))


#if WITH_PENDING_BATTERY

BYTE * pendingBatteryValue();
BYTE * pendingBatteryCount();

#endif

#ifdef ZW050x

#if (BATTERY_TYPE != BATTERY_TYPE_CUSTOM)
BYTE BatteryLevel(void) {  
        BYTE b, r;
        WORD wb, v;
        //WORD aux;
        

        /*initalise the the adc in battery monitor mode, note that the reference , and input are ignored in the battery monitor mode*/
        /*The adc can be initaised any where in the application but in this sample it done in here this we will only use */
      /*in battery monitor mode*/
      ZW_ADC_init(ADC_BATT_MON_MODE,
              ADC_REF_U_VDD,
              ADC_REF_L_VSS,
              ADC_PIN_BATT);
      ZW_ADC_resolution_set(ADC_8_BIT); /*we use 8 bit resolution since it faster and use less code space*/
      /*set the threshold mode so that the threhold is trigggered if the conversion value is higher than the thrshold*/
      /*The higher the asdc value the lower the battery level*/
      ZW_ADC_threshold_mode_set(ADC_THRES_UPPER);
      /*The battery low level voltage threshold shoud always be lower than the POR limit which has max value of 2.22 volte */
      /*In our sample we set the low level threshold to 2.5*/
      ZW_ADC_threshold_set(BATT_LOW_ADC_THRES);

      ZW_ADC_batt_monitor_enable(TRUE);
      ZW_ADC_int_enable(FALSE);
      ZW_ADC_int_clear();


      ZW_ADC_enable(TRUE);
      ZW_ADC_power_enable(TRUE);
      while ( (v = ZW_ADC_result_get()) == ADC_NOT_FINISHED )
        ZW_POLL(); // ?
      
      ZW_ADC_enable(FALSE);
      ZW_ADC_power_enable(FALSE);

      if (ZW_ADC_is_fired())
      {
         return BATTERY_WARNING_REPORT;
      }
       
          
      // Напряжение в mV
      //aux = (ADC_REF_MV * ADC_RES) / ((DWORD)(v&0x00ff));
      wb = (31488 / (v & 0x00ff))*10; 
      // Переводим в %
      wb  = (wb - (100 * BATTERY_RANGE_LOW)) / (BATTERY_RANGE_HIGH - BATTERY_RANGE_LOW);
      
      //aux /= 100;
      //wb = ((123*256*10) /(BATTERY_RANGE_HIGH - BATTERY_RANGE_LOW)) / (v&0xff) - (100*BATTERY_RANGE_LOW)/(BATTERY_RANGE_HIGH - BATTERY_RANGE_LOW);

      if (wb > 100)
          wb = 100; // voltage is even biger than calibration top border*/

      b = (BYTE)(wb & 0x00ff);

      
      #if WITH_PENDING_BATTERY
      
      if(*(pendingBatteryCount()) < 10)
      {
        if(*(pendingBatteryCount()) == 0)
          *(pendingBatteryValue()) = b;
        else
            b = *(pendingBatteryValue());
        *(pendingBatteryCount()) = *(pendingBatteryCount()) + 1;
      }

      #endif
      
      return b;
}
#endif
#endif
#endif
