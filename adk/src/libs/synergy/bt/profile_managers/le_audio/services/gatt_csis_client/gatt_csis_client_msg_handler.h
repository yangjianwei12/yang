/* Copyright (c) 2019 - 2022 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_CSIS_CLIENT_MSG_HANDLER_H
#define GATT_CSIS_CLIENT_MSG_HANDLER_H


/***************************************************************************
NAME
    sendCsisClientInitCfm

DESCRIPTION
   Utility function to send init confirmation to application  
*/
void sendCsisClientInitCfm(GCsisC *const csisClient,
                                           const GattCsisClientStatus status,
                                           bool sizeSupport,
                                           bool lockSupport);

/***************************************************************************
NAME
    sendCsisClientTerminateCfm

DESCRIPTION
   Utility function to send terminate confirmation to application  
*/

void sendCsisClientTerminateCfm(GCsisC *const csisClient,
                                                   const GattCsisClientStatus status,
                                                   connection_id_t cid);

#endif /* GATT_CSIS_CLIENT_MSG_HANDLER_H */

