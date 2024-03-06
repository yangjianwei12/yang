/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#ifndef AUDIO_ANNOUNCEMENT_PARSER_LIB_H
#define AUDIO_ANNOUNCEMENT_PARSER_LIB_H

#include "csr_types.h"
#include "csr_bt_profiles.h"

/*!
    \brief Audio Announcement Parser status code type.
*/
typedef uint16 AudioAnnouncementParserStatus;

/*! { */
/*! Values for the Audio Announcement Parser status code */
#define AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS            ((AudioAnnouncementParserStatus)0x0000u)  /*!> Request was a success*/
#define AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_LENGTH     ((AudioAnnouncementParserStatus)0x0001u)  /*!> Invalid advertising data length */
#define AUDIO_ANNOUNCEMENT_PARSER_STATUS_INVALID_PARAMETER  ((AudioAnnouncementParserStatus)0x0002u)  /*!> Invalid parameter was supplied*/
#define AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND          ((AudioAnnouncementParserStatus)0x0003u)  /*!> The request Audio Announcement or
                                                                                                           parameter was not found */
#define AUDIO_ANNOUNCEMENT_PARSER_STATUS_BAD_FORMAT         ((AudioAnnouncementParserStatus)0x0004u)  /*!> The Audio Announcement has
                                                                                                           a bad format*/
/*! } */

/*!
    \brief LE Audio context types.

    Note: It's a bitfield type
*/
typedef uint16 AudioAnnouncementParserAudioContextType;

#define AUDIO_ANNOUNCEMENT_PARSER_AUDIO_CONTEXT_TYPE_UNKNOWN         ((AudioAnnouncementParserAudioContextType)0x0000u)
#define AUDIO_ANNOUNCEMENT_PARSER_AUDIO_CONTEXT_TYPE_UNSPECIFIED     ((AudioAnnouncementParserAudioContextType)0x0001u)
#define AUDIO_ANNOUNCEMENT_PARSER_AUDIO_CONTEXT_TYPE_CONVERSATIONAL  ((AudioAnnouncementParserAudioContextType)0x0002u)
#define AUDIO_ANNOUNCEMENT_PARSER_AUDIO_CONTEXT_TYPE_MEDIA           ((AudioAnnouncementParserAudioContextType)0x0004u)
#define AUDIO_ANNOUNCEMENT_PARSER_AUDIO_CONTEXT_TYPE_GAME            ((AudioAnnouncementParserAudioContextType)0x0008u)
#define AUDIO_ANNOUNCEMENT_PARSER_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL   ((AudioAnnouncementParserAudioContextType)0x0010u)
#define AUDIO_ANNOUNCEMENT_PARSER_AUDIO_CONTEXT_TYPE_VOICE_ASSISTANT ((AudioAnnouncementParserAudioContextType)0x0020u)
#define AUDIO_ANNOUNCEMENT_PARSER_AUDIO_CONTEXT_TYPE_LIVE            ((AudioAnnouncementParserAudioContextType)0x0040u)
#define AUDIO_ANNOUNCEMENT_PARSER_AUDIO_CONTEXT_TYPE_SOUND_EFFECTS   ((AudioAnnouncementParserAudioContextType)0x0080u)
#define AUDIO_ANNOUNCEMENT_PARSER_AUDIO_CONTEXT_TYPE_NOTIFICATIONS   ((AudioAnnouncementParserAudioContextType)0x0100u)
#define AUDIO_ANNOUNCEMENT_PARSER_AUDIO_CONTEXT_TYPE_RINGTONE        ((AudioAnnouncementParserAudioContextType)0x0200u)
#define AUDIO_ANNOUNCEMENT_PARSER_AUDIO_CONTEXT_TYPE_ALERTS          ((AudioAnnouncementParserAudioContextType)0x0400u)
#define AUDIO_ANNOUNCEMENT_PARSER_AUDIO_CONTEXT_TYPE_EMERGENCY_ALARM ((AudioAnnouncementParserAudioContextType)0x0800u)

/*!
    \brief Sampling Frequency type.
*/
typedef uint8 AudioAnnouncementParserSamplingFreqType;

#define AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_8000       ((AudioAnnouncementParserSamplingFreqType)0x01u)
#define AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_11025      ((AudioAnnouncementParserSamplingFreqType)0x02u)
#define AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_16000      ((AudioAnnouncementParserSamplingFreqType)0x03u)
#define AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_22050      ((AudioAnnouncementParserSamplingFreqType)0x04u)
#define AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_24000      ((AudioAnnouncementParserSamplingFreqType)0x05u)
#define AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_32000      ((AudioAnnouncementParserSamplingFreqType)0x06u)
#define AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_44100      ((AudioAnnouncementParserSamplingFreqType)0x07u)
#define AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_48000      ((AudioAnnouncementParserSamplingFreqType)0x08u)
#define AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_88200      ((AudioAnnouncementParserSamplingFreqType)0x09u)
#define AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_96000      ((AudioAnnouncementParserSamplingFreqType)0x0Au)
#define AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_176400     ((AudioAnnouncementParserSamplingFreqType)0x0Bu)
#define AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_192000     ((AudioAnnouncementParserSamplingFreqType)0x0Cu)
#define AUDIO_ANNOUNCEMENT_PARSER_SAMPLING_FREQ_TYPE_384000     ((AudioAnnouncementParserSamplingFreqType)0x0Du)

/*!
    \brief Frame Duration type.
*/
typedef uint8 AudioAnnouncementParserFrameDurationType;

#define AUDIO_ANNOUNCEMENT_PARSER_FRAME_DURATION_TYPE_7_5            ((AudioAnnouncementParserFrameDurationType)0x00u)
#define AUDIO_ANNOUNCEMENT_PARSER_FRAME_DURATION_TYPE_10             ((AudioAnnouncementParserFrameDurationType)0x01u)

/*!
    \brief Audio Quality type.

    Note: It's a bitfield type
*/
typedef uint8 AudioAnnouncementParserAudioQualityType;

#define AUDIO_ANNOUNCEMENT_PARSER_AUDIO_QUALITY_TYPE_STANDARD  ((AudioAnnouncementParserAudioQualityType)0x01u)
#define AUDIO_ANNOUNCEMENT_PARSER_AUDIO_QUALITY_TYPE_HIGH      ((AudioAnnouncementParserAudioQualityType)0x02u)

/*!
    \brief Audio Active State type.
*/
typedef uint16 AudioAnnouncementParserAudioActiveState;

#define AUDIO_ANNOUNCEMENT_PARSER_AUDIO_ACTIVE_STATE_TYPE_NO_AUDIO           ((AudioAnnouncementParserAudioActiveState)0x00u)
#define AUDIO_ANNOUNCEMENT_PARSER_AUDIO_ACTIVE_STATE_TYPE_AUDIO_TRANSMITTED  ((AudioAnnouncementParserAudioActiveState)0x01u)

typedef struct
{
    uint8 bisIndex;
    uint8 codecSpecificConfigLen;
    uint8 *codecSpecificConfig;
}AudioAnnouncementParserBaseLvl3Data;

typedef struct
{
    uint8 codingFormat;
    uint16 companyId;
    uint16 vendorSpecificCodecId;
}AudioAnnouncementParserBaseCodecId;

typedef struct
{
    uint8 numBis;
    AudioAnnouncementParserBaseCodecId codecId;
    uint8 codecSpecificConfigLen;
    uint8 *codecSpecificConfig;
    uint8 metadataLen;
    uint8 *metadata;
    AudioAnnouncementParserBaseLvl3Data *bisData;
}AudioAnnouncementParserBaseLevel2Data;

/*! @brief Basic Audio Announcement data.

    This structure contains the data contained in the Basic Audio
    Announcement.
 */
typedef struct
{
    uint32 presentationDelay;
    uint8 numSubgroups;
    AudioAnnouncementParserBaseLevel2Data *subGroupsData;
}AudioAnnouncementParserBaseData;

/*! @brief Public Audio Announcement data.

    This structure contains the data contained in the Public Audio
    Announcement.
 */
typedef struct
{
    uint8 publicBroadcastAnnouncementFeatures;
    bool encryption;
    AudioAnnouncementParserAudioQualityType audioQuality;
    uint8 metadataLen;
    uint8 *metadata;
}AudioAnnouncementParserPbpDataType;

/*!
    @brief Check if the Broadcast name is present in the advertising data
           and if it is, it gets the name.

    @param advDataLen        Advertising data length
    @param advData           Advertising data to parse
    @param[out] nameLen      Pointer to the length of the name.
                             Note: The upper layer must pass a reference to the required
                                   variable on the stack or allocate memory for it.

    @return uint8 *          Pointer to the name will be copied in. Will be
                             Allocated in this function.
                             In case no name is found, this pointer will be
                             NULL.
                             Note: it's upper layer responsability to free
                                   this memory
*/
uint8 *AudioAnnouncementParserBcastNameParsing(uint16 advDataLen,
                                               const uint8 *advData,
                                               uint8 *nameLen);

/*!
    @brief Check if the Broadcast audio announcement is present in the advertising data
           and if it is, it gets the Broadcast Id in it.

    @param advDataLen        Advertising data length
    @param advData           Advertising data to parse
    @param[out] broadcastId  Pointer to the Broadcast Id value.
                             Note: The upper layer must pass a reference to the required
                                   variable on the stack or allocate memory for it.

    @return AudioAnnouncementParserStatus AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS if
                                          the Broadcast Audio Announcement is found,
                                          otherwise an error code will be returned
                                          (see AudioAnnouncementParserStatus definition).

    NOTE: In case the returned status is not success, the function will set the value pointed
          by broadcastId to zero.
*/
AudioAnnouncementParserStatus AudioAnnouncementParserBcastAudioAnnouncementParsing(uint16 advDataLen,
                                                                                   uint8 *advData,
                                                                                   uint32 *broadcastId);

/*!
    @brief Check if the Basic Audio Announcement is present in the advertising data
           and if it is, it gets the Base data.

    @param advDataLen  Advertising data length
    @param advData     Advertising data to parse
    @param[out] base   Pointer to the Base data structure
                       Note: The upper layer must pass a reference to the required
                             variable on the stack or allocate memory for it.

    @return AudioAnnouncementParserStatus AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS if
                                          the Basic Audio Announcement is found,
                                          otherwise an error code will be returned
                                          (see AudioAnnouncementParserStatus definition).

    NOTE: In case the returned status is not success, the function will set to zero/NULL
          all the data in base parameter.
          It is responsability of the upper layer that is calling this function to free
          the memory associate to all the pointers in the base structure.
*/
AudioAnnouncementParserStatus AudioAnnouncementParserBasicAudioAnnouncementParsing(uint16 advDataLen,
                                                                                   uint8 *advData,
                                                                                   AudioAnnouncementParserBaseData *base);

/*! \brief Utility function to get the sampling frequency from the codec configuration data

    \param codecSpecificConfigLen  Length of the codec configuration
    \param codecSpecificConfig     Pointer to the codec configuration
    \param[out] samplingFrequency  Pointer to the samplying frequency value
                                   Note: The upper layer must pass a reference to the required
                                         variable on the stack or allocate memory for it.

    \returns AudioAnnouncementParserStatus AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS if
                                           the sampling frequency LTV structure is present,
                                           otherwise an error code will be returned
                                           (see AudioAnnouncementParserStatus definition).

    NOTE: In case the returned status is not success, the function will set the value pointed
          by samplingFrequency to zero.
 */
AudioAnnouncementParserStatus AudioAnnouncementParserGetSamplingFreqFromCodecCnfg(
                                                                uint8 codecSpecificConfigLen,
                                                                uint8 *codecSpecificConfig,
                                                                AudioAnnouncementParserSamplingFreqType *samplingFrequency);

/*! \brief Utility function to get the frame duration from the codec configuration data

    \param codecSpecificConfigLen  Length of the codec configuration
    \param codecSpecificConfig     Pointer to the codec configuration
    \param[out] frameDuration      Pointer to the frame duration value
                                   Note: The upper layer must pass a reference to the required
                                         variable on the stack or allocate memory for it.

    \returns AudioAnnouncementParserStatus AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS if
                                           the frame duration LTV structure is present,
                                           otherwise an error code will be returned
                                           (see AudioAnnouncementParserStatus definition).

    NOTE: In case the returned status is not success, the function will set the value pointed
          by frameDuration to zero.
 */
AudioAnnouncementParserStatus AudioAnnouncementParserGetFrameDurationFromCodecCnfg(
                                                              uint8 codecSpecificConfigLen,
                                                              uint8 *codecSpecificConfig,
                                                              AudioAnnouncementParserFrameDurationType *frameDuration);

/*! \brief Utility function to get the audio channel allocation from the codec configuration data.

    \param codecSpecificConfigLen       Length of the codec configuration
    \param codecSpecificConfig          Pointer to the codec configuration
    \param[out] audioChannelAllocation  Pointer to the audio channel allocation value
                                        Note: The upper layer must pass a reference to the required
                                              variable on the stack or allocate memory for it.

    \returns AudioAnnouncementParserStatus AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS if
                                           the audio channel allocation LTV structure is present,
                                           otherwise an error code will be returned
                                           (see AudioAnnouncementParserStatus definition).

    NOTE: In case the returned status is not success, the function will set the value pointed
          by audioChannelAllocation to zero.
 */
AudioAnnouncementParserStatus AudioAnnouncementParserGetAudioChannelAllocationFromCodecCnfg(
                                                                     uint8 codecSpecificConfigLen,
                                                                     uint8 *codecSpecificConfig,
                                                                     uint32 *audioChannelAllocation);

/*! \brief Utility function to get the octets per codec frame from the codec configuration data.

    \param codecSpecificConfigLen    Length of the codec configuration
    \param codecSpecificConfig       Pointer to the codec configuration
    \param[out] octetsPerCodecframe  Pointer to the octets per codec frame value
                                     Note: The upper layer must pass a reference to the required
                                           variable on the stack or allocate memory for it.

    \returns AudioAnnouncementParserStatus AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS if
                                           the octets per codec frame LTV structure is present,
                                           otherwise an error code will be returned
                                           (see AudioAnnouncementParserStatus definition).

    NOTE: In case the returned status is not success, the function will set the value pointed
          by octetsPerCodecframe to zero.
 */
AudioAnnouncementParserStatus AudioAnnouncementParserGetOctetsPerCodecFrameFromCodecCnfg(
                                                                  uint8 codecSpecificConfigLen,
                                                                  uint8 *codecSpecificConfig,
                                                                  uint16 *octetsPerCodecframe);

/*! \brief Utility function to get the codec frame blocks per sdu from the codec configuration data.

    \param codecSpecificConfigLen       Length of the codec configuration
    \param codecSpecificConfig          Pointer to the codec configuration
    \param[out] codecFrameBlocksPerSdu  Pointer to the codec frame blocks per sdu value
                                        Note: The upper layer must pass a reference to the required
                                              variable on the stack or allocate memory for it.

    \returns AudioAnnouncementParserStatus AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS if
                                           the codec frame blocks per sdu LTV structure is present,
                                           otherwise an error code will be returned
                                           (see AudioAnnouncementParserStatus definition).

    NOTE: In case the returned status is not success, the function will set the value pointed
          by codecFrameBlocksPerSdu to zero.
 */
AudioAnnouncementParserStatus AudioAnnouncementParserGetCodecFrameBlocksPerSduFromCodecCnfg(
                                                                     uint8 codecSpecificConfigLen,
                                                                     uint8 *codecSpecificConfig,
                                                                     uint8 *codecFrameBlocksPerSdu);

/*! \brief Utility function to get the preferred audio context from the metadata.

    \param metadataLen                 Length of the metadata
    \param metadata                    Pointer to the metadata
    \param[out] preferredAudioContext  Pointer to the preferred audio context value
                                       Note: The upper layer must pass a reference to the required
                                             variable on the stack or allocate memory for it.

    \returns AudioAnnouncementParserStatus AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS if
                                           the preferred audio context LTV structure is present,
                                           otherwise an error code will be returned
                                           (see AudioAnnouncementParserStatus definition).

    NOTE: In case the returned status is not success, the function will set the value pointed
          by preferredAudioContext to zero.
 */
AudioAnnouncementParserStatus AudioAnnouncementParserGetPrefAudioContextFromMetadata(
                                                                 uint8 metadataLen,
                                                                 uint8 *metadata,
                                                                 AudioAnnouncementParserAudioContextType *preferredAudioContext);

/*! \brief Utility function to get the streaming audio context from the metadata.

    \param metadataLen                 Length of the metadata
    \param metadata                    Pointer to the metadata
    \param[out] streamingAudioContext  Pointer to the streaming audio context value
                                       Note: The upper layer must pass a reference to the required
                                             variable on the stack or allocate memory for it.

    \returns AudioAnnouncementParserStatus AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS if
                                           the streaming audio context LTV structure is present,
                                           otherwise an error code will be returned
                                           (see AudioAnnouncementParserStatus definition).

    NOTE: In case the returned status is not success, the function will set the value pointed
          by streamingAudioContext to zero.
 */
AudioAnnouncementParserStatus AudioAnnouncementParserGetStreamAudioContextFromMetadata(
                                                                 uint8 metadataLen,
                                                                 uint8 *metadata,
                                                                 AudioAnnouncementParserAudioContextType *streamingAudioContext);

/*! \brief Utility function to get the program info from the metadata.

    \param metadataLen         Length of the metadata
    \param metadata            Pointer to the metadata
    \param[out] programInfoLen Program Info string length
                               Note: The upper layer must pass a reference to the required
                                     variable on the stack or allocate memory for it.

    \returns uint8 If the Program Info LTV structure is found,
                   the library will allocate memory for its value,
                   it will return the pointer to the value and it will set the
                   length of Program Info value in programInfoLen.
                   It will be resposanbility of the upper layer
                   that is calling this function to free the memory of the value.
                   Otherwise the function will return NULL and
                   it will set the length of the value to zero.
 */
uint8 *AudioAnnouncementParserGetProgramInfoFromMetadata(uint8 metadataLen,
                                                         uint8 *metadata,
                                                         uint8 *programInfoLen);

/*! \brief Utility function to get the language from the metadata.

    \param metadataLen       Length of the metadata
    \param metadata          Pointer to the metadata
    \param[out] languageLen  Pointer to the length of the language value
                             Note: The upper layer must pass a reference to the required
                                   variable on the stack or allocate memory for it.

    \returns uint8 If the Language LTV structure is found,
                   the library will allocate memory for its value,
                   it will return the pointer to the value and it will set the
                   length of language value (always 3) in languageLen.
                   It will be resposanbility of the upper layer
                   that is calling this function to free the memory of the value.
                   Otherwise the function will riturn NULL and
                   it will set the length of the value to zero.
 */
char *AudioAnnouncementParserGetLanguageFromMetadata(uint8 metadataLen,
                                                     uint8 *metadata,
                                                     uint8 *languageLen);

/*! \brief Utility function to get the audio active state from the metadata.

    \param metadataLen            Length of the metadata
    \param metadata               Pointer to the metadata
    \param[out] audioActiveState  Pointer to the audio active state value
                                  Note: The upper layer must pass a reference to the required
                                        variable on the stack or allocate memory for it.

    \returns AudioAnnouncementParserStatus AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS if
                                           the audio active state LTV structure is present,
                                           otherwise an error code will be returned
                                           (see AudioAnnouncementParserStatus definition).

    NOTE: In case the returned status is not success, the function will set the value pointed
          by audioActiveState to zero.
 */
AudioAnnouncementParserStatus AudioAnnouncementParserGetAudioActiveStateFromMetadata(uint8 metadataLen,
                                                                                     uint8 *metadata,
                                                                                     AudioAnnouncementParserAudioActiveState *audioActiveState);

/*!
    @brief Check if the Public Broadcast audio announcement is present
           in the advertising data and if it is, it gets the PBP features in it.

    @param advDataLen    Advertising data length
    @param advData       Advertising data to parse
    @param[out] pbpData  Pointer to the PBP features value.
                         Note: The upper layer must pass a reference to the required
                               variable on the stack or allocate memory for it.

    @return AudioAnnouncementParserStatus AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS if
                                          the Public Broadcast Audio Announcement is found,
                                          otherwise an error code will be returned
                                          (see AudioAnnouncementParserStatus definition).

    NOTE: In case the returned status is not success,the function will set to zero/NULL
          all the data in pbpData parameter.
          It is responsability of the upper layer that is calling this function to free
          the memory associate to all the pointers in the pbpData structure.
*/
AudioAnnouncementParserStatus AudioAnnouncementParserPublicBcastAudioAnnouncementParsing(
                                                                    uint16 advDataLen,
                                                                    uint8 *advData,
                                                                    AudioAnnouncementParserPbpDataType *pbpData);

#endif /* AUDIO_ANNOUNCEMENT_PARSER_LIB_H */
