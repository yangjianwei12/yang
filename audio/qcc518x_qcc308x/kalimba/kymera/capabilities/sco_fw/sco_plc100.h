/****************************************************************************
 * Copyright (c) 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup sco_fw Sco
 * \file  plc100_sco_fw.h
 *
 * SCO framework header for NBS and WBS PLC
 *
 */

#ifndef SCO_PLC100_H
#define SCO_PLC100_H

/****************************************************************************
Include Files
*/
#include "plc100_c.h"

/****************************************************************************
Public Function Declarations
*/

/**
 * \brief Create PLC data structure
 *
 * \param  sp_buf_len     Length of the SP buffer.
 * \param  ola_len        Length of the OLA buffer.
 * \param  plc_constants  Plc data offset in the shared constants table.
 *
 * \return Pointer to the created struct.
 */
PLC100_STRUC* sco_plc100_create(unsigned sp_buf_len,
                                unsigned ola_len,
                                unsigned plc_constants);

/**
 * \brief Initialize PLC data
 *
 * \param  plc100_struc  Pointer to the PLC100 data structure.
 */
void sco_plc100_initialize(PLC100_STRUC *plcstruct);

/**
 * \brief Delete all PLC data
 *
 * \param  plc100_struc  Pointer to the PLC100 data structure.
 */
void sco_plc100_free_data(PLC100_STRUC *plc100_struc);

void sco_plc100_call_plc(unsigned force_plc_off,
                         PLC100_STRUC *plc100_struc,
                         int bfi,
                         unsigned output_size);

#endif /* SCO_PLC100_H */
