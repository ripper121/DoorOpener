#ifndef _BUTTONS_H_
#define _BUTTONS_H_

#if NUM_BUTTONS > 0

#define DEBOUNCE_TIME			2
#define BUTTON_HOLD_STATE_TIME		250 // 2.5 sec

#define BUTTONS_CONFIG_NO		0
#define BUTTONS_CONFIG_YES		1
#define BUTTONS_CONFIG_CONFIGURABLE	2

extern BYTE buttonState[NUM_BUTTONS];
extern BYTE buttonEvent[NUM_BUTTONS];
#if BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE || BUTTONS_WITH_HOLDS == BUTTONS_CONFIG_CONFIGURABLE || BUTTONS_WITH_LONG_HOLDS == BUTTONS_CONFIG_CONFIGURABLE
//extern BYTE buttonWaitDoubl...[NUM_BUTTONS];
#endif

// Some checks
#if BUTTONS_WITH_TRIPPLE_CLICKS != BUTTONS_CONFIG_NO && BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_NO
	#error You must enable BUTTONS_WITH_DOUBLE_CLICKS to have BUTTONS_WITH_TRIPPLE_CLICKS functionality
#endif
#if BUTTONS_WITH_LONG_HOLDS != BUTTONS_CONFIG_NO && BUTTONS_WITH_HOLDS == BUTTONS_CONFIG_NO
	#error You must enable BUTTONS_WITH_HOLDS to have BUTTONS_WITH_LONG_HOLDS functionality
#endif
	
// Button states
enum  {    
	// idle
	BUTTON_IDLE = 0, // button was released a long ago

	// first press
	BUTTON_DEBOUNCE_WAIT, // pressed less than debounce time
	BUTTON_PRESS_WAIT, // pressed more than debounce time, but less than hold time
	
	// holds
	#if BUTTONS_WITH_HOLDS == BUTTONS_CONFIG_YES || BUTTONS_WITH_HOLDS == BUTTONS_CONFIG_CONFIGURABLE
	BUTTON_HOLDING, // holding
	BUTTON_CLICK_WAIT, // waiting for second press after a click
	#endif

	// second press
	#if BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_YES || BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
	BUTTON_CLICK_DEBOUNCE_WAIT, // pressed less than debounce time after previous click
	BUTTON_CLICK_PRESS_WAIT, // pressed more than debounce time, but less than hold time after previous click

	// click holds
	#if BUTTONS_WITH_HOLDS == BUTTONS_CONFIG_YES || BUTTONS_WITH_HOLDS == BUTTONS_CONFIG_CONFIGURABLE
	BUTTON_CLICK_HOLDING, // click-holding
	BUTTON_CLICK_CLICK_WAIT, // waiting for third press after a second click
	#endif

	// third click
	#if BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_YES || BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
	BUTTON_DOUBLE_CLICK_DEBOUNCE_WAIT, // pressed less than debounce time after previous click
	BUTTON_DOUBLE_CLICK_PRESS_WAIT, // pressed more than debounce time after previous click
	#endif
	#endif

	// long hold states
	#if BUTTONS_WITH_LONG_HOLDS == BUTTONS_CONFIG_YES
	BUTTON_HOLDING_2, // holding for more than 2.5 seconds
	BUTTON_HOLDING_5, // holding for more than 5.0 seconds  
	#endif
};

// Button events
enum  {    
	E_BUTTON_NO_EVENT = 0,

	E_BUTTON_CLICK,
	
	#if BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_YES || BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
	E_BUTTON_DOUBLE_CLICK,
	#if BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_YES || BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
	E_BUTTON_TRIPPLE_CLICK,
	#endif
	#endif
	
	#if BUTTONS_WITH_HOLDS == BUTTONS_CONFIG_YES || BUTTONS_WITH_HOLDS == BUTTONS_CONFIG_CONFIGURABLE
	E_BUTTON_HOLD,
	E_BUTTON_HOLD_RELEASE,
	
	#if BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_YES || BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
	E_BUTTON_CLICK_HOLD,
	E_BUTTON_CLICK_HOLD_RELEASE,
	#endif
	#endif
	
	#if BUTTONS_WITH_LONG_HOLDS == BUTTONS_CONFIG_YES
	E_BUTTON_HOLD_2,
	E_BUTTON_HOLD_5,
	#endif
};

void ButtonsInit(void);
void ButtonsPoll(void);
void ClearButtonEvents(void);
#if !PRODUCT_LISTENING_TYPE  || PRODUCT_DYNAMIC_TYPE
BOOL ButtonsAreIdle(void);
#endif

#endif /* NUM_BUTTONS > 0 */

#endif /* _BUTTONS_H_ */