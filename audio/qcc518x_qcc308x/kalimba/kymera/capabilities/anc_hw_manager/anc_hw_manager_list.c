/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  anc_hw_manager_list.c
 * \ingroup anc_hw_manager
 *
 * ANC HW Manager (AHM) functions for linked lists that describe shared
 * ANC attributes.
 */

/****************************************************************************
Include Files
*/

#include "anc_hw_manager_list.h"

/******************************************************************************
Private Constant Definitions
*/

/****************************************************************************
Static Function Definitions
*/

/****************************************************************************
Public Function Definitions
*/
AHM_SHARED_FINE_GAIN* ahm_list_fine_gain_add(AHM_FINE_GAIN_NODE **p_head, EXT_OP_ID ext_op_id)
{
    AHM_FINE_GAIN_NODE *p_node;

    /* Create the new node */
    p_node = xzppnew(AHM_FINE_GAIN_NODE, MALLOC_PREFERENCE_SHARED);
    if (p_node == NULL)
    {
        L0_DBG_MSG1("OPID: %x, AHM failed to allocate shared gain element", ext_op_id);
        return NULL;
    }

    /* Insert the node at the head of the list */
    p_node->p_next = *p_head;
    *p_head = p_node;

    return &p_node->data;
}

void ahm_list_fine_gain_remove(AHM_FINE_GAIN_NODE **p_head,
                               AHM_SHARED_FINE_GAIN *p_gain,
                               EXT_OP_ID ext_op_id)
{
    AHM_FINE_GAIN_NODE *p_node, *p_prev_node;

    /* Validate the pointers */
    if (*p_head == NULL || p_gain == NULL)
    {
        L2_DBG_MSG3("OPID: %x, AHM failed to remove gain node from list, \
                    p_head = %p, p_gain = %p", ext_op_id, *p_head, p_gain);
        return;
    }

    /* Head node is special cased to ensure that the pointer is stored
     * correctly.
     */
    p_node = *p_head;
    if (&p_node->data == p_gain)
    {
        *p_head = p_node->p_next;
        pdelete(p_node);
        return;
    }

    /* Loop through the nodes looking for the matching node */
    p_prev_node = *p_head;
    for(p_node=p_prev_node->p_next; p_node; p_node=p_node->p_next)
    {
        if (&p_node->data == p_gain)
        {
            p_prev_node->p_next = p_node->p_next;
            pdelete(p_node);
            break;
        }
        p_prev_node = p_node;
    }
}

void ahm_list_destroy(AHM_FINE_GAIN_NODE **p_head)
{
    AHM_FINE_GAIN_NODE *p_node, *p_temp;

    p_node = *p_head;

    while (p_node != NULL)
    {
        p_temp = p_node->p_next;
        pdelete(p_node);
        p_node = p_temp;
    }

    *p_head = NULL;
}
