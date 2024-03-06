/*!
\copyright  Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       charger_case_config.h
\brief      Application configuration file
*/

#ifndef CHARGER_CASE_CONFIG_H_
#define CHARGER_CASE_CONFIG_H_

/*! The number of battery servers that are initialized for charger_case application */
#define NUMBER_BATTERY_SERVERS_CHARGER_CASE (1)

/*! The number of Trusted Devices supported by ChargerCase */
#define appConfigChargerCaseMaxDevicesSupported()  (8)

/*! Timeout for a handset to initiate pairing, after charger case has entered
    DFU handset pairing mode. */
#define appConfigDfuHandsetPairingModeTimeoutMs() (D_SEC(60))

/*! Initialize major and minor upgrade version information */
#define UPGRADE_INIT_VERSION_MAJOR (1)
#define UPGRADE_INIT_VERSION_MINOR (0)

/*! The factory-set PS config version. After a successful upgrade the values from
    the upgrade header will be written to the upgrade PS key and used in future.*/
#define UPGRADE_INIT_CONFIG_VERSION (1)

#endif /* CHARGER_CASE_CONFIG_H_ */

//void ChargerCaseUi_ConfigureFocusSelection(void);
