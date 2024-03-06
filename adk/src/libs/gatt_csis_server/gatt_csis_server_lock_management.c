/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <stdio.h>
#include <stdlib.h>

#include "gatt_csis_server_private.h"
#include "gatt_csis_server_lock_management.h"

#include "gatt_csis_server_notify.h"


/*! @brief csis data shared across multiple instances */
typedef struct
{
    GattCsisServerLocktype     lock;          /* Lock value */
    uint16                     lock_timeout;  /* lock_timeout which runs when lock taken */
    connection_id_t            cid;           /* the remote client which takes the lock*/
    ServiceHandle              srvc_hndl;     /* the service handle instance
                                               * will be useful when multiple 
                                               * instances of CSIS server enabled
                                               */
    tp_bdaddr                  tp_addr;        /* bluetooth address of the set coordinator
                                               * which locked 
                                               */
} gCsisLock_t;

/* Lock is shared resource across all the csis server instances */
gCsisLock_t csis_server_lock;

void csisServerUpdateLock(GattCsisServerLocktype lock,
                    uint16 lock_timeout,
                    ServiceHandle handle,
                    connection_id_t  cid,
                    tp_bdaddr *tp_addr)
{
    csis_server_lock.lock = lock;
    csis_server_lock.lock_timeout = lock_timeout;
    csis_server_lock.srvc_hndl = handle;
    csis_server_lock.cid = cid;

    if ((lock == CSIS_SERVER_LOCKED) && tp_addr != NULL)
    {
        csis_server_lock.tp_addr.transport = tp_addr->transport;
        csis_server_lock.tp_addr.taddr = tp_addr->taddr;
    }
    else
        memset(&csis_server_lock.tp_addr, 0, sizeof(tp_bdaddr));
}

GattCsisServerLocktype csisServerGetLockState(void)
{
    return csis_server_lock.lock;
}

uint16 csisServerGetLockTimeout(void)
{
    return csis_server_lock.lock_timeout;
}

connection_id_t csisServerGetClientLockConnId(void)
{
    return csis_server_lock.cid;
}

void csisServerUpdateLockCid(connection_id_t cid)
{
    csis_server_lock.cid = cid;
}

static tp_bdaddr csisServerGetClientLockTpAddr(void)
{
    return csis_server_lock.tp_addr;
}

tp_bdaddr csisServerGetTpAddrFromCid(connection_id_t cid)
{
    tp_bdaddr tpaddr;
    tp_bdaddr resolved_tpaddr = {0};

    memset(&tpaddr, 0, sizeof(tp_bdaddr));

    if (VmGetBdAddrtFromCid(cid, &tpaddr))
    {
        if(   tpaddr.transport != TRANSPORT_BLE_ACL
           || tpaddr.taddr.type != TYPED_BDADDR_RANDOM
           || !VmGetPublicAddress(&tpaddr, &resolved_tpaddr))
        {
            resolved_tpaddr = tpaddr;
        }
    }
    else
    {
        GATT_CSIS_SERVER_PANIC(("No Bd_addr found!\n"));
    }

    return resolved_tpaddr;
}

void csisServerHandleLockTimerExpiry(GCSISS_T *csis_server, connection_id_t cid)
{
    /* Cancel Lock timer */
    MessageCancelAll(
                (Task) &csis_server->lib_task,
                 CSIS_SERVER_INTERNAL_MSG_LOCK_TIMEOUT_TIMER);

    /* Release the Lock */
    csisServerUpdateLock(CSIS_SERVER_UNLOCKED, csisServerGetLockTimeout(), csis_server->srvc_hndl, 0, NULL);

    /* Notify all connected clients */
    csisServerNotifyLockChange(csis_server, 0);

    /* Inform application about Lock release */
    MAKE_CSIS_SERVER_MESSAGE(GATT_CSIS_LOCK_STATE_IND);
    message->csisHandle = GattCsisServerGetLock();
    message->cid = cid;
    message->lockValue = CSIS_SERVER_UNLOCKED;

    MessageSend(csis_server->app_task, GATT_CSIS_LOCK_STATE_IND, message);

}

uint8 csisServerEvaluateLockValue(connection_id_t cid,
      uint16 size,
      GattCsisServerLocktype value,
      uint8 *update_timer,
      tp_bdaddr *tpAddr)
{
    uint8 result = GATT_CSIS_INVALID_LOCK_VALUE;
    GattCsisServerLocktype curLockValue = csisServerGetLockState();
    connection_id_t curCid = csisServerGetClientLockConnId();
    *update_timer = LOCK_TIMER_UPDATE_NOT_REQUIRED;
    tp_bdaddr lockTpAddr = csisServerGetClientLockTpAddr();
    tp_bdaddr clientTpAddr = csisServerGetTpAddrFromCid(cid);

    if (size == GATT_CSIS_SERVER_LOCK_SIZE && (value == CSIS_SERVER_LOCKED || value == CSIS_SERVER_UNLOCKED))
    {
        /* If server is locked and lock addr is same as the one requesting to lock
         * and cid diff then this is a re-connection case when lock timer is
         * still running
         */
        if (curLockValue == CSIS_SERVER_LOCKED &&
            BdaddrTpIsSame(&lockTpAddr, &clientTpAddr) &&
            curCid != cid)
        {
            result = GATT_CSIS_LOCK_ALREADY_GRANTED;
            if (value == CSIS_SERVER_UNLOCKED)
            {
                *update_timer = LOCK_TIMER_UPDATE_REQUIRED;
                result = gatt_status_success;
            }
        }
        else if (curCid != cid)
        {
            if (curLockValue == CSIS_SERVER_LOCKED && value == CSIS_SERVER_LOCKED)
                result = GATT_CSIS_LOCK_DENIED;
            else if (curLockValue == CSIS_SERVER_LOCKED && value == CSIS_SERVER_UNLOCKED)
                result = GATT_CSIS_LOCK_RELEASE_NOT_ALLLOWED;
            else if (curLockValue == CSIS_SERVER_UNLOCKED && value == CSIS_SERVER_UNLOCKED)
                result = gatt_status_success;
            else
            {
                result = gatt_status_success;
                *update_timer = LOCK_TIMER_UPDATE_REQUIRED;
            }
        }
        else
        {
            if (curLockValue == CSIS_SERVER_LOCKED && value == CSIS_SERVER_LOCKED)
            {
                result = GATT_CSIS_LOCK_ALREADY_GRANTED;
            }
            else if (curLockValue == CSIS_SERVER_UNLOCKED && value == CSIS_SERVER_UNLOCKED)
            {
                result = gatt_status_success;
            }
            else
            {
                result = gatt_status_success;
                *update_timer = LOCK_TIMER_UPDATE_REQUIRED;
            }
        }

        if (result == gatt_status_success)
        {
            *tpAddr = clientTpAddr;
        }
    }
    return result;
}

bool GattCsisServerSetLock(CsisServerServiceHandleType handle,
                         GattCsisServerLocktype lock,
                         uint16 lockTimeout)
{
    bool notify = FALSE;
    GCSISS_T *csis_server = (GCSISS_T *) ServiceHandleGetInstanceData(handle);
    GattCsisServerLocktype curLockValue = csisServerGetLockState();
    ServiceHandle curHandle = GattCsisServerGetLock();
    bool status = FALSE;

    if (csis_server == NULL)
        return status;

    /*
      If server is currently unlocked and 'lock' is set Locked by application,
      then if connected notify all connected clients who have registered for
      'lock' notification. Application setting 'locked' would mean it does not
      want any client to take the 'lock' bcoz of lack of resource or anything.

      If server is currently locked by application and
       a) If 'lock' is set unlocked then if connected notify all connected
          clients who have registered for 'lock' notification.

      If server is currently locked by remote and 
       a) If 'lock' is set csis_server_invalid_lock_value then reset the lock
          timer and start the lock timer again for lock_timeout seconds.
       b) If 'lock' is set locked or unlocked by application then reset the
          lock timer and if connected notify all connected clients who have
          registered for 'lock' notification.

      curHandle non-zero means lock taken by remote.

    */

    if (curLockValue == CSIS_SERVER_UNLOCKED)
    {
        if (lock == CSIS_SERVER_LOCKED)
        {
            csisServerUpdateLock(lock, lockTimeout, 0, 0, NULL);

            /* Inform all connected clients about lock state */
            notify = TRUE;

            /* operation successful */
            status = TRUE;
        }
    }
    else
    {
        /* Already locked */

        if (lock == CSIS_SERVER_INVALID_LOCK_VALUE)
        {
             if (curHandle != 0)
             {
                 tp_bdaddr tpaddr;

                 /* Get address from cid for the set coordinator */
                 tpaddr = csisServerGetClientLockTpAddr();

                 /* Reset the timer and start lock timer with a given value
                 * for already existing client who has the lock
                 */
                 csisServerUpdateLock(
                         CSIS_SERVER_LOCKED,
                         lockTimeout,
                         curHandle,
                         csisServerGetClientLockConnId(),
                         &tpaddr);

                 MessageCancelAll(
                         (Task) &csis_server->lib_task,
                          CSIS_SERVER_INTERNAL_MSG_LOCK_TIMEOUT_TIMER);

                 MAKE_CSIS_SERVER_MESSAGE(CSIS_SERVER_INTERNAL_MSG_LOCK_TIMEOUT_TIMER);
                 message->cid = csisServerGetClientLockConnId();

                 MessageSendLater(
                         (Task) &csis_server->lib_task,
                          CSIS_SERVER_INTERNAL_MSG_LOCK_TIMEOUT_TIMER,
                          message,
                          D_SEC(lockTimeout));

                 status = TRUE;
             }
        }
        else if (curHandle == 0)
        {
            /* Remote has not taken the lock, update lock state */
            csisServerUpdateLock(lock, lockTimeout, 0, 0, NULL);

            /* If unlocked by application, notify connected clients */
            if (lock == CSIS_SERVER_UNLOCKED)
                notify = TRUE;

            status = TRUE;
        }
        else if (curHandle != 0)
        {
            /* Application wants to override the current lock taken by
             * remote client
             * Update the lock value set by application.
             * Cancel the lock timer and notify all connected clients
             * about lock state
             */
             csisServerUpdateLock(lock, lockTimeout, 0, 0, NULL);
             
             MessageCancelAll(
                        (Task) &csis_server->lib_task,
                         CSIS_SERVER_INTERNAL_MSG_LOCK_TIMEOUT_TIMER);

             /* Inform all connected clients about lock state */
             notify = TRUE;

             status = TRUE;
        }
    }

    if (notify)
    {
        csisServerNotifyLockChange(csis_server, 0);
    }

    return status;
}

CsisServerServiceHandleType GattCsisServerGetLock(void)
{
    /* Identify which service_handle has taken the lock */
    return csis_server_lock.srvc_hndl;
}



