#ifndef _LED_H_
#define _LED_H_

BOOL LEDIsOff(void);
void LEDTick(void);
void LEDNWIOff(void);
void LEDSet(BYTE mode);

enum LED_CODES {
	LED_NO,
	LED_DONE,
	LED_FAILED,
	LED_NWI,
	#ifdef CUSTOM_LED_CODES
	CUSTOM_LED_CODES
	#endif
};

enum LED_COLOR_CODES {
	LED_COLOR_CODES_ARRAY
};

#endif /* _LED_H_ */