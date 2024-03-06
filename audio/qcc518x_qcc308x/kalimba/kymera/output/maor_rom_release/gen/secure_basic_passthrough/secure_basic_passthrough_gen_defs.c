// -----------------------------------------------------------------------------
// Copyright (c) 2023                  Qualcomm Technologies International, Ltd.
//
#include "secure_basic_passthrough_gen_c.h"

#ifndef __GNUC__ 
_Pragma("datasection CONST")
#endif /* __GNUC__ */

static unsigned defaults_secure_basic_passthroughBPT[] = {
   0x00002080u,			// CONFIG
   0x00000000u			// GAIN
};

unsigned *SECURE_BASIC_PASSTHROUGH_GetDefaults(unsigned capid){
	switch(capid){
		case 0xC000: return defaults_secure_basic_passthroughBPT;
		case 0xC001: return defaults_secure_basic_passthroughBPT;
	}
	return((unsigned *)0);
}
