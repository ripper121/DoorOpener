
#include <string.h>
#include "ZME_Includes.h"
#include "ZME_I2C.h"

#include <intrins.h>

#if WITH_I2C_INTERFACE

#define LONG_SYNC_WAIT 		4
#define NORMAL_SYNC_WAIT 	2
#define SHORT_SYNC_WAIT 	1

BYTE nopper=0;

#define NOP_DELAY			nopper++;//_nop_ ()

#define DELAY_5US 	wait_sync(5);//{int iind; for(iind=0;iind<20;iind++) nopper++;}//delay_1us();//delay_1us();//DELAY_2US;DELAY_3US
#define DELAY_4US 	wait_sync(4);//{int iind; for(iind=0;iind<20;iind++) nopper++;}//delay_1us();//delay_1us();//DELAY_2US;DELAY_3US//DELAY_2US;DELAY_2US
#define DELAY_3US 	wait_sync(3);//{int iind; for(iind=0;iind<20;iind++) nopper++;}//delay_1us();//delay_1us();//DELAY_2US;DELAY_3US//DELAY_1US;DELAY_1US;DELAY_1US
#define DELAY_2US 	wait_sync(2);//{int iind; for(iind=0;iind<20;iind++) nopper++;}//delay_1us(); //delay_1us();//DELAY_2US;DELAY_3US//DELAY_1US;DELAY_1US
#define DELAY_1US 	wait_sync(1);//{int iind; for(iind=0;iind<20;iind++) nopper++;}//delay_1us();//{int iind; for(iind=0;iind<20;iind++) NOP_DELAY;}
						//NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;\ 
					//NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY; \
					//NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY; \
					//NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;

//delay_1us();//NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;

 
 

//------------------------------------------------------------------------------
// I2C Functions - Bit Banged
//------------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
// 	Routine:	wait_sync
//	Inputs:		counter value to stop delaying
//	Outputs:	none
//	Purpose:	To pause execution for pre-determined time
////////////////////////////////////////////////////////////////////////////////
void wait_sync(unsigned int time_end)
{
	unsigned int index;
	for (index = 0; index < time_end; index++);
}

/*
void delay_1us()
 {
 	//nopper = 40;
 	//nopper--;
 	//nopper--;
 	//nopper--;
 	
 	//wait_sync(2);
 	//NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;
 	//NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;
 	//NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;
 	//NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;
 	
 	NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;
 	NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;
 	NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;
 	NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY

 	NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;
 	NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;
 	NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;
 	NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;NOP_DELAY;
 	
 	
 }*/
// ------------------------------------------------------------------------------
//  Метод:		i2c_init
//	Описание:	Инициализация обмена данными с шиной i2c
// ------------------------------------------------------------------------------
void i2c_init(void)
{
	// Переводит пин, на котором SCLK всегда на вывод
	// т.к. мы - мастер 
	PIN_OUT(SCLK);
	// В начале SDATA тоже используется на вывод
	PIN_OUT(SDATA);

	// ----
	/*
	PIN_OFF(SCLK);
	PIN_ON(SDATA);
	wait_sync(3200);
	PIN_ON(SCLK);
	PIN_OFF(SDATA);
	wait_sync(3200);
	PIN_OFF(SCLK);
	PIN_ON(SDATA);
	wait_sync(3200);*/
	// ---
	
	// Выводим оба пина на логическую единицу - шина свободна
	PIN_ON(SCLK);
	PIN_ON(SDATA);
	
	
}

// ------------------------------------------------------------------------------
//  Метод:		i2c_start
//	Описание:	Выдает начальное условие для передачи даннын по шине i2c
// ------------------------------------------------------------------------------

void i2c_start (void)
{
	// В начале SDATA используется на вывод
	PIN_OUT(SDATA);
	
	
	// Выводим оба пина на логическую единицу - шина свободна
	PIN_ON(SCLK);
	PIN_ON(SDATA);
	wait_sync(SHORT_SYNC_WAIT);
	
	
	PIN_ON(SCLK);
	
	// Создаем условие начала передачи
	// При sclk, установленном в единицу меняем SDATA c 1 на 0
	PIN_OFF(SDATA);
	DELAY_5US;
	
	
	
}

// ------------------------------------------------------------------------------
//  Метод:		i2c_start
//	Описание:	 Выдает на шину условие для завершения передачи даннын по шине i2c
// ------------------------------------------------------------------------------
void i2c_stop (void)
{
	
	PIN_OFF(SCLK);
	DELAY_1US
	// SDATA используется на вывод
	PIN_OUT(SDATA);
	
	//  SDATA должна перейти из 0 в единицу - начальное состояние 0
	PIN_OFF(SDATA);
	DELAY_2US;
	//wait_sync(SHORT_SYNC_WAIT);
	// SCLK должен выдавать логическую единицу
	PIN_ON(SCLK);
	DELAY_2US;
	//wait_sync(SHORT_SYNC_WAIT);
	// 	Переход на уровень 1
	PIN_ON(SDATA);
	DELAY_5US;
	// Чтобы было совсем корректно - переводим SDATA на режим ввода
	// логическуе единицу на нем буду поддерживать pull-up резисторы
	PIN_IN(SDATA, 1);
	DELAY_5US;
	DELAY_5US;

	

	//wait_sync(NORMAL_SYNC_WAIT);
	
	
}

void i2c_restart (void)
{
	/*
	PIN_OFF(SCLK);
	// SDATA используется на вывод
	PIN_OUT(SDATA);
	PIN_ON(SDATA);
	//wait_sync(SHORT_SYNC_WAIT);
	//  SDATA должна перейти из 0 в единицу - начальное состояние 0
	
	PIN_ON(SCLK); 
	*/
	i2c_stop();
	//wait_sync(SHORT_SYNC_WAIT);
	i2c_start();
	
	
}

// ------------------------------------------------------------------------------
//  Метод:				i2c_write
//  Входные параметры:	output_data - байт, который пишется на шину i2c
//	Выход:				1-был ACK, 0-ACK не было
//	Описание:			Пишет данные на шину i2c
// ------------------------------------------------------------------------------
BYTE i2c_write (BYTE output_data)
{
	BYTE i;
	BYTE ack;

	
	for (i = 0; i < 8; i++, output_data <<= 1)
    {
    	// Изменение данных только при 0 на SCLK
		PIN_OFF(SCLK);
		if(i==0){
			// SDATA используется на вывод
			PIN_OUT(SDATA);	
		}
		// Изменение на шине SDATA
		
        if(output_data & 0x80)
			PIN_ON(SDATA) 
		else 
			PIN_OFF(SDATA);
        // Подождем, чтобы это заметили
        DELAY_4US;
        //wait_sync(NORMAL_SYNC_WAIT);
        PIN_ON(SCLK); 
        DELAY_5US;
        //wait_sync(NORMAL_SYNC_WAIT);
    }

/*
	// Bit1
	PIN_OFF(SCLK);
	PIN_OUT(SDATA);	
    (output_data & 0x80) ?  PIN_ON(SDATA) : PIN_OFF(SDATA);
    DELAY_3US
    PIN_ON(SCLK); 
    DELAY_5US
    
    // Bit2
	PIN_OFF(SCLK);
	(output_data & 0x40) ?  PIN_ON(SDATA) : PIN_OFF(SDATA);
    DELAY_4US
    PIN_ON(SCLK); 
    DELAY_5US
    
    // Bit3
	PIN_OFF(SCLK);
	(output_data & 0x20) ?  PIN_ON(SDATA) : PIN_OFF(SDATA);
    DELAY_4US
    PIN_ON(SCLK); 
    DELAY_5US
    
    // Bit4
	PIN_OFF(SCLK);
	(output_data & 0x10) ?  PIN_ON(SDATA) : PIN_OFF(SDATA);
    DELAY_4US
    PIN_ON(SCLK); 
    DELAY_5US
    
    // Bit5
	PIN_OFF(SCLK);
	(output_data & 0x08) ?  PIN_ON(SDATA) : PIN_OFF(SDATA);
    DELAY_4US
    PIN_ON(SCLK); 
    DELAY_5US
	
	// Bit6
	PIN_OFF(SCLK);
	(output_data & 0x04) ?  PIN_ON(SDATA) : PIN_OFF(SDATA);
    DELAY_4US
    PIN_ON(SCLK); 
    DELAY_5US
    
    // Bit7
	PIN_OFF(SCLK);
	(output_data & 0x02) ?  PIN_ON(SDATA) : PIN_OFF(SDATA);
    DELAY_4US
    PIN_ON(SCLK); 
    DELAY_5US
	
	// Bit8
	PIN_OFF(SCLK);
	(output_data & 0x01) ?  PIN_ON(SDATA) : PIN_OFF(SDATA);
    DELAY_4US
    PIN_ON(SCLK); 
    DELAY_5US
	*/
	//PIN_ON(SDATA);
	// Теперь ждем ACK от принимающей стороны
	
	// Говорим принимающей стороне о начале передачи
	PIN_OFF(SCLK);   
	// SDATA на ввод
	PIN_IN(SDATA, 1);  		        		
	DELAY_5US;
	PIN_ON(SCLK);	
	ack = !(PIN_GET(SDATA)>0); // ACK = 0
	
	DELAY_4US;
	//wait_sync(NORMAL_SYNC_WAIT);
	
	return ack;
			
}

// ------------------------------------------------------------------------------
//  Метод:				i2c_read
//  Входные параметры:	ack = 1 если необходимо отправить ACK отправителю,
//						ack = 0 в противном случае
//	Выход:				Байт, полученный с шины
//	Описание:			Читает данные с шины i2c
// ------------------------------------------------------------------------------
BYTE i2c_read(BYTE ack)
{
	BYTE input_data=0, d,  i;
	

	//char i;
	
	
    for (i = 0; i < 8; i++)
    {
    	// Мы готовы читать - говорим об этом slav'у
		PIN_OFF(SCLK);
		if(i==0){
			//PIN_ON(SDATA);
			// SDATA используется на ввод
			PIN_IN(SDATA, 1);
		}	
		//wait_sync(NORMAL_SYNC_WAIT);
        DELAY_4US;
        // Завершаем синхроимпульс
        PIN_ON(SCLK);
         
        d =  (PIN_GET(SDATA) > 0); // Нам нужно либо 0, либо 1
        input_data = input_data | (d << (7-i));
        DELAY_4US;
        //wait_sync(NORMAL_SYNC_WAIT);
        
    }
	
	/*
	
	// bit1
	PIN_OFF(SCLK);
	PIN_ON(SDATA);
	PIN_IN(SDATA, 1);
	DELAY_3US
	PIN_ON(SCLK);
	input_data |= ((PIN_GET(SDATA) > 0) << 7);
    DELAY_4US   
    
    // bit2
	PIN_OFF(SCLK);
	DELAY_5US
	PIN_ON(SCLK);
	input_data |= ((PIN_GET(SDATA) > 0) << 6);
    DELAY_4US   
        	
    // bit3
	PIN_OFF(SCLK);
	DELAY_5US
	PIN_ON(SCLK);
	input_data |= ((PIN_GET(SDATA) > 0) << 5);
    DELAY_4US   
    
    // bit4
	PIN_OFF(SCLK);
	DELAY_5US
	PIN_ON(SCLK);
	input_data |= ((PIN_GET(SDATA) > 0) << 4);
    DELAY_4US   
   
   	// bit5
	PIN_OFF(SCLK);
	DELAY_5US
	PIN_ON(SCLK);
	input_data |= ((PIN_GET(SDATA) > 0) << 3);
    DELAY_4US
    
	// bit6
	PIN_OFF(SCLK);
	DELAY_5US
	PIN_ON(SCLK);
	input_data |= ((PIN_GET(SDATA) > 0) << 2);
    DELAY_4US  
    
    // bit7
	PIN_OFF(SCLK);
	DELAY_5US
	PIN_ON(SCLK);
	input_data |= ((PIN_GET(SDATA) > 0) << 1);
    DELAY_4US   
    
    // bit8
	PIN_OFF(SCLK);
	DELAY_5US
	PIN_ON(SCLK);
	input_data |= ((PIN_GET(SDATA) > 0) << 0);
    DELAY_4US;  
    */
        		
    
	// Пишем ACK
	PIN_OFF(SCLK);	
	PIN_OUT(SDATA);
	if(ack) 
		PIN_OFF(SDATA) 
	else 
		PIN_ON(SDATA);
	DELAY_5US
	//wait_sync(NORMAL_SYNC_WAIT);     
    // Завершаем синхроимпульс
	PIN_ON(SCLK); 
	DELAY_4US;
	
	//wait_sync(NORMAL_SYNC_WAIT);
	
    return input_data;

   
}


// pcf8574 remote 8-bit i/o expander support

#if defined(WITH_I2CDEVICE_PCF8574)

void write8574OutputPort(BYTE portNumber, BYTE Value)
{
	i2c_start();                  			
   	i2c_write(0x40 | (portNumber << 1) );
   	i2c_write(Value);
   	i2c_stop();                   			

   	 
}
BYTE read8574InputPort(BYTE portNumber)
{
	BYTE data_in;

	i2c_start();                  			
   	i2c_write(0x40 | (portNumber << 1) | 0x01 );
   	data_in = i2c_read(1);					
   	i2c_stop(); 
   	
   	return data_in;

}
#endif


#if defined(I2C_DEBUG_INTERFACE)
//#define I2C_DEBUGGER_ADDR 	0xDE
/*
#include <stdio.h>

#include <stdarg.h>
*/
/*
void i2c_debug_print(char * message, ...)
{
	BYTE destbuff[32];
	BYTE * pmessage;
	
	va_list argptr;
    va_start(argptr, message);
    vsprintf(destbuff, message, argptr);
    va_end(argptr);
   

   	i2c_start();
	i2c_write(I2C_DEBUGGER_ADDR);
	for(pmessage = destbuff;pmessage != '\0' ; pmessage++)
		i2c_write(*pmessage);
	i2c_stop(); 	
}*/
void i2c_debug_dump(BYTE control, BYTE* dta)
{
	BYTE i;

   	i2c_start();
	i2c_write(I2C_DEBUGGER_ADDR);
	i2c_write(control);
	for(i = 0; i < I2C_DEBUG_XTRACT_SIZE(control) ; i++)
		i2c_write(dta[i]);
	i2c_stop(); 	
}

#endif


// pcf8583 RTC with calendar
#if defined(WITH_I2CDEVICE_PCF8583)

BYTE bcd_to_byte(BYTE bcd){
  return ((bcd >> 4) * 10) + (bcd & 0x0f);
}

BYTE int_to_bcd(BYTE in){
  return ((in / 10) << 4) + (in % 10);
}


/*
#define BCD_TO_BYTE(B) ((BYTE)(((B >> 4) * 10) + (B & 0x0f)))
#define BYTE_TO_BCD(I) ((BYTE)( ((I / 10) << 4) + (I % 10)))
*/
#define BCD_TO_BYTE(B) bcd_to_byte(B)//((BYTE)(((B >> 4) * 10) + (B & 0x0f)))
#define BYTE_TO_BCD(I) int_to_bcd(I)//((BYTE)( ((I / 10) << 4) + (I % 10)))


void write8583Memory(BYTE cellAddress, BYTE Value)
{
	i2c_start();                  			
   	i2c_write(0xA0);
   	i2c_write(cellAddress);
   	i2c_write(Value);
   	i2c_stop();                   			
}
BYTE read8583Memory(BYTE cellAddress)
{
	BYTE data_in;

	i2c_start();                  			
   	i2c_write(0xA0);
   	i2c_write(cellAddress);
   	i2c_start();                  			
   	i2c_write(0xA1);
   	data_in = i2c_read(1);					
	i2c_stop();    
   
	return data_in;
}
void	read8583Time(WORD * year, 
					 BYTE * month, 
					 BYTE * day, 
					 BYTE * hour, 
					 BYTE * minute, 
					 BYTE * second)
{
	BYTE  year_cycle, year_base, year_date;
    
	i2c_start();                  			
   	i2c_write(0xA0);
   	i2c_write(0x02);
   	i2c_restart();                 			
   	i2c_write(0xA1);
   	
  	*second 		= BCD_TO_BYTE(i2c_read(1));
  	*minute 		= BCD_TO_BYTE(i2c_read(1));
  	*hour   		= BCD_TO_BYTE(i2c_read(1));
  	year_date		= i2c_read(1);
  	*month  		= BCD_TO_BYTE(i2c_read(0) & 0x1f); // Мы не используем день недели

  
  	*day    		= BCD_TO_BYTE(year_date & 0x3f);
  	year_cycle   	= (BYTE)((year_date >> 6) & 0x03);   // Стандартно RTC поддерживает только 4-х годовой цикл  	
	i2c_stop();
	
	
	i2c_start();                  			
   	i2c_write(0xA0);
   	i2c_write(0x10); // Пользовательские данные - там храним год
   	i2c_restart();                  			
   	i2c_write(0xA1);
   	year_base = i2c_read(0);
   	i2c_stop();
   	
	
     *year = (year_base & 0x3F);
     
     if(((year_base >> 6)&0x03) != year_cycle) // поменялся год - нужно догонять
     {
     	*year++;
     	
     	// Обновляем год на RTC
     	i2c_start();                  			
   		i2c_write(0xA0);
   		i2c_write(0x10); // Пользовательские данные - там храним год
   		i2c_write((year_cycle << 6) | *year); 
   		i2c_stop(); 
     	
     }
     
  	 *year += 1970; // POSIX-стандарт
  	 //*year = year_cycle(); 
  
}

void	write8583Time(WORD year, 
					  BYTE month, 
					  BYTE day, 
					  BYTE hour, 
					  BYTE minute, 
					  BYTE second)
{


	BYTE  year_cycle = (BYTE)(year % 4);
	// Останавливаем счетчик перед записью времени
	i2c_start();                  			
   	i2c_write(0xA0);
   	i2c_write(0x00);
   	i2c_write(0xC0); 	
   	i2c_stop();  
   	
   	
   	i2c_start();                  			
   	i2c_write(0xA0);
   	i2c_write(0x02);
   	i2c_write(BYTE_TO_BCD(second));
   	i2c_write(BYTE_TO_BCD(minute));
   	i2c_write(BYTE_TO_BCD(hour));
   	i2c_write(((BYTE)year_cycle << 6) | BYTE_TO_BCD(day));
   	i2c_write(BYTE_TO_BCD(month)); 	 	
   	i2c_stop();  
   	
   	// Обновляем год на RTC
    i2c_start();                  			
   	i2c_write(0xA0);
   	i2c_write(0x10); // Пользовательские данные - там храним год
   	i2c_write((year_cycle << 6) | (year-1970)); // Пользовательские данные - там храним год
   	i2c_stop();  
   	
   	// Запускаем RTC
	i2c_start();                  			
   	i2c_write(0xA0);
   	i2c_write(0x00);
   	i2c_write(0x00); 	
   	i2c_stop();     
   	             			
   	
}
					  
#endif

#if defined(WITH_I2CDEVICE_GY521)
void writeGY521register(BYTE regAdress,BYTE value) {
	i2c_start();                  			
   	i2c_write(DEFAULT_CONNECTED_TO_GND_ADRESS);
   	i2c_write(regAdress);
   	i2c_write(value);
   	i2c_stop();  
}

BYTE readGY521register(BYTE regAdress) {
	BYTE regData;
	
	i2c_start();                  			
   	i2c_write(START_WRITE(DEFAULT_CONNECTED_TO_GND_ADRESS));
   	i2c_write(regAdress);
   	i2c_restart();                  			
   	i2c_write(START_READ(DEFAULT_CONNECTED_TO_GND_ADRESS));
   	regData = i2c_read(0);	
	i2c_stop();  

	return regData;
}

/*
This func tries to read value from WHO_I_AM_REGISTER.
Returns:	1 - success
			0 - failed
*/
BOOL checkGY521connection(void) {
	return (readGY521register(GY_521_REGISTER_WHO_AM_I) == 0x68);
}

/*
This func sets p all main MPU registers.
Returns:	1 - success
			0 - failed
*/
void initGY521(void){
	//Reset the device first
	writeGY521register(GY_521_REGISTER_PWR_MGMT_1,GY_521_RESET);
	//Wake up the chip
	writeGY521register(GY_521_REGISTER_PWR_MGMT_1,GY_521_SLEEP_DISABLED_MASK);
	//Setting the gyro full scale range
	writeGY521register(GY_521_REGISTER_GYRO_CONFIG,(INV_FSR_2000DPS << 3)); 
	//Setting the accel full scale range
	writeGY521register(GY_521_REGISTER_ACCEL_CONFIG,(INV_FSR_2G << 3));
	//Setting the digital low pass filter
	writeGY521register(GY_521_REGISTER_CONFIG,INV_FILTER_42HZ);
	//Setting the sample rate
	writeGY521register(GY_521_REGISTER_SMPRT_DIV,19); //I don't know why 19 (just took from the library)

	//Leave FIFO and interrupts unconfigured, cause by default they are disabled.

#warning "initGY521 not finished"
	//TODO!! 
	/*
    if (mpu_set_bypass(0))
        return -1;

    mpu_set_sensors(0);
    return 0;
    */
}
#endif



#endif // WITH_I2C_INTERFACE
