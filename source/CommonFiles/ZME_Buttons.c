#include "ZME_Includes.h"

BYTE buttonState[NUM_BUTTONS];
BYTE buttonEvent[NUM_BUTTONS];
static BYTE buttonTickCounter[NUM_BUTTONS];

void ButtonsPoll(void);

#if !defined(ButtonPressed)
 BOOL ButtonPressed(BYTE buttonIndex); 
#endif

//////////////////////////////////////////////////////

void ButtonsInit(void) {
	register BYTE i;

	BUTTONS_HW_INIT();
	for (i = 0; i < NUM_BUTTONS; i++) {
		buttonEvent[i] = E_BUTTON_NO_EVENT;
		buttonState[i] = BUTTON_IDLE;
		buttonTickCounter[i] = 0;
	}
}

#if !PRODUCT_LISTENING_TYPE  || PRODUCT_DYNAMIC_TYPE
BOOL ButtonsAreIdle(void) {
	register BYTE i;

	for (i = 0; i < NUM_BUTTONS; i++) {
		if (buttonState[i] != BUTTON_IDLE)
			return FALSE;
	}
	return TRUE;
}
#endif

void ClearButtonEvents(void) {
	register BYTE i;

	for (i = 0; i < NUM_BUTTONS; i++) {
		buttonEvent[i] = E_BUTTON_NO_EVENT;
	}
}

void ButtonsPoll(void) {
	register BYTE i;
	BOOL buttonPressed, tickFired;
	
	#if defined(BUTTONS_PRE_POLL_HANDLER)
		BUTTONS_PRE_POLL_HANDLER;
	#endif

	for (i = 0; i < NUM_BUTTONS; i++) {
		if (buttonTickCounter[i] > 0) 
			buttonTickCounter[i]--;

		buttonPressed = ButtonPressed(i);
        	tickFired = (buttonTickCounter[i] == 0); // to optimize the code

		switch (buttonState[i]) {
			case BUTTON_IDLE:
				if (buttonPressed) {
					buttonState[i]++; // BUTTON_DEBOUNCE_WAIT,
					buttonTickCounter[i] = DEBOUNCE_TIME;
				}
				break;

			case BUTTON_DEBOUNCE_WAIT:
			#if BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_YES || BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
			case BUTTON_CLICK_DEBOUNCE_WAIT:
			#endif
				if (buttonPressed) {
					if (tickFired) {
						buttonState[i]++; // BUTTON*_PRESS_WAIT;
                                                buttonTickCounter[i] = buttonClickTimeout;
					}
				} else {
				  #if BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_YES || BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
					if (buttonState[i] == BUTTON_CLICK_DEBOUNCE_WAIT) {
						buttonEvent[i] = E_BUTTON_CLICK;
					}
					#endif
					buttonState[i] = BUTTON_IDLE;
				}
				break;

			#if BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_YES || BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
			case BUTTON_DOUBLE_CLICK_DEBOUNCE_WAIT:
				if (buttonPressed) {
					if (tickFired) {
						buttonState[i]++; // BUTTON*_PRESS_WAIT;
                                                // no timer needed, since no holds // buttonTickCounter[i] = buttonClickTimeout;
                                                // immediatelly issue the tripple click event
						buttonEvent[i] = E_BUTTON_TRIPPLE_CLICK;
					}
				} else {
					buttonEvent[i] = E_BUTTON_DOUBLE_CLICK;
					buttonState[i] = BUTTON_IDLE;
				}
				break;
			#endif
		
			case BUTTON_PRESS_WAIT:
				if (buttonPressed) {
                                        #if BUTTONS_WITH_HOLDS == BUTTONS_CONFIG_YES || BUTTONS_WITH_HOLDS == BUTTONS_CONFIG_CONFIGURABLE
                                        #if BUTTONS_WITH_HOLDS == BUTTONS_CONFIG_CONFIGURABLE
                                        if (BUTTONS_HOLDS_ENABLED)
                                        #endif
                                                if (tickFired) {
                                                        buttonState[i]++; // BUTTON_HOLDING;
                                                        buttonTickCounter[i] = BUTTON_HOLD_STATE_TIME;
                                                        buttonEvent[i] = E_BUTTON_HOLD;
                                                }
                                        #endif
				} else {
					#if BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_YES || BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
					#if BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
					if (BUTTONS_DOUBLE_CLICKS_ENABLED) {
					#endif
						buttonState[i] = BUTTON_CLICK_WAIT;
						buttonTickCounter[i] = buttonClickTimeout;
					#if BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
					} else {
						buttonState[i] = BUTTON_IDLE;
						buttonEvent[i] = E_BUTTON_CLICK;
					}
					#endif
					#else
						buttonState[i] = BUTTON_IDLE;
						buttonEvent[i] = E_BUTTON_CLICK;
					#endif
				}
				break;

      #if BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_YES || BUTTONS_WITH_DOUBLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
			case BUTTON_CLICK_PRESS_WAIT:
				if (buttonPressed) {
                                        #if BUTTONS_WITH_HOLDS == BUTTONS_CONFIG_YES || BUTTONS_WITH_HOLDS == BUTTONS_CONFIG_CONFIGURABLE
                                        #if BUTTONS_WITH_HOLDS == BUTTONS_CONFIG_CONFIGURABLE
                                        if (BUTTONS_HOLDS_ENABLED)
                                        #endif
                                                if (tickFired) {
                                                        buttonState[i]++; // BUTTON_CLICK_HOLDING;
                                                        buttonEvent[i] = E_BUTTON_CLICK_HOLD;
                                                }
                                        #endif
				} else {
					#if BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_YES || BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
					#if BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
					if (BUTTONS_TRIPPLE_CLICKS_ENABLED) {
					#endif
						buttonState[i] = BUTTON_CLICK_CLICK_WAIT;
						buttonTickCounter[i] = buttonClickTimeout;
					#if BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
					} else {
						buttonState[i] = BUTTON_IDLE;
						buttonEvent[i] = E_BUTTON_DOUBLE_CLICK;
					}
					#endif
					#else
						buttonState[i] = BUTTON_IDLE;
						buttonEvent[i] = E_BUTTON_DOUBLE_CLICK;
					#endif
				}
				break;

				
      #if BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_YES || BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
			case BUTTON_DOUBLE_CLICK_PRESS_WAIT:
				if (!buttonPressed) {
					buttonState[i] = BUTTON_IDLE;
				}
				break;
      #endif
                        
			case BUTTON_CLICK_WAIT:
				if (tickFired) {
					buttonState[i] = BUTTON_IDLE;
					#if BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_YES || BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
					if (buttonState[i] == BUTTON_CLICK_CLICK_WAIT) {
					#endif
					        buttonEvent[i] = E_BUTTON_DOUBLE_CLICK;
                      #if BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_YES || BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
                      } else {
                      #endif
					        buttonEvent[i] = E_BUTTON_CLICK;
                                        #if BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_YES || BUTTONS_WITH_TRIPPLE_CLICKS == BUTTONS_CONFIG_CONFIGURABLE
                                        }
                                        #endif
				} else {
					if (buttonPressed) {
						buttonState[i]++; // BUTTON_*DEBOUNCE_WAIT
						buttonTickCounter[i] = DEBOUNCE_TIME;
					}
				}
				break;
		case BUTTON_CLICK_CLICK_WAIT:
				if (tickFired) {
					buttonState[i] = BUTTON_IDLE;
					buttonEvent[i] = E_BUTTON_DOUBLE_CLICK;
                    
				} else {
					if (buttonPressed) {
						buttonState[i]++; // BUTTON_*DEBOUNCE_WAIT
						buttonTickCounter[i] = DEBOUNCE_TIME;
					}
				}
				break;
	   #endif
       
     #if BUTTONS_WITH_HOLDS == BUTTONS_CONFIG_YES || BUTTONS_WITH_HOLDS == BUTTONS_CONFIG_CONFIGURABLE
			case BUTTON_HOLDING:
				if (buttonPressed) {
					if (tickFired) {
						buttonState[i] = BUTTON_HOLDING_2;
						buttonEvent[i] = E_BUTTON_HOLD_2;
						buttonTickCounter[i] = BUTTON_HOLD_STATE_TIME;
					}
				} else {
					buttonState[i] = BUTTON_IDLE;
					buttonEvent[i] = E_BUTTON_HOLD_RELEASE;
				}
				break;
		
      #if BUTTONS_WITH_LONG_HOLDS == BUTTONS_CONFIG_YES
			case BUTTON_HOLDING_2:
				if (buttonPressed) {
					if(tickFired) {
						buttonEvent[i] = E_BUTTON_HOLD_5;
						buttonState[i] = BUTTON_HOLDING_5;
					}
				} else {
					buttonState[i] = BUTTON_IDLE;
					buttonEvent[i] = E_BUTTON_HOLD_RELEASE;
				}
				break;
		
			case BUTTON_HOLDING_5:
				if (!buttonPressed) {
					buttonState[i] = BUTTON_IDLE;
					buttonEvent[i] = E_BUTTON_HOLD_RELEASE;
				}
				break;
      #endif

			case BUTTON_CLICK_HOLDING:
				if (!buttonPressed) {
					buttonEvent[i] = E_BUTTON_CLICK_HOLD_RELEASE;
					buttonState[i] = BUTTON_IDLE;
				}
				break;
      #endif
		}
	}
}
