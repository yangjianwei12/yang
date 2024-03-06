/****************************************************************************
 * Copyright (c) 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \defgroup sco_fw Sco
 * \file  sco_plc100.c
 *
 * SCO framework source for NBS and WBS PLC
 *
 */

/****************************************************************************
Include Files
*/

#include "capabilities.h"
#include "sco_drv/sco_global.h"
#include "sco_plc100.h"

/****************************************************************************
Private Constant Definitions
*/


/****************************************************************************
Private Macro Declarations
*/

/****************************************************************************
Private Variable Definitions
*/

/****************************************************************************
Private Function Definitions
*/



/****************************************************************************
Public Function Definitions
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
                                unsigned plc_constants)
{
    PLC100_STRUC* plcstruct = xzpnew(PLC100_STRUC);
    if (plcstruct == NULL)
    {
        return NULL;
    }
    /* allocate speech buffer */
    plcstruct->speech_buf = xpnewn(sp_buf_len, int);
    if (plcstruct->speech_buf == NULL)
    {
        /* Free PLC structure and associated allocs */
        sco_plc100_free_data(plcstruct);
        return NULL;
    }
    plcstruct->speech_buf_start = plcstruct->speech_buf;

    /* allocate the ola buffer */
    plcstruct->ola_buf = xpnewn(ola_len, int);
    if (plcstruct->ola_buf == NULL)
    {
        /* Free PLC structure and associated allocs */
        sco_plc100_free_data(plcstruct);
        return NULL;
    }

    /* initialise the rest of the structure */
    plcstruct->consts = get_plc100_constants(plc_constants);
    if (plcstruct->consts == NULL)
    {
        /* Free PLC structure and associated allocs */
        sco_plc100_free_data(plcstruct);
        return NULL;
    }

    plcstruct->per_threshold = PLC100_PER_THRESHOLD;

    return plcstruct;
}

/**
 * \brief Initialize PLC data
 *
 * \param  plc100_struc  Pointer to the PLC100 data structure.
 */
void sco_plc100_initialize(PLC100_STRUC *plcstruct)
{
    /* reset PLC attenuation value and initialise PLC */
    if(plcstruct != NULL)
    {
        plcstruct->attenuation = PLC100_INITIAL_ATTENUATION;

        plc100_initialize(plcstruct);
    }

}

/**
 * \brief Delete all PLC data
 *
 * \param  plc100_struc  Pointer to the PLC100 data structure.
 */
void sco_plc100_free_data(PLC100_STRUC *plc100_struc)
{
    /* Free the PLC data if PLC was used */
    if(plc100_struc != NULL)
    {
        if (plc100_struc->ola_buf != NULL)
        {
            pdelete(plc100_struc->ola_buf);
        }
        if (plc100_struc->speech_buf_start != NULL)
        {
            pdelete(plc100_struc->speech_buf_start);
        }
        if (plc100_struc->consts != NULL)
        {
            release_plc100_constants(plc100_struc->consts);
        }
        pdelete(plc100_struc);
    }
}

/**
 * \brief Call the PLC processing function
 *
 * \param  force_plc_off Flag indicating PLC should not fake any data.
 * \param  plc100_struc  Pointer to the PLC100 data structure.
 * \param  bfi           Bad Frame Indicator.
 * \param  output_size   Number of Samples expected in output.
 */
void sco_plc100_call_plc(unsigned force_plc_off,
                         PLC100_STRUC *plc100_struc,
                         int bfi,
                         unsigned output_size)
{
    if (force_plc_off)
    {
        plc100_struc->bfi = OK;
    }
    else
    {
        plc100_struc->bfi = bfi;
    }
    plc100_struc->packet_len = output_size;

    /* Do packet loss concealment (PLC). */
    plc100_process(plc100_struc);

}
