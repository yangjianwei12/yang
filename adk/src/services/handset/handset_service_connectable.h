/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup handset_service
    @{
    \brief      Handset service connectable
*/

#ifndef HANDSET_SERVICE_CONNECTABLE_H_
#define HANDSET_SERVICE_CONNECTABLE_H_

#include "handset_service.h"

typedef enum
{
    handset_service_connectable_disable,
    handset_service_connectable_enable_fast,
    handset_service_connectable_enable_slow
} handset_service_connectable_t;

/*! \brief Register as observer for connect/disconnect indications from connection_manager */
void handsetService_ObserveConnections(void);

/*! \brief Un-register as observer for connect/disconnect indications from connection_manager */
void handsetService_DontObserveConnections(void);

/*! \brief Initialise connectable module */
void handsetService_ConnectableInit(void);

/*! \brief Enable BR/EDR connections */
void handsetService_ConnectableEnableBredr(handset_service_connectable_t setting);

/*! \brief Allow BR/EDR connections */
void handsetService_ConnectableAllowBredr(bool allow);

/*! \brief Handler for BT_DEVICE_EARBUD_CREATED_IND */
void HandsetService_HandleEarbudCreated(void);

/*! \brief Stop fast page scan */
void handsetService_StopFastPageScan(void);

/*! \brief Handle fast page scan timeout */
void handsetService_HandleFastPageScanTimeout(void);

/*! \brief Enable Truncated Page Scan */
void HandsetService_EnableTruncatedPageScan(void);

/*! \brief Disable Truncated Page Scan */
void HandsetService_DisableTruncatedPageScan(void);

/*! \brief Handle timed message to enable truncated page scan */
void HandsetService_HandleInternalTruncatedPageScanEnable(void);


#endif /* HANDSET_SERVICE_CONNECTABLE_H_ */
/*! @} */