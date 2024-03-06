// -----------------------------------------------------------------------------
// Copyright (c) 2023                  Qualcomm Technologies International, Ltd.
//
#include "mixer_gen_c.h"

#ifndef __GNUC__ 
_Pragma("datasection CONST")
#endif /* __GNUC__ */

static unsigned defaults_mixerMX[] = {
   0x00002080u			// CONFIG
};

unsigned *MIXER_GetDefaults(unsigned capid){
	switch(capid){
		case 0x000A: return defaults_mixerMX;
		case 0x405B: return defaults_mixerMX;
	}
	return((unsigned *)0);
}
