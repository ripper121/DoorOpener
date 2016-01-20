/*
	Copyright Poltorak Serguei 2011

	Duwi dimmer uses rectified signal, having upper ZEROX state only on half sine.

	To have more symmetrical signal on both half sines we use the following trick:
	we do one pulse for the half period after zero cross and repeat the pulse after 10ms.
	This allows us to use dimmer with enductive loads making no DC.

	The alog is simple:
	1. detect zero cross
	2. wait 1.8ms (to power the dimmer PCB)
	3. wait 0-7ms (depending on light level)
	4. do wide pulse ending ~1.2ms before next zero cross
	5. do next pulse 10ms after first one
	6. mask zerox during both pulses
	7. go to 1
*/

#include "ZME_TRIAC.h"

#define BIT(b) (1 << b)
#define SETBIT(v, b) v |= BIT(b)
#define CLRBIT(v, b) v &= ~BIT(b)
#define ISSETBIT(v, b) (v & BIT(b))
    
#define TRICON_PULSES_SHIFT 5
#define TRICON_INITMASK     4
#define TRICON_FULLBRI      3
#define TRICON_ZDET         2
#define TRICON_EN           1
#define TRICON_ACCEPT       0

sfr TRI0 = 0xbd;
sfr TRI1 = 0xbc;
sfr TRI2 = 0xbb;
sfr TRI3 = 0xff;
sfr TRICON = 0xa9;
sfr TRICOR = 0xba;

extern BYTE eeParams[];

#define tBeforePulse		eeParams[EEOFFSET_T_BEFORE_PULSE]
#define tAfterPulse			eeParams[EEOFFSET_T_AFTER_PULSE]
#define tPulseWidth			eeParams[EEOFFSET_T_PULSE_WIDTH]
#define pulseType			eeParams[EEOFFSET_PULSE_TYPE]

void TRIAC_Init(void) {
	TRIAC_Set(0);

	TRI3 = 156;
	TRICOR = 0;
	TRICON = 1 << TRICON_PULSES_SHIFT;
	SETBIT(TRICON, TRICON_INITMASK);
	SETBIT(TRICON, TRICON_FULLBRI);
	SETBIT(TRICON, TRICON_ACCEPT);
}

// Set TRIAC to level (0-99)
void TRIAC_Set(BYTE level) {
	if (level == 0) {
		CLRBIT(TRICON, TRICON_EN);
		PIN_OUT(TRIACpin);
		PIN_OFF(TRIACpin);
	} else {
		if (!ISSETBIT(TRICON, TRICON_ACCEPT)) { // check that register are released
			TRI0  = (BYTE) ((WORD) (156 - tAfterPulse - tBeforePulse) * (WORD) (99 - level) / (WORD) 99) + tBeforePulse; // 28..128
			if (pulseType == TRIAC_PULSE_LONG)
				TRI1 = 156 - tAfterPulse + tPulseWidth - TRI0; // pulse width = 10..110
			else
				TRI1 = tPulseWidth; // exact pusle width
			TRI2 = TRI1 + 4; // mask a little bit more (256 us) than pulse width
		
			SETBIT(TRICON, TRICON_EN);
			SETBIT(TRICON, TRICON_ACCEPT);
		}
	}
}
