/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_bap
    \brief
    @{
*/

#ifndef PACS_UTILITIES_H_
#define PACS_UTILITIES_H_

#include "gatt_pacs_server.h"
#ifdef USE_SYNERGY
#include "bap_server_prim.h"
#else
#include "bap_server.h"
#endif
#include "bt_types.h"
#include "pacs_config.h"

#define PACS_PREFERRED_AUDIO_CONTEXTS_TYPE  0x1

/*! \brief Structure storing PACS record handle list. */
typedef struct
{
    uint8   handle_count;
    uint16  handles[1];
} le_bap_pacs_records_t;


/*! \brief Initialises the Published Audio Capabilities Service (PACS),

     Creates the PACS server instance, note this must be called before GATT Initialisation is completed
 */
void LeBapPacsUtilities_Init(void);

/*! \brief Allows the addition of a single sink/source PAC record.

    \param sink_or_source Indicates if the record to add is a sink or source PAC.
    \param additional_pac Pointer to the PAC record to add.
 */
void LeBapPacsUtilities_AddPacRecord(PacsServerDirectionType sink_or_source, const void * additional_pac, bool vendor_specific);

/*! \brief Gets PACS record (list of handles) for sink or source PACS.

    \param[in] for_sink TRUE indicates for source, else for sink PACS.

    \return PACS record pointer (Shouldn't freed or altered or stored)
 */
const le_bap_pacs_records_t* LeBapPacsUtilities_GetPacHandles(bool for_sink);

/*! \brief Gets the Sink audio context availability as currently set in PACS

    \return The current audio context availability
 */
uint16 LeBapPacsUtilities_GetSinkAudioContextAvailability(void);

/*! \brief Sets the Sink audio context availability in PACS

    \param audio_contexts
 */
void LeBapPacsUtililties_SetSinkAudioContextAvailability(uint16 audio_contexts);

/*! \brief Checks if the specified Sink audio context is available in PACS

    \param audio_contexts
 */
bool LeBapPacsUtilities_IsSinkAudioContextAvailable(PacsAudioContextType audio_context);

/*! \brief Utility function to check if a given codec ID and codec specific configuration is supported according to the current PACS records

    \param coding_format
    \param company_id
    \param vendor_specific_codec_id
    \param sampling_frequency
    \param frame_duration
    \param octets_per_frame

    \returns TRUE if a matching PAC record is found, else FALSE
 */
bool LeBapPacsUtilities_IsCodecIdAndSpecificCapabilitiesSupportedBySink(uint8 coding_format, 
                                                                        uint16 company_id, 
                                                                        uint16 vendor_specific_codec_id,
                                                                        PacsSamplingFrequencyType sampling_frequency,
                                                                        PacsFrameDurationType frame_duration,
                                                                        uint16 octets_per_frame);

/*! \brief Gets the Source audio context availability as currently set in PACS

    \return The current audio context availability
 */
uint16 LeBapPacsUtilities_GetSourceAudioContextAvailability(void);

/*! \brief Sets the Source audio context availability in PACS

    \param audio_contexts
 */
void LeBapPacsUtililties_SetSourceAudioContextAvailability(uint16 audio_contexts);

/*! \brief Checks if the specified Source audio context is available in PACS

    \param audio_contexts
 */
bool LeBapPacsUtilities_IsSourceAudioContextAvailable(PacsAudioContextType audio_context);

/*! \brief Utility function to check if a given codec config is supported according to the current PACS records

    \param coding_format
    \param company_id
    \param vendor_specific_codec_id

    \returns TRUE if a matching PAC record is found, else FALSE
 */
bool LeBapPacsUtilities_IsCodecConfigSupportedBySource(uint8 coding_format, uint16 company_id, uint16 vendor_specific_codec_id);

/*! \brief Gets the Sink audio location as currently set in PACS

    \return The current audio location
 */
uint32 LeBapPacsUtilities_GetSinkAudioLocation(void);


/*! \brief Set the Bap handle as PAC instance handle 
    \param bap_handle   BAP profile handle 
*/
void LeBapPacsSetBapHandle(ServiceHandle bap_handle);

/*! \brief Claim the Sink audio context from the available audio contexts.

    \param audio_context
 */
void LeBapPacsUtilities_ClaimSinkAudioContext(AudioContextType audio_context);

/*! \brief Claim the Source audio context from the available audio contexts.

    \param audio_context
 */
void LeBapPacsUtilities_ClaimSourceAudioContext(AudioContextType audio_context);

/*! \brief Restore the Sink audio context back to available audio contexts.

    \param audio_context
 */
void LeBapPacsUtilities_RestoreSinkAudioContext(AudioContextType audio_context);

/*! \brief Restore the Source audio context back to available audio contexts.

    \param audio_context
 */
void LeBapPacsUtilities_RestoreSourceAudioContext(AudioContextType audio_context);

/*! \brief Check LC3 EPC license.
 
    \return Returns TRUE if license is available
 */
bool LeBapPacsUtilities_Lc3EpcLicenseCheck(void);

/*! \brief Get the LC3 frame length in the SDU
 
    \return Returns LC3 frame length
 */
uint16 LeBapPacsUtilities_Lc3GetFrameLength(uint16 max_sdu_size, uint16 blocks_per_sdu, uint16 audio_channel_mask);

/*! \brief Get PACS record of the Source/Sink

    \param is_source TRUE if PACS to be retrieved is from Source else FALSE
    \param pacs_count

    \return Returns PACS record of the Source/Sink
 */
const GattPacsServerRecordType* LeBapPacsUtilities_GetPacs(bool is_source, uint16 *pacs_count);

/*! \brief Converts sampling frequency to PACS sampling frequency type

    \param sampling_frequency sampling frequency to be converted

    \return Returns PACS sampling frequency
 */
PacsSamplingFrequencyType LeBapPacsUtilities_GetPacsSamplingFreqBitMaskFromFreq(uint32 sampling_frequency);

/*! \brief Gets the preferred Audio context from the PACS record

    \param pac_metadata metadata from PACS record
    \param pac_metadata_len length of metadata

    \return Returns Preferred Audio context from PACS recrd
 */

AudioContextType LeBapPacsUtilities_GetPreferredAudioContext(const uint8 *pac_metadata , uint8 pac_metadata_len);

/*! \brief Retrieve Client config
    \param cid Connection id 

    \return Returns the retrieved client config
 */
void * LeBapPacsUtilities_RetrieveClientConfig(gatt_cid_t cid);

/*! \brief Store Client config
    \param cid Connection id 
    \param config The config data to be stored
    \param size Size of config data
 */
void LeBapPacsUtilities_StoreClientConfig(gatt_cid_t cid, void * config, uint8 size);

#if (defined (INCLUDE_LE_APTX_ADAPTIVE) || defined (INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE)) && defined (USE_SYNERGY)

/*! \brief Get PACS record of the Source/Sink (where the PACs record is of VS type)

    \param is_source TRUE if PACS to be retrieved is from Source else FALSE
    \param pacs_count

    \return Returns VS PACS record of the Source/Sink
 */

const GattPacsServerVSPacRecord* LeBapPacsUtilities_GetPacsVS(bool is_source, uint16 *pacs_count, uint8 vs_codec);

/*! \brief Gets the preferred Audio context from the VS PACS record

    \param pac_record PACS record from which audio context to be fetched

    \return Returns Preferred Audio context from VS PACS recrd
 */

PacsSamplingFrequencyType LeBapPacsUtilities_GetSampleRateFromVSPac(const GattPacsServerVSPacRecord *pacs_record);

#endif /* INCLUDE_LE_APTX_ADAPTIVE */

#endif /* PACS_UTILITIES_H_ */
/*! @} */