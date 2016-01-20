
#ifndef I2C_LIB
#define I2C_LIB

#if !defined(SDATA) || !defined(SCLK)
//sbit P1_0 = P1^0;
//sbit P1_1 = P1^1;

#if defined(ZW050x)

#define SDATAPort          P3
#define SDATASHADOW        P3Shadow
#define SDATADIR           P3DIR
#define SDATASHADOWDIR     P3ShadowDIR
#define	SDATADIR_PAGE      P3DIR_PAGE
#define SDATA              5

#define SCLKPort          	P3
#define SCLKSHADOW        	P3Shadow
#define SCLKDIR           	P3DIR
#define SCLKSHADOWDIR     	P3ShadowDIR
#define	SCLKDIR_PAGE      	P3DIR_PAGE
#define SCLK            	4

#else

#define 	SDATA 		RXDpin
#define		SCLK		TXDpin
#define 	SDATADDR 	P1DIR
#define 	SDATAPort 	P1
#define 	SDATAPULL 	P1PULLUP
#define		SCLKDDR		P1DIR
#define		SCLKPULL	P1PULLUP
#define 	SCLKPort 	P1

#endif // defined(ZW050x)

#warning Notice: Use default I2C pins SCL=TXDpin RXD=SDA. Macroses SDATA & SCLK is not defined
#endif

#define START_WRITE(A) A << 1 
#define START_READ(A) A << 1 | 1 


// i2c common interface functions

//i2c_init is used in ZME_i2c.c.
void i2c_init(void);

//i2c_start is called before communication
void i2c_start(void);

//i2c_stop is called to end the communication
void i2c_stop(void);

//writes the output_data to i2c interface. returns the ACK
BYTE i2c_write(BYTE output_data);

//returns the value on i2c bus. you may pass the ack to the device.
BYTE i2c_read(BYTE ack);

// Отладка устройства через i2c
#if defined(I2C_DEBUG_INTERFACE)
#if  !defined(I2C_DEBUGGER_ADDR)
#define I2C_DEBUGGER_ADDR 	0xDE
#endif

#warning "I2C debugging is on!"

#define I2C_DEBUG_TYPE_BYTE 0x00
#define I2C_DEBUG_TYPE_WORD 0x01
#define I2C_DEBUG_TYPE_DWORD 0x02

#define I2C_DEBUG_VIEW_CHAR 0x00
#define I2C_DEBUG_VIEW_HEX 	0x04
#define I2C_DEBUG_VIEW_DEC 	0x08

#define I2C_DEBUG_XTRACT_SIZE(C) ((C >> 4) & 0x0F)
#define I2C_DEBUG_PUT_SIZE(C) (C << 4)

#define I2C_DEBUG_DUMP(C, D) i2c_debug_dump(C, D)

void i2c_debug_dump(BYTE control, BYTE* dta);


#else

#define I2C_DEBUG_DUMP(C, D)

#endif

#define I2C_DEBUG_PRINT(M, S) I2C_DEBUG_DUMP(I2C_DEBUG_PUT_SIZE(S),(BYTE*)M)

#define I2C_DEBUG_DUMP_BYTE_HEX(B, S) I2C_DEBUG_DUMP(I2C_DEBUG_PUT_SIZE(S)|I2C_DEBUG_VIEW_HEX,&B)
#define I2C_DEBUG_DUMP_BYTE_DEC(B, S) I2C_DEBUG_DUMP(I2C_DEBUG_PUT_SIZE(S)|I2C_DEBUG_VIEW_DEC,&B)
#define I2C_DEBUG_DUMP_BYTE_CHR(B, S) I2C_DEBUG_DUMP(I2C_DEBUG_PUT_SIZE(S),&B)
#define I2C_DEBUG_DUMP_WORD_HEX(W, S) I2C_DEBUG_DUMP(I2C_DEBUG_PUT_SIZE(S*2)|I2C_DEBUG_VIEW_HEX|I2C_DEBUG_TYPE_WORD,(BYTE*)&W)
#define I2C_DEBUG_DUMP_WORD_DEC(W, S) I2C_DEBUG_DUMP(I2C_DEBUG_PUT_SIZE(S*2)|I2C_DEBUG_VIEW_DEC|I2C_DEBUG_TYPE_WORD,(BYTE*)&W)
#define I2C_DEBUG_DUMP_DWORD_HEX(W, S) I2C_DEBUG_DUMP(I2C_DEBUG_PUT_SIZE(S*4)|I2C_DEBUG_VIEW_HEX|I2C_DEBUG_TYPE_DWORD,(BYTE*)&W)
#define I2C_DEBUG_DUMP_DWORD_DEC(W, S) I2C_DEBUG_DUMP(I2C_DEBUG_PUT_SIZE(S*4)|I2C_DEBUG_VIEW_DEC|I2C_DEBUG_TYPE_DWORD,(BYTE*)&W)


// pcf8574 remote 8-bit i/o expander support

#if defined(WITH_I2CDEVICE_PCF8574)
void write8574OutputPort(BYTE portNumber, BYTE Value);
BYTE read8574InputPort(BYTE portNumber);
#endif


// pcf8583 RTC with calendar
#if defined(WITH_I2CDEVICE_PCF8583)


#define CELL_8583_STATUS 		0x00
#define CELL_8583_HUND_SECONDS 	0x01
#define CELL_8583_SECONDS 		0x01
#define CELL_8583_MINUTES 		0x03
#define CELL_8583_HOURS 		0x04
#define CELL_8583_YEAR_DATE 	0x05
#define CELL_8583_WEEKDAY_MONTH 0x06
#define CELL_8583_USER_DATA 	0x10

void	write8583MemoryCell(BYTE cellAddress, BYTE Value);
BYTE	read8583MemoryCell(BYTE cellAddress);
void	write8583Time(WORD year, 
					  BYTE month, 
					  BYTE day, 
					  BYTE hour, 
					  BYTE minute, 
					  BYTE second);
void	read8583Time(WORD * year, 
					 BYTE * month, 
					 BYTE * day, 
					 BYTE * hour, 
					 BYTE * minute, 
					 BYTE * second);

#endif

#if defined(WITH_I2CDEVICE_GY521)
/*
This function checks the connection (AD0 connected to GND)
	returns: TRUE - OK
			 FALSE - NO CONNECTION
			 */
BOOL checkGY521connection(void);

/*
This function initialize the GY 521
	Sets clock source to the X-Gyro reference(more accurate than default internal)
	returns: TRUE - OK
			 FALSE - FAILED
*/
void initGY521(void);


void writeGY521register(BYTE regAdress,BYTE value);
BYTE readGY521register(BYTE regAdress);

#define DEFAULT_CONNECTED_TO_GND_ADRESS		0X68
#define DEFAULT_CONNECTED_TO_VCC_ADRESS		0X69

#define GY_521_REGISTER_WHO_AM_I			0X75
#define GY_521_REGISTER_PWR_MGMT_1			0X6B
#define GY_521_REGISTER_GYRO_CONFIG			0X1B
#define GY_521_REGISTER_ACCEL_CONFIG		0X1C
#define GY_521_REGISTER_CONFIG 				0X1A 
#define GY_521_REGISTER_SMPRT_DIV			0X19

#define GY_521_RESET           				0x80
#define GY_521_CLOCK_INTERNAL_MASK			0X00
#define GY_521_CLOCK_XGYRO_MASK				0X01
#define GY_521_CLOCK_YGYRO_MASK				0X02
#define GY_521_CLOCK_ZGYRO_MASK				0X03
#define GY_521_SLEEP_DISABLED_MASK			0X00
#define GY_521_SLEEP_ENABLED_MASK			0X40
#define GY_521_TEMP_SENSOR_DISABLED_MASK	0X08
#define GY_521_TEMP_SENSOR_ENABLED_MASK		0X00

/* Full scale ranges. */
enum gyro_fsr_e {
    INV_FSR_250DPS = 0,
    INV_FSR_500DPS,
    INV_FSR_1000DPS,
    INV_FSR_2000DPS,
    NUM_GYRO_FSR
};

/* Full scale ranges. */
enum accel_fsr_e {
    INV_FSR_2G = 0,
    INV_FSR_4G,
    INV_FSR_8G,
    INV_FSR_16G,
    NUM_ACCEL_FSR
};

/* Filter configurations. */
enum lpf_e {
    INV_FILTER_256HZ_NOLPF2 = 0,
    INV_FILTER_188HZ,
    INV_FILTER_98HZ,
    INV_FILTER_42HZ,
    INV_FILTER_20HZ,
    INV_FILTER_10HZ,
    INV_FILTER_5HZ,
    INV_FILTER_2100HZ_NOLPF,
    NUM_FILTER
};
#endif



#endif // I2C_LIB


