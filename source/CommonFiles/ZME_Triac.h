#ifndef _TRIAC
#define _TRIAC

#include <ZW030x.h>
#include <ZW_typedefs.h>
#include <ZW_pindefs.h>
#include "_EEPROM.h"

void TRIAC_Init(void);
void TRIAC_Set(BYTE level);



#define TRIAC_PULSE_LONG		FALSE
#define TRIAC_PULSE_SHORT		TRUE
#define TRIAC_OUSIDE_PULSE_MIN	5
#define TRIAC_OUSIDE_PULSE_MAX	60
#define TRIAC_PULSE_WIDTH_MIN	3
#define TRIAC_PULSE_WIDTH_MAX	20

#endif /* _TRIAC */
