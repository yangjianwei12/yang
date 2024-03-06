/******************************************************************************
 Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
  DESCRIPTION
     Header file providing mapping for Synergy A2DP Profile's constant 
     definitions.
******************************************************************************/
#ifndef SYNERGY_INC_AV_A2DP_H_
#define SYNERGY_INC_AV_A2DP_H_
#include "csr_bt_av_prim.h"
#include <handover_if.h>

#define A2DP_SERVICE_MEDIA_CODEC (uint8)CSR_BT_AV_SC_MEDIA_CODEC
#define A2DP_MEDIA_TYPE_AUDIO CSR_BT_AV_AUDIO
#define A2DP_MEDIA_CODEC_NONA2DP CSR_BT_AV_NON_A2DP_CODEC
#define A2DP_SERVICE_CONTENT_PROTECTION (uint8)CSR_BT_AV_SC_CONTENT_PROTECTION
#define A2DP_SERVICE_DELAY_REPORTING (uint8)CSR_BT_AV_SC_DELAY_REPORTING
#define A2DP_SERVICE_MEDIA_TRANSPORT (uint8)CSR_BT_AV_SC_MEDIA_TRANSPORT
#define A2DP_MEDIA_CODEC_SBC CSR_BT_AV_SBC
#define A2DP_MEDIA_CODEC_MPEG2_4_AAC CSR_BT_AV_MPEG24_AAC
#define A2DP_MEDIA_CODEC_NONA2DP CSR_BT_AV_NON_A2DP_CODEC
/*capabilities*/
/*! The APT Vendor ID.*/
#define A2DP_APT_VENDOR_ID                 (0x4f000000)
/*! The CSR apt-X Codec ID.*/
#define A2DP_CSR_APTX_CODEC_ID             (0x0100)
/*! The QTI aptX LL Codec ID. */
#define A2DP_QTI_APTX_LL_CODEC_ID          (0x0200)
/*! The CSR aptX ACL Sprint Codec ID.*/
#define A2DP_CSR_APTX_ACL_SPRINT_CODEC_ID  (0x0200)
/*! The QTI Vendor ID */
#define A2DP_QTI_VENDOR_ID                 (0xd7000000U)
/*! The QTI aptX-HD Codec ID.*/
#define A2DP_QTI_APTXHD_CODEC_ID           (0x2400)
/*! The QTI aptX Adaptive Codec ID.*/
#define A2DP_QTI_APTX_AD_CODEC_ID          (0xAD00)
/*! brief The QTI aptX TWS+ Codec ID.*/
#define A2DP_QTI_APTX_TWS_PLUS_CODEC_ID    (0x2500)
/*! The QTI SBC TWS+ Codec ID.*/
#define A2DP_QTI_SBC_TWS_PLUS_CODEC_ID     (0x2600)
/*! The CSR Vendor ID.*/
#define A2DP_CSR_VENDOR_ID                 (0x0a000000)
/*! The CSR Faststream Codec ID.*/
#define A2DP_CSR_FASTSTREAM_CODEC_ID            (0x0100)
/*! The CSR True Wireless Stereo v3 Codec ID for AAC.*/
#define A2DP_CSR_TWS_AAC_CODEC_ID          (0x0401)
/*! The CSR True Wireless Stereo v3 Codec ID for SBC.*/
#define A2DP_CSR_TWS_SBC_CODEC_ID               (0x0301)
/*!  The CSR True Wireless Stereo v3 Codec ID for AAC.*/
#define A2DP_CSR_TWS_AAC_CODEC_ID               (0x0401)
/*! The CSR True Wireless Stereo v3 Codec ID for MP3.*/
#define A2DP_CSR_TWS_MP3_CODEC_ID               (0x0501)
/*! The CSR True Wireless Stereo v3 Codec ID for AptX.*/
#define A2DP_CSR_TWS_APTX_CODEC_ID               (0x0601)
/*! The CSR True Wireless Stereo v3 Codec ID for aptX Adaptive.*/
#define A2DP_CSR_TWS_APTX_AD_CODEC_ID            (0x0701)
#define SBC_CONTENT_PROTECTION          4

/*! SCMS CP_TYPE value for the content protection capabilities (MSB).*/
#define A2DP_CP_TYPE_SCMS_MSB              (0x00)

/*! SCMS CP_TYPE value for the content protection capabilities (LSB).*/
#define A2DP_CP_TYPE_SCMS_LSB              (0x02)

#define A2DP_SEP_IS_AVAILABLE    (0)
#define A2DP_SEP_IS_UNAVAILABLE  (1)
#define A2DP_SEP_IS_IN_USE       (2)

#define A2DP_SEP_INDEX_INVALID       (0xFF)

/* A2DP generic service cababilities field offsets */
#define A2DP_SERVICE_CAPS_CATEGORY_OFFSET            (0x00)
#define A2DP_SERVICE_CAPS_LOSC_OFFSET                (0x01)

/* A2DP media codec cababilities field offsets */
#define A2DP_SERVICE_CAPS_MEDIA_TYPE_OFFSET          (0x02)
#define A2DP_SERVICE_CAPS_MEDIA_CODEC_TYPE_OFFSET    (0x03)

/* Stream Endpoint information */
#define A2DP_MEDIA_CODEC_UNDEFINED     (0xa5)

/* Assign Transaction Label */
#define A2DP_ASSIGN_TLABEL(theInst) ((uint8)(theInst->a2dp.tLabel++ % 16))

/*! \brief first instance is sink - fixing this makes the fetching task easy. */
#define AV_SINK_INSTANCE_INDEX (0)

#define AV_CONN_ID_INVALID       (0xFF)
#define AV_INSTANCE_SINK_INDEX   0
#define AV_PROFILE_DEFAULT_MTU_SIZE         (668)
/*!
    @brief Used to indicate an invalid AV Sync delay
*/
#define A2DP_INVALID_AV_SYNC_DELAY (0xFFFF)

/*! Number of SEPs */    
#define APP_A2DP_NUM_SEP        1

/*! Index for a2dp sink sep configuration*/
#define APP_A2DP_SEP_SINK       0

/*! Index for a2dp source sep configuration*/
#define APP_A2DP_SEP_SRC        1

#endif /*!SYNERGY_INC_AV_A2DP_H_*/

