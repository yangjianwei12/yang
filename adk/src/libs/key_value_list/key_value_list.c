/*!
\copyright  Copyright (c) 2018-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Source file for a data structure with a list of { key, value } elements.

*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <panic.h>
#include <vmtypes.h>

#include "key_value_list.h"

typedef struct
{
    void *value;
    size_t value_size;
    key_value_key_t key;
} kv_element_t;

/*! The linked list of elements that do not fit into 32 bits */
typedef struct large_kv_element_t
{
    struct large_kv_element_t *next;
    size_t len;
    key_value_key_t key;
    uint8 data[1];
} large_kv_element_t;

/*! Structure to store key/value data. Types <= 32 bits are stored in dynamic
    fixed-type arrays. Types > 32 bits are stored in a linked list. */
struct key_value_list_tag
{
    /*! Length of dynamic keys array */
    uint8 len_keys;
    /*! Length of dynamic values8 array */
    uint8 len8;
    /*! Length of dynamic values16 array */
    uint8 len16;
    /*! Length of dynamic values32 array */
    uint8 len32;

    /* Pointer to arrays of keys. */
    key_value_key_t *keys;
    /* Pointer to array of uint8s */
    uint8 *values8;
    /* Pointer to array of uint16s */
    uint16 *values16;
    /* Pointers to arrays of uint32/ptrs */
    uint32 *values32;

    /* Linked list of larger elements */
    large_kv_element_t *head;
};

struct key_value_list_iterator_tag
{
    key_value_list_t list;
    large_kv_element_t *current_large_element;
    uint8 current_keys_index;
};

/*****************************************************************************/

static bool keyValueList_addKeyValuePair(key_value_list_t list, key_value_key_t key, const void * value, size_t size)
{
    void *dest = NULL;
    unsigned key_index = 0;
    unsigned new_len;
    large_kv_element_t *ele;

    switch (size)
    {
        case sizeof(uint8):
            key_index = list->len8;
            new_len = list->len8 + 1;
            list->values8 = PanicNull(realloc(list->values8, sizeof(uint8) * new_len));
            dest = list->values8 + list->len8;
            list->len8 = new_len;
            break;

        case sizeof(uint16):
            key_index = list->len8 + list->len16;
            new_len = list->len16 + 1;
            list->values16 = PanicNull(realloc(list->values16, sizeof(uint16) * new_len));
            dest = list->values16 + list->len16;
            list->len16 = new_len;
            break;

        case sizeof(uint32):
            key_index = list->len8 + list->len16 + list->len32;
            new_len = list->len32 + 1;
            list->values32 = PanicNull(realloc(list->values32, sizeof(uint32) * new_len));
            dest = list->values32 + list->len32;
            list->len32 = new_len;
            break;

        default:
            ele = PanicUnlessMalloc(sizeof(*ele) + size - 1);
            ele->next = list->head;
            ele->len = size;
            ele->key = key;
            list->head = ele;
            dest = ele->data;
            break;
    }

    memmove(dest, value, size);

    if (size == sizeof(uint8) || size == sizeof(uint16) || size == sizeof(uint32))
    {
        key_value_key_t *key_p;
        new_len = list->len_keys + 1;
        list->keys = PanicNull(realloc(list->keys, sizeof(*list->keys) * new_len));
        key_p = list->keys + key_index;
        memmove(key_p + 1, key_p, sizeof(*key_p) * (list->len_keys - key_index));
        list->len_keys = new_len;
        *key_p = key;
    }

    return TRUE;
}

static kv_element_t keyValueList_GetElementUsingKeysIndex(key_value_list_t list, uint8 keys_index)
{
    kv_element_t element = {0};

    if (keys_index >= list->len_keys)
    {
        return element;
    }

    element.key = list->keys[keys_index];

    if (keys_index < list->len8)
    {
        element.value_size = sizeof(uint8);
        element.value = list->values8 + keys_index;
        return element;
    }

    keys_index -= list->len8;

    if (keys_index < list->len16)
    {
        element.value_size = sizeof(uint16);
        element.value = list->values16 + keys_index;
        return element;
    }

    keys_index -= list->len16;

    if (keys_index < list->len32)
    {
        element.value_size = sizeof(uint32);
        element.value = list->values32 + keys_index;
        return element;
    }

    /* Should have returned in one of the branches above */
    Panic();
    return element;
}

static key_value_element_t keyValueList_GetUsingKeysIndex(key_value_list_t list, uint8 keys_index)
{
    key_value_element_t value = {0};
    kv_element_t element = keyValueList_GetElementUsingKeysIndex(list, keys_index);
    value.key = element.key;
    value.value = element.value;
    value.value_size = element.value_size;
    return value;
}

static key_value_element_t keyValueList_GetFromNode(const large_kv_element_t *node)
{
    key_value_element_t value = {0};
    value.key = node->key;
    value.value = node->data;
    value.value_size = node->len;
    return value;
}

static unsigned keyValueList_GetLengthOfLargeElements(key_value_list_t list)
{
    unsigned length = 0;
    large_kv_element_t *current_node = list->head;

    while(current_node)
    {
        length++;
        current_node = current_node->next;
    }

    return length;
}

/*****************************************************************************/
key_value_list_t KeyValueList_Create(void)
{
    size_t size = sizeof(struct key_value_list_tag);
    key_value_list_t list = PanicUnlessMalloc(size);
    memset(list, 0, size);
    return list;
}

void KeyValueList_Destroy(key_value_list_t* list)
{
    PanicNull(list);
    KeyValueList_RemoveAll(*list);
    free(*list);
    *list = NULL;
}

bool KeyValueList_Add(key_value_list_t list, key_value_key_t key, const void *value, size_t size)
{
    bool success = FALSE;

    PanicNull(list);

    if (!KeyValueList_IsSet(list, key))
    {
        success = keyValueList_addKeyValuePair(list, key, value, size);
    }

    return success;
}

bool KeyValueList_Get(key_value_list_t list, key_value_key_t key, void **value_p, size_t *size_p)
{
    unsigned index;
    key_value_key_t *keys;
    large_kv_element_t *ele;

    PanicNull(list);
    PanicNull(value_p);
    PanicNull(size_p);

    for (ele = list->head; ele != NULL; ele = ele->next)
    {
        if (ele->key == key)
        {
            *size_p = ele->len;
            *value_p = ele->data;
            return TRUE;
        }
    }

    keys = list->keys;

    for (index = 0; index < list->len_keys; index++)
    {
        if (*keys++ == key)
        {
            kv_element_t  element = keyValueList_GetElementUsingKeysIndex(list, index);
            if (element.value)
            {
                *size_p = element.value_size;
                *value_p = element.value;
                return TRUE;
            }
            break;
        }
    }

    return FALSE;
}

void *KeyValueList_GetSized(key_value_list_t list, key_value_key_t key, size_t size)
{
    unsigned index;
    key_value_key_t *keys;
    large_kv_element_t *ele;

    PanicNull(list);

    keys = list->keys;

    switch (size)
    {
        case sizeof(uint8):
            for (index = 0; index < list->len8; index++)
            {
                if (*keys++ == key)
                {
                    return list->values8 + index;
                }
            }
            break;

        case sizeof(uint16):
            keys += list->len8;
            for (index = 0; index < list->len16; index++)
            {
                if (*keys++ == key)
                {
                    return list->values16 + index;
                }
            }
            break;

        case sizeof(uint32):
            keys += list->len8;
            keys += list->len16;
            for (index = 0; index < list->len32; index++)
            {
                if (*keys++ == key)
                {
                    return list->values32 + index;
                }
            }
            break;

        default:
            /* Key not found in fixed type list, now search in dynamic list */
            for (ele = list->head; ele != NULL; ele = ele->next)
            {
                if (ele->key == key)
                {
                    size = ele->len;
                    return ele->data;
                }
            }
            break;
    }

    /* Key not found based on size, logical error if the key exists with an
       unexpected size */
    PanicFalse(!KeyValueList_IsSet(list, key));
    return NULL;
}

void KeyValueList_Remove(key_value_list_t list, key_value_key_t key)
{
    unsigned key_index;
    unsigned index;
    key_value_key_t *keys;
    large_kv_element_t **elpp, *elp;

    PanicNull(list);

    for (elpp = &list->head; (elp = *elpp) != NULL; )
    {
        if (elp->key == key)
        {
            *elpp = elp->next;
            free(elp);
            return;
        }
        else
        {
            elpp = &elp->next;
        }
    }

    keys = list->keys;

    for (key_index = 0; key_index < list->len_keys; key_index++)
    {
        if (keys[key_index] == key)
        {
            index = key_index;
            if (index < list->len8)
            {
                uint8 *dest = list->values8 + index;
                memmove(dest, dest + 1, sizeof(uint8) * (list->len8 - index));
                list->len8 -= 1;
                if (list->len8)
                {
                    list->values8 = PanicNull(realloc(list->values8, sizeof(uint8) * list->len8));
                }
                else
                {
                    free(list->values8);
                    list->values8 = NULL;
                }
            }
            else
            {
                index -= list->len8;
                if (index < list->len16)
                {
                    uint16 *dest = list->values16 + index;
                    memmove(dest, dest + 1, sizeof(uint16) * (list->len16 - index));
                    list->len16 -= 1;
                    if (list->len16)
                    {
                        list->values16 = PanicNull(realloc(list->values16, sizeof(uint16) * list->len16));
                    }
                    else
                    {
                        free(list->values16);
                        list->values16 = NULL;
                    }
                }
                else
                {
                    index -= list->len16;
                    if (index < list->len32)
                    {
                        uint32 *dest = list->values32 + index;
                        memmove(dest, dest + 1, sizeof(uint32) * (list->len32 - index));
                        list->len32 -= 1;
                        if (list->len32)
                        {
                            list->values32 = PanicNull(realloc(list->values32, sizeof(uint32) * list->len32));
                        }
                        else
                        {
                            free(list->values32);
                            list->values32 = NULL;
                        }
                    }
                    else
                    {
                        /* Invalid lengths */
                        Panic();
                    }
                }
            }
            break;
        }
    }

    if (key_index < list->len_keys)
    {
        key_value_key_t *dest = list->keys + key_index;
        memmove(dest, dest + 1, sizeof(*dest) * (list->len_keys - key_index));
        list->len_keys -= 1;
        if (list->len_keys)
        {
            list->keys = PanicNull(realloc(list->keys, sizeof(*list->keys) * list->len_keys));
        }
        else
        {
            free(list->keys);
            list->keys = NULL;
        }
    }
}

void KeyValueList_RemoveAll(key_value_list_t list)
{
    large_kv_element_t *head = list->head;
    while (head)
    {
        large_kv_element_t *tmp = head;
        head = head->next;
        free(tmp);
    }
    free(list->keys);
    free(list->values8);
    free(list->values16);
    free(list->values32);
    memset(list, 0, sizeof(*list));
}

bool KeyValueList_IsSet(key_value_list_t list, key_value_key_t key)
{
    size_t size;
    void *addr;
    return KeyValueList_Get(list, key, &addr, &size);
}

/*! \brief Merge key from source list to target list

    \param[in] source_list Source key-value list which is merging to the target key-value list
    \param[in] target_list Target key-value list
    \param[in] source_key Key to merge
    \param[in] source_data Value associated with the key
    \param[in] source_len Length of source_data
    \param[in] resolve_callback Callback function to call in case of any conflicts during the merge
*/
static void keyValueList_MergeKey(key_value_list_t source_list, key_value_list_t target_list,
                                  key_value_key_t source_key, void *source_data, size_t source_len,
                                  key_value_list_merge_resolve_callback_t resolve_callback)
{
    size_t target_size;
    void *target_value_p;
    key_value_list_merge_action_t resolve_action;

    /* Check whether the given key exists in target list */
    if (KeyValueList_Get(target_list, source_key, &target_value_p, &target_size))
    {
        /* Same key exists in target list as well */
        if (target_size != source_len || memcmp(target_value_p, source_data, target_size) != 0)
        {
            /* Contents of the key value pair are different in source and target lists.
               This needs to be resolved. Panic if resolve callback is not present */
            PanicNull((void*)resolve_callback);

            resolve_action = resolve_callback(source_list, target_list, source_key);

            if (resolve_action == KV_LIST_MERGE_CONFLICT_ACTION_ACCEPT_TARGET_LIST)
            {
                /* Do nothing as there is no changes needed in target list */
            }
            else if (resolve_action == KV_LIST_MERGE_CONFLICT_ACTION_ACCEPT_SOURCE_LIST)
            {
                /* Source and target size should match */
                PanicFalse(target_size == source_len);

                /* Copy the source list value to target list */
                memcpy(target_value_p, source_data, source_len);
            }
            else
            {
                /* Panic as merge conflict is not resolvable */
                Panic();
            }
        }
    }
    else
    {
        /* The key not exists in the target list. Add key value pair to the target list */
        KeyValueList_Add(target_list, source_key, source_data, source_len);
    }
}


bool KeyValueList_Merge(key_value_list_t source_list, key_value_list_t target_list,
                        key_value_list_merge_resolve_callback_t resolve_callback)
{
    unsigned index;
    key_value_key_t *source_keys;
    large_kv_element_t *source_ele;

    PanicNull(source_list);
    PanicNull(target_list);

    /* Merge larger element linked list first */
    for (source_ele = source_list->head; source_ele != NULL; source_ele = source_ele->next)
    {
        /* Merge the key value pair from source list to target list */
        keyValueList_MergeKey(source_list, target_list, source_ele->key, source_ele->data, source_ele->len,
                              resolve_callback);

    }

    /* Now merge the u8, u16 and u32 key value pairs */
    source_keys = source_list->keys;

    for (index = 0; index < source_list->len_keys; index++)
    {
        key_value_key_t source_key;
        void *source_value_p;
        size_t source_size;
        unsigned key_index;

        source_key = *source_keys++;

        if (index >= (source_list->len8 + source_list->len16 + source_list->len32))
        {
            /* The key index is out of range. This isn't expected */
            Panic();
        }

        /* Get the value corresponding to the source key from source list */
        if (index < source_list->len8)
        {
            source_size = sizeof(uint8);
            source_value_p = (void*)(source_list->values8 + index);
        }
        else if (index < (source_list->len8 + source_list->len16))
        {
            key_index = index - source_list->len8;
            source_size = sizeof(uint16);
            source_value_p = (void*)(source_list->values16 + key_index);
        }
        else
        {
            key_index = index - source_list->len8 - source_list->len16;
            source_size = sizeof(uint32);
            source_value_p = (void*)(source_list->values32 + key_index);
        }

        /* Merge the key value pair from source list to target list */
        keyValueList_MergeKey(source_list, target_list, source_key, source_value_p, source_size,
                              resolve_callback);
    }

    return TRUE;
}

key_value_list_iterator_t KeyValueList_CreateIterator(key_value_list_t list)
{
    key_value_list_iterator_t iterator;
    PanicNull(list);
    iterator = PanicUnlessMalloc(sizeof(struct key_value_list_iterator_tag));
    iterator->list = list;
    iterator->current_keys_index = 0;
    iterator->current_large_element = list->head;
    return iterator;
}

void KeyValueList_DestroyIterator(key_value_list_iterator_t *iterator)
{
    PanicNull(iterator);
    free(*iterator);
    *iterator = NULL;
}

key_value_element_t KeyValueList_Next(key_value_list_iterator_t *iterator)
{
    key_value_element_t element = {0};
    PanicFalse(iterator && *iterator);

    if ((*iterator)->current_large_element)
    {
        element = keyValueList_GetFromNode((*iterator)->current_large_element);
        (*iterator)->current_large_element = (*iterator)->current_large_element->next;
    }
    else if ((*iterator)->current_keys_index < (*iterator)->list->len_keys)
    {
        element = keyValueList_GetUsingKeysIndex((*iterator)->list, (*iterator)->current_keys_index);
        (*iterator)->current_keys_index++;
    }
    else
    {
        KeyValueList_DestroyIterator(iterator);
    }

    return element;
}

unsigned KeyValueList_GetLength(key_value_list_t list)
{
    PanicNull(list);
    return list->len_keys + keyValueList_GetLengthOfLargeElements(list);
}
