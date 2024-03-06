// -----------------------------------------------------------------------------
// Copyright (c) 2023                  Qualcomm Technologies International, Ltd.
//
// Generated by /home/svc-audio-dspsw/kymera_builds/builds/2023/kymera_2312060823/kalimba/kymera/../../util/CommonParameters/DerivationEngine.py
// code v3.5, file //depot/dspsw/maor_rom_v20/util/CommonParameters/DerivationEngine.py, revision 2
// namespace {com.csr.cps.4}UnifiedParameterSchema on 2023-12-06 08:42:54 by svc-audio-dspsw
//
// input from EMPTY
// last change  by  on 
// -----------------------------------------------------------------------------
#ifndef __AANC_GEN_ASM_H__
#define __AANC_GEN_ASM_H__

// CodeBase IDs
.CONST $M.AANC_AANC_MONO_16K_CAP_ID       	0x00C7;
.CONST $M.AANC_AANC_MONO_16K_ALT_CAP_ID_0       	0x409F;
.CONST $M.AANC_AANC_MONO_16K_SAMPLE_RATE       	16000;
.CONST $M.AANC_AANC_MONO_16K_VERSION_MAJOR       	1;

// Constant Values
.CONST $M.AANC.CONSTANT.IN_OUT_EAR_CTRL            		0x00000003;
.CONST $M.AANC.CONSTANT.FF_FINE_GAIN_CTRL          		0x00000004;
.CONST $M.AANC.CONSTANT.CHANNEL_CTRL               		0x00000005;
.CONST $M.AANC.CONSTANT.FEEDFORWARD_CTRL           		0x00000006;
.CONST $M.AANC.CONSTANT.FF_COARSE_GAIN_CTRL        		0x00000007;
.CONST $M.AANC.CONSTANT.FB_FINE_GAIN_CTRL          		0x00000008;
.CONST $M.AANC.CONSTANT.FB_COARSE_GAIN_CTRL        		0x00000009;
.CONST $M.AANC.CONSTANT.EC_FINE_GAIN_CTRL          		0x0000000A;
.CONST $M.AANC.CONSTANT.EC_COARSE_GAIN_CTRL        		0x0000000B;
.CONST $M.AANC.CONSTANT.FILTER_CONFIG_CTRL         		0x0000000C;
.CONST $M.AANC.CONSTANT.RX_FFA_MIX_FINE_GAIN_CTRL  		0x0000000D;
.CONST $M.AANC.CONSTANT.RX_FFA_MIX_COARSE_GAIN_CTRL		0x0000000E;
.CONST $M.AANC.CONSTANT.RX_FFB_MIX_FINE_GAIN_CTRL  		0x0000000F;
.CONST $M.AANC.CONSTANT.RX_FFB_MIX_COARSE_GAIN_CTRL		0x00000010;
.CONST $M.AANC.CONSTANT.SAMPLE_RATE_CTRL           		0x00000011;


// Piecewise Disables
// AANC_CONFIG bits
.CONST $M.AANC.CONFIG.AANC_CONFIG.DISABLE_ED_INT              		0x00000001;
.CONST $M.AANC.CONFIG.AANC_CONFIG.DISABLE_ED_EXT              		0x00000002;
.CONST $M.AANC.CONFIG.AANC_CONFIG.DISABLE_ED_PB               		0x00000004;
.CONST $M.AANC.CONFIG.AANC_CONFIG.DISABLE_SELF_SPEECH         		0x00000008;
// DISABLE_AG_CALC bits
.CONST $M.AANC.CONFIG.DISABLE_AG_CALC.DISABLE_AG_CALC         		0x00000001;
// AANC_DEBUG bits
.CONST $M.AANC.CONFIG.AANC_DEBUG.DISABLE_EAR_STATUS_CHECK     		0x00000002;
.CONST $M.AANC.CONFIG.AANC_DEBUG.DISABLE_ANC_CLOCK_CHECK      		0x00000004;
.CONST $M.AANC.CONFIG.AANC_DEBUG.DISABLE_EVENT_MESSAGING      		0x00000008;
.CONST $M.AANC.CONFIG.AANC_DEBUG.DISABLE_ED_INT_E_FILTER_CHECK		0x00000010;
.CONST $M.AANC.CONFIG.AANC_DEBUG.DISABLE_ED_EXT_E_FILTER_CHECK		0x00000020;
.CONST $M.AANC.CONFIG.AANC_DEBUG.DISABLE_ED_PB_E_FILTER_CHECK 		0x00000040;
.CONST $M.AANC.CONFIG.AANC_DEBUG.MUX_SEL_ALGORITHM            		0x00000100;
.CONST $M.AANC.CONFIG.AANC_DEBUG.DISABLE_FILTER_OPTIM         		0x00000200;
.CONST $M.AANC.CONFIG.AANC_DEBUG.DISABLE_CLIPPING_DETECT_INT  		0x00001000;
.CONST $M.AANC.CONFIG.AANC_DEBUG.DISABLE_CLIPPING_DETECT_EXT  		0x00002000;
.CONST $M.AANC.CONFIG.AANC_DEBUG.DISABLE_CLIPPING_DETECT_PB   		0x00004000;


// Statistic Block
.CONST $M.AANC.STATUS.CUR_MODE           		0*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.OVR_CONTROL        		1*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.IN_OUT_EAR_CTRL    		2*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.CHANNEL            		3*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.FILTER_CONFIG      		4*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.FEEDFORWARD_PATH   		5*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.LICENSE_STATUS     		6*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.FLAGS              		7*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.AG_CALC            		8*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.FF_FINE_GAIN_CTRL  		9*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.FF_COARSE_GAIN_CTRL		10*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.FF_GAIN_DB         		11*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.FB_FINE_GAIN_CTRL  		12*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.FB_COARSE_GAIN_CTRL		13*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.FB_GAIN_DB         		14*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.EC_FINE_GAIN_CTRL  		15*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.EC_COARSE_GAIN_CTRL		16*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.EC_GAIN_DB         		17*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.SPL_EXT            		18*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.SPL_INT            		19*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.SPL_PB             		20*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.PEAK_EXT           		21*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.PEAK_INT           		22*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.PEAK_PB            		23*ADDR_PER_WORD;
.CONST $M.AANC.STATUS.BLOCK_SIZE              	24;

// System Mode
.CONST $M.AANC.SYSMODE.STANDBY    		0;
.CONST $M.AANC.SYSMODE.MUTE_ANC   		1;
.CONST $M.AANC.SYSMODE.FULL       		2;
.CONST $M.AANC.SYSMODE.STATIC     		3;
.CONST $M.AANC.SYSMODE.FREEZE     		4;
.CONST $M.AANC.SYSMODE.GENTLE_MUTE		5;
.CONST $M.AANC.SYSMODE.QUIET      		6;
.CONST $M.AANC.SYSMODE.MAX_MODES  		7;

// System Control
.CONST $M.AANC.CONTROL.MODE_OVERRIDE		0x2000;

// In/Out of Ear Status
.CONST $M.AANC.IN_OUT_EAR_CTRL.IN_EAR 		0x0001;
.CONST $M.AANC.IN_OUT_EAR_CTRL.OUT_EAR		0x0000;

// Selected Channel
.CONST $M.AANC.CHANNEL.ANC0		0x0001;
.CONST $M.AANC.CHANNEL.ANC1		0x0002;

// Selected Filter Configuration
.CONST $M.AANC.FILTER_CONFIG.SINGLE  		0x0000;
.CONST $M.AANC.FILTER_CONFIG.PARALLEL		0x0001;

// Selected Path
.CONST $M.AANC.FEEDFORWARD_PATH.FFA		0x0001;
.CONST $M.AANC.FEEDFORWARD_PATH.FFB		0x0002;

// License Status
.CONST $M.AANC.LICENSE_STATUS.ED                    		0x00000001;
.CONST $M.AANC.LICENSE_STATUS.FxLMS                 		0x00000002;
.CONST $M.AANC.LICENSE_STATUS.LICENSING_BUILD_STATUS		0x10000000;

// Flags
.CONST $M.AANC.FLAGS.ED_INT            		0x00000010;
.CONST $M.AANC.FLAGS.ED_EXT            		0x00000020;
.CONST $M.AANC.FLAGS.ED_PLAYBACK       		0x00000040;
.CONST $M.AANC.FLAGS.SELF_SPEECH       		0x00000080;
.CONST $M.AANC.FLAGS.CLIPPING_INT      		0x00000100;
.CONST $M.AANC.FLAGS.CLIPPING_EXT      		0x00000200;
.CONST $M.AANC.FLAGS.CLIPPING_PLAYBACK 		0x00000400;
.CONST $M.AANC.FLAGS.SATURATION_INT    		0x00001000;
.CONST $M.AANC.FLAGS.SATURATION_EXT    		0x00002000;
.CONST $M.AANC.FLAGS.SATURATION_PLANT  		0x00004000;
.CONST $M.AANC.FLAGS.SATURATION_CONTROL		0x00008000;
.CONST $M.AANC.FLAGS.QUIET_MODE        		0x00100000;

// ANC FF Fine Gain

// ANC FF Coarse Gain

// ANC FB Fine Gain

// ANC FB Coarse Gain

// ANC EC Fine Gain

// ANC EC Coarse Gain

// Parameter Block
.CONST $M.AANC.PARAMETERS.OFFSET_AANC_CONFIG                          		0*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_DISABLE_AG_CALC                      		1*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_MU                                   		2*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_GAMMA                                		3*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_GENTLE_MUTE_TIMER                    		4*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_AANC_DEBUG                           		5*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_QUIET_MODE_HI_THRESHOLD              		6*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_QUIET_MODE_LO_THRESHOLD              		7*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_EXT_ATTACK                        		8*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_EXT_DECAY                         		9*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_EXT_ENVELOPE                      		10*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_EXT_INIT_FRAME                    		11*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_EXT_RATIO                         		12*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_EXT_MIN_SIGNAL                    		13*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_EXT_MIN_MAX_ENVELOPE              		14*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_EXT_DELTA_TH                      		15*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_EXT_COUNT_TH                      		16*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_EXT_HOLD_FRAMES                   		17*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_EXT_E_FILTER_MIN_THRESHOLD        		18*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_EXT_E_FILTER_MIN_COUNTER_THRESHOLD		19*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_INT_ATTACK                        		20*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_INT_DECAY                         		21*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_INT_ENVELOPE                      		22*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_INT_INIT_FRAME                    		23*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_INT_RATIO                         		24*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_INT_MIN_SIGNAL                    		25*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_INT_MIN_MAX_ENVELOPE              		26*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_INT_DELTA_TH                      		27*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_INT_COUNT_TH                      		28*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_INT_HOLD_FRAMES                   		29*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_INT_E_FILTER_MIN_THRESHOLD        		30*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_INT_E_FILTER_MIN_COUNTER_THRESHOLD		31*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_PB_ATTACK                         		32*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_PB_DECAY                          		33*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_PB_ENVELOPE                       		34*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_PB_INIT_FRAME                     		35*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_PB_RATIO                          		36*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_PB_MIN_SIGNAL                     		37*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_PB_MIN_MAX_ENVELOPE               		38*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_PB_DELTA_TH                       		39*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_PB_COUNT_TH                       		40*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_PB_HOLD_FRAMES                    		41*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_PB_E_FILTER_MIN_THRESHOLD         		42*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_ED_PB_E_FILTER_MIN_COUNTER_THRESHOLD 		43*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_DENOMINATOR_COEFF_EXT_0          		44*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_DENOMINATOR_COEFF_EXT_1          		45*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_DENOMINATOR_COEFF_EXT_2          		46*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_DENOMINATOR_COEFF_EXT_3          		47*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_DENOMINATOR_COEFF_EXT_4          		48*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_NUMERATOR_COEFF_EXT_0            		49*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_NUMERATOR_COEFF_EXT_1            		50*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_NUMERATOR_COEFF_EXT_2            		51*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_NUMERATOR_COEFF_EXT_3            		52*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_NUMERATOR_COEFF_EXT_4            		53*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_DENOMINATOR_COEFF_INT_0          		54*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_DENOMINATOR_COEFF_INT_1          		55*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_DENOMINATOR_COEFF_INT_2          		56*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_DENOMINATOR_COEFF_INT_3          		57*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_DENOMINATOR_COEFF_INT_4          		58*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_NUMERATOR_COEFF_INT_0            		59*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_NUMERATOR_COEFF_INT_1            		60*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_NUMERATOR_COEFF_INT_2            		61*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_NUMERATOR_COEFF_INT_3            		62*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_BPF_NUMERATOR_COEFF_INT_4            		63*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_CLIPPING_DURATION_EXT                		64*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_CLIPPING_DURATION_INT                		65*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_CLIPPING_DURATION_PB                 		66*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_FXLMS_MIN_BOUND                      		67*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_FXLMS_MAX_BOUND                      		68*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_FXLMS_MAX_DELTA                      		69*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_FXLMS_INITIAL_VALUE                  		70*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_EVENT_GAIN_STUCK                     		71*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_EVENT_ED_STUCK                       		72*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_EVENT_QUIET_DETECT                   		73*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_EVENT_QUIET_CLEAR                    		74*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_EVENT_CLIP_STUCK                     		75*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_EVENT_SAT_STUCK                      		76*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_EVENT_SELF_TALK                      		77*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_SELF_SPEECH_THRESHOLD                		78*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_LAMBDA                               		79*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_TARGET_NOISE_REDUCTION               		80*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_QUIET_MODE_TIMER                     		81*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_EVENT_SPL                            		82*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_EVENT_SPL_THRESHOLD                  		83*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_FF_FINE_RAMP_UP_TIMER                		84*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_FB_FINE_RAMP_UP_TIMER                		85*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.OFFSET_FB_FINE_RAMP_DELAY_TIMER             		86*ADDR_PER_WORD;
.CONST $M.AANC.PARAMETERS.STRUCT_SIZE                                		87;


#endif // __AANC_GEN_ASM_H__
