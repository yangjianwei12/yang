/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef CONTAINERCAST_H_
#define CONTAINERCAST_H_

#include <stddef.h>
#include <assert.h>
#include "bap_client_type_name.h"
#include "csr_panic.h"
#include "csr_bt_panic.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! Code assertion that can be checked at run time. This will cause a panic. */
#define BAP_ASSERT(x) (!(x) ? CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, "bap_list_container PANIC"): (void) 0 )

/*
 * CONTAINER CAST
 *
 * NOTE: The compiler (that we tested with) optimised the whole macro down
 * to a single assignment operation in optimised (release) builds where the
 * 'member' was the first element in the 'type' structure.
 *
 * This WILL cause compilation errors (in debug builds) if casting to
 * 'entities' that do not have RTTI enabled: use the macros in qbl_type_name.h
 * to enable RTTI. NOTE: The 'type_name' set of macros are empty (zero cost)
 * in release builds.
 *
 * Some explanation...
 *
 * The construct: (((ptr) != NULL)?
 *                     (type*) (.....)
 *                :
 *                     (type*)(NULL))
 *
 * This protection code ensures that when a NULL pointer is passed in, a
 * NULL pointer is returned. It is useful protection for when the resulting
 * pointer is then checked against NULL (before dereferencing).
 *
 * The assert line with '(&CONTAINER_PTR(ptr, type, member)->member == ptr)' has:
 * 1. The same compiler type checking that is provided by the 'union
 *    of derived classes inside the base class' approach.
 * 2. 'Easily human readable' verification that the 'member' really
 *    is at the same address as 'ptr'.
 * 3. Zero cost (code/data) in optimised (release) builds.
 *
 * The 'type_name_verify(type, CONTAINER_PTR(ptr, type, member))'...
 *
 * Firstly: the vanilla 'union of derived classes inside the base class'
 * approach suffers from a human error problem: a user can write into
 * the union using derived class X and read from the union using derived
 * class Y. The compiler won't complain, but the code might not behave
 * as expected. The CONTAINER_CAST approach protects against this inadvertent
 * misuse by calling 'type_name_verify'.
 *
 * Calling 'type_name_verify' has:
 * 1. An increased level of type checking (when compared with vanilla 'union of
 *    derived classes inside the base class').
 * 2. A guarantee that the derived class really has been constructed (by a previous
 *    call to 'classname_initialise()')
 * 3. Zero cost (code/data) in optimised (release) builds (it is removed completely)
 *
 */

#define CONTAINER_PTR(ptr, type, member) (type *)((size_t)(ptr) - (size_t)offsetof(type, member))

#define CONTAINER_CAST(ptr, type, member)                                        \
    (((ptr) != NULL)?                                                            \
        (type*) (BAP_ASSERT(&((CONTAINER_PTR(ptr, type, member))->member) == (ptr)), \
                 type_name_verify(type, CONTAINER_PTR(ptr, type, member)),        \
                 CONTAINER_PTR(ptr, type, member))                               \
    :                                                                            \
        (type*)(BAP_ASSERT(FALSE), NULL))


#ifdef __cplusplus
}
#endif

#endif /* CONTAINERCAST_H_ */
