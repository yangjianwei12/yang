// -----------------------------------------------------------------------------
// Copyright (c) 2021                  Qualcomm Technologies International, Ltd.
//
#include "xover_gen_c.h"

#ifndef __GNUC__ 
_Pragma("datasection CONST")
#endif /* __GNUC__ */

static unsigned defaults_xoverXOVER[] = {
   0x00000000u,			// XOVER_CONFIG
   0x00000000u,			// XOVER_FILTER_TYPE
   0x00000003u,			// LP_TYPE
   0x000FA000u,			// LP_FC
   0x00000001u,			// LP_ORDER
   0x00000003u,			// HP_TYPE
   0x000FA000u,			// HP_FC
   0x00000001u,			// HP_ORDER
   0x00000003u,			// LP_TYPE_F2
   0x009C4000u,			// LP_FC_F2
   0x00000001u,			// LP_ORDER_F2
   0x00000003u,			// HP_TYPE_F2
   0x009C4000u,			// HP_FC_F2
   0x00000001u			// HP_ORDER_F2
};

unsigned *XOVER_GetDefaults(unsigned capid){
	switch(capid){
		case 0x000033: return defaults_xoverXOVER;
		case 0x406B: return defaults_xoverXOVER;
		case 0x000034: return defaults_xoverXOVER;
		case 0x409D: return defaults_xoverXOVER;
	}
	return((unsigned *)0);
}
