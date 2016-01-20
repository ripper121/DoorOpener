#ifndef _EUROTRONICS_STELLAF_UART_
#define _EUROTRONICS_STELLAF_UART_

// UART defines

sfr UARTCON = 0xD8;
sfr UARTRX = 0xBF;
sfr UARTTX = 0xCB;

#define UART_RX_STATUS		(UARTCON & 0x01)
#define UART_TX_STATUS		(UARTCON & 0x02)
#define UART_CLEAR_RX		UARTCON &= 0xFD
#define UART_CLEAR_TX		UARTCON &= 0xFE
#define UART_ENABLE		UARTCON |= 0x80
#define UART_DISABLE		UARTCON &= 0x7F
#define UART_INIT		UARTCON = 0
#define UART_SET_9600		{ UARTCON = 0x00; UARTCON |= 0x9b; }
#define UART_SET_38400		{ UARTCON = 0x20; UARTCON |= 0x9b; }
#define UART_SET_115200		{ UARTCON = 0x40; UARTCON |= 0x9b; }

// StellaZ status
#define MOTOR_IDLE(status)		((status & 0x07) == 0)
#define MOTOR_ACTIVE(status)		((status & 0x07) == 1)
#define ERR_VALVE_NOT_MOUNTED(status)	((status & 0x07) == 2)
#define ERR_TO_SHORT_MOV_RANGE(status)	((status & 0x07) == 3)
#define ERR_CLOSE_NOT_DETECTED(status)	((status & 0x07) == 4)
#define WAITING_FOR_ADJ(status)		((status & 0x07) == 5)
#define BATTERY_EMPTY(status)		(status & (1 << 3))
#define KEY_PRESSED(status)		(status & (1 << 4))
#define WINDOW_OPEN_DETECTED(status)	(status & (1 << 6))

// Exported functions
void StellaZUARTOff(void);
void StellaZUARTInit(void);
void StellaZUARTBreak(void);
void StellaZUARTPoll(void);
void StellaZWakeUpMode3(void);
void StellaZReleaseChip(void);
/* not used void StellaZReleaseChipWithoutTempRegulation(void); */
BYTE StellaZGetStatus(void);
WORD StellaZGetValvePosition(WORD *positionMax);
void StellaZSetValvePosition(WORD position);
void StellaZSetHeatingTemperature(BYTE temp);
void StellaZSetSavingTemperature(BYTE temp);
void StellaZSetFreeTemperature(BYTE temp);
BYTE StellaZGetTemperature(void);
void StellaZSetOpenWindowDetection(BYTE tempDrop, BYTE timerMinutes);
void StellaZSetReadapTimer(WORD periodMinutes);
void StellaZSetInstallMode(void);
void StellaZLeaveInstallMode(void);
BYTE StellaZGetBatteryLevel(void);
#endif /* _EUROTRONICS_STELLAF_UART_ */