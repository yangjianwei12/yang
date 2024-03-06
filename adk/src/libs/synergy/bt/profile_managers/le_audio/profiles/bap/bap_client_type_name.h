/*******************************************************************************

Copyright (C) 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef TYPENAME_H_
#define TYPENAME_H_

#include <assert.h>
#include "qbl_types.h"
#include "qbl_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(DEBUG) && !defined(__KCC__)
#define TYPE_NAME_NUM_CHARS 4

StaticAssert(((sizeof(char) * TYPE_NAME_NUM_CHARS) == sizeof(uint32)), The_TYPE_NAME_implementation_has_expectations_on_uint_sizes);
typedef struct TYPE_NAME
{
    union
    {
        char     value[TYPE_NAME_NUM_CHARS];
        uint32 v;
    } u;
} TYPE_NAME;

/* Public interface */

/*
 * Overview
 *
 * Run Time Type Information (RTTI) is used to verify that:
 *   1) 'Entities' used within the software have actually been
 *      initialised before any attempt to use them.
 *   2) Any use of a VTable to call a virtual function uses a valid
 *      (and initialised) pointer to the derived class.
 *
 * RTTI only affects debug builds; for release builds the RTTI macros
 * are empty and take no code or data space
 *
 */
/*
 * type_name_declare_rtti_member_variable
 *
 * Each 'entity' (e.g. 'struct') within the software must declare an
 * RTTI member variable using this macro. For example:
 *
 * typedef struct SOME_ENTITY
 * {
 *     uint32 some_data;
 *     uint32 some_more_data;
 *     type_name_declare_rtti_member_variable * Note: The absence of a trailing semi-colon *
 * } SOME_ENTITY;
 *
 */
#define type_name_declare_rtti_member_variable      TYPE_NAME type_name;
/*
 * type_name_declare_and_initialise_const_rtti_variable(T, c0, c1, c2, c3)
 *
 * When following the coding guidelines; each 'entity' has a corresponding
 * 'struct' and a corresponding .h and .c file.
 *
 * The .h file includes a 'struct' (that must declare an RTTI member variable
 * using the macro: type_name_declare_rtti_member_variable) and the .c file
 * must declare a const variable using the
 * type_name_declare_and_initialise_const_rtti_variable macro. For example:
 *
 * file some_entity.c :
 * -----------------------------------------------
 * #include "some_entity.h"
 *
 * type_name_declare_and_initialise_const_rtti_variable(SOME_ENTITY, 'S', 'o', 'm', 'E')
 *
 * void some_entity_initialise(SOME_ENTITY* some_entity)
 * {
 * ...
 * }
 * .
 * .
 * .
 * -----------------------------------------------
 *
 * Params:
 *
 *    1) T: The 'type' for which RTTI is being enabled
 *    2) c0, c1, c2, c3: A unique set of four characters that do not clash with
 *       the characters used by any other invocation of
 *       type_name_declare_and_initialise_const_rtti_variable. Preferably the four
 *       characters will bear some resemblance to the entity name for which
 *       RTTI is being enabled.

 */
#define type_name_declare_and_initialise_const_rtti_variable(T, c0, c1, c2, c3) \
                                                const TYPE_NAME T##_const_type_name = {{{c0, c1, c2, c3}}};
/*
 * type_name_initialise_rtti_member_variable(T, obj)
 *
 * This macro must be called from within the 'initialise' function of an 'entity',
 * e.g. for an entity called 'SOME_ENTITY' it must be called from within the
 * some_entity_initialise(SOME_ENTITY* const some_entity) function.
 *
 * Params:
 *
 *    1) T: The 'type' for which RTTI is being enabled, e.g. in the description
 *       above the 'type' is: SOME_ENTITY (upper case).
 *    2) obj: A pointer to the instance of the entity being initialised, e.g.
 *       in the description above, the pointer to the instance is: some_entity
 *       (lower case).
 *
 */
#define type_name_initialise_rtti_member_variable(T, obj)   \
                                                ((obj)->type_name = (T##_const_type_name))
/*
 * type_name_verify(T, obj)
 *
 * This macro is called to verify that a pointer to an 'entity' is valid and
 * that it's 'initialise' function has been called. This macro must be called
 * at the beginning of all functions and macros (with one exception) that
 * operate on an 'entity' to verify that the pointer is valid and points to
 * an initialised instance of the entity.
 * There is one exception to this rule: within the entity's 'initialise' function
 * the macro: type_name_initialise_rtti_member_variable() is called (not type_name_verify()).
 *
 * Note: To use this macro the entity must have used the following macros correctly:
 *
 * type_name_declare_rtti_member_variable
 * type_name_declare_and_initialise_const_rtti_variable()
 * type_name_initialise_rtti_member_variable()
 *
 * Params:
 *    T: The expected 'type' of the entity being verified
 *    obj: A pointer to the instance of the entity being verified
 *
 */
/* Note: the outermost surrounding braces are deliberately omitted: they're not essential and they cause warnings in some (useful) situations */
#define type_name_verify(T, obj)     assert(obj), assert((obj)->type_name.u.v == (T##_const_type_name).u.v)
/*
 * type_name_enable_verify_of_external_type(T)
 *
 * This macro enables RTTI for entities defined in _other_ .h/.c files.
 * Typically the macro 'type_name_verify(SOME_ENTITY, some_entity)' is called from
 * within some_entity.c. However, sometimes it is necessary to verify
 * an entity defined/declared in other .h/.c files. For this to work correctly
 * this macro must be used. For example:
 *
 * file some_entity.c :
 * -----------------------------------------------
 * #include "some_entity.h"
 * #include "other_entity.h"
 *
 * type_name_declare_and_initialise_const_rtti_variable(SOME_ENTITY, 'S', 'o', 'm', 'E')
 *
 * type_name_enable_verify_of_external_type(OTHER_ENTITY)
 *
 * void some_entity_do_useful_things(SOME_ENTITY* some_entity)
 * {
 *     type_name_verify(SOME_ENTITY, some_entity);
 * ...
 *     OTHER_ENTITY* other_entity = stack_get_other_entity(&some_entity->stack);
 *     type_name_verify(OTHER_ENTITY, other_entity);
 * ...
 * }
 * .
 * .
 * .
 * -----------------------------------------------
 * *
 *
 * Params:
 *    T: The 'type' of the external entity for which RTTI is being enabled.
 */
#define type_name_enable_verify_of_external_type(T) extern const TYPE_NAME T##_const_type_name;

#else

#define type_name_declare_rtti_member_variable
#define type_name_declare_and_initialise_const_rtti_variable(T, c0, c1, c2, c3)
#define type_name_initialise_rtti_member_variable(T, obj)  ((void)obj)
#define type_name_verify(T, obj)    ((void)obj)
#define type_name_enable_verify_of_external_type(T)

#endif

#ifdef __cplusplus
}
#endif

#endif /* TYPENAME_H_ */

