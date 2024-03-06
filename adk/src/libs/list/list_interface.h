/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      List interface definition
*/

#ifndef LIST_INTERFACE_H_
#define LIST_INTERFACE_H_

#include <list.h>
#include <data_blob_types.h>

typedef struct list_protected_data_t
{
    // The interface of the specific implementation used for this list
    const struct list_interface_t* implementation_if;
    // The user configuration to be used for this list
    const list_config_t* config;
} list_protected_data_t;

typedef struct list_interface_t
{
    list_t          (*ListCreate)(void);
    void            (*ListDestroy)(list_t *list);
    void            (*ListGotoHead)(list_t *list);
    void            (*ListGotoTail)(list_t *list);
    void            (*ListGotoEndOfList)(list_t *list);
    void            (*ListGotoNext)(list_t *list);
    void            (*ListGotoPrevious)(list_t *list);
    void            (*ListAppend)(list_t *list, const void *element, size_t size);
    void            (*ListPrepend)(list_t *list, const void *element, size_t size);
    void            (*ListInsertAfterCurrent)(list_t *list, const void *element, size_t size);
    void            (*ListInsertBeforeCurrent)(list_t *list, const void *element, size_t size);
    void            (*ListRemoveCurrentElement)(list_t *list);
    data_blob_t     (*ListGetCurrentElement)(list_t *list);
    size_t          (*ListGetLength)(list_t *list);
    bool            (*ListIsAtEndOfList)(list_t *list);
} list_interface_t;

const list_interface_t* ListGetInterface(const list_config_t *config);

#endif /* LIST_INTERFACE_H_ */
