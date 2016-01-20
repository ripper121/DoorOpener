#ifdef WITH_AES_CRYPTO

#include <ZW_typedefs.h>
#include <ZW_basis_api.h>
#include <ZW_aes_api.h>
#include <ZW_firmware_bootloader_defs.h>
#include <ZW_firmware_update_nvm_api.h>
#include <ZW_uart_api.h>

//extern const WORD firmwareUpdate_NVM_Offset;
#define BOOTCRYPTO_FW_SIZE			0x18800
#ifdef APP_TEST
#warning "BootCrypto with APP_TEST"
#define BOOTCRYPTO_FW_START			((DWORD)FIRMWARE_NVM_OFFSET_1MBIT) 
#define LEGTH_ADDR					0x2000+32
#else
	#ifdef BOOT_NVM_1M
	#define BOOTCRYPTO_FW_START		((DWORD)FIRMWARE_NVM_OFFSET_1MBIT)
	#message "1MBIT NVM"
	#else
		#ifdef BOOT_NVM_2M
		#message "2MBIT NVM"
		#define BOOTCRYPTO_FW_START			((DWORD)FIRMWARE_NVM_OFFSET_2MBIT)
		#else
			#define BOOTCRYPTO_FW_START			((DWORD)firmwareUpdate_NVM_Offset)//FIRMWARE_NVM_OFFSET_1MBIT 
			#message "AUTO DETECT NVM"
		#endif
	#endif
#define LEGTH_ADDR					0x17a0+24
#endif
#define KEY_ADDR					0x17a0
#define PARAMS_ADDR					0x2000

#define AES_DATA_LENGTH                 16
#define AES_DATA_HLENGTH                8

// Все переменные в нижних 4К для AES. Чтобы этого добится используется 
// специальный пользовательский класс XDATA_LOW12
//#ifndef APP_TEST
#pragma userclass (xdata = low12)
//#endif


#define ZMESC_AES_ECB(K, I, O) ZW_AES_ECB(K, I, O)


XBYTE g_encrypt_buff_1[AES_DATA_LENGTH];
XBYTE g_aes_bootkey_m[AES_DATA_LENGTH];
XBYTE g_initial_iv[AES_DATA_LENGTH];
XBYTE g_crypted_data[AES_DATA_LENGTH];

BYTE g_crypto_initialized = 0;
WORD g_crc_param;
WORD g_crc_addr;
//WORD g_flash_addr_start = (WORD)BOOTCRYPTO_FW_START;

WORD * index_ptr;
WORD *  sum_ptr;

extern XDWORD firmwareUpdate_NVM_Offset;

void zmeStartHandler()
{
	
}

// Вспомогательный макрос для сокращения записи проверок 
// попадения адреса в интервал кратный AES_DATA_LENGTH 
#define auxInBlock(addr, start)  ((addr >= start) && (addr < (start+AES_DATA_LENGTH)))

// ФУНКЦИЯ zmeReadLongByte подменяет в бутлодере ФУНКЦИЮ BYTE NVM_ext_read_long_byte(DWORD offset)
// Подмена производится линковщиком
BYTE zmeReadLongByte(DWORD offset)
{
	BYTE ret;
	NVM_ext_read_long_buffer(offset, &ret, 1);	

	return ret;
}

// ФУНКЦИЯ zmeDecryptFWSegment подменяет в бутлодере ФУНКЦИЮ void NVM_ext_read_long_buffer(DWORD offset, BYTE *buf, WORD length)
// Для кода бутлодера вызов остается прозрачным, он как бы работает с незашифрованной прошивкой
// Расшифровка прошивки осуществляется "на лету" для запрашиваемого блока данных
// Подмена производится утилитой FixCryptoBoot
void zmeDecryptFWSegment(DWORD offset, BYTE *buf, WORD length)
{

	BYTE tmp[3];
	BYTE shift;
	BYTE j;
	
	
	// Данная функция вызывается вместо NVM_ext_read_long_buffer и все ее параметры 
	// передаются через регистры кроме длины. Длина передается через память. Память
	// жестко связана с сегментом NVM_ext_read_long_buffer и нам нужно ее прочитать.
	// ВАЖНО: Адрес нужной переменной выставляется в code-пространстве автоматически
	// утилитой FixCryptoBoot
	WORD param_addr  = ((unsigned int volatile code  *) 0) [LEGTH_ADDR >> 1];
	length =  ((unsigned char volatile xdata *) 0) [param_addr ];
	length <<= 8;
	length |=  ((unsigned char volatile xdata *) 0) [param_addr+1];

	
	// Инициализация всего, что нужно для шифрования. Происходит только один раз при первом 
	// вызове функции NVM_ext_read_long_buffer
	if(!g_crypto_initialized)
	{
		// Инициализация вспомогательных указателей.
		// ОСТОРОЖНО: Этот код нельзя выносить в глобальную область т.к. бутлодер 
		// НЕ делает начальную инициализацию переменных. Наверное они экономили память...
		index_ptr = (WORD*)(g_encrypt_buff_1+14);
		sum_ptr = (WORD*)(g_encrypt_buff_1+12);
		
		#ifndef APP_TEST
		// Инициализация отладки
		// ВАЖНО: для включения отладки используется ключ компиляции ZW_DEBUG
		ZW_DEBUG_INIT(1152);
		#endif 
		
		// Формируем IV
		// Вытягиваем часть из CONST CODE - параметров
		// Первая половинка IV = (MANUFACTURER_ID, PRODUCT_TYPE_ID, APP_VERSION, PRODUCT_FIRMWARE_VERSION)
		// Вторая половина IV из специального сегмента
		// Последние два байта формируем из адреса
		memcpy(g_aes_bootkey_m,     ((unsigned char volatile code  *) KEY_ADDR),		AES_DATA_LENGTH);
		memcpy(g_initial_iv,        ((unsigned char volatile code  *) PARAMS_ADDR), 	AES_DATA_HLENGTH);
		memcpy(g_initial_iv + 8,    ((unsigned char volatile code  *) KEY_ADDR + 16), 	AES_DATA_HLENGTH);
	
   		 	
   }
   // Переносим IV в рабочий массив шифрования.
   // Для этого сделан разрыв в блоке инициализации. Экономим место...
   memcpy(g_encrypt_buff_1, g_initial_iv, AES_DATA_LENGTH);
	
   // Инициализация шифрования (продолжение)
   if(!g_crypto_initialized)
   {
	    
        //1.1 Считываем зашифрованный адрес FW-дескриптора
        NVM_ext_read_long_buffer(BOOTCRYPTO_FW_START + (DWORD)0x0008 , tmp, 2);
	
		// Подготавливаем IV. CRC для этого блока не используется поэтому подменяем 
		// только индекс блока = (0x1808 >> 4)
		*index_ptr = 0x0180;


        ZMESC_AES_ECB(g_aes_bootkey_m, g_encrypt_buff_1, g_crypted_data);

        /*
        // Код для отладки...
        ZW_DEBUG_SEND_BYTE(g_encrypt_buff_1[13]);
   		ZW_DEBUG_SEND_BYTE(g_encrypt_buff_1[14]);
		ZW_DEBUG_SEND_BYTE(g_encrypt_buff_1[15]);
   		ZW_DEBUG_SEND_BYTE(g_encrypt_buff_1[16]);
		*/

   		// 1.2 Расшифровываем адрес структуры FW-дескриптора и относительно него вычисляем
   		// адрес контрольной суммы (+12 т.к. перед ней находится в структуре 6 WORD-переменных)	
        g_crc_addr = (*((WORD*)&g_crypted_data[8]) ^ *((WORD*)&tmp[0])) + 12;
        
        // 1.3  Считываем из внешней памяти зашифрованные данные контрольной суммы.
        // ВАЖНО: Размер = 3 для того, чтобы компилятор не свернул код NVM_ext_read_long_buffer 
        // вместе с предыдущим вызовом в одну вспомогательную функцию, что усложнит отслеживание
        // подмены функций
        NVM_ext_read_long_buffer(BOOTCRYPTO_FW_START+ (DWORD)g_crc_addr -0x1800 , tmp, 3);
        // Расшифровываем два байта контрольной суммы
        // ВАЖНО: Это специально сделано побайтно, т.к. никто не гарантирует что эти два байта
        // лежат в одном 16-байтном блоке

        // Дешифруем старший байт контрольной суммы
        shift = g_crc_addr % AES_DATA_LENGTH;
        *index_ptr = g_crc_addr >> 4;
        ZMESC_AES_ECB(g_aes_bootkey_m, g_encrypt_buff_1, g_crypted_data);
        
        // Сохраняем старший байт контрольной суммы
        g_crc_param = tmp[0]^g_crypted_data[shift];
    	g_crc_param <<= 8;
        
        // Дешифруем младший байт контрольной суммы      
        shift = (g_crc_addr+1) % AES_DATA_LENGTH;
        *index_ptr = (g_crc_addr+1) >> 4;
        ZMESC_AES_ECB(g_aes_bootkey_m, g_encrypt_buff_1, g_crypted_data);
        
        // Сохраняем младший байт контрольной суммы
        g_crc_param |= tmp[1]^g_crypted_data[shift];
    		
	
    	
    	// Инициализация завершена. Больше сюда не заходим...
   		g_crypto_initialized = 1;

   		#ifndef APP_TEST
   		// Отладочный код
   		ZW_DEBUG_SEND_BYTE(g_crc_addr >> 8);
   		ZW_DEBUG_SEND_BYTE(g_crc_addr & 0xFF);
   		
   		ZW_DEBUG_SEND_BYTE(g_crc_param >> 8);
   		ZW_DEBUG_SEND_BYTE(g_crc_param & 0xFF);
   		
   		#endif
   		//
   		#ifndef APP_TEST
		// Отладочный код
		ZW_DEBUG_SEND_BYTE((firmwareUpdate_NVM_Offset >> 8)&0xFF);
   		ZW_DEBUG_SEND_BYTE(firmwareUpdate_NVM_Offset & 0xFF);
		#endif
	}

			

   	// 2.1. Возможно, что будет запрошена область памяти, которая не зашифрована (она находится "ниже" прошивки)
	// Обрабатываем этот случай
	// Определяем границы зашифрованной области
	if(offset < BOOTCRYPTO_FW_START)
	{	

		WORD delta = BOOTCRYPTO_FW_START-offset;
               
        if(delta > length)
             delta = length;
        // Считываем не шифрованные данные "как есть" 
		NVM_ext_read_long_buffer(offset, buf, delta);

		// Передвигаем границы зашифрованной области
		offset  +=   delta;
		length  -=   delta;
        buf  	+=   delta;
		
	}
	
	// 3.1 Основной блок. Дешифрация данных прошивки
	while(length)
	{
		// Дешифрация производится блоками выравненными на 16 байт и размер 16 байт
		// Вычисляем размер и смещение
		BYTE xsz = AES_DATA_LENGTH;
		shift = offset & 0x0F;
		

		// Первый блок не выровненный...
		xsz -= shift;
		// Последний пакет
		if( xsz > length)
			xsz = length;

		// Считываем зашифрованные данные
		NVM_ext_read_long_buffer(offset, buf, xsz);
	
		// Заполняем в IV индекс блока.
		// Индекс считает от начала прошивки т.е. с адреса 0x0000.
		// В памяти прошивка лежит без бутлодера, т.е. без первых 0x1800 байт.
		// Индекс = Адрес_блока / 16 ( отсюда вылезает >> 4)
		*index_ptr = ((offset-(DWORD)BOOTCRYPTO_FW_START+(DWORD)0x1800) >> 4);
        
        // Некоторые блоки дешифруются без контрольной суммы
        if(auxInBlock(offset,  (DWORD)BOOTCRYPTO_FW_START) || // Блок, содержащий адрес FW-сегмента
           auxInBlock(offset, (DWORD)(BOOTCRYPTO_FW_START - 0x1800 + (g_crc_addr &0XFFF0))) || // Блок, содержащий 1-ый байт контрольной суммы
           auxInBlock(offset, (DWORD)(BOOTCRYPTO_FW_START - 0x1800 + ((g_crc_addr+1) &0XFFF0)))) // Блок, содержащий 2-ой байт контрольной суммы
        	*sum_ptr = *((WORD*)&(g_initial_iv[12])); // В этом случае подставляем сюда начальные значения IV
        else	
   			*sum_ptr = g_crc_param; // Подставляем контрольную сумму
		
		// Дешифрация блока
        ZMESC_AES_ECB(g_aes_bootkey_m, g_encrypt_buff_1, g_crypted_data);
        for(j=0; j<xsz; j++)
            if(j < (AES_DATA_LENGTH-shift))
                    buf[j] = buf[j]^g_crypted_data[j+shift]; 

        // Переходим к следующему блоку        
		offset 	+= (DWORD)xsz;
		length 	-= (WORD)xsz;
		buf 	+= (WORD)xsz;	 

	}
	
}

#endif