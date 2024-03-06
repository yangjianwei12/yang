
/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   list list
\ingroup vm_libs
\brief      List API
*/

#ifndef LIST_H_
#define LIST_H_

#include <stdint.h>
#include <stdbool.h>

/*\{*/

typedef struct list_protected_data_t* list_t;

typedef enum list_type_t
{
    /** Additional Guarantees:
     *  Memory addresses to element's data provided to the user will always be aligned and remain valid as long as the element itself is not removed from the list
     */
    list_type_linked_single = 0,
    list_type_array,
} list_type_t;

typedef enum list_store_t
{
    list_store_variable_size_element = -1,
    list_store_reference = 0,
} list_store_t;

typedef struct list_config_t
{
    /** Select List implementation
     * \see list_type_t
     * 
     * Default: list_type_linked_single
     */
    list_type_t type;

    /** Element store
     * list_store_reference: Store the element as a mere reference (not its contents)
     * list_store_variable_size_element: Store a copy of the element. Variable element size
     * value > 0: Store a copy of the element. All elements have the size specified by "value"
     *
     * Default: list_store_reference
     */
    union {
        list_store_t type;
        size_t size;
    } element_store;
} list_config_t;

/**
 * \brief Iterate over a List starting from head
 * \warning Many List functions such as ListRemoveCurrentElement will effect or even break the loop logic provided here, use with caution when combining it with calls that update the lists current position
 */
#define LIST_FOREACH(list_handle_reference) for(ListGotoHead(list_handle_reference); !ListIsAtEndOfList(list_handle_reference); ListGotoNext(list_handle_reference))

/**
 * \brief Gets the default configuration for the library
 *
 * To know default values for each config option:
 * \see list_config_t
 *
 * Default Config is const, to use it as a base for a custom config it must be copied first
 * \returns A pointer to the Default Configuration
 */
const list_config_t* ListGetDefaultConfig(void);

/**
 * \brief Creates a List
 *
 * \param [in] config: Specify the List configuration (can be NULL for default configuration), this reference must remain valid and the contents unchanged throughout the lifetime of the List
 *
 * \return The List handle
 */
list_t ListCreate(const list_config_t *config);

/**
 * \brief Destroys a List
 *
 * \param [in/out] list: List to destroy (list handle will be set to NULL)
 */
void ListDestroy(list_t *list);

/**
 * \brief Move to the head element of the list (current position = head)
 *        If list is empty it will remain at end-of-list position
 *
 * \param [in/out] list: The List
 */
void ListGotoHead(list_t *list);

/**
 * \brief Move to the tail element of the list (current position = tail)
 *        If list is empty it will remain at end-of-list position
 *
 * \param [in/out] list: The List
 */
void ListGotoTail(list_t *list);

/**
 * \brief Move to the end-of-list position of the list (current position = end-of-list)
 *
 * \param [in/out] list: The List
 */
void ListGotoEndOfList(list_t *list);

/**
 * \brief Move to the next element of the list, if the current position is the lists tail, then it will be moved to the end-of-list
 *
 * \warning Will panic if already at end-of-list
 *
 * \param [in/out] list: The List
 */
void ListGotoNext(list_t *list);

/**
 * \brief Move to the previous element of the list, if the current position is the lists head, then it will be moved to the end-of-list
 *
 * \warning Will panic if already at end-of-list
 *
 * \param [in/out] list: The List
 */
void ListGotoPrevious(list_t *list);

/**
 * \brief Add element as the new tail of the list
 *        The current position will stay the same (same element)
 *
 * \warning Can only be used with fixed size element lists, will panic otherwise
 *
 * \param [in/out] list: List to append to
 * \param [in] element: Element to append
 */
void ListAppend(list_t *list, const void *element);

/**
 * \brief Same as ListAppend with support for variable size element lists
 *
 * \warning For fixed size element lists unless the size matches the fixed size specified at list creation this call will panic
 *
 * \param [in/out] list: List to append to
 * \param [in] element: Element to append
 * \param [in] size: Element size in bytes
 */
void ListAppendWithSize(list_t *list, const void *element, size_t size);

/**
 * \brief Add element as the new head of the list
 *        The current position will stay the same (same element)
 *
 * \warning Can only be used with fixed size element lists, will panic otherwise
 *
 * \param [in/out] list: List to prepend to
 * \param [in] element: Element to prepend
 */
void ListPrepend(list_t *list, const void *element);

/**
 * \brief Same as ListPrepend with support for variable size element lists
 *
 * \warning For fixed size element lists unless the size matches the fixed size specified at list creation this call will panic
 *
 * \param [in/out] list: List to prepend to
 * \param [in] element: Element to prepend
 * \param [in] size: Element size in bytes
 */
void ListPrependWithSize(list_t *list, const void *element, size_t size);

/**
 * \brief Insert the new element after the current position in the list
 *        If the current position is end-of-list, then the new element will be added as the new tail of the list
 *        The current position will stay the same (same element)
 *
 * \warning Can only be used with fixed size element lists, will panic otherwise
 *
 * \param [in/out] list: List to insert to
 * \param [in] element: Element to insert
 */
void ListInsertAfterCurrent(list_t *list, const void *element);

/**
 * \brief Same as ListInsertAfterCurrent with support for variable size element lists
 *
 * \warning For fixed size element lists unless the size matches the fixed size specified at list creation this call will panic
 *
 * \param [in/out] list: List to append to
 * \param [in] element: Element to append
 * \param [in] size: Element size in bytes
 */
void ListInsertAfterCurrentWithSize(list_t *list, const void *element, size_t size);

/**
 * \brief Insert the new element before the current position in the list
 *        If the current position is end-of-list, then the new element will be added as the new head of the list
 *        The current position will stay the same (same element)
 *
 * \warning Can only be used with fixed size element lists, will panic otherwise
 *
 * \param [in/out] list: List to insert to
 * \param [in] element: Element to insert
 */
void ListInsertBeforeCurrent(list_t *list, const void *element);

/**
 * \brief Same as ListInsertBeforeCurrent with support for variable size element lists
 *
 * \warning For fixed size element lists unless the size matches the fixed size specified at list creation this call will panic
 *
 * \param [in/out] list: List to prepend to
 * \param [in] element: Element to prepend
 * \param [in] size: Element size in bytes
 */
void ListInsertBeforeCurrentWithSize(list_t *list, const void *element, size_t size);

/**
 * \brief Update the lists current element contents with those provided
 *
 * \warning Will panic if at end-of-list
 *
 * \param [in/out] list: The List
 * \param [in] element: The contents to update the current element with
 */
void ListUpdateCurrentElement(list_t *list, const void *element);

/**
 * \brief Same as ListUpdateCurrentElement but asserting that the size requested matches the current element's size
 *
 * \warning Will panic if at end-of-list
 *
 * \param [in/out] list: The List
 * \param [in] element: The contents to update the current element with
 * \param [in] size: The element's size, has to match the current element's size or the call will panic
 */
void ListUpdateCurrentElementWithSize(list_t *list, const void *element, size_t size);

/**
 * \brief Remove the current element from the list, the current position will then point to the next element in the list (until end-of-list is reached)
 *
 * \warning Will panic if already at end-of-list
 *
 * \param [in/out] list: The List
 */
void ListRemoveCurrentElement(list_t *list);

/**
 * \brief Get the data stored as the current element in the list
 *
 * \warning Will panic if at end-of-list
 *
 * \param [in/out] list: The List
 * \param [out] element: The memory address to use to copy the current element's data
 */
void ListGetCurrentElement(list_t *list, void *element);

/**
 * \brief Same as ListGetCurrentElement but asserting that the size requested matches the current element's size
 *
 * \warning Will panic if at end-of-list
 *
 * \param [in/out] list: The List
 * \param [out] element: The memory address to use to copy the current element's data
 * \param [in] size: The element's size, has to match the current element's size or the call will panic
 */
void ListGetCurrentElementWithSize(list_t *list, void *element, size_t size);

/**
 * \brief Get direct access to the memory block containing the current element in the list
 *
 * \warning Any subsequent call to the library using the same list can invalidate the pointer returned (see list_type_t for additional implementation specific guarantees)
 * \warning The address returned could be unaligned (see list_type_t for additional implementation specific guarantees)
 * \warning Will panic if at end-of-list
 *
 * \param [in/out] list: The List
 *
 * \return Pointer to the memory block containing the current element in the list
 */
void* ListGetCurrentElementAddress(list_t *list);

/**
 * \brief Get current reference element from the list
 *
 * \warning Will panic if at end-of-list
 * \warning Will panic if not a reference list
 *
 * \param [in/out] list: The List
 *
 * \return The current reference element (not a pointer to memory internal to the list, merely the same reference that was added to the list)
 */
void* ListGetCurrentReference(list_t *list);

/**
 * \brief Get the current element's size
 *
 * \warning Will panic if at end-of-list
 *
 * \param [in/out] list: The List
 *
 * \return The size of the current element
 */
size_t ListGetCurrentElementSize(list_t *list);

/**
 * \brief Get the current length of the list (number of elements stored)
 *
 * \param [in/out] list: The List
 *
 * \return The length of the list
 */
size_t ListGetLength(list_t *list);

/**
 * \brief Check if the list is empty or not
 *
 * \param [in/out] list: The List
 *
 * \return TRUE if empty, FALSE otherwise
 */
bool ListIsEmpty(list_t *list);

/**
 * \brief Check if the current position is end-of-list
 *
 * \param [in/out] list: The List
 *
 * \return TRUE if at end-of-list, FALSE otherwise
 */
bool ListIsAtEndOfList(list_t *list);

/*\}*/

#endif /* LIST_H_ */
