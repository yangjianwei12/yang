/*******************************************************************************

Copyright (C) 2018-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "bap_client_lifo_list.h"
#include "bap_client_list_container_cast.h"


bool bapClientLifoListPush(BapClientLifoList * const bapClientLifoList, BapClientListElement * const bapClientListElement)
{
    if (bapClientListElement == NULL)
    {
        return FALSE;
    }

    /* This qbl_list_element is being added to the head of the list, make
     * sure its 'next' pointer points to the previous head of the list */
    bapClientListElementSetNext(bapClientListElement, bapClientLifoList->head);
    bapClientLifoList->head = bapClientListElement;

    return TRUE;
}

bool bapClientLifoListRemove(BapClientLifoList * const this, BapClientListElement * const elementToRemove)
{
    typedef struct
    {
        BapClientListElement listElement;
    } DummyListElementSpecialization;

    bool anElementWasRemoved = FALSE;

    bapClientLifoListRemoveIf(this,
                              listElement,  /* The field name of a BapClientListElement (within DummyListElementSpecialization) */
                              DummyListElementSpecialization,
                              dummyListElementSpecialisation,
                              &dummyListElementSpecialisation->listElement == elementToRemove, 
                              anElementWasRemoved = TRUE);

    return anElementWasRemoved;
}

BapClientListElement *bapClientLifoListPop(BapClientLifoList * const bapClientLifoList)
{
    BapClientListElement *el = bapClientLifoList->head;

    /* Anything there? */
    if (el != NULL)
    {
        /* 'Detach' the head */
        bapClientLifoList->head = bapClientListElementGetNext(bapClientLifoList->head);
    }

    return el;
}

size_t bapClientLifoListGetSize(BapClientLifoList * const bapClientLifoList)
{
    BapClientListElement* el;
    size_t size = 0;

    for (el = bapClientLifoList->head; /* set 'el' to the second element in the list */
         el != NULL;
         el  = bapClientListElementGetNext(el))
    {
        size++;
    }

    return size;
}
