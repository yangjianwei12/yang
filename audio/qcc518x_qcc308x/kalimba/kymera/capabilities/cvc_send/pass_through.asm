// *****************************************************************************
// Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// $Change$  $DateTime$
// *****************************************************************************

#include "cvc_send_data.h"

// -----------------------------------------------------------------------------
// inputs:
//    -r7 = &cvc_streams
//    -r8 = &stream_z0
//    -r9 = $root 
// 
// outputs:
//    -None
// -----------------------------------------------------------------------------
.MODULE $M.cvc_send.passthrough.voice;
   .CODESEGMENT PM;

$cvc_send.passthrough.voice:  
   // check requested feature
   // if voice feature is not requested, return   
   r2 = M[r9 + $cvc_send.data.cap_root_ptr];
   r1 = M[r2 + $cvc_send.cap.OP_FEATURE_REQUESTED];
   Null = r1 AND $cvc_send.REQUESTED_FEATURE_VOICE;
   if Z rts;

   r6 = $M.GEN.CVC_SEND.PARAMETERS.OFFSET_SNDGAIN_PT;

$cvc_send.passthrough:   
   // check system mode
   r0 = M[r2 + $cvc_send.cap.CUR_MODE];   
   NULL = r0 - $M.GEN.CVC_SEND.SYSMODE.STANDBY;
   if LE jump $cvc.stream_mute;
  
   // get voice input source offset  
   r1 = $cvc_send.stream.sndin_left;
   Null = r0 - $M.GEN.CVC_SEND.SYSMODE.PASS_THRU_LEFT;
   if Z jump get_source;   

   r1 = $cvc_send.stream.sndin_right;
   Null = r0 - $M.GEN.CVC_SEND.SYSMODE.PASS_THRU_RIGHT;
   if Z jump get_source;
  
   r1 = $cvc_send.stream.sndin_mic3;
   Null = r0 - $M.GEN.CVC_SEND.SYSMODE.PASS_THRU_MIC3;
   if Z jump get_source;

   r1 = $cvc_send.stream.sndin_mic4;
   Null = r0 - $M.GEN.CVC_SEND.SYSMODE.PASS_THRU_MIC4;
   if Z jump get_source;
   
   r1 = $cvc_send.stream.refin;
   Null = r0 - $M.GEN.CVC_SEND.SYSMODE.PASS_THRU_AEC_REF;
   if Z jump get_source;

   // Default connect to mic1
   r1 = $cvc_send.stream.sndin_left;

get_source:
   // get input source pointer
   r7 = M[r7 + r1];

   // get and apply gain with input source in r7, destination in r8, cvc root object in r9
   jump $cvc_send.stream_gain.passthrough;

.ENDMODULE;

// -----------------------------------------------------------------------------
// inputs:
//    -r7 = &cvc_streams
//    -r8 = &stream_z1
//    -r9 = $root 
// 
// outputs:
//    -None
// -----------------------------------------------------------------------------
.MODULE $M.cvc_send.passthrough.va;
   .CODESEGMENT PM;
   
$cvc_send.passthrough.va:   
   // check requested feature
   // if VA feature is not requested, return  
   r2 = M[r9 + $cvc_send.data.cap_root_ptr];
   r1 = M[r2 + $cvc_send.cap.OP_FEATURE_REQUESTED];
   Null = r1 AND $cvc_send.REQUESTED_FEATURE_VA;
   if Z rts;

   r6 = $M.GEN.CVC_SEND.PARAMETERS.OFFSET_SNDGAIN_VA;
   jump $cvc_send.passthrough;

.ENDMODULE;

//------------------------------------------------------------------------------
// inputs:
//    -r7 = ADC input stream pointer
//    -r8 = VA output stream pointer
//    -r9 = $root
// 
// outputs:
//    None 
// -----------------------------------------------------------------------------  
.MODULE $M.cvc_send.passthrough.speaker_va;
   .CODESEGMENT PM;   
   
$cvc_send.passthrough.speaker_va:
   // check requested feature
   // if voice feature is not requested, return
   r2 = M[r9 + $cvc_send.data.cap_root_ptr];
   r1 = M[r2 + $cvc_send.cap.OP_FEATURE_REQUESTED];
   Null = r1 AND $cvc_send.REQUESTED_FEATURE_VA;
   if Z rts;
 
   // check system mode 
   r0 = M[r2 + $cvc_send.cap.CUR_MODE];   
   NULL = r0 - $M.GEN.CVC_SEND.SYSMODE.STANDBY;
   if LE jump $cvc.stream_mute;

   // get and apply gain with input source in r7, destination in r8, cvc root object in r9
   r6 = $M.GEN.CVC_SEND.PARAMETERS.OFFSET_SNDGAIN_VA;
   jump $cvc_send.stream_gain.passthrough;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $cvc_send.stream_gain.passthrough
//
// DESCRIPTION:
//
// MODIFICATIONS:
//
// INPUTS:
//    - r6 - offset in cvc param array to stream gain (log2 dB in Q8.N)
//    - r7 - source stream pointer
//    - r8 - destination stream pointer
//    - r9 - cvc root object
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//
// CPU USAGE:
//
// NOTE:
// *****************************************************************************
.MODULE $M.CVC_SEND.stream_gain.passthrough;

   .CODESEGMENT PM;

$cvc_send.stream_gain.passthrough:
   // save source streme
   r4 = r7;
   // no extra scaling
   r5 = 0;
   // get gain pointer, r6 from caller
   r1 = M[r9 + $cvc_send.data.param];
   r7 = r6 + r1;

   jump $cvc.stream_gain.apply;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $M.CVC_SEND.mute_control
//
// DESCRIPTION:
//    Mute control
//
// MODIFICATIONS:
//
// INPUTS:
//    r7 - mute control pointer
//    I8 - source & target stream for in-place processing
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//
// CPU USAGE:
// *****************************************************************************
.MODULE $M.CVC_SEND.mute_control;
   .CODESEGMENT PM;

$cvc_send.mute_control:
   r7 = M[r7];
   Null = 1 - r7;
   if NZ rts;
   jump $cvc.stream_mute;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $cvc.stream_gain.process
//
// DESCRIPTION:
//
// MODIFICATIONS:
//
// INPUTS:
//    - r7 - point to stream gain (log2 dB in Q8.N)
//    - r8 - source/destination stream pointer (in-place)
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//
// CPU USAGE:
//
// NOTE:
// *****************************************************************************
.MODULE $M.CVC.stream_gain;

   .CODESEGMENT PM;

$cvc.stream_gain.mult4.process:
   r5 = 1.0;
   r6 = 2;
   r7 = r8;
   jump $cvc.stream_gain;


$cvc.stream_gain.process:
   r5 = $filter_bank.QBIT_Z;
   r4 = r8;

// -----------------------------------------------------------------------------
// r8 -> target streme
// r4 -> source streme
// r5 -> gain extra shift
// -----------------------------------------------------------------------------
$cvc.stream_gain.apply:
   push rLink;

   // transfer gain from L2dB to exp/mantisa
   call calc_L2dB_Gain;
   // gain exp
   r6 = r6 + r5;
   // gain mantisa
   r5 = r1;
   // source in r7, and destination in r8
   r7 = r4;
   call $cvc.stream_gain;

   jump $pop_rLink_and_rts;

// -----------------------------------------------------------------------------
// L2dBLin  = L2dB./2;
// Exp      = (floor(L2dBLin) + 1);
// Mts      = pow2(L2dBLin-Exp);
// -----------------------------------------------------------------------------
calc_L2dB_Gain:
   // r0 -> Streme Gain in Log2 dB format
   r0 = M[r7];
   if Z jump zero_db_gain;
   r0 = r0 ASHIFT -1;
   jump $math.pow2_table;

zero_db_gain:
   // exp
   r6 = 1;
   // mantisa
   r1 = 0.5;
   // gain
   r0 = 1.0;
   rts;

.ENDMODULE;
