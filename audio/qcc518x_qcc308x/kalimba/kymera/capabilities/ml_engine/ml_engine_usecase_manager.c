/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  ml_engine_usecase_manager.c
 * \ingroup  capabilities
 *
 *  An implementation for managing the use-cases of the ML Engine capability
 *
 */

#include "capabilities.h"
#include "ml_engine_usecase_manager.h"
#include "macros.h"
#include "ml_runtime_if.h"


/****************************************************************************
Private Function Definitions
*/


/****************************************************************************
Private Constant Declarations
*/



void ml_engine_free_usecase_node(void *usecase)
{
    USECASE_INFO *use_case = (USECASE_INFO *)usecase;
    ml_free_context(use_case->ml_model_context);
    pfree(use_case->input_tensor.tensors);
    pfree(use_case->output_tensor.tensors);
    pfree(use_case);
}

void ml_engine_delete_node(ML_ENGINE_NODE **p_head, uint16 node_id, free_ml_engine_node fn_free)
{
    ML_ENGINE_NODE *temp = *p_head;
    ML_ENGINE_NODE *prev = NULL;

    // Check if first node matches
    if (temp != NULL && temp->node_id == node_id)
    {
        *p_head = temp->p_next;

        // Delete node data
        fn_free(temp->p_data);

         // Delete the node
      
        pfree(temp);
        return;

    }

    // Search node with node id
    while (temp != NULL && temp->node_id != node_id) {
        prev = temp;
        temp = temp->p_next;
    }
    // If node id was not present in linked list
    if (temp == NULL )
        return;
    // Change the previous node
    prev->p_next = temp->p_next;
    // Delete node data
    fn_free(temp->p_data);

    // Delete the node
   
    pfree(temp);
}


void ml_engine_delete_all_node(ML_ENGINE_NODE **p_head, free_ml_engine_node fn_free)
{
    ML_ENGINE_NODE *current = *p_head;
    ML_ENGINE_NODE *next = NULL;
    while (current != NULL)
    {
        next = current->p_next;

        // Delete node data
        fn_free(current->p_data);

         // Delete the node
        pfree(current);

        current = next;
    }
    *p_head = NULL;


}

void * ml_engine_list_find(ML_ENGINE_NODE *p_head, uint16 node_id)
{
   ML_ENGINE_NODE *temp = p_head;
   void *value = NULL;
    // Search for node which contains node id as that node
    while (temp != NULL && temp->node_id != node_id) {
        temp = temp->p_next;
    }
    if(temp)
        value = temp->p_data;

    return value;
}


bool ml_engine_list_add(ML_ENGINE_NODE **head, void *value, uint16 node_id)
{

    ML_ENGINE_NODE *node = xppmalloc(sizeof(ML_ENGINE_NODE),DM2);
    if (node == NULL)
    {
        return FALSE;
    }
    node->node_id = node_id;
    node->p_data =  value;
    node->p_next = NULL;

    /* Add current item to the 'front' of the list */
    if (*head != NULL)
    {
        node->p_next = *head;
    }
    *head = node;
    return TRUE;
}
