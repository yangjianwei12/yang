/* Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef MCP_PRIVATE_H
#define MCP_PRIVATE_H

#include <stdlib.h>

#include "csr_bt_gatt_lib.h"

#include "mcp.h"
#include "service_handle.h"

#define McpMessageSend(TASK, MSG) CsrSchedMessagePut(TASK, MCP_PRIM, MSG)

/* Element of the list of MCS service handles */
struct McsSrvcHndl
{
    ServiceHandle srvcHndl;
    struct McsSrvcHndl *next;
};

typedef struct McsSrvcHndl McpMcsSrvcHndl;

typedef struct ProfileHandleListElement
{
    struct ProfileHandleListElement    *next;
    struct ProfileHandleListElement    *prev;
    ServiceHandle                   profile_handle;
} ProfileHandleListElm_t;

/* The Media Control Profile internal structure. */
typedef struct
{
    AppTaskData libTask;
    AppTask appTask;

    /*! ID of the connection */
    McpConnectionId cid;

    /*! Profile handle of the MCP instance*/
    McpProfileHandle mcpSrvcHndl;
    /*! Service handle of the MCS client associated to this MCP instance*/
    ServiceHandle mcsSrvcHndl;

    /*! MCS instance counter */
    uint16 mcsCounter;
    /*! MCS instance number */
    uint16 mcsNum;

    /*! List of Service handle of the MCS instances discovered in the remote device */
    McpMcsSrvcHndl * firstMcsSrvcHndl;
    McpMcsSrvcHndl * lastMcsSrvcHndl;

} MCP;

typedef struct
{
    CsrCmnList_t profileHandleList;
} McpMainInst;

CsrBool mcpInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data);

CsrBool mcpProfileHndlFindByBtConnId(CsrCmnListElm_t *elem, void *data);

CsrBool mcpProfileHndlFindByMcsSrvcHndl(CsrCmnListElm_t *elem, void *data);

McpMainInst *mcpGetMainInstance(void);

#define ADD_MCP_CLIENT_INST(_List) \
                         ServiceHandleNewInstance((void**)(&(_List)),sizeof(MCP))

#define FREE_MCP_CLIENT_INST(_Handle) \
                           ServiceHandleFreeInstanceData(_Handle)

#define MCP_ADD_SERVICE_HANDLE(_List) \
    (ProfileHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ProfileHandleListElm_t))

#define MCP_REMOVE_SERVICE_HANDLE(_List,_ServiceHandle) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        mcpInstFindBySrvcHndl,(void *)(&(_ServiceHandle)))

#define FIND_MCP_INST_BY_PROFILE_HANDLE(_Handle) \
                              (MCP *)ServiceHandleGetInstanceData(_Handle)

#define MCP_FIND_PROFILE_HANDLE_BY_BTCONNID(_List,_id) \
                              ((ProfileHandleListElm_t *)CsrCmnListSearch(&(_List), \
                                        mcpProfileHndlFindByBtConnId,(void *)(&(_id))))

#define MCP_FIND_PROFILE_HANDLE_BY_MCS_SERVICE_HANDLE(_List,_ServiceHandle) \
                              ((ProfileHandleListElm_t *)CsrCmnListSearch(&(_List), \
                                        mcpProfileHndlFindByMcsSrvcHndl,(void *)(&(_ServiceHandle))))

#endif /* MCP_PRIVATE_H */
