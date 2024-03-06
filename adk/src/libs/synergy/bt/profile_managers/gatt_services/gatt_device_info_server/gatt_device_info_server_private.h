/* Copyright (c) 2014 - 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_DEVICE_INFO_SERVER_PRIVATE_H
#define GATT_DEVICE_INFO_SERVER_PRIVATE_H

#include "gatt_device_info_server.h"

/* Macros for creating messages */
#define MAKE_DEVICE_INFO_MESSAGE(TYPE) MESSAGE_MAKE(message,TYPE##_T);

/* Device Info Server expose only one instance on a device */
typedef struct __GDISS_T
{
    gdiss_t *deviceInfo;
} GDISS_T;

typedef struct DisInstanceListElement
{
    struct DisInstanceListElement    *next;
    struct DisInstanceListElement    *prev;
    CsrBtGattId                      gattId;
    gdiss_t                          *pDis;
} DisInstanceListElm_t;


#endif /* GATT_DEVICE_INFO_SERVER_PRIVATE_H */
