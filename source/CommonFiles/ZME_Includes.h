#ifndef _INCLUDES_H_
#define _INCLUDES_H_

#include <ZW_typedefs.h>
#include <ZW_basis_api.h>
#include <ZW_pindefs.h>
#include <ZW_timer_api.h>
#include <ZW_adcdriv_api.h>
#include <ZW_transport_api.h>
#include <ZW_nvr_api.h>
#ifdef ZW050x
#include <ZW_NVM_DESCRIPTOR.h>
#endif

#if defined(ZW050x)
#include <ZW_flash_api.h>
#endif

#define FLIRS_250	 25
#define FLIRS_1000 100

#define DYNAMIC_MODE_ALWAYS_LISTENING		0
#define DYNAMIC_MODE_WAKE_UP						1
#define DYNAMIC_MODE_FLIRS							2

#include "Custom.h"
#include "Custom_pindefs.h"

#if PRODUCT_FLIRS_TYPE 
#define ZW_BEAM_RX_WAKEUP
#endif
#include <ZW_power_api.h>

#if WITH_TRIAC
#include <ZW_triac_api.h>
#endif

#ifdef ZW_SLAVE
#ifdef ZW_SLAVE_32
#include <ZW_slave_32_api.h>
#else
#include <ZW_slave_routing_api.h>
#endif
#include "ZME_Slave_Learn.h"
#else
#include <ZW_controller_api.h>
#include "ZME_Controller_Learn.h"
#endif

#include "ZME_Global.h"
#include "ZME_Utils.h"
#if WITH_CC_ASSOCIATION
#include "ZME_Association.h"
#endif
#include "ZME_Queue.h"
#include "ZME_Buttons.h"
#include "ZME_LED.h"
#include "ZME_Time.h"

#if WITH_I2C_INTERFACE
#include "ZME_I2C.h"
#endif

#if WITH_CC_POWERLEVEL
#include "ZME_PowerLevel.h"
#endif


#include "Custom_Includes.h" // should be last in the list

//#ifdef WITH_CC_BATTERY
#include "ZME_Battery.h"
//#endif

#endif /* _INCLUDES_H_ */