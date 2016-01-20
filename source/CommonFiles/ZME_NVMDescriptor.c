
#include <ZW_basis_api.h>
#include <ZW_nvm_descriptor.h>

#ifdef SERIAL_PROJ
#include "config_app.h"
#else
#include "Custom.h"
#endif
#define TO_WORD_ID(ID) (((ID##_MAJOR) << 8) | ID##_MINOR)

//extern unsigned char code _APP_VERSION__;
extern unsigned char _ZW_VERSION_;

/* NVM descriptor for firmware */
t_nvmDescriptor far nvmDescriptor =
{
  /* TODO: Fill in your unique and assigned manufacturer ID here                */
  TO_WORD_ID(MANUFACTURER_ID),                          /* WORD manufacturerID;         */
  /* TODO: Fill in your own unique firmware ID here                             */
  TO_WORD_ID(PRODUCT_FIRMWARE_VERSION),                              /* WORD firmwareID;             */
  /* TODO: Fill in your own unique Product Type ID here                         */
  TO_WORD_ID(PRODUCT_TYPE_ID),                              /* WORD firmwareID;             */
                           /* WORD productTypeID;          */
  /* TODO: Fill in your own unique Product ID here                              */
  TO_WORD_ID(PRODUCT_ID),                               /* WORD productID;              */
  /* Unique Application Version (from config_app.h)                             */
  TO_WORD_ID(PRODUCT_ID),   // _APP_VERSION_
	//(WORD)&_APP_VERSION__,                         /* WORD applicationVersion;     */
  /* Unique Z-Wave protocol Version (from config_lib.h)                         */
  (WORD)&_ZW_VERSION_,                          /* WORD zwaveProtocolVersion;   */
};

