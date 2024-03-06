/****************************************************************************
 * Copyright (c) 2016 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  buffer_metadata_private_data.c
 *
 * \ingroup buffer
 *
 * Metadata private data common handling
 */

/****************************************************************************
Include Files
*/
#include "buffer_private.h"
#include "buffer_metadata_eof_copy_del.h"
#include <string.h>

/****************************************************************************
Private Macro Declarations
*/

/* Rounded length of a private data item (in allocation units) */
#define PRIV_ITEM_LENGTH(length) ROUND_UP_TO_WHOLE_WORDS(sizeof(metadata_priv_item) + (length))

/* Get a pointer to the next private data item in the array */
#define PRIV_ITEM_NEXT(item) (metadata_priv_item *)((unsigned *)(item) + PRIV_ITEM_LENGTH((item)->length)/sizeof(unsigned));

/****************************************************************************
Private Function Declarations
*/
static void tag_deletion_scan(void* cb_data, metadata_tag *tag, METADATA_PRIV_KEY item_key, unsigned item_length, void *item_data, bool *done);
static void tag_copy_scan(void* cb_data, metadata_tag *tag, METADATA_PRIV_KEY item_key, unsigned item_length, void *item_data, bool *done);

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief Get total length (in allocation units) of existing private data
 */
unsigned buff_metadata_get_priv_data_length(metadata_tag *tag)
{
    unsigned count = 0, num_items;
    metadata_priv_item *item;

    PL_ASSERT(tag->xdata != NULL);

    num_items = tag->xdata->item_count;
    item = (metadata_priv_item *)&tag->xdata->items[0];
    while (count < num_items)
    {
        item = PRIV_ITEM_NEXT(item);
        count++;
    }

    /* Pointer subtraction will give size in addresses */
    return ((char *)item - (char *)(tag->xdata));
}

/*
 * \brief Add private data to a metadata tag
 */
void* buff_metadata_add_private_data(metadata_tag *tag, METADATA_PRIV_KEY key, unsigned length, void *data)
{
    unsigned old_size,new_size;
    metadata_priv_data *new_data = NULL;
    metadata_priv_item *new_item;

    patch_fn_shared(buff_metadata);

    if (tag->xdata == NULL)
    {
        /* Allow space for the item count in the first word */
        old_size = sizeof(unsigned);
    }
    else
    {
        old_size = buff_metadata_get_priv_data_length(tag);
        /* If the old size isn't at least a word, something has gone badly wrong */
        PL_ASSERT(old_size >= sizeof(unsigned));
    }

    /* First check if we can reuse the existing allocation */
    new_size = old_size + PRIV_ITEM_LENGTH(length);
    /* Note psizeof(NULL) returns zero, so this is always safe */
    if (new_size > psizeof(tag->xdata))
    {
#if defined(COMMON_SHARED_HEAP)
        /* New allocation needed */
        if ((new_data = (metadata_priv_data *)xppmalloc(new_size, MALLOC_PREFERENCE_SHARED)) == NULL)
#else
        /* New allocation needed */
        if ((new_data = (metadata_priv_data *)xpmalloc(new_size)) == NULL)
#endif /* COMMON_SHARED_HEAP */
        {
            /* Allocation failed, just return NULL without changing anything */
            return NULL;
        }
    }
    if (tag->xdata != NULL)
    {
        /* Existing data, so we need to copy it if there was a new allocation */
        if (new_data != NULL)
        {
            /* Copy all of the existing data (including the item count)... */
            memcpy(new_data, tag->xdata, old_size);
            /* ...and free the old data */
            pfree(tag->xdata);
            tag->xdata = new_data;
        }
        /* Increment the item count for the new item */
        tag->xdata->item_count++;
    }
    else
    {
        /* No existing data, so this must be a new allocation */
        PL_ASSERT(new_data != NULL);
        tag->xdata = new_data;
        tag->xdata->item_count = 1;
    }
    /* Populate the new item */
    new_item = (metadata_priv_item *)((char *)(tag->xdata) + old_size);
    new_item->key = key;
    new_item->length = length;

    /* Copy the new data if provided */
    if (data != NULL)
    {
        memcpy(&(new_item->data), data, length);
    }

    return &(new_item->data);
}

/*
 * \brief Find private data in a metadata tag
 */
bool buff_metadata_find_private_data(metadata_tag *tag, METADATA_PRIV_KEY key, unsigned *out_length, void **out_data)
{
    unsigned count = 0, num_items;
    metadata_priv_item *item;

    patch_fn_shared(buff_metadata);

    if (tag->xdata != NULL)
    {
        num_items = tag->xdata->item_count;
        item = (metadata_priv_item *)&tag->xdata->items[0];
        while (count < num_items)
        {
            if (item->key == key)
            {
                *out_length = item->length;
                *out_data = &item->data;
                return TRUE;
            }
            item = PRIV_ITEM_NEXT(item);
            count++;
        }
    }
    return FALSE;
}

/*
 * \brief Call a processing function for each piece of private data found
 *
 * \param tag The metadata tag whose private data should be processed
 * \param process_fn A private data handler function
 * \param cb_data State for the private data handler function
 */
void buff_metadata_scan_private_data(metadata_tag *tag, metadata_scan_private_data_fn process_fn, void *cb_data)
{
    unsigned count = 0, num_items;
    metadata_priv_item *item;
    bool done;

    patch_fn_shared(buff_metadata);

    if (tag->xdata != NULL)
    {
        num_items = tag->xdata->item_count;
        item = (metadata_priv_item *)&tag->xdata->items[0];
        while (count < num_items)
        {
            done = FALSE;
            (process_fn)(cb_data, tag, item->key, item->length, &item->data, &done);
            if (done)
            {
                return;
            }
            item = PRIV_ITEM_NEXT(item);
            count++;
        }
    }
}

/**
 * \brief A tag is being deleted. For private data types
 *        which need it, call the tag deletion handler.
 *
 * \param cb_data Scan function state
 * \param tag The metadata tag containing the private data
 * \param item_key The private data key
 * \param item_length The length of the private data, in address units
 * \param item_data Pointer to the start of the private data
 * \param done Set this to TRUE to stop scanning the private data
 */
static void tag_deletion_scan(void* cb_data, metadata_tag *tag, METADATA_PRIV_KEY item_key, unsigned item_length, void *item_data, bool *done)
{
    patch_fn_shared(buff_metadata);

    PL_ASSERT(item_data != NULL);
    switch (item_key)
    {
    case META_PRIV_KEY_EOF_CALLBACK:
    {
        metadata_eof_handle_deletion(cb_data, tag, item_length, item_data);
        break;
    }

    default:
        break;
    }
}

/**
 * \brief Scan private data, if any, and call private data deletion handlers
 *
 * \param tag The metadata tag to be deleted
 */
void metadata_handle_tag_deletion(metadata_tag *tag)
{
    patch_fn_shared(buff_metadata);

    buff_metadata_scan_private_data(tag, &tag_deletion_scan, NULL);
}

/*
 * \brief A tag is being copied. For private data types
 *        which need it, call the tag copy handler.
 *
 * \param cb_data The remote_copy argument to metadata_handle_tag_copy.
 * \param tag The metadata tag containing the private data
 * \param item_key The private data key
 * \param item_length The length of the private data, in address units
 * \param item_data Pointer to the start of the private data
 * \param done Set this to TRUE to stop scanning the private data
 */
static void tag_copy_scan(void* cb_data, metadata_tag *tag, METADATA_PRIV_KEY item_key, unsigned item_length, void *item_data, bool *done)
{
    patch_fn_shared(buff_metadata);

    PL_ASSERT(item_data != NULL);
    switch (item_key)
    {
    case META_PRIV_KEY_EOF_CALLBACK:
    {
        metadata_eof_handle_copy(cb_data, tag, item_length, item_data);
        break;
    }

    default:
        break;
    }
}

/**
 * \brief Scan private data, if any, and call private data copy handlers.
 * \param tag The metadata tag to be copied
 * \param remote_copy TRUE if the copy is sent to the other core in dual
 *                    core operation. This parameter is passed to
 *                    tag_copy_scan function as the cb_data argument.
 */
void metadata_handle_tag_copy(metadata_tag *tag, bool remote_copy)
{
    patch_fn_shared(buff_metadata);

    buff_metadata_scan_private_data(tag, &tag_copy_scan, (void*)(uintptr_t)remote_copy);
}

#ifdef DESKTOP_TEST_BUILD

void print_metadata_priv(metadata_tag *tag)
{
    unsigned count = 0, num_items;
    metadata_priv_item *item;
    if (tag == NULL)
    {
        printf("Tag is NULL\n");
        return;
    }
    if (tag->xdata == NULL)
    {
        printf("Private data is NULL\n");
        return;
    }
    num_items = tag->xdata->item_count;
    item = (metadata_priv_item *)&tag->xdata->items[0];
    while (count < num_items)
    {
        printf("Item at %p key: %u length: %u data[0]: 0x%X \n", item, item->key, item->length, item->data[0]);

        item = PRIV_ITEM_NEXT(item);
        count++;
    }
}

#endif /* DESKTOP_TEST_BUILD */

