/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup device_test_service
    @{
    \brief      Useful helper functions for implementing Device Test Service commands.
*/

#include <csrtypes.h>

/*! \brief Convert a hex character to a number

    Converts the character 0-9, A-F and a-f to the equivalent
    decimal value.

    \param hex_char Character to convert

    \returns the decimal representation of the hex character. Illegal
    characters will return 0
 */
uint8 deviceTestService_HexToNumber(char hex_char);

/*! @} End of group documentation */
