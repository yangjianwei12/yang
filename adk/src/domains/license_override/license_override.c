/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\ingroup    license_override
\brief      Implementation of API to override licenses for certain platforms
@{
*/

#include "license_override.h"

#include "bt_device.h"

#include <connection_no_ble.h>
#include <logging.h>
#include <vm.h>

static feature_license_key_override_mapping_t * local_feature_license_key_override_mapping = NULL;
static uint8 local_feature_license_key_override_mapping_size = 0;

static void licenseOverride_Handler(Task task, MessageId id, Message message);
static TaskData earbud_license_override_task = { .handler = licenseOverride_Handler };

static void licenseOverride_Override(bdaddr * my_bdaddr)
{
    unsigned i;

    for(i=0; i<local_feature_license_key_override_mapping_size; i++)
    {
        bool my_bdaddr_in_override_range = (my_bdaddr->nap >= local_feature_license_key_override_mapping[i].min_addr.nap) &&
                                           (my_bdaddr->nap <= local_feature_license_key_override_mapping[i].max_addr.nap) &&
                                           (my_bdaddr->uap >= local_feature_license_key_override_mapping[i].min_addr.uap) &&
                                           (my_bdaddr->uap <= local_feature_license_key_override_mapping[i].max_addr.uap) &&
                                           (my_bdaddr->lap >= local_feature_license_key_override_mapping[i].min_addr.lap) &&
                                           (my_bdaddr->lap <= local_feature_license_key_override_mapping[i].max_addr.lap);

        if(my_bdaddr_in_override_range)
        {
            DEBUG_LOG_INFO("licenseOverride_Override, overriding license");
            DEBUG_LOG_DATA_INFO(local_feature_license_key_override_mapping[i].license_key_override, FEATURE_LICENSE_KEY_SIZE);
            if(!VmSetLicenseKey(local_feature_license_key_override_mapping[i].license_key_override, FEATURE_LICENSE_KEY_SIZE))
            {
                DEBUG_LOG_ALWAYS("licenseOverride_Override, failed to override");
            }

            free(local_feature_license_key_override_mapping);
            local_feature_license_key_override_mapping = NULL;
            local_feature_license_key_override_mapping_size = 0;
            return;
        }
    }

    DEBUG_LOG_INFO("licenseOverride_Override, no mapping found");
    free(local_feature_license_key_override_mapping);
    local_feature_license_key_override_mapping = NULL;
    local_feature_license_key_override_mapping_size = 0;
}

static void licenseOverride_Handler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(id);
    
    bdaddr my_bdaddr = {0};

#ifdef USE_SYNERGY
    PanicFalse(*(CsrBtCmPrim *) message == CSR_BT_CM_READ_LOCAL_BD_ADDR_CFM);
    CsrBtCmReadLocalBdAddrCfm * cfm = (CsrBtCmReadLocalBdAddrCfm *) message;
    BdaddrConvertBluestackToVm(&my_bdaddr, &cfm->deviceAddr);
#else
    CL_DM_LOCAL_BD_ADDR_CFM_T * cfm = (CL_DM_LOCAL_BD_ADDR_CFM_T *)message;
    BdaddrConvertBluestackToVm(&my_bdaddr, &cfm->bd_addr);
#endif

    licenseOverride_Override(&my_bdaddr);
}

void LicenseOverride_Override(feature_license_key_override_mapping_t * mapping, uint8 mapping_size)
{
    if(mapping && mapping_size)
    {
        local_feature_license_key_override_mapping = (feature_license_key_override_mapping_t *)PanicUnlessMalloc(mapping_size * sizeof(feature_license_key_override_mapping_t));
        memcpy(local_feature_license_key_override_mapping, mapping, mapping_size * sizeof(feature_license_key_override_mapping_t));
        local_feature_license_key_override_mapping_size = mapping_size;
        ConnectionReadLocalAddr(&earbud_license_override_task);
    }
    else
    {
        DEBUG_LOG_WARN("LicenseOverride_Override, invalid mapping");
    }
}
/*! @}*/
