/*******************************************************************************
Copyright (c) 2017 Qualcomm Technologies International, Ltd.


FILE NAME
    anc_config_read.h

DESCRIPTION

*/

#ifndef ANC_CONFIG_READ_H_
#define ANC_CONFIG_READ_H_

#include "anc.h"

bool ancConfigReadPopulateAncData(anc_config_t * config_data, anc_mode_t set_mode);

/*!
    @brief Read fine gain from PS store for the gain path and mode specified

    @param mode            ANC mode in use
    @param gain_path       ANC gain path
    @param gain            ANC gain value

    @return TRUE indicating PS update was successful else FALSE
*/
bool ancReadFineGain(anc_mode_t mode, audio_anc_path_id gain_path, uint8 *gain);

/*!
    @brief Read fine gain from PS store for the gain path and mode specified for both the instances

    @param mode            ANC mode in use
    @param gain_path       ANC gain path
    @param gain            ANC gain value for instance 0
    @param gain            ANC gain value for instance 1

    @return TRUE indicating PS update was successful else FALSE
*/
bool ancReadFineGainDual(anc_mode_t mode, audio_anc_path_id gain_path, uint8 *instance_0_gain, uint8 *instance_1_gain);


/*!
    @brief Read coarse gain from specific ANC HW instance

    @param instance      ANC HW Instance
    @param gain_path       ANC gain path
    @param gain            ANC gain value
*/
void ancReadCoarseGainFromInst(audio_anc_instance inst, audio_anc_path_id gain_path, uint16 *gain);

/*!
    @brief Read fine gain from specific ANC HW instance

    @param instance      ANC HW Instance
    @param gain_path       ANC gain path
    @param gain            ANC gain value
*/
void ancReadFineGainFromInst(audio_anc_instance inst, audio_anc_path_id gain_path, uint8 *gain);

#ifdef ANC_UPGRADE_FILTER
/*!
    @brief Read RxMix coarse gain from specific ANC HW instance

    @param instance      ANC HW Instance
    @param gain_path       ANC gain path
    @param gain            ANC gain value
*/
void ancReadRxMixCoarseGainFromInst(audio_anc_instance inst, audio_anc_path_id path, uint16 *gain);

/*!
    @brief Read RxMix fine gain from specific ANC HW instance

    @param instance      ANC HW Instance
    @param gain_path       ANC gain path
    @param gain            ANC gain value
*/
void ancReadRxMixFineGainFromInst(audio_anc_instance inst, audio_anc_path_id path, uint8 *gain);
#endif

/*!
    @brief Read coefficients for the specified path of the exisiting mode

    @param instance      ANC HW Instance
    @param path           Coeffiecient path
    @param denominator Output parameter to hold the coeffiecients
    @param numerator Output parameter to hold the coeffiecients
*/
void ancReadModelCoefficients(audio_anc_instance inst, audio_anc_path_id path, uint32 *denominator, uint32* numerator);

/*!
    @brief Read LPF coefficients for the specified path of the exisiting mode

    @param instance         ANC HW Instance
    @param path             Coeffiecient path
    @param lpf_shift_1      Output parameter to hold the lpf shift 1
    @param lpf_shift_2      Output parameter to hold the lpf shift 2

    @return TRUE indicating the LPF coefficients were successfully read otherwise FALSE.
*/
bool ancReadLpfCoefficients(audio_anc_instance inst, audio_anc_path_id path, uint8* lpf_shift_1, uint8* lpf_shift_2);

/*!
    @brief Read DC Filter coefficients for the specified path of the exisiting mode

    @param instance         ANC HW Instance
    @param path             Coeffiecient path
    @param filter_shift     Output parameter to hold the filter shift
    @param filter_enable    Output parameter to hold the filter enable

    @return TRUE indicating the DC Filter coefficients were successfully read otherwise FALSE.
*/
bool ancReadDcFilterCoefficients(audio_anc_instance inst, audio_anc_path_id path, uint8* filter_shift, uint8* filter_enable);

/*!
    @brief Read Small LPF coefficients for the specified instance of the exisiting mode

    @param instance         ANC HW Instance
    @param filter_shift     Output parameter to hold the filter shift
    @param filter_enable    Output parameter to hold the filter enable

    @return TRUE indicating the Small LPF coefficients were successfully read otherwise FALSE.
*/
bool ancReadSmallLpfCoefficients(audio_anc_instance inst, uint8* filter_shift, uint8* filter_enable);

#endif
