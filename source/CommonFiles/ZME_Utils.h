#ifndef ZME_UTILS
#define ZME_UTILS

#define NODE_MASK_LENGTH								(ZW_MAX_NODES / 8)
#define NODE_MASK_IS_SET(mask, nodeId)					(((mask)[nodeId / 8] &   (1 << (nodeId % 8))) != 0)
#define NODE_MASK_SET(mask, nodeId)						{(mask)[nodeId / 8]  |=  (1 << (nodeId % 8));}
#define NODE_MASK_CLEAR(mask, nodeId)					{(mask)[nodeId / 8]  &= ~(1 << (nodeId % 8));}
#define NODE_MASK_ZERO(mask)							memset(mask, 0, NODE_MASK_LENGTH)

BOOL inCCArray(BYTE *array, BYTE arraySize, BYTE needle, BOOL beforeMarkOnly);
BOOL inArray(BYTE *array, BYTE arraySize, BYTE needle);
void zme_memmove(BYTE *dst, BYTE *src, BYTE size);
void zme_memmove_(BYTE* v_dst, BYTE *v_src, BYTE c);

#endif