/*******************************************************************************

Copyright (C) 2010 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*! \brief Get public device address or static random address for a given
           resolvable random address.

    If DM is able to resolve the given random address with any of IRKs 
    present in paired device list from device database, corresponding public
    device address or static random address will be returned in public_addr.

    Note: To be used only on an established link.

    \param random_addr pointer to Transport Bluetooth Address structure to 
           get corresponding public device address or static random address.
    \param public_addr pointer to Transport Bluetooth Address structure (IAI) to
           initialise.
    \return TRUE  : If given random address was successfully resolved
            FALSE : Otherwise.
*/

bool_t dm_get_public_address(const TP_BD_ADDR_T * const random_addr,
                                TP_BD_ADDR_T * public_addr )
{
/* This trap is implemented for onchip and Crescendo (which is off-subsystem) */
#ifdef INSTALL_SM_MODULE
    if ((random_addr->tp_type == LE_ACL) && 
            TBDADDR_IS_PRIVATE_RESOLVABLE(random_addr->addrt))
    {
        TYPED_BD_ADDR_T* p_resolved_addr;
#ifdef SM_HAS_FUNCTION_FOR_AES
        /*
         * In onchip, we synchronously try to resolve against IRK present in the
         * paired device database.
         */
        if((p_resolved_addr = dm_sm_resolve_address(&random_addr->addrt))!=NULL)
#else
        /*
         * In offchip, we attempt to asynhronously resolve as part of connection
         * establishment. And since upper layers are instructed to invoke this
         * trap post connection establishment, its pointless to again make an
         * attempt to asynhronously resolve. Hence we just scan our resolved
         * list to figure out address resolution outcome.
         */
        if((p_resolved_addr = sm_get_resolved_addr(&random_addr->addrt)) !=NULL)
#endif
        {
            public_addr->tp_type = LE_ACL;
            tbdaddr_copy (&public_addr->addrt, p_resolved_addr);
            return TRUE;
        }
    }
#else
    QBL_UNUSED(random_addr);
    QBL_UNUSED(public_addr);
#endif
    return FALSE;
}

