/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  ml_engine_usecase_manager.h
 * \ingroup capabilities
 *
 * ML Engine use-case manager header file. <br>
 *
 */

#ifndef ML_ENGINE_USECASE_MANAGER_H
#define ML_ENGINE_USECASE_MANAGER_H

#include "../../capabilities/ml_engine/ml_engine_struct.h"

/* Function pointer to delete the usecase/tensor */
typedef void(*free_ml_engine_node)(void *);

/**
 * @fn int ml_engine_list_add(ML_ENGINE_NODE **head, void *value, uint16 node_id)
 * @brief Helper function to add usecase/tensor to linked list
 * @param head head pointer of linked list
 * @param value pointer of the usecase/tensor structure to be addded
 * @param node_id tensor id/use-case id
 * @return status (TRUE/FALSE)
 */
bool ml_engine_list_add(ML_ENGINE_NODE **head, void *value, uint16 node_id);

/**
 * @fn void * ml_engine_list_find(ML_ENGINE_NODE *p_head, uint16 node_id)
 * @brief Helper function to get the usecase/tensor from linked list for a given tensor-id/usecase-id
 * @param p_head head pointer of linked list
 * @param node_id id of the tensor/use-case
 * @return return the pointer of the use-case/tensor structure
 */
void * ml_engine_list_find(ML_ENGINE_NODE *p_head, uint16 node_id);

/**
 * @fn void ml_engine_delete_node(ML_ENGINE_NODE **p_head, uint16 node_id, free_ml_engine_node fn_free)
 * @brief Deletes a node from linked-list with given node-id
 * @param p_head head pointer of linked list
 * @param node_id id of the tensor/use-case
 * @param fn_free function pointer to delete the node and associated data
 * @return
 */
void ml_engine_delete_node(ML_ENGINE_NODE **p_head, uint16 node_id, free_ml_engine_node fn_free);

/**
 * @fn void ml_engine_delete_all_node(ML_ENGINE_NODE **p_head, free_ml_engine_node fn_free)
 * @brief Deletes all nodes of a given linked-list
 * @param p_head head pointer of linked list
 * @param fn_free function pointer to delete the node and associated data
 * @return
 */
void ml_engine_delete_all_node(ML_ENGINE_NODE **p_head, free_ml_engine_node fn_free);

/**
 * @fn free_usecase_node(void *usecase)
 * @brief Deletes all items associated with use-case node.
 * @param usecase pointer
 * @return
 */
void ml_engine_free_usecase_node(void *usecase);
#endif /* ML_ENGINE_H */
