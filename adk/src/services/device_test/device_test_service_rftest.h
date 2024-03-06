/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup device_test_service
    @{
    \brief      Interface file for RF Test functions of the device test service.
*/


#ifdef INCLUDE_DEVICE_TEST_SERVICE_RADIOTEST_V2

#ifdef USE_SYNERGY

void DeviceTestService_HandleCmRfTestPrim(Message message);

#else

/*! \brief Message handler for connection library messages

    \param id               The message ID
    \param message          The message payload
    \param already_handled  Whether the message has been handled by another task
                            before this function was called.

    \return TRUE if the message was handled
 */
bool DeviceTestService_HandleConnectionLibraryMessages_RfTest(MessageId id, 
                                                              Message message, 
                                                              bool already_handled);

#endif


#else /* !INCLUDE_DEVICE_TEST_SERVICE_RADIOTEST_V2 */

#ifdef USE_SYNERGY

#define DeviceTestService_HandleCmRfTestPrim(_msg) \
                    (UNUSED(_msg))

#else

#define DeviceTestService_HandleConnectionLibraryMessages_RfTest(_id, _msg, _handled) \
                    (UNUSED(_id), UNUSED(_msg), UNUSED(_handled), FALSE)

#endif


#endif /* INCLUDE_DEVICE_TEST_SERVICE_RADIOTEST_V2 */

/*! @} */