/* Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd. */
/*  */

/*!
@file    gatt_pacs_server.h
@brief   Header file for the Published Audio Capabilities Service library.

        This file provides documentation for the GATT PACS Server library
        API (library name: Gatt_pacs_server).
*/

#ifndef GATT_PACS_SERVER_H
#define GATT_PACS_SERVER_H

#include <library.h>
#include <gatt.h>
#include <service_handle.h>

typedef ServiceHandle ServiceHandleType;

typedef connection_id_t ConnectionIdType;


/* Maximum number of GATT connections */
#define GATT_PACS_MAX_CONNECTIONS (3)

/*!
    \brief Error codes for PAC Record Handles
*/

typedef uint16 PacsRecordErrorType;

#define PACS_RECORD_INVALID_PARAMETERS          (PacsRecordErrorType)0xFFF0
#define PACS_RECORD_ALREADY_ADDED               (PacsRecordErrorType)0xFFF1
#define PACS_RECORD_HANDLES_EXHAUSTED           (PacsRecordErrorType)0xFFF2
#define PACS_RECORD_VENDOR_CODEC_NOT_SUPPORTED  (PacsRecordErrorType)0xFFF3
#define PACS_RECORD_METATDATA_NOT_ADDED         (PacsRecordErrorType)0xFFF4

#define PACS_RECORD_ERRORCODE_BASE PACS_RECORD_INVALID_PARAMETERS

/*! Audio Contexts configuration */

typedef uint8 PacsServerAudioContextType;

#define PACS_SERVER_SUPPORTED_AUDIO_CONTEXTS     (PacsServerAudioContextType)0x01
#define PACS_SERVER_AVAILABLE_AUDIO_CONTEXTS     (PacsServerAudioContextType)0x02

typedef uint8 PacsServerDirectionType;

/*! Describes the Server is capable of receiving Audio data */
#define PACS_SERVER_IS_AUDIO_SINK    (PacsServerDirectionType)0x01
/*! Describes the Server is capable of transmitting Audio data */
#define PACS_SERVER_IS_AUDIO_SOURCE  (PacsServerDirectionType)0x02

/*! @brief Client Config data.

    This structure contains the client configuration of all the characteristics
    of the Published Audio Capabilities Service
 */

typedef struct
{
    unsigned    sinkPacClientCfg1:2;
    unsigned    sinkAudioLocationsClientCfg:2;
    unsigned    sourcePacClientCfg:2;
    unsigned    sourceAudioLocationsClientCfg:2;
    unsigned    availableAudioContextsClientCfg:2;
    unsigned    supportedAudioContextsClientCfg:2;
    unsigned    sinkPacClientCfg2:2;
    unsigned    vsSinkPacClientCfg:2;
    unsigned    vsSourcePacClientCfg:2;
    unsigned    unused:14;
} GattPacsServerConfigType;


/* PACS Specification:
 * Published_Audio_ Capabilities_Service_d09r08
 * Audio Location values from BAP d09r07 Appendix B.3
 */

/*! @brief Published Audio Capabilities Service library  data structure type .
 */

typedef uint32 PacsAudioLocationType;

#define PACS_AUDIO_LOCATION_LEFT                      (PacsAudioLocationType)0x00000001
#define PACS_AUDIO_LOCATION_RIGHT                     (PacsAudioLocationType)0x00000002
#define PACS_AUDIO_LOCATION_CENTER                    (PacsAudioLocationType)0x00000004
#define PACS_AUDIO_LOCATION_LOW_FREQUENCY_EFFECTS1    (PacsAudioLocationType)0x00000008
#define PACS_AUDIO_LOCATION_BACK_LEFT                 (PacsAudioLocationType)0x00000010
#define PACS_AUDIO_LOCATION_BACK_RIGHT                (PacsAudioLocationType)0x00000020
#define PACS_AUDIO_LOCATION_FRONT_LEFT_OF_CENTER      (PacsAudioLocationType)0x00000040
#define PACS_AUDIO_LOCATION_FRONT_RIGHT_OF_CENTER     (PacsAudioLocationType)0x00000080
#define PACS_AUDIO_LOCATION_BACK_CENTER               (PacsAudioLocationType)0x00000100
#define PACS_AUDIO_LOCATION_LOW_FREQUENCY_EFFECTS2    (PacsAudioLocationType)0x00000200
#define PACS_AUDIO_LOCATION_SIDE_LEFT                 (PacsAudioLocationType)0x00000400
#define PACS_AUDIO_LOCATION_SIDE_RIGHT                (PacsAudioLocationType)0x00000800
#define PACS_AUDIO_LOCATION_TOP_FRONT_LEFT            (PacsAudioLocationType)0x00001000
#define PACS_AUDIO_LOCATION_TOP_FRONT_RIGHT           (PacsAudioLocationType)0x00002000
#define PACS_AUDIO_LOCATION_TOP_FRONT_CENTER          (PacsAudioLocationType)0x00004000
#define PACS_AUDIO_LOCATION_TOP_CENTER                (PacsAudioLocationType)0x00008000
#define PACS_AUDIO_LOCATION_TOP_BACK_LEFT             (PacsAudioLocationType)0x00010000
#define PACS_AUDIO_LOCATION_TOP_BACK_RIGHT            (PacsAudioLocationType)0x00020000
#define PACS_AUDIO_LOCATION_TOP_SIDE_LEFT             (PacsAudioLocationType)0x00040000
#define PACS_AUDIO_LOCATION_TOP_SIDE_RIGHT            (PacsAudioLocationType)0x00080000
#define PACS_AUDIO_LOCATION_TOP_BACK_CENTER           (PacsAudioLocationType)0x00100000
#define PACS_AUDIO_LOCATION_BOTTOM_FRONT_CENTER       (PacsAudioLocationType)0x00200000
#define PACS_AUDIO_LOCATION_BOTTOM_FRONT_LEFT         (PacsAudioLocationType)0x00400000
#define PACS_AUDIO_LOCATION_BOTTOM_FRONT_RIGHT        (PacsAudioLocationType)0x00800000
#define PACS_AUDIO_LOCATION_FRONT_LEFT_WIDER          (PacsAudioLocationType)0x01000000
#define PACS_AUDIO_LOCATION_FRONT_RIGHT_WIDER         (PacsAudioLocationType)0x02000000
#define PACS_AUDIO_LOCATION_SURROUND_LEFT             (PacsAudioLocationType)0x04000000
#define PACS_AUDIO_LOCATION_SURROUND_RIGHT            (PacsAudioLocationType)0x08000000


typedef uint16 PacsAudioContextType;


#define PACS_CONTEXT_TYPE_PROHIBITED           (PacsAudioContextType)0x0000
#define PACS_CONTEXT_TYPE_UNSPECIFIED          (PacsAudioContextType)0x0001  /* Unspecified. Matches any audio content. */
#define PACS_CONTEXT_TYPE_CONVERSAIONAL        (PacsAudioContextType)0x0002  /* Phone Call, Conversation between humans */
#define PACS_CONTEXT_TYPE_MEDIA                (PacsAudioContextType)0x0004  /* Music, Radio, Podcast, Video Soundtrack or TV audio */
#define PACS_CONTEXT_TYPE_GAME                 (PacsAudioContextType)0x0008  /* Audio associated with gaming */
#define PACS_CONTEXT_TYPE_INSTRUCTIONAL        (PacsAudioContextType)0x0010  /* Satnav, User Guidance, Traffic Announcement */
#define PACS_CONTEXT_TYPE_VOICE_ASSISTANT      (PacsAudioContextType)0x0020  /* Virtual Assistant, Voice Recognition */
#define PACS_CONTEXT_TYPE_LIVE                 (PacsAudioContextType)0x0040  /* Live Audio */
#define PACS_CONTEXT_TYPE_SOUND_EFFECTS        (PacsAudioContextType)0x0080  /*Sound effects including keyboard and touch feedback;
                                                                               menu and user interface sounds; and other system sounds */
#define PACS_CONTEXT_TYPE_NOTIFICATIONS        (PacsAudioContextType)0x0100  /* Incoming Message Alert, Keyboard Click */
#define PACS_CONTEXT_TYPE_RINGTONE             (PacsAudioContextType)0x0200  /* Incoming Call */
#define PACS_CONTEXT_TYPE_ALERTS               (PacsAudioContextType)0x0400  /* Low Battery Warning, Alarm Clock, Timer Expired */
#define PACS_CONTEXT_TYPE_EMERGENCY_ALARM      (PacsAudioContextType)0x0800

typedef uint8 PacsCodecIdType;

#define PACS_CODEC_ID_UNKNOWN             (PacsCodecIdType)0x00
#define PACS_LC3_CODEC_ID                 (PacsCodecIdType)0x06   /* LC3 Codec id */
#define PACS_VENDOR_CODEC_ID              (PacsCodecIdType)0xFF    /* Vendor Codec id */

/*! List of Sampling Frequency */
typedef uint16 PacsSamplingFrequencyType;

#define PACS_SAMPLING_FREQUENCY_8KHZ         (PacsSamplingFrequencyType)0x0001
#define PACS_SAMPLING_FREQUENCY_11_025KHZ    (PacsSamplingFrequencyType)0x0002
#define PACS_SAMPLING_FREQUENCY_16KHZ        (PacsSamplingFrequencyType)0x0004
#define PACS_SAMPLING_FREQUENCY_22_05KHZ     (PacsSamplingFrequencyType)0x0008
#define PACS_SAMPLING_FREQUENCY_24KHZ        (PacsSamplingFrequencyType)0x0010
#define PACS_SAMPLING_FREQUENCY_32KHZ        (PacsSamplingFrequencyType)0x0020
#define PACS_SAMPLING_FREQUENCY_44_1KHZ      (PacsSamplingFrequencyType)0x0040
#define PACS_SAMPLING_FREQUENCY_48KHZ        (PacsSamplingFrequencyType)0x0080
#define PACS_SAMPLING_FREQUENCY_88_2KHZ      (PacsSamplingFrequencyType)0x0100
#define PACS_SAMPLING_FREQUENCY_96KHZ        (PacsSamplingFrequencyType)0x0200
#define PACS_SAMPLING_FREQUENCY_176_4KHZ     (PacsSamplingFrequencyType)0x0400
#define PACS_SAMPLING_FREQUENCY_192KHZ       (PacsSamplingFrequencyType)0x0800
#define PACS_SAMPLING_FREQUENCY_384KHZ       (PacsSamplingFrequencyType)0x1000

typedef uint8 PacsFrameDurationType;

#define PACS_SUPPORTED_FRAME_DURATION_7P5MS     (PacsFrameDurationType)0x01
#define PACS_SUPPORTED_FRAME_DURATION_10MS      (PacsFrameDurationType)0x02
#define PACS_PREFERRED_FRAME_DURATION_7P5MS     (PacsFrameDurationType)0x10
#define PACS_PREFERRED_FRAME_DURATION_10MS      (PacsFrameDurationType)0x20

typedef uint8 PacsAudioChannelCountType;

#define PACS_AUDIO_CHANNEL_1        (PacsAudioChannelCountType)0x01
#define PACS_AUDIO_CHANNELS_2       (PacsAudioChannelCountType)0x02
#define PACS_AUDIO_CHANNELS_3       (PacsAudioChannelCountType)0x04
#define PACS_AUDIO_CHANNELS_4       (PacsAudioChannelCountType)0x08
#define PACS_AUDIO_CHANNELS_5       (PacsAudioChannelCountType)0x10
#define PACS_AUDIO_CHANNELS_6       (PacsAudioChannelCountType)0x20
#define PACS_AUDIO_CHANNELS_7       (PacsAudioChannelCountType)0x40
#define PACS_AUDIO_CHANNELS_8       (PacsAudioChannelCountType)0x80

typedef uint8 PacsSupportedMaxCodecFramePerSdu;

typedef struct
{
    PacsCodecIdType                   codecId;    /*! codec id */
    uint16                            companyId;            /*! Company_ID value of the PAC record  */
    uint16                            vendorCodecId;       /*! Vendor-specific codec_ID value of the PAC record. */

    PacsSamplingFrequencyType         supportedSamplingFrequencies;    /* Bit mask of values defined in
                                                 pacs_sampling_frequency_t */
    PacsFrameDurationType             supportedFrameDuration;    /* Bit mask of values defined in
                                          pacs_frame_duration_t */
    PacsAudioChannelCountType         audioChannelCounts;        /* Bit mask of values defined in
                                          pacs_audio_channel_count_t */
    uint16                            minSupportedOctetsPerCodecFrame;
    uint16                            maxSupportedOctetsPerCodecFrame;

    PacsSupportedMaxCodecFramePerSdu supportedMaxCodecFramePerSdu;

    uint8                             metadataLength;     /*! Length of the Metadata field of the PAC record  */
    const uint8                       *metadata;            /*! Metadata applicable to the PAC record..*/
} GattPacsServerRecordType;

/*!
    @brief Instantiate the GATT Published Audio Capability Library.

    The GATT Service Init function is responsible for allocating its instance memory
    and returning a unique service handle for that instance. The Service handle is
    then used for the rest of the API.

    @param appTask The Task that will receive the messages sent from this Published Audio Capability service library.
    @param startHandle This indicates the start handle of the PAC service
    @param endHandle This indicates the end handle of the PAC service

    @return ServiceHandleType If the service handle returned is 0, this indicates a failure
    during GATT Service initialisation.


*/
ServiceHandleType GattPacsServerInit(Task appTask, uint16 startHandle, uint16 endHandle);

/*!
    @brief This API is used to add a PAC record for SINK/SOURCE.

    @param handle The server instance handle returned in GattPacsServerInit()
    @param direction Direction specifies SINK/SOURCE PAC record.
    @param pacRecord A pointer to GattPacsServerRecordType of SINK/SOURCE PAC.
	       Metadata information is mandatory to be added in pacRecord. Failure to add
		   will return error with pacs_record_metatdata_not_added

    @return pacRecordHandle if successful, errors mentioned in PacsRecordErrorType otherwise.

*/
uint16 GattPacsServerAddPacRecord(ServiceHandleType handle,
                               PacsServerDirectionType direction,
                               const GattPacsServerRecordType *pacRecord);


/*!
    @brief This API is used to remove a registered PAC record corresponds to the pac_record_handle.

    @param handle The server instance handle returned in GattPacsServerInit().
    @param pacRecordHandle Returned as part of GattPacsServerAddPacRecord() for PAC record

    @return TRUE if success, FALSE otherwise

*/
bool GattPacsServerRemovePacRecord(ServiceHandleType handle,
                                   uint16 pacRecordHandle);

/*!
    @brief This API is used to get a registered PAC record corresponds to the pac_record_handle.

    @param handle The server instance handle returned in GattPacsServerInit().
    @param pacRecordHandle Returned as part of GattPacsServerAddPacRecord() for PAC record

    @return A const pointer to GattPacsServerRecordType if success, NULL otherwise

*/
const GattPacsServerRecordType * GattPacsServerGetPacRecord(
                                       ServiceHandleType handle,
                                       uint16 pacRecordHandle);


/*!
    @brief This API is used to add Audio Location of SINK/SOURCE PAC

    @param handle The server instance handle returned in GattPacsServerInit().
    @param direction Direction specifies SINK/SOURCE Audio location.
    @param audioLocations Device wide bitmap of supported Audio Location values
           for all PAC records where the server supports reception/transmission
           of audio data


    @return TRUE if success, FALSE otherwise

*/
bool GattPacsServerAddAudioLocation(
                          ServiceHandleType handle,
                          PacsServerDirectionType direction,
                          PacsAudioLocationType audioLocations);

/*!
    @brief This API is used to remove Audio Location of SINK/SOURCE PAC

    @param handle The server instance handle returned in GattPacsServerInit().
    @param direction Direction specifies SINK/SOURCE Audio location.
    @param audioLocations Device wide bitmap of supported Audio Location values
           for all PAC records where the server supports reception/transmission
           of audio data

    @return TRUE if success, FALSE otherwise

*/
bool GattPacsServerRemoveAudioLocation(
                          ServiceHandleType handle,
                          PacsServerDirectionType direction,
                          PacsAudioLocationType audioLocations);

/*!
    @brief This API is used to get Audio Location set for SINK or SOURCE configuration.

    @param handle The server instance handle returned in GattPacsServerInit().
    @param direction Direction specifies the SINK or SOURCE audio locations.

    @return Audio Location if success, 0 otherwise

*/
PacsAudioLocationType GattPacsServerGetAudioLocation(
                                 ServiceHandleType handle,
                                 PacsServerDirectionType direction);


/*!
    @brief This API is used to add Supported Audio Contexts for SINK and SOURCE contexts.
           The API is also used to add Available Audio Contexts for SINK and SOURCE contexts
           at any point from Supported Audio Contexts list.

    @param handle The server instance handle returned in GattPacsServerInit().
    @param direction Direction specifies the SINK or SOURCE audio contexts.
    @param audioContexts Bitmask of audio data Context Type values for transmission/reception.
    @param context Supported or Available audio context defined by PacsServerAudioContextType

    @return TRUE if success, FALSE otherwise

*/
bool GattPacsServerAddAudioContexts(
                          ServiceHandleType handle,
                          PacsServerDirectionType direction,
                          uint16 audioContexts,
                          PacsServerAudioContextType context);


/*!
    @brief This API is used to get Available Audio Contexts for SINK or SOURCE configuration

    @param handle The server instance handle returned in GattPacsServerInit().
    @param direction Direction specifies the SINK or SOURCE supported audio contexts.

    @return Available Audio Contexts if success, 0 otherwise

*/
uint16 GattPacsServerGetAudioContextsAvailability(
                              ServiceHandleType handle,
                              PacsServerDirectionType direction);

/*!
    @brief This API is used to get Supported or Available Audio Contexts for SINK or SOURCE
           configuration

    @param handle The server instance handle returned in GattPacsServerInit().
    @param direction Direction specifies the SINK or SOURCE supported audio contexts.
    @param context Supported or Available audio context defined by PacsAudioContextType.

    @return Available Audio Contexts if success, 0 otherwise

*/
uint16 GattPacsServerGetAudioContexts(
                          ServiceHandleType handle,
                          PacsServerDirectionType direction,
                          PacsServerAudioContextType context);

/*!
    @brief This API is used to remove Supported or Available Audio Contexts for SINK or SOURCE
           configuration

    @param handle The server instance handle returned in GattPacsServerInit().
    @param direction Direction specifies SINK/SOURCE Audio location.
    @param audioContexts Bitmask of audio data Context Type values for transmission/reception.
    @param context Supported or Available audio context defined by PacsServerAudioContextType.


    @return TRUE if success, FALSE otherwise

*/
bool GattPacsServerRemoveAudioContexts(
                          ServiceHandleType handle,
                          PacsServerDirectionType direction,
                          uint16 audioContexts,
                          PacsServerAudioContextType context);

/*!
    @brief Remove the configuration for a peer device, identified by its
           Connection ID.

    This removes the configuration for that peer device from the
    service library, freeing the resources used for that config.
    This should only be done when the peer device is disconnecting.

    @param handle The server instance handle returned in GattPacsServerInit().
    @param cid  The cid wth the remote device.

    @return GattPacsServerConfigType  Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.

*/
GattPacsServerConfigType *GattPacsServerRemoveConfig(
                          ServiceHandleType handle,
                          ConnectionIdType cid);


/*!
    @brief Add configuration for a paired peer device, identified by its
           Connection ID (CID).

    @param handle The server instance returned in GattPacsServerInit().
    @param cid The Connection ID to the peer device.
    @param config  Client characteristic configurations for this connection.
           If this is NULL, this indicates a default config should be used for the
           peer device identified by the CID.

    @return gatt_status_t status of the Add Configuration operation.

*/

gatt_status_t GattPacsServerAddConfig(
                          ServiceHandleType handle,
                          ConnectionIdType cid,
                          GattPacsServerConfigType *config);

/*!
    @brief This API should be exercised carefully so that PACS server does not
           generate unnecessary notification for the bonded client on
           re-connection with same char values.

           By default if this API is not called, only Available Audio Context
           notification will be sent to remote client on re-connection as other
           char values remain unchanged.

           This API needs to be called once after GattPacsServerInit() if
           application finds that char values have changed.

*/
void GattPacsServerCharChangesDynamically(void);

#endif /* GATT_PACS_SERVER_H */

