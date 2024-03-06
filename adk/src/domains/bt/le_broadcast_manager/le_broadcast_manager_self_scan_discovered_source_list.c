/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    leabm
    \brief      LE Audio Broadcast Self-Scan Discovered Sources List
*/

#include "le_broadcast_manager_self_scan.h"
#include "le_broadcast_manager_self_scan_discovered_source_list.h"

#include <bdaddr.h>
#include <logging.h>
#include <stdlib.h>

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)


typedef struct le_bm_self_scan_ds_list_item_tag {
    struct le_bm_self_scan_ds_list_item_tag * next;

    le_bm_self_scan_discovered_source_t *source;
} le_bm_self_scan_ds_list_item_t;


le_bm_self_scan_ds_list_item_t *discovered_source_list = NULL;


static inline void leBroadcastManager_SelfScanDiscoveredSourceDestroyEaData(le_bm_self_scan_discovered_source_t *source)
{
   if (source)
   {
       free(source->ea_data.broadcast_name);
   }
}

static void leBroadcastManager_SelfScanDiscoveredSourceFree(le_bm_self_scan_discovered_source_t *source)
{
    leBroadcastManager_SelfScanDiscoveredSourceDestroyEaData(source);
    free(source);
}

static void leBroadcastManager_SelfScanDiscoveredSourceListAppend(le_bm_self_scan_discovered_source_t *source)
{
    le_bm_self_scan_ds_list_item_t *new_item = PanicUnlessMalloc(sizeof(*new_item));

    new_item->next = NULL;
    new_item->source = source;

    if (!discovered_source_list)
    {
        discovered_source_list = new_item;
    }
    else
    {
        le_bm_self_scan_ds_list_item_t *item = discovered_source_list;
        while (item->next)
        {
            item = item->next;
        }

        item->next = new_item;
    }
}

static void leBroadcastManager_SelfScanDiscoveredSourceListRemove(le_bm_self_scan_discovered_source_t *source)
{
    if (!discovered_source_list)
    {
        return;
    }

    if (source == discovered_source_list->source)
    {
        discovered_source_list = discovered_source_list->next;
    }
    else
    {
        le_bm_self_scan_ds_list_item_t *prev = discovered_source_list;
        le_bm_self_scan_ds_list_item_t *item = discovered_source_list->next;

        while (item)
        {
            if (source == item->source)
            {
                prev->next = item->next;
                free(item);
                break;
            }

            prev = item;
            item = item->next;
        }
    }
}

static le_bm_self_scan_discovered_source_t *leBroadcastManager_SelfScanDiscoveredSourceListFind(
        bool (*item_match)(const le_bm_self_scan_discovered_source_t *source,  void *data),
        void *data)
{
    le_bm_self_scan_ds_list_item_t *item = discovered_source_list;
    le_bm_self_scan_discovered_source_t *source = NULL;

    while (item)
    {
        if (item_match(item->source, data))
        {
            source = item->source;
            break;
        }

        item = item->next;
    }

    return source;
}

static bool leBroadcastManager_SelfScanDiscoveredSourceMatchByTypedAddress(const le_bm_self_scan_discovered_source_t *source, void *data)
{
    typed_bdaddr *tpaddr = (typed_bdaddr *)data;
    bool match = FALSE;

    if (BdaddrTypedIsSame(&source->source_tpaddr.taddr, tpaddr))
    {
        match = TRUE;
    }

    return match;
}


void leBroadcastManager_SelfScanDiscoveredSourceInit(void)
{
    leBroadcastManager_SelfScanDiscoveredSourceResetAll();
}

void leBroadcastManager_SelfScanDiscoveredSourceResetAll(void)
{
    le_bm_self_scan_ds_list_item_t *item = discovered_source_list;

    discovered_source_list = NULL;

    while (item)
    {
        le_bm_self_scan_ds_list_item_t *next_item = item->next;

        leBroadcastManager_SelfScanDiscoveredSourceFree(item->source);
        free(item);

        item = next_item;
    }
}

le_bm_self_scan_discovered_source_t *leBroadcastManager_SelfScanDiscoveredSourceCreate(const typed_bdaddr *tpaddr)
{
    le_bm_self_scan_discovered_source_t *new_source = PanicUnlessMalloc(sizeof(*new_source));

    memset(new_source, 0, sizeof(*new_source));

    new_source->source_tpaddr.taddr.type = tpaddr->type;
    new_source->source_tpaddr.taddr.addr.lap = tpaddr->addr.lap;
    new_source->source_tpaddr.taddr.addr.uap = tpaddr->addr.uap;
    new_source->source_tpaddr.taddr.addr.nap = tpaddr->addr.nap;

    leBroadcastManager_SelfScanDiscoveredSourceListAppend(new_source);

    return new_source;
}

void leBroadcastManager_SelfScanDiscoveredSourceDestroy(le_bm_self_scan_discovered_source_t *source)
{
    leBroadcastManager_SelfScanDiscoveredSourceListRemove(source);
    leBroadcastManager_SelfScanDiscoveredSourceFree(source);
}

le_bm_self_scan_discovered_source_t *leBroadcastManager_SelfScanDiscoveredSourceGetByAddress(const typed_bdaddr *tpaddr)
{
    le_bm_self_scan_discovered_source_t *source = NULL;

    source = leBroadcastManager_SelfScanDiscoveredSourceListFind(
                leBroadcastManager_SelfScanDiscoveredSourceMatchByTypedAddress,
                (void *)tpaddr);

    return source;
}

unsigned leBroadcastManager_SelfScanDiscoveredSourceCurrentCount(void)
{
    unsigned count = 0;
    le_bm_self_scan_ds_list_item_t *item = discovered_source_list;

    while (item)
    {
        count++;

        item = item->next;
    }

    return count;
}

/* Set EA data */
void leBroadcastManager_SelfScanDiscoveredSourceSetEaData(le_bm_self_scan_discovered_source_t *source, const le_bm_self_scan_ea_data_t *ea_data)
{
    /* The new EA data replaces any existing EA data. */
    leBroadcastManager_SelfScanDiscoveredSourceDestroyEaData(source);

    source->ea_data = *ea_data;
    source->data_flags |= le_bm_self_scan_ea_data;
}

/* Set PA data */
void leBroadcastManager_SelfScanDiscoveredSourceSetPaData(le_bm_self_scan_discovered_source_t *source, const le_bm_self_scan_pa_data_t *pa_data)
{
    source->pa_data = *pa_data;
    source->data_flags |= le_bm_self_scan_pa_data;
}

/* Set BigInfo data */
void leBroadcastManager_SelfScanDiscoveredSourceSetBigInfoData(le_bm_self_scan_discovered_source_t *source, const le_bm_self_scan_biginfo_data_t *biginfo_data)
{
    source->biginfo_data = *biginfo_data;
    source->data_flags |= le_bm_self_scan_biginfo_data;
}

/* Is EA data set */
/* Is PA data set */
/* Is BigInfo data set */
bool leBroadcastManager_SelfScanDiscoveredSourceCheckDataSetFlags(const le_bm_self_scan_discovered_source_t *source,
                                                                  le_bm_self_scan_discovered_source_data_set_t data_flags)
{
    bool flags_set = ((source->data_flags & data_flags) == data_flags);

    DEBUG_LOG("leBroadcastManager_SelfScanDiscoveredSourceCheckDataSetFlags source %p flags 0x%x flags_set %u",
              source, data_flags, flags_set);

    return flags_set;
}

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN) */
