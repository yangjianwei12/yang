/*!
\copyright  Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\ingroup    adk_test_common
\brief      Interface for voice assistant application testing functions
*/

/*! @{ */

#ifndef ADK_TEST_VA_H
#define ADK_TEST_VA_H

/*! \brief Simulates a "Button Down -> Button Up -> Single Press Detected" sequence
           for the default configuration of a dedicated VA button.
*/
void appTestVaTap(void);

/*! \brief Simulates a "Button Down -> Button Up -> Button Down -> Button Up -> Double Press Detected"
           sequence for the default configuration of a dedicated VA button.
*/
void appTestVaDoubleTap(void);

/*! \brief Simulates a "Button Down -> Hold" sequence for the default configuration
           of a dedicated VA button.
*/
void appTestVaPressAndHold(void);

/*! \brief Simulates a "Button Up" event for the default configuration
           of a dedicated VA button.
*/
void appTestVaRelease(void);

/*! \brief Simulates a "Button Down -> Long Press" sequence for the default configuration
           of a dedicated VA button.
*/
void appTestVaHeldRelease(void);

/*! \brief Set the active voice assistant to GAA.
*/
void appTestSetActiveVa2GAA(void);

/*! \brief Set the active voice assistant to AMA.
*/
void appTestSetActiveVa2AMA(void);

/*! \brief Check if any voice assistant audio use-case is active.
 *  \return TRUE if any voice assistant audio use-case is active, otherwise FALSE.
*/
bool appTestIsVaAudioActive(void);

/*! \brief Get the selected voice assistant provider.
 *  \return The ID for the selected voice assistant provider (see voice_ui_provider_t).
*/
unsigned appTest_VaGetSelectedAssistant(void);

#ifdef INCLUDE_AMA
/*! \brief Print the current AMA locale to the log output.
*/
void appTestPrintAmaLocale(void);

/*! \brief Set the AMA locale.
 *  \param locale A pointer to the locale string.
*/
void appTestSetAmaLocale(const char *locale);
#endif /* INCLUDE_AMA */

/*! \brief Check if the VA active device is in sniff mode.
 *  \return TRUE if VA is active and device is in sniff mode, otherwise FALSE.
*/
bool appTestIsVaDeviceInSniff(void);

/*! \brief Set the privacy mode flag.
 *  \param enabled Set to TRUE to enable or FALSE to disable.
*/
void appTestSetVaPrivacyMode(bool enabled);

/*! \brief Check if the device is in privacy mode.
 *  \return TRUE if privacy mode is enabled, otherwise FALSE.
*/
bool appTestIsVaPrivacyModeEnabled(void);

#endif /* ADK_TEST_VA_H */

/*! @} */
