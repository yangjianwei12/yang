#ifndef CSR_BT_PB_UTIL_H__
#define CSR_BT_PB_UTIL_H__
/******************************************************************************
 Copyright (c) 2008-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "csr_synergy.h"

#ifdef __cplusplus
extern "C" {
#endif

/* == GLOBAL TYPES ========================================================== */
typedef CsrUint32 PbAppParFlag;

/* Parameter Header Tag IDs */
#define CSR_BT_OBEX_PB_ORDER_ID             (0x01)
#define CSR_BT_OBEX_PB_SEARCH_VAL_ID        (0x02)
#define CSR_BT_OBEX_PB_SEARCH_ATT_ID        (0x03)
#define CSR_BT_OBEX_PB_MAX_LST_CNT_ID       (0x04)
#define CSR_BT_OBEX_PB_LST_START_OFF_ID     (0x05)
#define CSR_BT_OBEX_PB_PROP_SEL_ID          (0x06)
#define CSR_BT_OBEX_PB_FORMAT_ID            (0x07)
#define CSR_BT_OBEX_PB_PHONEBOOK_SIZE_ID    (0x08)
#define CSR_BT_OBEX_PB_MISSED_CALLS_ID      (0x09)
#define CSR_BT_OBEX_PB_PRIM_VER_ID          (0x0A)
#define CSR_BT_OBEX_PB_SEC_VER_ID           (0x0B)
#define CSR_BT_OBEX_PB_VCARD_SEL_ID         (0x0C)
#define CSR_BT_OBEX_PB_DATABASE_ID_ID       (0x0D)
#define CSR_BT_OBEX_PB_VCARD_SEL_OP_ID      (0x0E)
#define CSR_BT_OBEX_PB_RST_MISSED_CALL_ID   (0x0F)
#define CSR_BT_OBEX_PB_SUPP_FEATURES_ID     (0x10)

#define CSR_BT_OBEX_PB_LAST_ID              (0x10)

#define CSR_BT_OBEX_PB_ID_TO_FLAG(_id)      (PbAppParFlag)(1 << _id)

/* == GLOBAL DEFINES ======================================================== */
#define CSR_BT_OBEX_PB_ORDER_FLAG           CSR_BT_OBEX_PB_ID_TO_FLAG(CSR_BT_OBEX_PB_ORDER_ID)
#define CSR_BT_OBEX_PB_SEARCH_VAL_FLAG      CSR_BT_OBEX_PB_ID_TO_FLAG(CSR_BT_OBEX_PB_SEARCH_VAL_ID)
#define CSR_BT_OBEX_PB_SEARCH_ATT_FLAG      CSR_BT_OBEX_PB_ID_TO_FLAG(CSR_BT_OBEX_PB_SEARCH_ATT_ID)
#define CSR_BT_OBEX_PB_MAX_LST_CNT_FLAG     CSR_BT_OBEX_PB_ID_TO_FLAG(CSR_BT_OBEX_PB_MAX_LST_CNT_ID)
#define CSR_BT_OBEX_PB_LST_START_OFF_FLAG   CSR_BT_OBEX_PB_ID_TO_FLAG(CSR_BT_OBEX_PB_LST_START_OFF_ID)
#define CSR_BT_OBEX_PB_PROP_SEL_FLAG        CSR_BT_OBEX_PB_ID_TO_FLAG(CSR_BT_OBEX_PB_PROP_SEL_ID)
#define CSR_BT_OBEX_PB_FORMAT_FLAG          CSR_BT_OBEX_PB_ID_TO_FLAG(CSR_BT_OBEX_PB_FORMAT_ID)
#define CSR_BT_OBEX_PB_PHONEBOOK_SIZE_FLAG  CSR_BT_OBEX_PB_ID_TO_FLAG(CSR_BT_OBEX_PB_PHONEBOOK_SIZE_ID)
#define CSR_BT_OBEX_PB_MISSED_CALLS_FLAG    CSR_BT_OBEX_PB_ID_TO_FLAG(CSR_BT_OBEX_PB_MISSED_CALLS_ID)
#define CSR_BT_OBEX_PB_PRIM_VER_FLAG        CSR_BT_OBEX_PB_ID_TO_FLAG(CSR_BT_OBEX_PB_PRIM_VER_ID)
#define CSR_BT_OBEX_PB_SEC_VER_FLAG         CSR_BT_OBEX_PB_ID_TO_FLAG(CSR_BT_OBEX_PB_SEC_VER_ID)
#define CSR_BT_OBEX_PB_VCARD_SEL_FLAG       CSR_BT_OBEX_PB_ID_TO_FLAG(CSR_BT_OBEX_PB_VCARD_SEL_ID)
#define CSR_BT_OBEX_PB_DATABASE_ID_FLAG     CSR_BT_OBEX_PB_ID_TO_FLAG(CSR_BT_OBEX_PB_DATABASE_ID_ID)
#define CSR_BT_OBEX_PB_VCARD_SEL_OP_FLAG    CSR_BT_OBEX_PB_ID_TO_FLAG(CSR_BT_OBEX_PB_VCARD_SEL_OP_ID)
#define CSR_BT_OBEX_PB_RST_MISSED_CALL_FLAG CSR_BT_OBEX_PB_ID_TO_FLAG(CSR_BT_OBEX_PB_RST_MISSED_CALL_ID)
#define CSR_BT_OBEX_PB_SUPP_FEATURES_FLAG   CSR_BT_OBEX_PB_ID_TO_FLAG(CSR_BT_OBEX_PB_SUPP_FEATURES_ID)


/* Header Tag size */
#define CSR_BT_OBEX_PB_TAG_SIZE                 (2)

/* Header value lengths */
#define CSR_BT_OBEX_PB_ORDER_VAL_LEN            (1)
#define CSR_BT_OBEX_PB_SEARCH_VAL_LEN           (0) /* Variable length */
#define CSR_BT_OBEX_PB_SEARCH_ATT_VAL_LEN       (1)
#define CSR_BT_OBEX_PB_MAX_LST_CNT_VAL_LEN      (2)
#define CSR_BT_OBEX_PB_LST_START_OFF_VAL_LEN    (2)
#define CSR_BT_OBEX_PB_PROP_SEL_VAL_LEN         (8)
#define CSR_BT_OBEX_PB_FORMAT_VAL_LEN           (1)
#define CSR_BT_OBEX_PB_PHONEBOOK_SIZE_VAL_LEN   (2)
#define CSR_BT_OBEX_PB_MISSED_CALLS_VAL_LEN     (1)
#define CSR_BT_OBEX_PB_PRIM_VER_VAL_LEN         (16)
#define CSR_BT_OBEX_PB_SEC_VER_VAL_LEN          (16)
#define CSR_BT_OBEX_PB_VCARD_SEL_VAL_LEN        (8)
#define CSR_BT_OBEX_PB_DATABASE_ID_VAL_LEN      (16)
#define CSR_BT_OBEX_PB_VCARD_SEL_OP_VAL_LEN     (1)
#define CSR_BT_OBEX_PB_RST_MISSED_CALL_VAL_LEN  (1)
#define CSR_BT_OBEX_PB_SUPP_FEATURES_VAL_LEN    (4)

/* Header lengths */
#define CSR_BT_OBEX_PB_ORDER_HDR_LEN            (CSR_BT_OBEX_PB_TAG_SIZE + \
                                                 CSR_BT_OBEX_PB_ORDER_VAL_LEN)
#define CSR_BT_OBEX_PB_SEARCH_VAL_HDR_LEN       (CSR_BT_OBEX_PB_TAG_SIZE + \
                                                 CSR_BT_OBEX_PB_SEARCH_VAL_LEN)
#define CSR_BT_OBEX_PB_SEARCH_ATT_HDR_LEN       (CSR_BT_OBEX_PB_TAG_SIZE + \
                                                 CSR_BT_OBEX_PB_SEARCH_ATT_VAL_LEN)
#define CSR_BT_OBEX_PB_MAX_LST_CNT_HDR_LEN      (CSR_BT_OBEX_PB_TAG_SIZE + \
                                                 CSR_BT_OBEX_PB_MAX_LST_CNT_VAL_LEN)
#define CSR_BT_OBEX_PB_LST_START_OFF_HDR_LEN    (CSR_BT_OBEX_PB_TAG_SIZE + \
                                                 CSR_BT_OBEX_PB_LST_START_OFF_VAL_LEN)
#define CSR_BT_OBEX_PB_PROP_SEL_HDR_LEN         (CSR_BT_OBEX_PB_TAG_SIZE + \
                                                 CSR_BT_OBEX_PB_PROP_SEL_VAL_LEN)
#define CSR_BT_OBEX_PB_FORMAT_HDR_LEN           (CSR_BT_OBEX_PB_TAG_SIZE + \
                                                 CSR_BT_OBEX_PB_FORMAT_VAL_LEN)
#define CSR_BT_OBEX_PB_PHONEBOOK_SIZE_HDR_LEN   (CSR_BT_OBEX_PB_TAG_SIZE + \
                                                 CSR_BT_OBEX_PB_PHONEBOOK_SIZE_VAL_LEN)
#define CSR_BT_OBEX_PB_MISSED_CALLS_HDR_LEN     (CSR_BT_OBEX_PB_TAG_SIZE + \
                                                 CSR_BT_OBEX_PB_MISSED_CALLS_VAL_LEN)
#define CSR_BT_OBEX_PB_PRIM_VER_HDR_LEN         (CSR_BT_OBEX_PB_TAG_SIZE + \
                                                 CSR_BT_OBEX_PB_PRIM_VER_VAL_LEN)
#define CSR_BT_OBEX_PB_SEC_VER_HDR_LEN          (CSR_BT_OBEX_PB_TAG_SIZE + \
                                                 CSR_BT_OBEX_PB_SEC_VER_VAL_LEN)
#define CSR_BT_OBEX_PB_VCARD_SEL_HDR_LEN        (CSR_BT_OBEX_PB_TAG_SIZE + \
                                                 CSR_BT_OBEX_PB_VCARD_SEL_VAL_LEN)
#define CSR_BT_OBEX_PB_DATABASE_ID_HDR_LEN      (CSR_BT_OBEX_PB_TAG_SIZE + \
                                                 CSR_BT_OBEX_PB_DATABASE_ID_VAL_LEN)
#define CSR_BT_OBEX_PB_VCARD_SEL_OP_HDR_LEN     (CSR_BT_OBEX_PB_TAG_SIZE + \
                                                 CSR_BT_OBEX_PB_VCARD_SEL_OP_VAL_LEN)
#define CSR_BT_OBEX_PB_RST_MISSED_CALL_HDR_LEN  (CSR_BT_OBEX_PB_TAG_SIZE + \
                                                 CSR_BT_OBEX_PB_RST_MISSED_CALL_VAL_LEN)
#define CSR_BT_OBEX_PB_SUPP_FEATURES_HDR_LEN    (CSR_BT_OBEX_PB_TAG_SIZE + \
                                                 CSR_BT_OBEX_PB_SUPP_FEATURES_LEN)

#define CSR_BT_OBEX_TYPE_PB_PHONEBOOK           "x-bt/phonebook"

#define CSR_BT_OBEX_TYPE_PB_LISTING             "x-bt/vcard-listing"

#define CSR_BT_OBEX_TYPE_PB_VCARD               "x-bt/vcard"


#define CSR_BT_OBEX_TARGET_PB_UUID { \
    0x79, 0x61, 0x35, 0xF0, \
    0xF0, 0xC5, 0x11, 0xD8, \
    0x09, 0x66, 0x08, 0x00, \
    0x20, 0x0C, 0x9A, 0x66 }

/* == GLOBAL PROTOTYPES =========================================================== */
extern void CsrBtPbReverseCopy(CsrUint8* dest, CsrUint8* src, CsrUint8 size);
extern CsrUint8 CsrBtPbHeaderValSize(CsrUint8 header);
extern CsrUint16 CsrBtPbApplicationHeaderLen(PbAppParFlag flag);
extern CsrUint16 CsrBtPbPutHeaderParam(CsrUint8 *buf,
                                       CsrUint8 header,
                                       CsrUint8 *pHeaderVal);

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_PB_UTIL_H__ */

