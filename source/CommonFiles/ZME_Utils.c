#include "ZME_Includes.h"

BOOL inCCArray(BYTE *array, BYTE arraySize, BYTE needle, BOOL beforeMarkOnly) {
	BYTE i;
	
	for (i = 0; i < arraySize; i++)
	{
		if ((array[i] == COMMAND_CLASS_MARK) && beforeMarkOnly)
			break;

		if (array[i] == needle)
			return TRUE;
	}
	return FALSE;
}

BOOL inArray(BYTE *array, BYTE arraySize, BYTE needle) {
	BYTE i;
	
	for (i = 0; i < arraySize; i++)
		if (array[i] == needle)
			return TRUE;
	return FALSE;
}


// memmove name is intrinsic in Keil, so we use zme_memmove
void zme_memmove(BYTE *dst, BYTE *src, BYTE size) {
	src += size;
	dst += size;

	while (size--)
		*--dst = *--src;
}
void zme_memmove_(BYTE* v_dst, BYTE *v_src, BYTE c)
{
         
          /* Simple, byte oriented memmove. */
          while (c--)
                  *v_dst++ = *v_src++;
}