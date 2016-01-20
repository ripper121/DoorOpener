
#include <ZW_basis_api.h>
#include <ZW_firmware_bootloader_defs.h>
#include <ZW_firmware_descriptor.h>

#ifdef SERIAL_PROJ
#include "config_app.h"
#else
#include "Custom.h"
#endif

#define TO_WORD_ID(ID) (((ID##_MAJOR) << 8) | ID##_MINOR)

/****************************************************************************/
/*                              EXTERNALS                                   */
/****************************************************************************/
//extern unsigned char _APP_VERSION_;
//extern unsigned char _ZW_VERSION_;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#define BANK_OFFSET   MCU_BANK_SIZE


/****************************************************************************/
/*                              EXTERNALS                                   */
/****************************************************************************/

extern code BYTE bBank1EndMarker;
extern code BYTE bBank2EndMarker;
extern code BYTE bBank3EndMarker;


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/



/* Firmware descriptor for OTA firmware update */
code t_firmwareDescriptor firmwareDescriptor =
{
 /* Total amount of code used in COMMON bank */
  (WORD)&firmwareDescriptor + sizeof(firmwareDescriptor),
#ifdef BOOTLOADER_ENABLED
  /* Above Size is inclusive the Bootloader code - substract FIRMWARE_BOOTLOADER_SIZE for real Firmware COMMON BANK usage */
  /* Total amount of code saved in NVM for the BANK1 bank */
  (WORD)&bBank1EndMarker + sizeof(bBank1EndMarker) - BANK_OFFSET,
  /* Total amount of code saved in NVM for the BANK2 bank */
  (WORD)&bBank2EndMarker + sizeof(bBank2EndMarker) - BANK_OFFSET,
  /* Total amount of code saved in NVM for the BANK3 bank */
  (WORD)&bBank3EndMarker + sizeof(bBank3EndMarker) - BANK_OFFSET,
#else
  0,
  0,
  0,
#endif
  /* TODO: Fill in your unique and assigned manufacturer ID here                */
  TO_WORD_ID(MANUFACTURER_ID),  //APP_MANUFACTURER_ID,                  /* WORD manufacturerID; */
  /* TODO: Fill in your own unique firmware ID here                             */
  TO_WORD_ID(PRODUCT_FIRMWARE_VERSION),           //APP_FIRMWARE_ID,                      /* WORD firmwareID; */
  /* A CRC-CCITT must be, and will be, filled in here by a software build tool (fixbootcrc.exe) */
  0                                     /* WORD checksum; */
};
