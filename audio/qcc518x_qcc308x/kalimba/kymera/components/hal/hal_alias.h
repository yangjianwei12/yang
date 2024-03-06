/****************************************************************************
 * Copyright (c) 2011 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file hal_alias.h
 * \ingroup HAL
 *
 * Public header file for HAL functions.
 * Currently just initialisation
 * Likely to get split between functional areas later.
 */

/****************************************************************************
Include Files
*/

#include "hal_macros.h"
#include "io_defs.h"

#ifndef HAL_ALIAS_H
#define HAL_ALIAS_H

#define hal_get_reg_proc_pio_status() hal_get_reg_audio_sys_pio_status()
#define hal_get_reg_proc_pio_status2() hal_get_reg_audio_sys_pio_status2()

#ifdef CHIP_CRESCENDO
#define hal_set_audio_di_ref_micbias_en(x) hal_set_audio_ana_ref_micbias_en(x)
#define hal_get_audio_di_ref_micbias_en(x) hal_get_audio_ana_ref_micbias_en(x)
#define hal_set_reg_enable_fast_private_ram(x) hal_set_reg_enable_private_ram(x)
#define hal_set_audio_di_li1_selgain(x) hal_set_audio_ana_adc_ch1_gain_sel(x)
#define hal_set_audio_di_li2_selgain(x) hal_set_audio_ana_adc_ch2_gain_sel(x)
#endif /* CHIP_CRESCENDO */

#define hal_set_clkgen_audio_enables_pcm0_en(x) hal_set_clkgen_audio_pcm0_en(x)

/* Add macros for other PCM instances, if they exist */
#ifdef INSTALL_AUDIO_INTERFACE_PCM
#if NUMBER_PCM_INSTANCES > 1
#define hal_set_clkgen_audio_enables_pcm1_en(x) hal_set_clkgen_audio_pcm1_en(x)
#endif
#if NUMBER_PCM_INSTANCES > 2
#define hal_set_clkgen_audio_enables_pcm2_en(x) hal_set_clkgen_audio_pcm2_en(x)
#endif
#if NUMBER_PCM_INSTANCES > 3
#define hal_set_clkgen_audio_enables_pcm3_en(x) hal_set_clkgen_audio_epcm0_en(x)
#define hal_set_audio_enables_pcm3_en(x) hal_set_audio_enables_epcm0_en(x)
#define hal_set_audio_enables_pcm3_in_en(x) hal_set_audio_enables_epcm0_in_en(x)
#define hal_set_audio_enables_pcm3_out_en(x) hal_set_audio_enables_epcm0_out_en(x)
#define hal_get_audio_enables_pcm3_en()  hal_get_audio_enables_epcm0_en()
#define hal_get_audio_enables_pcm3_in_en()  hal_get_audio_enables_epcm0_in_en()
#define hal_get_audio_enables_pcm3_out_en()  hal_get_audio_enables_epcm0_out_en()
#endif
#endif /* INSTALL_AUDIO_INTERFACE_PCM */

#define  hal_set_reg_proc_pio_drive(x) hal_set_reg_audio_sys_proc_pio_drive(x)
#define  hal_get_reg_proc_pio_drive() hal_get_reg_audio_sys_proc_pio_drive()

#define  hal_set_reg_proc_pio_drive2(x) hal_set_reg_audio_sys_proc_pio_drive2(x)
#define  hal_get_reg_proc_pio_drive2() hal_get_reg_audio_sys_proc_pio_drive2()

#define  hal_set_reg_proc_pio_drive_enable(x) hal_set_reg_audio_sys_proc_pio_drive_enable(x)
#define  hal_get_reg_proc_pio_drive_enable() hal_get_reg_audio_sys_proc_pio_drive_enable()

#define  hal_set_reg_proc_pio_drive_enable2(x) hal_set_reg_audio_sys_proc_pio_drive_enable2(x)
#define  hal_get_reg_proc_pio_drive_enable2() hal_get_reg_audio_sys_proc_pio_drive_enable2()





#define CHIP_INT_SOURCE_SW0 INT_SOURCE_SW0

/* Some chips allow blocking access to a specific bus. The firmware does not
   make use of it. The bits are organized in such a way that the position of
   the bits from the secondary core can be calculated by shifting the position
   of the bits from the primary core. */
#if defined(DM_ARBITER_BLOCK_HAS_PER_BUS_PERMISSIONS)
#define FAST_DM_ARBITER_BLOCK_WR_CORE0_MASK (FAST_DM_ARBITER_BLOCK_WR_CORE0_DM1_MASK | \
                                             FAST_DM_ARBITER_BLOCK_WR_CORE0_DM2_MASK)
#define FAST_DM_ARBITER_BLOCK_WR_CORE0_SIZE (FAST_DM_ARBITER_BLOCK_WR_CORE1_DM1_POSN - \
                                             FAST_DM_ARBITER_BLOCK_WR_CORE0_DM1_POSN)
#define FAST_DM_ARBITER_BLOCK_ALL_MASK      (FAST_DM_ARBITER_BLOCK_RD_CORE0_DM1_MASK | \
                                             FAST_DM_ARBITER_BLOCK_RD_CORE0_DM2_MASK | \
                                             FAST_DM_ARBITER_BLOCK_RD_CORE1_DM1_MASK | \
                                             FAST_DM_ARBITER_BLOCK_RD_CORE1_DM2_MASK | \
                                             FAST_DM_ARBITER_BLOCK_RD_AUX_MASK       | \
                                             FAST_DM_ARBITER_BLOCK_WR_CORE0_DM1_MASK | \
                                             FAST_DM_ARBITER_BLOCK_WR_CORE0_DM2_MASK | \
                                             FAST_DM_ARBITER_BLOCK_WR_CORE1_DM1_MASK | \
                                             FAST_DM_ARBITER_BLOCK_WR_CORE1_DM2_MASK | \
                                             FAST_DM_ARBITER_BLOCK_WR_AUX_MASK)
#else
#define FAST_DM_ARBITER_BLOCK_WR_CORE0_SIZE (FAST_DM_ARBITER_BLOCK_WR_CORE1_POSN - \
                                             FAST_DM_ARBITER_BLOCK_WR_CORE0_POSN)
#define FAST_DM_ARBITER_BLOCK_ALL_MASK      (FAST_DM_ARBITER_BLOCK_RD_CORE0_MASK | \
                                             FAST_DM_ARBITER_BLOCK_RD_CORE1_MASK | \
                                             FAST_DM_ARBITER_BLOCK_WR_CORE0_MASK | \
                                             FAST_DM_ARBITER_BLOCK_WR_CORE1_MASK | \
                                             FAST_DM_ARBITER_BLOCK_AUX_MASK)

#endif
#define FAST_DM_ARBITER_OWNER_MASK ((1 << (FAST_DM_BANKX_CONFIG_FAST_DM_ARBITER_OWNER_MSB_POSN + 1)) - 1)

#if !defined(PM_BANKX_BLOCK_CACHE_MASK)
#define PM_BANKX_BLOCK_CACHE_MASK (PM_BANKX_BLOCK_RD_CACHE_MASK | \
                                   PM_BANKX_BLOCK_WR_CACHE_MASK)
#endif
#if !defined(PM_BANKX_BLOCK_CORE0_MASK)
#define PM_BANKX_BLOCK_CORE0_MASK (PM_BANKX_BLOCK_RD_CORE0_MASK | \
                                   PM_BANKX_BLOCK_WR_CORE0_MASK)
#endif
#if !defined(PM_BANKX_BLOCK_CORE1_MASK)
#define PM_BANKX_BLOCK_CORE1_MASK (PM_BANKX_BLOCK_RD_CORE1_MASK | \
                                   PM_BANKX_BLOCK_WR_CORE1_MASK)
#endif

#define PM_BANKX_BLOCK_ALL_MASK (PM_BANKX_BLOCK_CORE0_MASK | \
                                 PM_BANKX_BLOCK_CORE1_MASK | \
                                 PM_BANKX_BLOCK_CACHE_MASK )
							

/* Chips with less that 32 DM banks have the name of the controls differ from larger
   chips. Alias them for readability. */
#if defined(hal_set_reg_dm_awake_ls_ctrl)
#define hal_set_reg_dm_awake_ls_ctrl_bnk0_to_bnk31(x) hal_set_reg_dm_awake_ls_ctrl(x)
#endif
#if defined(hal_set_reg_dm_asleep_ls_ctrl)
#define hal_set_reg_dm_asleep_ls_ctrl_bnk0_to_bnk31(x) hal_set_reg_dm_asleep_ls_ctrl(x)
#endif
#if defined(hal_set_reg_dm_awake_ds_ctrl)
#define hal_set_reg_dm_awake_ds_ctrl_bnk0_to_bnk31(x) hal_set_reg_dm_awake_ds_ctrl(x)
#endif
#if defined(hal_set_reg_dm_asleep_ds_ctrl)
#define hal_set_reg_dm_asleep_ds_ctrl_bnk0_to_bnk31(x) hal_set_reg_dm_asleep_ds_ctrl(x)
#endif

#endif /* HAL_ALIAS_H */
