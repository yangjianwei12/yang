/*!
    \copyright  Copyright (c) 2022 - 2023  Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file       
    \addtogroup user_accounts
    \brief      user_accounts_sync component
    @{
*/
#ifndef USER_ACCOUNTS_SYNC_H
#define USER_ACCOUNTS_SYNC_H

#include <marshal_common.h>
#include <marshal.h>
#include <message.h>
#include <task_list.h>

/*! Complete Account Keys Data Length in words(uint16) */
#define ACCOUNT_KEY_DATA_LEN            (40)
#define ACCOUNT_KEY_INDEX_LEN           (10)

/*! Account key sync task data. */
typedef struct
{
    TaskData task;
} account_key_sync_task_data_t;

/*! Component level visibility of Account Key Sync Task Data */
extern account_key_sync_task_data_t account_key_sync;

#define accountKeySync_GetTaskData() (&account_key_sync)
#define accountKeySync_GetTask() (&account_key_sync.task)

typedef struct account_key_sync_req
{
    uint32 account_key_index[ACCOUNT_KEY_INDEX_LEN/2];
    uint16 account_keys[ACCOUNT_KEY_DATA_LEN];
} account_key_sync_req_t;

typedef struct account_key_sync_cfm
{
    bool synced;
} account_key_sync_cfm_t;

/*! Create base list of marshal types the account key sync will use. */
#define MARSHAL_TYPES_TABLE_ACCOUNT_KEY_SYNC(ENTRY) \
    ENTRY(account_key_sync_req_t) \
    ENTRY(account_key_sync_cfm_t)

/*! X-Macro generate enumeration of all marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),

enum MARSHAL_TYPES_ACCOUNT_KEY_SYNC
{
    /*! common types must be placed at the start of the enum */
    DUMMY_ACCOUNT_SYNC = NUMBER_OF_COMMON_MARSHAL_OBJECT_TYPES-1,
    /*! now expand the marshal types specific to this component. */
    MARSHAL_TYPES_TABLE_ACCOUNT_KEY_SYNC(EXPAND_AS_ENUMERATION)
    NUMBER_OF_MARSHAL_OBJECT_TYPES
};
#undef EXPAND_AS_ENUMERATION

/*! Make the array of all message marshal descriptors available. */
extern const marshal_type_descriptor_t * const account_key_sync_marshal_type_descriptors[];

/*! \brief Account Key Sync Initialization
    This is used to initialize account key sync interface.
 */
void UserAccountsSync_Init(void);

/*! \brief Account Key Synchronization API
    This is the API which triggers the synchronization between the peers.
 */
void UserAccountsSync_Sync(void);

#endif /*! USER_ACCOUNTS_SYNC_H */
/*! @} */