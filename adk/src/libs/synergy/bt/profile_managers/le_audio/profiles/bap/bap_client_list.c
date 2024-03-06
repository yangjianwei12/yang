/*******************************************************************************

Copyright (C) 2018-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "bap_client_list.h"

void bapClientListInitialise(BapClientList * const bapClientList)
{
    bapClientLifoListInitialise(&bapClientList->bapClientLifoList);
    bapClientList->tail = NULL;
}

Bool bapClientListPush(BapClientList * const bapClientList, BapClientListElement * const bapClientListElement)
{
    if (bapClientListElement == NULL)
    {
        return FALSE;
    }

    /* This qbl_list_element is being added to the tail of the list, make
     * sure it's 'next' pointer doesn't point to anything */
    bapClientListElementSetNext(bapClientListElement, NULL);

    if (bapClientLifoListIsEmpty(&bapClientList->bapClientLifoList)) /* If this is an empty list */
    {
        /* Set the head of the list (add this element to the head of the list) */
        bapClientList->bapClientLifoList.head = bapClientListElement;
    }
    else
    {
        /* Add this element to the back (tail) of the list */
        bapClientListElementSetNext(bapClientList->tail, bapClientListElement);
    }
    bapClientList->tail = bapClientListElement;

    return TRUE;
}


Bool bapClientListRemove(BapClientList * const bapClientList, BapClientListElement * const bapClientListElement)
{
    BapClientListElement* elem1 = NULL;
    BapClientListElement* elem2 = NULL;

    if ((bapClientListElement == NULL) ||
        (bapClientLifoListIsEmpty(&bapClientList->bapClientLifoList)))
    {
        return FALSE;
    }

    /* Trasnverse through the list for the list element  */
    for (elem2 = bapClientList->bapClientLifoList.head;
         elem2 != NULL;
         elem1 = elem2, elem2 = bapClientListElementGetNext(elem2))
    {
        if (elem2 == bapClientListElement)
        {
            /* Check if list element is at the head of the list */
            if (elem1 == NULL)
            {
                /* List element to be removed is at the head of the list */
                /* Update the head of the list with the next element in the list */
                bapClientList->bapClientLifoList.head = bapClientListElementGetNext(elem2);
            }
            else
            {
                /* List element to be removed is not at the head of the list */
                elem2  = bapClientListElementGetNext(elem2);
                bapClientListElementSetNext(elem1, elem2);
                /* Check if list element is at tail of the list */
                if (elem2 == NULL)
                {
                    /* Update the tail of the list */
                    bapClientList->tail = elem1;
                }
            }
            return TRUE;
        }
    }

    return FALSE;
}
