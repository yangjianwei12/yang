#ifndef __DEBUG_PARTITION_API_H__
#define __DEBUG_PARTITION_API_H__
#include <app/debug_partition/debug_partition_if.h>

    /*! file  @brief Access Traps for Debug Partition.
    **
    **
    Debug Partition traps.
    */
    
#if TRAPSET_DEBUG_PARTITION

/**
 *  \brief Configure Debug Partition
 *         Can be used to configure Debug Partition parameters.
 *         If an error is returned, configuration does not change and the trap
 *         needs to be re-tried after rectifying the problem.
 *         
 *  \param key Configuration key.
 *  \param value Value for selected configuration key.
 *  \return Result code indicating success or error reason.
 * 
 * \ingroup trapset_debug_partition
 */
debug_partition_result DebugPartitionConfig(debug_partition_config_key key, uint32 value);

/**
 *  \brief Request Debug Partition info
 *         Can be used to request Debug Partition info.
 *         
 *  \param key Information key to request.
 *  \param value Location to store the requested info.
 *  \return Result code indicating success or error reason.
 * 
 * \ingroup trapset_debug_partition
 */
debug_partition_result DebugPartitionInfo(debug_partition_info_key key, uint32 * value);

/**
 *  \brief Erase Debug Partition
 *         Debug Partition will be erased, the trap blocks for the duration
 *         of erase. This can take up to several seconds and application
 *         will be unresponsive during this time.
 *         Erase can't be started if Debug Partition Source Stream is open,
 *         this has to be closed first.
 *         
 *  \return Result code indicating success or error reason.
 * 
 * \ingroup trapset_debug_partition
 */
debug_partition_result DebugPartitionErase(void );

/**
 *  \brief Save the contents of the on-chip miniture coredump into the Debug partition
 *         Some devices include a feature that when a panic occurs, some
 *  additional debug information can be captured from other places in the system
 *  as a small coredump.
 *         If such data is present then it will be saved into the debug partition.
 *         
 *  \return Result code indicating success or error reason.
 * 
 * \ingroup trapset_debug_partition
 */
debug_partition_result DebugPartitionSaveMiniCoreDump(void );

/**
 *  \brief Configure Debug Partition
 *         Can be used to configure Debug partition parameters.
 *         If an error is returned, configuration does not change and the trap
 *         needs to be re-tried after rectifying the problem.
 *         
 *  \param key Configuration key.
 *  \param value1 The start offset to dump the bytes
 *  \param value2 The length of bytes to dump
 *  \return Result code indicating success or error reason.
 * 
 * \ingroup trapset_debug_partition
 */
debug_partition_result DebugPartitionConfig32(debug_partition_config32_key key, uint32 value1, uint32 value2);
#endif /* TRAPSET_DEBUG_PARTITION */
#endif
