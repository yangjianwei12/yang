/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Header file for the production test mode
*/

#ifndef EARBUD_PRODUCTION_TEST_H_
#define EARBUD_PRODUCTION_TEST_H_

#ifdef PRODUCTION_TEST_MODE

/*! \brief SM boot mode */
typedef enum
{
    sm_boot_normal_mode,
    sm_boot_production_test_mode,
    sm_boot_mute_mode,

} sm_boot_mode_t;

typedef enum
{
    LSM6DSTQ_SENSOR_ID = 1,
    IQS269A_SENSOR_ID,
    LIS25BA_SENSOR_ID,
    TXCPA224_SENSOR_ID,
} sm_production_test_sensor_id_t;

#define HOST_MESSAGE_PRODUCTION_TEST_LENGTH 0x0003
#define HOST_MESSAGE_PRODUCTION_TEST_ID 0x0001
#define HOST_MESSAGE_SENSORS_TEST 0x0001
#define HOST_MESSAGE_SET_PTM_STEP 0x0002
#define HOST_MESSAGE_NEXT_PTM_STEP 0x0003


/*! \brief Handle request to enter FCC test mode.
*/
void appSmHandleInternalEnterProductionTestMode(void);

/*! \brief Handle request to enter DUT test mode.
*/
void appSmHandleInternalEnterDUTTestMode(void);

/*! \brief Request To enter Production Test mode
*/
void appSmEnterProductionTestMode(void);

/*! \brief Write Test mode PS Key
*/
void appSmTestService_SaveBootMode(sm_boot_mode_t mode);

/*! \brief Check boot mode, expose API so earbud UI can rely on this
    to play production test tones
*/
sm_boot_mode_t appSmTestService_BootMode(void);
/*! \brief Write Test Step
*/

void appSmTestService_SetTestStep(uint8 step);
/*! \brief Handle Host Message for Production Test
*/
void appSmHandleHostMessagesForProductionTest(Message message);

#else
#define appSmTestService_BootMode(void) FALSE
#define appSmTestService_SaveBootMode(mode) (UNUSED(mode))
#define appSmTestService_SetTestStep(step) (UNUSED(step))

#endif /*PRODUCTION_TEST_MODE*/

#endif /*EARBUD_PRODUCTION_TEST_H_*/
