
#include <ZW_basis_api.h>
#include <ZW_firmware_bootloader_defs.h>
#include <ZW_firmware_descriptor.h>



/* Size field of firmware descriptor for OTA firmware update.                 */
/* This firmware descriptor addr field must be, and will be, located at */
/* address FIRMWARE_INTVECTOR_OFFSET + 8 */
code WORD firmwareDescriptorAddr = (WORD)&firmwareDescriptor;

