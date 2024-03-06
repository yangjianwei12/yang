/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       fast_pair_hkdf_test.h
\brief      Test code for the fast pair hkdf implementation.
*/

#ifndef FAST_PAIR_HKDF_TEST_H
#define FAST_PAIR_HKDF_TEST_H

bool fastPair_HkdfTestEncryptedConnectionFieldTest(void);

bool fastPair_HkdfTestAesCtrTest(void);

bool fastPair_HkdfTestAesCtrExampleVector(void);

bool fastPair_HkdfTestAesCtrExampleVectorNoEndianSwap(void);

#endif // FAST_PAIR_HKDF_TEST_H
