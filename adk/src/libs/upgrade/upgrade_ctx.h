/****************************************************************************
Copyright (c) 2015-2021 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_ctx.h

DESCRIPTION
    Header file for the Upgrade context.
*/

#ifndef UPGRADE_CTX_H_
#define UPGRADE_CTX_H_

#include <message.h>

#include "imageupgrade.h"

#include "upgrade_partition_data_priv.h"
#include "upgrade_fw_if_priv.h"
#include "upgrade_sm.h"
#include "upgrade_private.h"
#include "upgrade.h"
#include "upgrade_psstore_priv.h"

/* Enum to track status of Case DFU
 */
typedef enum
{
    UPGRADE_CASE_DFU_NOT_STARTED,

    UPGRADE_CASE_DFU_IN_PROGRESS,

    UPGRADE_CASE_DFU_ABORTING,
} upgrade_case_dfu_status_t;

typedef struct
{
    TaskData smTaskData;
    Task mainTask;
    Task clientTask;
    Task transportTask;
    /* An indication of whether the transport needs a UPGRADE_TRANSPORT_DATA_CFM
     * or not.
     */
    upgrade_data_cfm_type_t data_cfm_type;
    /* The maximum number of bytes (possibly in multiple packets) that the
     * transport can handle as a response to a single UPGRADE_DATA_BYTES_REQ.
     * This might be due to buffer size constraints, for example. 0 = no limit.
     */
    uint32 max_request_size;
    UpgradeState smState;
    UpgradePartitionDataCtx *partitionData;
    UpgradeFWIFCtx fwCtx;
    const UPGRADE_UPGRADABLE_PARTITION_T *upgradeLogicalPartitions;
    uint16 upgradeNumLogicalPartitions;
    /* Storage for PSKEY management.
     *
     * The library uses a (portion of) PSKEY supplied by the application
     * so needs to remember the details of the PSKEY.
     */
    uint16 upgrade_library_pskey;
    uint16 upgrade_library_pskeyoffset;
    uint32 partitionDataBlockSize;
    /*! Storage for upgrade library information maintained in PSKEYS.
     *
     * The values are only @b read from PSKEY on boot.
     * The local copy can be read/written at any time, but should be written
     * @b to storage as little as possible.
     *
     * Since this PSKEY storage structure has multiple uses be aware that
     * any value you write to the local copy may be committed at any time.
     *
     * The values will always be initialised, to 0x00 at worst.
     */
    UPGRADE_LIB_PSKEY UpgradePSKeys;

    /* Current level of permission granted the upgrade library by the VM
     * application */
    upgrade_permission_t perms;

    /*! The current power management mode (disabled/battery powered) */
    upgrade_power_management_t power_mode;

    /*! The current power management state informed by the VM app */
    upgrade_power_state_t power_state;

    /*! device variant */
    char dev_variant[UPGRADE_HOST_VARIANT_CFM_BYTE_SIZE+1];

    const upgrade_response_functions_t *funcs;

    /* P0 Hash context */
    hash_context_t vctx;

    /* CSR Valid message received flag */
    bool isCsrValidDoneReqReceived;

    /*! Force erase when a START_REQ is received */
    bool force_erase;

    /*! The flow control scheme using dfu_rx_flow_off and pendingDataReq
     *  is especially needed for DFU over LE where the upgrade data is
     *  packet oriented and the max MTU is 64.
     *  Being packet oriented and smaller MTU as compared to BR/EDR, leads to
     *  more short (complete or partial) messages queued for processing as the
     *  Source Buffer is drained.
     *  Since these messages are allocated from heap (pmalloc pool), its highly
     *  probable to run out of pmalloc pools. So a flow control scheme is
     *  required to keep the queued messages for processing within acceptable
     *  limits. In case of LE, the GATT Source stream size is 512 which can hold
     *  at max 8 upgrade data packets as max MTU is 64. So the acceptable
     *  queued message limit is 8.
     *  TODO: Upgrade library is commonly used both for Host<->Primary upgrade
     *        and also Primary<->Secondary upgrade.
     *        Primary<->Secondary upgrade is always using BR/EDR link and
     *        as such this flow control scheme is not required owing to larger
     *        MTU and upgrade data being non-packet oriented.
     *        Also Host<->Primary upgrade can be over BR/EDR link, in which case
     *        this flow control scheme is not required as reasoned above.
     *        But even if this flow control scheme is commonly applied for
     *        DFU over LE or BR/EDR, this won't radically impact the KPIs for
     *        DFU over BR/EDR.
     *        In future, if need arises to localize this flow control scheme to
     *        DFU over LE then logic needs to be added to distinguish
     *        Host<->Primary transport and whether Host<->Primary upgrade or
     *        Primary<->Secondary upgrade.
     */

    /*! Track whether processing of upgrade data from Source Buffer is flowed
     *  off or not.
     *  Used for DFU over LE flow control scheme.
     */
    bool dfu_rx_flow_off;

    /*! Outstanding upgrade data messages extracted from Source Buffer
     *  and queued to be processed (i.e. written to flash).
     *  Used for DFU over LE flow control scheme.
     */
    uint32 pendingDataReq;

     /*! Flag included to defer sending of data request
     *  during DFU if SCO is active.
     *  Used to pause DFU during incoming/outgoing active calls.
     *  Type: isScoActive is uint16 as used in MessageSendConditionally
     */
    uint16 isScoActive;

    /*! If user has sent the commit_cfm message with action 1(no) then this flag will be set
     *  If set then app should do the reboot after handling abort to revert the image
     */
    bool isImageRevertNeededOnAbort:1;

    /* Flag to indicate whether peer dfu is supported or not */
    bool isUpgradePeerDfuSupported:1;

    /* Hold the commit mode information */
    uint8 commitMode:2;

    /* Hold the transfer complete response information */
    bool transferCompleteResReceived:1;

    /*! ImageUpgradeErase() is completed
     *  i.e. received MESSAGE_IMAGE_UPGRADE_ERASE_STATUS.
     */
    uint16 isImgUpgradeEraseDone;

    upgrade_reconnect_recommendation_t reconnect_reason;

    /*! this flag will be set if we send abort req to the peer and reset when
     *  we get abort cfm from peer. primary device should wait for the peer
     *  abort cfm before sending it to host. */
    uint16 waitForPeerAbort;

     /*! Flag to indicate if silent commit is supported */
    uint8 isSilentCommitSupported;

    /* Flag to indicate whether case dfu is supported or not */
    bool isCaseDfuSupported;

    /* Status of the ongoing case DFU. */
    upgrade_case_dfu_status_t caseDfuStatus;

    /* Set this variable to false to send the delayed data confirmation to the transport */
    uint16 delayDataCfm;

    /* True if we need to delay the data confirmation to the transport so that the
     * application can process the data. Received data will be preserved in transport buffer
     * till we confirm it.
     */
    bool isTransportCfmDelayed;

    /* Length field in the header of case dfu file */
    uint32 caseDfuHeaderLength;
    
    /*! Offset from the start of the DFU file calculated at the resume time
     */
    uint32 dfu_file_offset;

    /* Variable to store information of DFU Abort of SYNC ID Mismatch */
    bool isDfuAbortDueToSyncIdMismatch;

    /*! True if we have received all data including footer.
     * This doesn't include hash verification of the data.
     */
    bool isDataXferComplete;

    /* hold the commit status */
    upgrade_fw_if_commit_status commitStatus;

    /* Hold the commit status for peer 
     peer's commit status will be stored optionally if peer device exists */
    upgrade_fw_if_commit_status peercommitStatus;
} UpgradeCtx;

void UpgradeCtxSet(UpgradeCtx *ctx);
UpgradeCtx *UpgradeCtxGet(void);

/*!
    @brief Set the partition data context in the upgrade context.
*/
void UpgradeCtxSetPartitionData(UpgradePartitionDataCtx *ctx);

/*! @brief Get the partition data context.

    @return Pointer to the partition data context. It may be
            NULL if it has not been set.
*/
UpgradePartitionDataCtx *UpgradeCtxGetPartitionData(void);

UpgradeFWIFCtx *UpgradeCtxGetFW(void);
UPGRADE_LIB_PSKEY *UpgradeCtxGetPSKeys(void);

/*!
    @brief Clear upgrade related local Pskey info maintained in context.
    @param none
    
    Returns none
*/
void UpgradeCtxClearPSKeys(void);
#endif /* UPGRADE_CTX_H_ */
