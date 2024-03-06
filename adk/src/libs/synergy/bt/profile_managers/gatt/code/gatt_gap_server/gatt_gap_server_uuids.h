/*******************************************************************************
Copyright (c) 2015-2023 Qualcomm Technologies International, Ltd.
 %%version
*******************************************************************************/

#ifndef __GATT_GAP_SERVER_UUIDS_H__
#define __GATT_GAP_SERVER_UUIDS_H__

#define UUID_GAP                                       0x1800
#define UUID_DEVICE_NAME                               0x2A00
#define UUID_APPEARANCE                                0x2A01
#define UUID_GATT_SECURITY_LEVELS                      0x2BF5

/*--------------------------------------------------------------------------
 * Defines for the LE GATT Security Levels charactertic.
 * The Attribute Value is a sequence of Security Level Requirements, each with
 * the type uint8[2]. Each Security Level Requirement consists of a
 * Security Mode field followed by a Security Level field.
 *--------------------------------------------------------------------------*/
#define LE_MODE1_LEVEL1  0x01,0x01
#define LE_MODE1_LEVEL2  0x01,0x02
#define LE_MODE1_LEVEL3  0x01,0x03
#define LE_MODE1_LEVEL4  0x01,0x04

#define LE_MODE2_LEVEL1  0x02,0x01
#define LE_MODE2_LEVEL2  0x02,0x02

#define LE_MODE3_LEVEL1  0x03,0x01
#define LE_MODE3_LEVEL2  0x03,0x02
#define LE_MODE3_LEVEL3  0x03,0x03

#define LE_GATT_SECURITY_LEVELS_VALUE                  [LE_MODE1_LEVEL3]


#endif /* __GATT_GAP_SERVER_UUIDS_H__ */
