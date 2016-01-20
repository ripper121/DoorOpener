#include "ZME_Includes.h"

extern BOOL isStellaZReady;
extern BYTE stellaZUARTTimeoutTick;
extern BYTE stellaZUARTBreakTick;
#define STELLAZ_UART_TIMEOUT		5 // seconds
#define STELLAZ_UART_BREAK_TIME		2 // seconds

void RestartStellaZCommunications(void);
static void StellaZReboot(void);

static void SendByte(BYTE b) {
	UARTTX = b;
	stellaZUARTTimeoutTick = STELLAZ_UART_TIMEOUT;
	while (!UART_TX_STATUS) {
		ZW_POLL();
		if (stellaZUARTTimeoutTick == 0) {
			StellaZReboot();
		}
	}
	UART_CLEAR_TX;
}

static void SendWord(WORD w) {
	// Big endian, LSB comes first
	SendByte(*(((BYTE *)&w) + 1));
	SendByte(*(((BYTE *)&w)    ));
}

static BYTE ReadByte(void) {
	BYTE r;
	stellaZUARTTimeoutTick = STELLAZ_UART_TIMEOUT;
	while (TRUE) {
		if (UART_RX_STATUS) { // RX have data
			r = UARTRX;
			UART_CLEAR_RX;
			break;
		}
		ZW_POLL();
		if (stellaZUARTTimeoutTick == 0) {
			StellaZReboot();
		}
	}
	return r;
}

static WORD ReadWord(void) {
	WORD r;
	// Big endian, LSB comes first
	*(((BYTE *)&r) + 1) = ReadByte();
	*(((BYTE *)&r)    ) = ReadByte();
	return r;
}

void StellaZUARTInit(void) {
	// Init Serial
	PIN_IN(UART0_RXD, 1);
	PIN_OUT(UART0_TXD);
	UART_INIT;
	UART_SET_9600;
}

void StellaZUARTOff(void) {
	// Disable Serial
	UART_DISABLE;
}

void StellaZUARTBreak(void) {
	// Disable Serial and send a break
	UART_DISABLE;
	PIN_OUT(UART0_TXD);
	PIN_OFF(UART0_TXD);
}

void StellaZUARTPoll(void) {
	while (UART_RX_STATUS) { // RX have data
		(void)ReadByte(); // discard the packet
	}
	//case 0xFF: // LED On
	//case 0xFE: // LED Off
	//case 0xFD: // Syntax Error, unknown command
	//case 0xFC: // Timeout
}

void StellaZReboot(void) {
	LEDSet(LED_STELLAZ_ERROR);
	isStellaZReady = FALSE;
	stellaZUARTBreakTick = STELLAZ_UART_BREAK_TIME;
	StellaZUARTBreak();
}

void StellaZWakeUpMode3(void) {
	// Wakeup to Mode 3
	SendByte(0xFF);
	SendByte(0xFF);
	SendByte(0xFF);
}

void StellaZReleaseChip(void) {
	// Let StellaZ do it's job and go to sleep
	SendByte(0xF0);
	SendByte(0x04);
}

/* not used
void StellaZReleaseChipWithoutTempRegulation(void) {
	// Let StellaZ do it's job and go to sleep without temperature regulation
	SendByte(0xF0);
	SendByte(0x08);
}
*/

BYTE StellaZGetStatus(void) {
	SendByte(0xEF);
	return ReadByte();
}

BYTE StellaZGetBatteryLevel(void) {
	BYTE retVal;

	SendByte(0xEB);
	retVal = ReadByte();

	// returns -1, 0..120
	// 0xFF (-1) means that battery still has to be measured after power up. This may take a few minutes as the battery is only measured
	// while the motor is running with a defined load to get best reliable battery status. To gain battery life this is only done when
	// motor movement is neccessary for positioning.

	if (retVal != 0xFF) {
		retVal = (BYTE) ((((DWORD) retVal) * 214) >> 8); // calibrate 0..120 to 0..100 // (120 * 214) >> 8 = 0x64 = 100
	}
	return retVal;
}

WORD StellaZGetValvePosition(WORD *positionMax) {
	WORD posCur, posMax;

	SendByte(0xEC);
	posCur = ReadWord();
	posMax = ReadWord();
	if (positionMax != NULL)
		*positionMax = posMax;

	return posCur;
}

void StellaZSetValvePosition(WORD position) {
	SendByte(0xF4);
	SendWord(position);
}

void StellaZSetHeatingTemperature(BYTE temp) {
	// Heat
	SendByte(0xFA);
	SendByte(temp);
}

void StellaZSetSavingTemperature(BYTE temp) {
	// Energy Saving Heat
	SendByte(0xF9);
	SendByte(temp);
}

void StellaZSetFreeTemperature(BYTE temp) {
	// Free value
	SendByte(0xF8);
	SendByte(temp);
}

BYTE StellaZGetTemperature(void) {
	SendByte(0xEE);
	return ReadByte();
}

void StellaZSetOpenWindowDetection(BYTE tempDrop, BYTE timerMinutes) {
	SendByte(0xF3);
	SendByte(tempDrop);
	SendByte(timerMinutes);	
}

void StellaZSetReadapTimer(WORD periodMinutes) {
	SendByte(0xF2);
	SendWord(periodMinutes);
}

void StellaZSetInstallMode() {
	SendByte(0xF0);
	SendByte(0x48);
}

void StellaZLeaveInstallMode() {
	SendByte(0xF0);
	SendByte(0x05);
}

/* not used
WORD StellaZGetADCTemperature(void) {
	// read temperature from ADC
	SendByte(0xED);
	return ReadWord();
}
*/

/*
If memory footprint is 0x00 (power on of the device)
   Wait 5 seconds
   Read Status to wait for motor idle or other error code. Send it via Z-Wave to update the gateway.
   Disable automatic readjustment 0xF2 0xFE
ElseIf WUT wakeup (Periodical wakeup of the device by WUT (every 60 seconds)
   Check wakeup ticks to enable Z-Wave communications using Wakeup CC
   Wakeup the Valve using 0xFF 0xFF 0xFF
   If readjustment counter reached 7 days, run the process 0xF0 0x01
Else (wakeup or via Key Press)
   Wakeup the Valve using 0xFF 0xFF 0xFF
Set Temperature Heating (or other mode) 0xFA 0xTT
Set Open Window Detection 0xF3 0xTT 0xMM (depending on Configuration CC parameters)
If requested by Z-Wave (to send as SensorMultilevel), Read Temperature 0xEE
If requested by Z-Wave (to send as Configuration parameter for debug), Read Position 0xEC
Read Status to see if the button was pressed and handle it 0xEF (also report as Configuration parameter for debug the Open Window detection flag)
If special button sequence is used, go to position Open (to release and the Valve) 0xF0 0x40
If in Open position, special button sequence will run adjustment 0xF0 0x01
Set Status 0xF0 0x24 (Mode 3 sleep bit and block button)
Finish Z-Wave operations and go to sleep
 
During all the time:
Handle LED commands from the Valve 0xFF and 0xFE
Handle Error codes 0x02-0x05 from the Valve and send as Configuration Parameter for debug
*/