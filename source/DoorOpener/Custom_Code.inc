
void LockTheDoor(BYTE mode);
void BasicAction(BYTE value);
BYTE BasicGet(void);

BYTE relayState = 2;
BOOL relayVib = FALSE;
WORD relayVibTime = 0;
BOOL canGoSleep = TRUE;
#define RELAY_SET 1
#define RELAY_RESET 2
	
// It might be inlined in Common.h if ButtonPressed is defined there
#if !defined(ButtonPressed)
BOOL ButtonPressed(BYTE buttonIndex) {
	switch (buttonIndex) {
		case 0:
			return !PIN_GET(Button0pin);
		case 1:
			return !PIN_GET(Button1pin);
		default:
			return FALSE;
	}
}
#endif

//Main Loop

static void ApplicationPollFunction(void) {

	//if idel+button then start learnmode	
	switch(mode) {
		case MODE_IDLE:
			switch(buttonEvent[0]) {
				case E_BUTTON_CLICK:
				//do stuff on button click
				LEARN_MODE_START();
				break;
			}
			break;	

		default:
			if ((buttonEvent[0]!=E_BUTTON_NO_EVENT)) {
				LEARN_MODE_STOP();
			}
			break;

	}
	
	switch(buttonEvent[1]) {
		case E_BUTTON_CLICK:			
			PIN_ON(RedLEDpin);
		break;
	
	}
	
}





/*
static void ApplicationPollFunction(void) {
	switch (mode) {
		case MODE_IDLE:
		switch (buttonEvent[0]) {
			case E_BUTTON_CLICK:
				PIN_TOGGLE(RedLEDpin);
			break;

			case E_BUTTON_DOUBLE_CLICK:
				LEARN_MODE_START();
			break;

			case E_BUTTON_TRIPPLE_CLICK:
				createDoorLockLoggingReport(DOOR_LOCK_LOGGING_EVENT_Z_WAVE_NO_CODE_LOCK);
			break;
		}
		break;

		default:
			if ((buttonEvent[0]!=E_BUTTON_NO_EVENT)) {
				LEARN_MODE_STOP();
			}
		break;
	}
	
			case MODE_LEARN:
			switch(buttonEvent[0]) {
				case E_BUTTON_CLICK:
				//do stuff on button click
				LEARN_MODE_STOP();
				break;
			}
			break;
}*/


//SEND Val back to controller
BYTE BasicGet(void) {
	return 0;
}

void relayOn(void){			
		PIN_ON(Relay0pin);	
}

void relayOff(void){	
		PIN_ON(Relay1pin);	
}

void relaySet(void){
	PIN_OFF(Relay0pin);		
	PIN_OFF(Relay1pin);
	if(relayState==RELAY_SET){
	relayOn();
	}
	if(relayState==RELAY_RESET){
	relayOff();
	}
	relayState=0;		
}


void Tick(void) {
	if(relayVib){
		if(relayVibTime<(VAR(RELAY_VIBRATION_TIMEOUT)*20)){
			if(relayVibTime	%2 == 0){
				relayState=RELAY_SET;
			}else{
				relayState=RELAY_RESET;
			}			
			relayVibTime++;				
			PIN_TOGGLE(RedLEDpin);
		}
		else{			
			relayVibTime=0;
			relayVib=FALSE;
			relayState=RELAY_RESET;
			canGoSleep = TRUE;			
		}
	}
	relaySet();
}


void LockTheDoor(BYTE mode) {	
	switch(mode) {
		case DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED:
		case DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED_FOR_INSIDE_DOOR_HANDLES:		
		case DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED_FOR_OUTSIDE_DOOR_HANDLES_WITH_TIMEOUT:
		case DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED_WITH_TIMEOUT:
			canGoSleep = FALSE;
			if(VAR(RELAY_VIBRATION)==TRUE)
				relayVib=TRUE;	
			else
				relayState=RELAY_SET;	
		break;
		case DOOR_LOCK_OPERATION_SET_DOOR_SECURED: //TIMEOUT Operation
			relayState=RELAY_RESET;	
		break;
		default:
		break;
	}
}

unsigned char groupName(unsigned char g, unsigned char * name)
{
	BYTE gname_lifeline[] = "Life line"; // last symbol is null terminated - included in sizeof
	
	memcpy(name, gname_lifeline, sizeof(gname_lifeline) - 1);
	return sizeof(gname_lifeline) - 1;
}

BYTE groupCommandCount(BYTE g)
{
	return 2;

}

DWORD groupCommand(BYTE g, BYTE c)
{
	switch(c)
	{
		case 0:
			return ((DWORD)COMMAND_CLASS_BATTERY<<8)|BATTERY_REPORT;
		case 1:
			return ((DWORD)COMMAND_CLASS_DEVICE_RESET_LOCALLY<<8)|DEVICE_RESET_LOCALLY_NOTIFICATION;

	}
}
