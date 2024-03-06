/*
 * FreeRTOS Kernel V10.2.1
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

#include "FreeRTOS.h"
#include "pmalloc/pmalloc.h"

#if configSUPPORT_DYNAMIC_ALLOCATION == 1

/**
 * Implements the FreeRTOS dynamic memory allocation on top of the pmalloc pool
 * implementation. If this malloc implementation is used the application's pools
 * will need to be re-configured to account for any dynamic memory allocations
 * made by FreeRTOS.
 */

/**
 * \brief Non-panicking malloc implementation using xpmalloc.
 *
 * \param [in] wanted_size_bytes  The requested allocation size in bytes.
 */
void *pvPortMalloc(size_t wanted_size_bytes)
{
    return xpmalloc(wanted_size_bytes);
}

/**
 * \brief free implementation using pfree.
 *
 * \param [in] ptr  The pointer to free.
 */
void vPortFree(void *ptr)
{
    pfree(ptr);
}

#endif /* configSUPPORT_DYNAMIC_ALLOCATION == 1 */
