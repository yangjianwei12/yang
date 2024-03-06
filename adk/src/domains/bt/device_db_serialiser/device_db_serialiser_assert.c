/*!
\copyright  Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Validation of device db.

*/

#include "device_db_serialiser_assert.h"
#include "device_db_serialiser_pdd_frame.h"

#include <byte_utils.h>
#include <device_list.h>
#include <bt_device.h>
#include <connection_no_ble.h>
#include <logging.h>
#include <panic.h>
#include <ps.h>
#include "device_types.h"

#ifdef USE_SYNERGY
#include "csr_bt_td_db.h"
#endif

#include <stdlib.h>

static bool deviceDbSerialiser_IsFirstPdduPayloadReadable(uint8 *pdd_frame)
{
    bool status = FALSE;
    uint8 *pddu_frame = NULL;

    if (DeviceDbSerialiser_IsValidPddFrame(pdd_frame))
    {
        pddu_frame = DeviceDbSerialiser_GetPddFramePayload(pdd_frame);
        status = DeviceDbSerialiser_IsValidPddFrame(pddu_frame);
    }

    return status;
}

inline static uint8 * deviceDbSerialiser_GetFirstPdduPayload(uint8 *pdd_frame)
{
    return DeviceDbSerialiser_GetPddFramePayload(DeviceDbSerialiser_GetPddFramePayload(pdd_frame));
}

inline static uint8 deviceDbSerialiser_GetDeviceType(uint8 *pdd_frame)
{
    return deviceDbSerialiser_GetFirstPdduPayload(pdd_frame)[3];
}

static uint16 deviceDbSerialiser_GetDeviceFlags(uint8 *pdd_frame)
{
    uint8 *payload = deviceDbSerialiser_GetFirstPdduPayload(pdd_frame);
    return MAKEWORD_HI_LO(payload[9], payload[10]);
}

inline static uint8 deviceDdSerialiser_GetFFIndex(uint8 *pdd_frame)
{
    return deviceDbSerialiser_GetFirstPdduPayload(pdd_frame)[30];
}

inline static uint8 deviceDdSerialiser_GetOddByteIndex(uint8 *pdd_frame)
{
    return deviceDbSerialiser_GetFirstPdduPayload(pdd_frame)[31];
}

bool DeviceDbSerialiser_IsPsStoreValid(void)
{
    uint8 type_cnt[DEVICE_TYPE_MAX] = {0};

    for (uint8 pdl_index = 0; pdl_index < DeviceList_GetMaxTrustedDevices(); pdl_index++)
    {
        typed_bdaddr taddr = {0};
        uint16 pdd_frame_length = ConnectionSmGetIndexedAttributeSizeNowReq(pdl_index);
        if(pdd_frame_length)
        {
            uint8 *pdd_frame = (uint8 *)PanicUnlessMalloc(pdd_frame_length);
            if(ConnectionSmGetIndexedAttributeNowReq(0, pdl_index, pdd_frame_length, pdd_frame, &taddr))
            {
                if (!deviceDbSerialiser_IsFirstPdduPayloadReadable(pdd_frame))
                {
                    free(pdd_frame);
                    return FALSE;
                }

                DEBUG_LOG_VERBOSE("DeviceDbSerialiser_IsPsStoreValid lap 0x%x, db: 0x%x, frame_len: 0x%x, type: 0x%x, flags: 0x%x, ff: 0x%x, odd byte: 0x%x",
                        taddr.addr.lap, DeviceDbSerialiser_GetPddFrameType(pdd_frame), DeviceDbSerialiser_GetPddFrameSize(pdd_frame), deviceDbSerialiser_GetDeviceType(pdd_frame),
                        deviceDbSerialiser_GetDeviceFlags(pdd_frame), deviceDdSerialiser_GetFFIndex(pdd_frame), deviceDdSerialiser_GetOddByteIndex(pdd_frame));

                if(deviceDbSerialiser_GetDeviceType(pdd_frame) < ARRAY_DIM(type_cnt))
                {
                    ++type_cnt[deviceDbSerialiser_GetDeviceType(pdd_frame)];
                }
                else
                {
                    free(pdd_frame);
                    return FALSE;
                }
            }
            free(pdd_frame);
        }
    }

    DEBUG_LOG_VERBOSE("DeviceDbSerialiser_IsPsStoreValid device type occurrences");
    DEBUG_LOG_DATA_VERBOSE(type_cnt, sizeof(type_cnt));

    /* - Only one device of type SELF or EARBUD is allowed to exist at a time.
     * - If a device of type EARBUD exists, a device of type SELF must also exist. */
    if (type_cnt[DEVICE_TYPE_EARBUD] > 1 || type_cnt[DEVICE_TYPE_SELF] > 1 ||
       (type_cnt[DEVICE_TYPE_EARBUD] > 0 && type_cnt[DEVICE_TYPE_SELF] == 0))
    {
        return FALSE;
    }

    return TRUE;
}

#define ATTRIBUTE_BASE_PSKEY_INDEX  100
#define GATT_ATTRIBUTE_BASE_PSKEY_INDEX  110
#define TDL_BASE_PSKEY_INDEX        142

#define TDL_INDEX_PSKEY             141
#define PSKEY_SM_KEY_STATE_IR_ER_INDEX 140

void DeviceDbSerialiser_HandleCorruption(void)
{
    DEBUG_LOG_ERROR("DeviceDbSerialiser_HandleCorruption");

#ifdef USE_SYNERGY
    /* Log the index. */
    uint8 count = BtDevice_GetMaxTrustedDevices();
    CsrBtTypedAddr *addrt = (CsrBtTypedAddr *) PanicUnlessMalloc(sizeof(*addrt) * count);

    count = CsrBtTdDbListDevices(count, addrt);

    DEBUG_LOG_ERROR("DeviceDbSerialiser_HandleCorruption no. of devices = %d", count);
    for (int i = 0; i < count; i++)
    {
        DEBUG_LOG_ERROR("DeviceDbSerialiser_HandleCorruption device lap 0x%x",
                        addrt[i].addr.lap);
    }
    free(addrt);

    /* Clear the Synergy trusted device database index key in PS without clearing other keys. */
    CsrBtTdDbDeleteAll(0);

#else

    for (int i=0; i < BtDevice_GetMaxTrustedDevices(); i++)
    {
        PsStore(ATTRIBUTE_BASE_PSKEY_INDEX+i, NULL, 0);
        PsStore(TDL_BASE_PSKEY_INDEX+i, NULL, 0);
        PsStore(GATT_ATTRIBUTE_BASE_PSKEY_INDEX+i, NULL, 0);
    }

    PsStore(TDL_INDEX_PSKEY, NULL, 0);
    PsStore(PSKEY_SM_KEY_STATE_IR_ER_INDEX, NULL, 0);
#endif

    Panic();
}
