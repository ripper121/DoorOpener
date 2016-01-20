#ifndef ZME_GLOBAL
#define ZME_GLOBAL

// For QueueUnitTest project
#ifndef keil_static
#define keil_static static
#endif
#ifndef assert
#define assert(x)
#endif

// Globally used macro commands
#if defined(ZW050x) && defined(BANKING)
	#define CBK_DEFINE(modif,type,func,params) 			\
									type func params; \
									code const type (code * func##_p) params = &func; \
									modif type func params

	#define CBK_POINTER(func)					func##_p
#else
	#define CBK_DEFINE(modif,type,func,params)			modif type func params
	
	#define CBK_POINTER(func)					func
#endif




#if defined(ZW030x)
//#warning ADC functions has changed
//  #define ZW_ADC_enable(s)                ADC_enable(s)
//  #define ZW_ADC_power_enable(e)          ADC_power_enable(e)
  #define ZW_ADC_pin_select(a)            ADC_SelectPin(a)
  #define ZW_ADC_threshold_set(t)         ZW_ADC_SET_THRESHOLD(t)
  #define ZW_ADC_int_enable(e)            ADC_Int(e)
  #define ZW_ADC_int_clear()              ADC_IntFlagClr()
  #define ZW_ADC_is_fired()               ZW_ADC_IS_FIRED()
  #define ZW_ADC_result_get()             ADC_GetRes()
  #define ZW_ADC_buffer_enable(e)         ADC_Buf(e)
//  #define ZW_ADC_auto_zero_set(a)         ADC_auto_zero_set(a)
  #define ZW_ADC_resolution_set(r)        ADC_SetResolution(r)
  #define ZW_ADC_threshold_mode_set(t)    ADC_SetThresMode(t)
//  #define ZW_ADC_batt_monitor_enable(e)   ADC_batt_monitor_enable(e)
  #define ZW_ADC_init(m, u, l, p)         ADC_init(m, u, l, p)
#endif

#define ZME_GET_BYTE(a)									ZW_MEM_GET_BYTE(a)
#define ZME_PUT_BYTE(a, v)							ZW_MEM_PUT_BYTE(a, v)
#define ZME_GET_BUFFER(a, v, s)					ZW_MEM_GET_BUFFER(a, v, s)
#define ZME_PUT_BUFFER(a, v, s, c)			ZW_MEM_PUT_BUFFER(a, v, s, c)
/*
// Helpers to debug EEPROM. Comment above and put this in ZME_Common.c: (may be should be moved into separate file like ZME_Debug.c?)
// This is to analyse this log:
// cat debug.kfob.txt | grep -En "(01 .. 00 04 00 E7 .. 20|\[ 20 .* \])" | awk 'function mtype(t) {if (t == "10") return "PUT BYTE"; if (t == "20") return "GET BYTE"; if (t == "11") return "PUT BUFFER"; if (t == "21") return "GET BUFFER";} /RECEIVED:/ { split($1, A, ":"); printf "%s\t \t%s\t%s%s\t", A[1], mtype($13), $14, $15; for (i = 0; i < NF - 18 + 1; i++) { printf "%s ", $(i+16)} printf "\n"} /CC Security/ {split($1, A, ":"); printf "%s\tS\t%s\t%s%s\t", A[1], mtype($15), $16, $17; for (i = 0; i < NF - 19 + 1; i++) { printf "%s ", $(i+18)} printf "\n"}' > debug.kfob.analyse.txt
BYTE ZME_GET_BYTE(WORD a) {
	BYTE aa[3];
	BYTE v = ZW_MEM_GET_BYTE(a);
	aa[0] = a >> 8;
	aa[1] = a;
	aa[2] = v;
	SendDataReportToNode(1, 0x20, 0x20, &aa, 3);
	return v;
}
BYTE ZME_PUT_BYTE(WORD a, BYTE v) {
	BYTE aa[3];
	aa[0] = a >> 8;
	aa[1] = a;
	aa[2] = v;
	ZW_MEM_PUT_BYTE(a, v);
	SendDataReportToNode(1, 0x20, 0x10, &aa, 3);
}

void ZME_GET_BUFFER(WORD a, BYTE * v, WORD s) {
	BYTE aa[40+2];
	BYTE i;
	aa[0] = a >> 8;
	aa[1] = a;
	ZW_MEM_GET_BUFFER(a, v, s);
	for (i = 0; i < s && i < 40; i++)
		aa[2+i] = v[i];
	SendDataReportToNode(1, 0x20, 0x21, &aa, (s>40) ? 42 : (2+s));
}

void ZME_PUT_BUFFER(WORD a, BYTE * v, WORD s, void * c) {
	BYTE aa[40+2];
	BYTE i;
	aa[0] = a >> 8;
	aa[1] = a;
	for (i = 0; i < s && i < 40; i++)
		aa[2+i] = v[i];
	SendDataReportToNode(1, 0x20, 0x11, &aa, (s>40) ? 42 : (2+s));
	ZW_MEM_PUT_BUFFER(a, v, s, c);
}
*/

#if WITH_CC_WAKE_UP && WITH_CC_BATTERY
// This is needed here to put this param in ZME_App_Version.c in code space
enum {
	ENABLE_WAKEUPS_NO = 0,
	ENABLE_WAKEUPS_YES
};
#endif

#endif // ZME_GLOBAL
