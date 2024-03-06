/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      List debug helpers
*/

#ifndef LIST_PRIVATE_H_
#define LIST_PRIVATE_H_

#include <logging.h>

#define ASSERT_NOT_NULL(ptr)    PanicFalse(ptr != NULL)
#define ASSERT_VALID_HANDLE_REFERENCE(reference) do{ASSERT_NOT_NULL(reference); ASSERT_NOT_NULL(*reference);}while(0)

#endif /* LIST_PRIVATE_H_ */
