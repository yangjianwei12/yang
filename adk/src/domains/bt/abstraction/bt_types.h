#ifndef BT_TYPES_H
#define BT_TYPES_H
#include "csrtypes.h"
#ifdef USE_SYNERGY
typedef uint32 gatt_cid_t;
#else
typedef uint16 gatt_cid_t;
#endif
#endif // BT_TYPES_H
