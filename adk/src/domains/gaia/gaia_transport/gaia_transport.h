/*!
   \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
               All Rights Reserved.
               Qualcomm Technologies International, Ltd. Confidential and Proprietary.
   \version    
   \file       gaia_transport.h
   \defgroup   gaia_transport GAIA Transport
   @{
      \ingroup gaia_domain
      \brief   Definitions for APIs relating to GAIA Transport
*/

#ifndef GAIA_TRANSPORT_H
#define GAIA_TRANSPORT_H

#include <gaia.h>

#ifdef INCLUDE_ACCESSORY
#include <gaia_transport_accessory.h>
#endif

/* TODO: Move into gaia_transport_private.h */
#define HIGH(x) (x >> 8)
#define LOW(x) (x & 0xFF)
#define W16(x) (((*(x)) << 8) | (*((x) + 1)))


void GaiaTransport_TestInit(void);
void GaiaTransport_RfcommInit(void);
void GaiaTransport_GattInit(void);

#endif // GAIA_TRANSPORT_H

/*! @} */