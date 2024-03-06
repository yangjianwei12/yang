/****************************************************************************
 * COMMERCIAL IN CONFIDENCE
* Copyright (c) 2008 - 2017 Qualcomm Technologies International, Ltd.
 *
 ************************************************************************//**
 * \file sections.h
 * Sections definitions for Kalimba
 *
 * MODULE : Sections
 *
 ****************************************************************************/

#ifndef SECTIONS_H
#define SECTIONS_H

/* INLINE_SECTION is used by capability download feature;
   Force inline functions in header files to be placed in PM
   so that they get garbage collected when not used.
 */
#ifdef __KCC__
#define INLINE_SECTION   _Pragma("codesection PM")
#else /* __KCC__ */
#define INLINE_SECTION
#endif /* __KCC__ */

/* RUN_FROM_PM_RAM leading a function definition
   instructs the compiler/linker (kcc) to place the code in RAM.
 */
#if defined(CRESCENDO_HOIST_CODE_TO_RAM) && defined(__KCC__)
#define RUN_FROM_PM_RAM  _Pragma("codesection PM_RAM")
#else /* defined(CRESCENDO_HOIST_CODE_TO_RAM) && defined(__KCC__) */
#define RUN_FROM_PM_RAM
#endif /* defined(CRESCENDO_HOIST_CODE_TO_RAM) && defined(__KCC__) */

/* Software control of ROM power controller shuts down all the ROM banks
 * except the zeroth bank of ROM. Hence important code needs to be placed 
 * in the a separate section that is placed in zeroth bank of ROM.*/
#ifdef __KCC__
#define PLACE_IN_ROM_BANK0   _Pragma("codesection ROM_BANK_0_PM")
#else /* __KCC__ */
#define PLACE_IN_ROM_BANK0
#endif /* __KCC__ */

#endif /* SECTIONS_H */
