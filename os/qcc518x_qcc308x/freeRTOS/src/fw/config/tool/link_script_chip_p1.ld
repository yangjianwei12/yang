/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/* Link script for QCC517x and QCC307x Processor P1 */

ENTRY($reset)

$DATA_ROM_WINDOW        = 0x78000000;

INCLUDE link_script_chip_common.ld  /* common to P0 and P1 */
INCLUDE link_script_common.ld       /* common across chips */
