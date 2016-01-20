#include "ZME_Includes.h"

static BYTE ledPermMode = LED_NO;
static BYTE ledTempMode = LED_NO;
static BYTE ledTick = 0;

code BYTE ledCodes[][1 + LED_TICK_RATE] = {
	BASIC_LED_CODES_ARRAY
	// custom codes below
	#ifdef CUSTOM_LED_CODES_ARRAY
	CUSTOM_LED_CODES_ARRAY
	#endif
};

BOOL LEDIsOff(void) {
	return (ledTempMode == LED_NO) && (ledPermMode == LED_NO);
}

void LEDNWIOff(void) {
	if (ledPermMode == LED_NWI) {
		LEDSet(LED_NO);
	}
}

void LEDSet(BYTE mode) {
	if (ledCodes[mode][0] != 0) {
		ledTick = ledCodes[mode][0];
		ledTempMode = mode;
	} else {
		ledPermMode = mode;
	}
}

// called every 100 ms
void LEDTick(void) {
	if (ledTick == 0) {
		ledTempMode = LED_NO;
		ledTick = LED_TICK_RATE;
	}
	LED_HW_SET_COLOR(ledCodes[(ledTempMode == LED_NO) ? ledPermMode : ledTempMode][((--ledTick) % LED_TICK_RATE) + 1]);
}
