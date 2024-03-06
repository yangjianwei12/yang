// -----------------------------------------------------------------------------
// Copyright (c) 2023                  Qualcomm Technologies International, Ltd.
//
// Generated by /home/svc-audio-dspsw/kymera_builds/builds/2023/kymera_2312060823/kalimba/kymera/../../util/CommonParameters/DerivationEngine.py
// code v3.5, file //depot/dspsw/maor_rom_v20/util/CommonParameters/DerivationEngine.py, revision 2
// namespace {com.csr.cps.4}UnifiedParameterSchema on 2023-12-06 08:42:23 by svc-audio-dspsw
//
// input from //depot/dspsw/crescendo_dev/kalimba/kymera/capabilities/qva/qva.xml#3
// last change 2721140 by ra22 on 2017/01/31 13:34:30
// -----------------------------------------------------------------------------
#include "apva_gen_c.h"

#ifndef __GNUC__ 
_Pragma("datasection CONST")
#endif /* __GNUC__ */

static unsigned defaults_apvaAPVA[] = {
   0x000001F4u			// APVA_THRESHOLD
};

unsigned *APVA_GetDefaults(unsigned capid){
	switch(capid){
		case 0x00C6: return defaults_apvaAPVA;
		case 0x409C: return defaults_apvaAPVA;
	}
	return((unsigned *)0);
}
