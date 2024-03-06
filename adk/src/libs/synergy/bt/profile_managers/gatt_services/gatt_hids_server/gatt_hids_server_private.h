/*******************************************************************************

Copyright (C) 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef GATT_HIDS_SERVER_PRIVATE_H
#define GATT_HIDS_SERVER_PRIVATE_H

#include "csr_list.h"
#include "gatt_hids_server.h"
#include "gatt_hids_server_db.h"
#include "gatt_hids_server_debug.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_lib.h"
#include "csr_bt_bluestack_types.h"
#include "csr_bt_core_stack_pmalloc.h"


/* Macros for creating messages */
#define MAKE_HIDS_SERVER_MESSAGE(TYPE) TYPE##_T* message = (TYPE##_T *)CsrPmemAlloc(sizeof(TYPE##_T))

#define GATT_HIDS_SERVER_CCC_NOTIFY                   (0x01)
#define GATT_HIDS_SERVER_CCC_INDICATE                 (0x02)

#define GATT_HIDS_CLIENT_CONFIG_MASK (GATT_HIDS_SERVER_CCC_NOTIFY | GATT_HIDS_SERVER_CCC_INDICATE)
#define GET_HIDS_CLIENT_CONFIG(config)          (config & GATT_HIDS_CLIENT_CONFIG_MASK )

#define FIND_HIDS_SERVER_INST_BY_PROFILE_HANDLE(_Handle) \
                              (HIDS *)ServiceHandleGetInstanceData(_Handle)

#define HidsMessageSend(TASK, ID, MSG) {\
    MSG->id = ID; \
    CsrSchedMessagePut(TASK, HIDS_SERVER_PRIM, MSG);\
    }
/* Two Input and three Feature */
#define MAX_NO_REPORT   5
#define MAX_NO_FEATURE_REPORT 3
#define MAX_NO_INPUT_REPORT   2

/*! @brief Definition of data required for association.
 */
typedef struct
{
    GattHidsServerInputReport inputReport[MAX_NO_INPUT_REPORT];
    GattHidsServerFeatureReport featureReport[MAX_NO_FEATURE_REPORT]; 
    GattHidsServerHidInformation hidInformation;
    uint8 controlPoint;
    GattHidsServerReportMap *reportMap;
    CsrCmnList_t             connectedClients;

    /* Maximum two Input Report and three Feature Report*/
    ReportMapData RMapData[MAX_NO_REPORT];
    /* Usages are Local scope items (see Figure 3-1) that provide information to application
    developers about the intended meaning and use of an Item declared in report map */
    uint8 usagePage;

    /* Total number of reports we will receieve. we get these info from report map */
    uint8 numberOfInputReport;
    uint8 numberOfFeatureReport;
    uint8 totalNumberOfReport;
} HidsData;

typedef struct __GHIDS
{
    AppTaskData libTask;
    AppTask appTask;
    CsrBtGattId gattId;

    ServiceHandle srvcHandle;
    uint16 startHandle;
    uint16 endHandle;

    HidsData data;
} GHIDS;

/* Connected client linked list structures */
typedef struct HidsClientDataElementTag
{
    struct HidsClientDataElementTag *next;
    struct HidsClientDataElementTag *prev;
    GattHidsServerClientData              clientData;
} HidsClientDataElement;

#define HIDS_ADD_CLIENT(_List) \
    (HidsClientDataElement *)CsrCmnListElementAddLast(&(_List), \
                                                        sizeof(HidsClientDataElement))

#define HIDS_REMOVE_CLIENT(_List,_Elem) \
    (CsrCmnListElementRemove((CsrCmnList_t *)&(_List), \
                             (CsrCmnListElm_t *)(_Elem)))

void hidsServerRespondToCharacteristicRead(CsrBtGattDbAccessReadInd *readInd, uint8 *value, uint16 valueLength, CsrBtGattId gattId);
#endif /* GATT_HIDS_SERVER_PRIVATE_H */

