/****************************************************************************
Copyright (c) 2004 - 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    a2dp.h

DESCRIPTION
    Header file for the A2DP profile library. The library exposes a functional
    downstream API and an upstream message based API.

*/

/*!
\defgroup a2dp a2dp
\ingroup vm_libs

\brief  Interface to the Advanced Audio Distribution Profile library.

\section a2dp_intro INTRODUCTION
        When a device wishes to start streaming audio content, the device must
        first set up a streaming connection.  During the stream setup, the
        devices select the most suitable audio streaming parameters.
        Application service capability and transport service capability are
        configured during this stream setup procedure.

        Once a streaming connection is established and the start streaming
        procedure is executed, audio can be streamed from the Source (SRC)
        to the Sink (SNK).

        This library provides the low level services to permit an audio stream
        to be configured, started, stopped and suspended.  This library
        provides the stream configuration and control.  The actual streaming of
        data is performed by the underlying firmware.  The audio stream is
        routed to the Digital Signal Processor (DSP) present on CSR BlueCore
        Multimedia devices.  The CPU intensive operation of encoding/decoding a
        media stream is performed by the DSP.

        The library exposes a functional downstream API and an upstream message
        based API.

@{

*/


#ifndef A2DP_H_
#define A2DP_H_


#include <connection.h>
#include <library.h>

#include "handover_if.h"

/*!
    @name Service_Categories

    These are service categories to be used in service capabilities of a Stream
    End Point (SEP).

*/

/*!
    @brief The capability to stream media. This is manditory for the Advance Audio
    Distribution Profile.
*/
#define AVDTP_SERVICE_MEDIA_TRANSPORT       (1)
/*!
    @brief The reporting capability. This is not currently supported.
*/
#define AVDTP_SERVICE_REPORTING             (2)
/*!
    @brief The recovery capability. This is not currently supported.
*/
#define AVDTP_SERVICE_RECOVERY              (3)
/*!
    @brief The content protection capability.
*/
#define AVDTP_SERVICE_CONTENT_PROTECTION    (4)
/*!
    @brief The header compression capability. This is not currently supported.
*/
#define AVDTP_SERVICE_HEADER_COMPRESSION    (5)
/*!
    @brief The multiplexing capability. This is not currently supported.
*/
#define AVDTP_SERVICE_MULTIPLEXING          (6)
/*!
    @brief The codec capability for the Stream End Point.
*/
#define AVDTP_SERVICE_MEDIA_CODEC           (7)
/*!
    @brief The Av Sync delay reporting capability for the Stream End Point.
*/
#define AVDTP_SERVICE_DELAY_REPORTING       (8)


/*!
    @name Service information.

    Used to fill out the fields in a media codec capabilities structure.
*/

/*!
    @brief Defines the codec type as audio.
*/
#define AVDTP_MEDIA_TYPE_AUDIO              (0)
/*!
    @brief Defines the codec type as video.
*/
#define AVDTP_MEDIA_TYPE_VIDEO              (1)
/*!
    @brief Defines the codec type as multimedia.
*/
#define AVDTP_MEDIA_TYPE_MULTIMEDIA         (2)
/*!
    @brief Defines the codec as SBC. Manditory to support for A2DP.
*/
#define AVDTP_MEDIA_CODEC_SBC               (0)
/*!
    @brief Defines the codec as MPEG1/2. Optional to support for A2DP.
*/
#define AVDTP_MEDIA_CODEC_MPEG1_2_AUDIO     (1)
/*!
    @brief Defines the codec as AAC. Optional to support for A2DP.
*/
#define AVDTP_MEDIA_CODEC_MPEG2_4_AAC       (2)
/*!
    @brief Defines the codec as ATRAC. Optional to support for A2DP.
*/
#define AVDTP_MEDIA_CODEC_ATRAC             (4)
/*!
    @brief Defines a codec not supported in the A2DP profile.
*/
#define AVDTP_MEDIA_CODEC_NONA2DP           (0xff)
/*!
     @brief SCMS CP_TYPE value for the content protection capabilities (LSB).
*/
#define AVDTP_CP_TYPE_SCMS_LSB              (0x02)
/*!
     @brief SCMS CP_TYPE value for the content protection capabilities (MSB).
*/
#define AVDTP_CP_TYPE_SCMS_MSB              (0x00)




/*!
    @name Role defines when initialising the A2DP library. A device could register Source and Sink service records.
*/

/*!
    @brief Bit to indicate that the device supports the Sink role. Use for the role in A2dpInit to register the default service record.
*/
#define A2DP_INIT_ROLE_SOURCE                   (1)
/*!
    @brief Bit to indicate that the device supports the Source role. Use for the role in A2dpInit to register the default service record.
*/
#define A2DP_INIT_ROLE_SINK                     (2)


/*!
    @name CSR Faststream IDs.
*/

/*!
    @brief The CSR Vendor ID.
*/
#define A2DP_CSR_VENDOR_ID                      (0x0a000000)

/*!
    @brief The CSR Faststream Codec ID.
*/
#define A2DP_CSR_FASTSTREAM_CODEC_ID            (0x0100)


/*!
    @brief The CSR True Wireless Stereo v3 Codec ID for SBC.
*/
#define A2DP_CSR_TWS_SBC_CODEC_ID               (0x0301)

/*!
    @brief The CSR True Wireless Stereo v3 Codec ID for AAC.
*/
#define A2DP_CSR_TWS_AAC_CODEC_ID               (0x0401)

/*!
    @brief The CSR True Wireless Stereo v3 Codec ID for MP3.
*/
#define A2DP_CSR_TWS_MP3_CODEC_ID               (0x0501)


/*!
    @brief The CSR True Wireless Stereo v3 Codec ID for AptX.
*/
#define A2DP_CSR_TWS_APTX_CODEC_ID               (0x0601)

/*!
    @brief The CSR True Wireless Stereo v3 Codec ID for aptX Adaptive.
*/
#define A2DP_CSR_TWS_APTX_AD_CODEC_ID            (0x0701)

/*!
    @name CSR\Qualcomm apt-X IDs.
*/

/*!
	@brief The APT Vendor ID.
*/
#define A2DP_APT_VENDOR_ID				        (0x4f000000)	

/*!
	@brief The CSR apt-X Codec ID.
*/
#define A2DP_CSR_APTX_CODEC_ID			        (0x0100)

/*!
    @brief The QTI aptX LL Codec ID.
*/
#define A2DP_QTI_APTX_LL_CODEC_ID               (0x0200)

/*!
    @brief The CSR aptX ACL Sprint Codec ID.
*/
#define A2DP_CSR_APTX_ACL_SPRINT_CODEC_ID       (0x0200)

/*!
    @brief The QTI Vendor ID.
*/
#define A2DP_QTI_VENDOR_ID                      (0xd7000000U)

/*!
    @brief The QTI aptX-HD Codec ID.
*/
#define A2DP_QTI_APTXHD_CODEC_ID                (0x2400)

/*!
    @brief The QTI aptX Adaptive Codec ID.
*/
#define A2DP_QTI_APTX_AD_CODEC_ID                (0xAD00)

/*!
    @brief The QTI aptX TWS+ Codec ID.
*/
#define A2DP_QTI_APTX_TWS_PLUS_CODEC_ID         (0x2500)

/*!
    @brief The QTI SBC TWS+ Codec ID.
*/
#define A2DP_QTI_SBC_TWS_PLUS_CODEC_ID          (0x2600)

/*!
    @brief Used to indicate an invalid AV Sync delay
*/
#define A2DP_INVALID_AV_SYNC_DELAY (0xFFFF)

#define INVALID_DEVICE_ID  0xFF
#define INVALID_STREAM_ID  0xFF

struct __A2DP;


/*!
    @brief The Advanced Audio Distribution Profile structure.
*/
typedef struct __A2DP A2DP;

/*!
    @brief The structure holding the information about the Stream End Points available on the device.
*/
typedef struct _device_sep_list device_sep_list;

/*!
    @brief Stream End Point type (source or sink).

    The Stream End Point type is defined in the AVDTP
    specification.

*/
typedef enum
{
    /*!  This states the device or Stream End Point takes the Source role. */
    a2dp_source,
    /*!  This states the device or Stream End Point takes the Sink role. */
    a2dp_sink,
    /*!  This states the device or Stream End Point has no defined role. */
    a2dp_role_undefined
} a2dp_role_type;


/*!
    @brief Stream End Point (SEP) Media type.

    The Media type of a SEP is defined in the Bluetooth assigned numbers
    document.
*/
typedef enum
{
    sep_media_type_audio,       /*!< Audio.*/
    sep_media_type_video,       /*!< Video.*/
    sep_media_type_multimedia   /*!< Multimedia.*/
} a2dp_sep_media_type;


/*!
    @brief Status code returned in messages from the A2DP library

    This status code indicates the outcome of the request.
*/
typedef enum
{
    a2dp_success,                   /*!< The operation succeeded. */
    a2dp_reconnect_success,         /*!< The library has managed to reconnect a signalling channel following a link loss. */
    a2dp_invalid_parameters,        /*!< Invalid parameters supplied by the client. */
    a2dp_sdp_fail,                  /*!< SDP registration has failed. */
    a2dp_l2cap_fail,                /*!< L2CAP registration has failed. */
    a2dp_operation_fail,            /*!< The operation has failed. */
    a2dp_insufficient_memory,       /*!< No memory to perform the required task. */
    a2dp_wrong_state,               /*!< The library is in the wrong state to perform the operation. */
    a2dp_no_signalling_connection,  /*!< No signalling connection. */
    a2dp_no_media_connection,       /*!< No media connection. */
    a2dp_rejected_by_remote_device, /*!< Was rejected by the remote device. */
    a2dp_disconnect_link_loss,      /*!< Link loss occurred. */
    a2dp_closed_by_remote_device,   /*!< Closed by remote device. */
    a2dp_max_connections,           /*!< Library can't support any more signalling/media connections to a remote device */
    a2dp_aborted,                   /*!< Connection was aborted. */
    a2dp_security_reject,           /*!< Security rejected. */
    a2dp_disconnect_transferred,    /*!< Link has been transferred to TWS Peer. */
    FORCE_ENUM_TO_MIN_16BIT(a2dp_status_code)
} a2dp_status_code;


/*!
    @brief Type of content protection in use.
*/
typedef enum
{
    /*! No content protection in use. */
    avdtp_no_protection = (0),
    /*! SCMS-T content protection in use. */
    avdtp_scms_protection
} a2dp_content_protection;


/*!
    @brief Audio stream channel mode.

    The specification defines the following channel modes. The SNK must support
    all modes. It is mandatory for the SRC to support mono and at least one of
    the remaining three modes.
*/
typedef enum
{
    a2dp_mono,                                  /*!< Mono channel mode. */
    a2dp_dual_channel,                          /*!< Dual channel mode. */
    a2dp_stereo,                                /*!< Stereo channel mode. */
    a2dp_joint_stereo                           /*!< Joint stereo channel mode. */
} a2dp_channel_mode;


/*!
    @brief Stream End Point (SEP) Information.

    Contains details about a local SEP. The information here is constant for the lifetime of the SEP.
*/
typedef struct
{
    uint8 seid;                                 /*!< Unique ID for the SEP. */
    uint8 resource_id;                          /*!< Resource ID associated with the SEP. If a SEP is configured then all SEPs with the same resource ID will become in use. */
    a2dp_sep_media_type media_type:2;           /*!< The media type of the SEP. */
    a2dp_role_type role:2;                      /*!< The role of the SEP.  */
    unsigned library_selects_settings:1;        /*!< The library_selects_settings is set to TRUE so that the library selects the optimal settings for a codec when initiating a media connection. Setting it to FALSE will allow the application to choose how the SEP should be configured based on the remote SEP capabilities. */
    uint16 flush_timeout;                       /*!< The flush timeout for the SEP. Should be set to 0 to use the default timeout. */
    uint16 size_caps;                           /*!< The size of the capabilities for the SEP. */
    const uint8 *caps;                          /*!< The capabilities for the SEP. These can be taken from one of the default codec capability header files that are supplied by CSR. The service capabilities section of the AVDTP specification details the format of these capabilities. */
} sep_config_type;


/*!
    @brief Holds the details of one local SEP.
*/
typedef enum{
    A2DP_SEP_UNAVAILABLE = 0x01,   /* Set and cleared by client */
    A2DP_SEP_IN_USE      = 0x02,   /* Set and cleared through media channel setup/teardown */
    A2DP_SEP_ERROR       = 0xFF,   /* This is a status code to report error */
    FORCE_ENUM_TO_MIN_16BIT(a2dp_sep_status)
}a2dp_sep_status;
typedef struct sep_data_type
{
    const sep_config_type *sep_config;          /*!< Pointer to the constant details of the SEP. */
    unsigned in_use:2;                          /*!< Used to indicate if the SEP is initially in use. */
} sep_data_type;


#ifndef A2DP_SBC_ONLY
/*!
    @brief aptX LL params type.

    Parameters used for configuring aptX LL.
*/
typedef struct
{
    uint16 target_codec_level;     /*!< Target codec buffer fill level.*/
    uint16 initial_codec_level;    /*!< Initial codec buffer fill level.*/
    uint16 sra_max_rate;           /*!< SRA Maximum rate.*/
    uint16 sra_avg_time;           /*!< SRA Averaging time.*/
    uint16 good_working_level;     /*!< Good Working Buffer Level.*/
} aptx_sprint_params_type;

/*!
    @brief Target TTP latencies for aptX Adaptive NQ2Q mode.
*/
typedef struct
{
    unsigned low_latency_0_in_ms:8; /*!< TTP latency to use in low latency mode 0 */
    unsigned low_latency_1_in_ms:8; /*!< TTP latency to use in low latency mode 1 */
    unsigned high_quality_in_2ms:8; /*!< TTP latency to use in high quality mode */
    unsigned tws_legacy_in_2ms:8;   /*!< TTP latency to use in TWS mode (Legacy Stereo or mono) */
} aptx_adaptive_ttp_latencies_t;

typedef enum
{
    aptx_ad_default_features   = ((1 << 3) | (1 << 2) | (1 << 1) | (1 << 0)),
    aptx_ad_twm_support        = (1 << 24),
    aptx_ad_2_1_kernel_support = (1 << 25),
    aptx_ad_split_streaming    = (1 << 26),
    aptx_ad_split_ack          = (1 << 27),

    aptx_ad_2_2_kern_enc_src   = (1 << 28),
    aptx_ad_2_2_kern_dec_src   = (1 << 29),
    aptx_ad_2_2_kern_enc_snk   = (1 << 30),
    aptx_ad_2_2_kern_dec_snk   = (1 << 31)

}aptx_ad_supported_features_t;


typedef enum
{
    aptx_ad_version_unknown = 0,
    aptx_ad_version_1_1 = 11,
    aptx_ad_version_2_0 = 20,
    aptx_ad_version_2_1 = 21,
    aptx_ad_version_2_2 = 22,
} aptx_ad_version_t;

/*!
    @brief Codec parameters specific to aptX Adaptive.
*/
typedef struct
{
    unsigned :13;
    unsigned avrcp_cmd_supported:1;         /*!< Source supports the custom AVRCP command for aptX Adaptive */
    unsigned q2q_enabled:1;                 /*!< If source is compatible Q2Q mode is used */
    unsigned is_twsp_mode:1;                /*!< If CODEC is in TWS+ mode */
    aptx_adaptive_ttp_latencies_t nq2q_ttp; /*!< Target TTP latencies used for NQ2Q mode */
    aptx_ad_version_t version;              /*!< version of aptX Adaptive */
    aptx_ad_supported_features_t features;  /*!< Extended features defined in codec negotiation */
} aptx_adaptive_params_t;
#endif

/*!
    @brief Holds details about the configured codec.

    These details about the configured codec are returned to the application, so it can supply this information to the audio library.
    The audio library will use this information to configure the DSP and route the audio depending on the codec in use.
*/
typedef struct
{
    /*! @todo if the unused is relevant, then order is different on Kalimba wrt Xap */
    unsigned :14;
    unsigned content_protection:1;              /*!< Content protection in use. */
    unsigned latency_reporting:1;               /*!< Latency (delay) reporting in use. */
    unsigned bitpool:8;                         /*!< The bitpool value. */
    unsigned format:8;                          /*!< The format. */
    uint16 packet_size;                         /*!< The packet size. */
    uint32 voice_rate;                          /*!< The voice rate. */
#ifndef A2DP_SBC_ONLY
    aptx_sprint_params_type aptx_sprint_params; /*!< aptX sprint parameters. */
    aptx_adaptive_params_t aptx_ad_params;      /*!< Codec parameters specific to aptX adaptive */
#endif
} codec_data_type;


/*!
    @brief Used to register up to 2 service records on initialisation of the A2DP library.
*/
typedef struct
{
    uint16 size_service_record_a;               /*!< Size of the first service record to be registered. */
    const uint8 *service_record_a;                  /*!< The first service record to be registered. */
    uint16 size_service_record_b;               /*!< Size of the second service record to be registered. */
    const uint8 *service_record_b;                  /*!< The second service record to be registered. */
} service_record_type;

typedef struct
{
    /*! The sampling rate for the PCM hardware. Can be used when calling AudioConnect. */
    uint32                  rate;
    /*! The channel mode for the audio being streamed. Can be used when calling AudioConnect.*/
    a2dp_channel_mode       channel_mode;
    /*! The local SEID in use. This is for informational purposes only. */
    uint8                   seid;
    /*! Sink for the media transport channel. */
    Sink                    sink;
    /*! The remote device's media channel MTU (maximum payload size it will accept). */
    uint16                  remote_mtu;
    /*! The codec parameters to pass into the audio library. Can be used when calling AudioConnect. */
    codec_data_type         codecData;
    /*! Size of the configured capabilities. */
    uint16                  size_configured_codec_caps;
    /*! The configured capabilities. They will have the form: [AVDTP_SERVICE_MEDIA_TRANSPORT] [0] [AVDTP_SERVICE_MEDIA_CODEC] [size media codec data] ... etc.*/
    uint8                   configured_codec_caps[1];
} a2dp_codec_settings;

#endif /* A2DP_H_ */


/*!
    @brief Exposes A2DP interface for handover.
*/
extern const handover_interface a2dp_handover;

/** @} */

