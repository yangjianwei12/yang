/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup leabm
    \brief      Header for the LE Audio Broadcast Self-Scan Discovered Sources List
    @{
*/

#ifndef LE_BROADCAST_MANAGER_SELF_SCAN_DISCOVERED_SOURCE_LIST_H
#define LE_BROADCAST_MANAGER_SELF_SCAN_DISCOVERED_SOURCE_LIST_H

#include "le_broadcast_manager_self_scan.h"

#include <bdaddr.h>

/*! \brief Flags to record what types of data have been set for a discovered source. */
typedef enum
{
    le_bm_self_scan_ea_data = 0x1,
    le_bm_self_scan_pa_data = 0x2,
    le_bm_self_scan_biginfo_data = 0x4
} le_bm_self_scan_discovered_source_data_set_t;

typedef struct
{
    /*! Broadcast ID for this source */
    uint32 broadcast_id;

    /*! Advert SID for this source */
    uint8 adv_sid;

    /*! RSSI of the received advert

        signed integer (-127 to 20 dBm)
        127 - RSSI not available.
    */
    int8 rssi;

    /*! The "Broadcast Name" AD Type is defined in the PBP spec. */
    uint8 broadcast_name_len;
    uint8 *broadcast_name;

} le_bm_self_scan_ea_data_t;

typedef struct
{
    /*! Periodic Advertisiment interval used by the Broadcast Source. */
    uint16 pa_interval;

    /*! BIG subgroup data */
    uint8 num_subgroups;
    big_subgroup_t *subgroups;
} le_bm_self_scan_pa_data_t;

typedef struct
{
    /*! Flag to say whether the BISes in the BIG are encrypted. (Determined from the BIGInfo) */
    bool encryption_required;
} le_bm_self_scan_biginfo_data_t;

typedef struct
{
    /*! Address of the Broadcast Source */
    tp_bdaddr source_tpaddr;

    /*! Bitfield that sores which items of data have been set. */
    le_bm_self_scan_discovered_source_data_set_t data_flags;

    /*! Data extracted from the Extended Adverts (EA) of a broadcast source */
    le_bm_self_scan_ea_data_t ea_data;

    /*! Data extracted from the Periodic Adverts (PA) of a broadcast source */
    le_bm_self_scan_pa_data_t pa_data;

    /*! Data extracted from the BIGInfo of a broadcast source */
    le_bm_self_scan_biginfo_data_t biginfo_data;

} le_bm_self_scan_discovered_source_t;


#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)

/*! \brief Initialise the discovered sources list module. */
void leBroadcastManager_SelfScanDiscoveredSourceInit(void);

/*! \brief Reset the discovered sources list.

    The memory owned by each discovered source is free when it is reset.
*/
void leBroadcastManager_SelfScanDiscoveredSourceResetAll(void);

/*! \brief Create a new discovered source based on the source address.

    \param[in] tpaddr Address of the discovered source.

    \return A pointer to a new discovered source, or NULL if it could not be created.
*/
le_bm_self_scan_discovered_source_t *leBroadcastManager_SelfScanDiscoveredSourceCreate(const typed_bdaddr *tpaddr);

/*! \brief Destroy a discovered source.

    Freeing a source also frees any memory buffers it owns.

    Afer this is called the source pointer will be invalid.

    \param[in] source Discovered source to destroy.
*/
void leBroadcastManager_SelfScanDiscoveredSourceDestroy(le_bm_self_scan_discovered_source_t *source);

/*! \brief Find a discovered source using the address.

    \param tpaddr Address of the source.

    \return A pointer to the discovered source, or NULL if no match was found.
*/
le_bm_self_scan_discovered_source_t *leBroadcastManager_SelfScanDiscoveredSourceGetByAddress(const typed_bdaddr *tpaddr);

/*! \brief Get how many discovered source are currently in the list.

    \return The number of discovered source in the list.
*/
unsigned leBroadcastManager_SelfScanDiscoveredSourceCurrentCount(void);

/*! \brief Set the EA data for a discovered source.

    If the EA data contains any pointers to data buffers, for example the
    broadcast name, then this function will take ownership of the buffer
    from the caller.

    \param[in] source The discovered source to set the EA data for.
    \param[in] ea_data Pointer to the ea_data to set.
*/
void leBroadcastManager_SelfScanDiscoveredSourceSetEaData(le_bm_self_scan_discovered_source_t *source, const le_bm_self_scan_ea_data_t *ea_data);

/*! \brief Get the EA data for a discovered source.

    \param[in] source Discovered source to get the EA data for.

    \return A pointer to the EA data of the source.
*/
#define leBroadcastManager_SelfScanDiscoveredSourceGetEaData(source) (&(source)->ea_data)

/*! \brief Set the PA data for a discovered source.

    If the PA data contains any pointers to data buffers then this function
    will take ownership of the buffer from the caller.

    \param[in] source The discovered source to set the PA data for.
    \param[in] pa_data Pointer to the pa_data to set.
*/
void leBroadcastManager_SelfScanDiscoveredSourceSetPaData(le_bm_self_scan_discovered_source_t *source, const le_bm_self_scan_pa_data_t *pa_data);

/*! \brief Get the PA data for a discovered source.

    \param[in] source Discovered source to get the EA data for.

    \return A pointer to the PA data of the source.
*/
#define leBroadcastManager_SelfScanDiscoveredSourceGetPaData(source) (&(source)->pa_data)

/*! \brief Set the BIGInfo data for a discovered source.

    If the PA data contains any pointers to data buffers then this function
    will take ownership of the buffer from the caller.

    \param[in] source The discovered source to set the BIGInfo data for.
    \param[in] biginfo_data Pointer to the biginfo_data to set.
*/
void leBroadcastManager_SelfScanDiscoveredSourceSetBigInfoData(le_bm_self_scan_discovered_source_t *source, const le_bm_self_scan_biginfo_data_t *biginfo_data);

/*! \brief Get the BIGInfo data for a discovered source.

    \param[in] source Discovered source to get the BIGInfo data for.

    \return A pointer to the BIGInfo data of the source.
*/
#define leBroadcastManager_SelfScanDiscoveredSourceGetBigInfoData(source) (&(source)->biginfo_data)

/*! \brief Check which data group(s) have been set for a discovered source.

    \param[in] source Discovered Source to check.
    \param[in] data_flags Bitfield of the data groups to check for.
                          See #le_bm_self_scan_discovered_source_data_set_t.

    \return TRUE if all data groups have been set; FALSE otherwise.
*/
bool leBroadcastManager_SelfScanDiscoveredSourceCheckDataSetFlags(const le_bm_self_scan_discovered_source_t *source,
                                                                  le_bm_self_scan_discovered_source_data_set_t data_flags);

/* Notify source to client */

/* Set source has been notified */

/* Get source has been notified */

#else

#define leBroadcastManager_SelfScanDiscoveredSourceInit()

#define leBroadcastManager_SelfScanDiscoveredSourceCreate(tpaddr)   (UNUSED(tpaddr), (le_bm_self_scan_discovered_source_t *)NULL)

#define leBroadcastManager_SelfScanDiscoveredSourceDestroy(source)  (UNUSED(source))

#define leBroadcastManager_SelfScanDiscoveredSourceGetByAddress(tpaddr) (UNUSED(tpaddr), (le_bm_self_scan_discovered_source_t *)NULL)

#define leBroadcastManager_SelfScanDiscoveredSourceCurrentCount()   (0)

#define leBroadcastManager_SelfScanDiscoveredSourceSetEaData(source, ea_data)   (UNUSED(source), UNUSED(ea_data))

#define leBroadcastManager_SelfScanDiscoveredSourceGetEaData(source)    (UNUSED(source), (le_bm_self_scan_ea_data_t *)NULL)

#define leBroadcastManager_SelfScanDiscoveredSourceSetPaData(source, pa_data)   (UNUSED(source), UNUSED(pa_data))

#define leBroadcastManager_SelfScanDiscoveredSourceGetPaData(source)    (UNUSED(source), (le_bm_self_scan_pa_data_t *)NULL)

#define leBroadcastManager_SelfScanDiscoveredSourceSetBigInfoData(source, biginfo_data)   (UNUSED(source), UNUSED(biginfo_data))

#define leBroadcastManager_SelfScanDiscoveredSourceGetBigInfoData(source)    (UNUSED(source), (le_bm_self_scan_biginfo_data_t *)NULL)

#define leBroadcastManager_SelfScanDiscoveredSourceCheckDataSetFlags(source, data_flags)    (UNUSED(source), UNUSED(data_flags), FALSE)

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN) */

#endif // LE_BROADCAST_MANAGER_SELF_SCAN_DISCOVERED_SOURCE_LIST_H
/*! @} */