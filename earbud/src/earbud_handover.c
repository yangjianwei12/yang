/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_handover.c
\brief      This module implements handover interface (handover_interface_t) and
            acts as an aggregator for all application components which require
            handover.
*/
#ifdef INCLUDE_MIRRORING
#include "earbud_handover.h"
#include "earbud_handover_marshal_typedef.h"
#include "earbud_sm_handover.h"
#include "earbud_sm.h"

#include <app_handover_if.h>
#include <handover_if.h>
#include <handover_profile.h>
#include <marshal_common.h>
#include <domain_marshal_types.h>
#include <service_marshal_types.h>
#include <tws_topology_marshal_types.h>
#include <mirror_profile_protected.h>
#include <avrcp.h>
#include <a2dp.h>
#include <hfp.h>
#include <dfu_peer.h>
#include <gaia_framework.h>
#include "kymera.h"
#include <bdaddr_.h>
#include <panic.h>
#include <marshal.h>
#include <logging.h>
#include <stdlib.h>
#include <fast_pair.h>
#include <hfp_profile_instance.h>
#ifdef INCLUDE_BTDBG
#include <btdbg_peer_profile.h>
#endif
#ifdef ENABLE_ACCESSORY_HANDOVER
#include <accessory_handover.h>
#endif

#define EB_HANDOVER_DEBUG_LOG_VERBOSE   DEBUG_LOG_VERBOSE
#define EB_HANDOVER_DEBUG_LOG_DEBUG     DEBUG_LOG_DEBUG
#define EB_HANDOVER_DEBUG_LOG_WARN      DEBUG_LOG_WARN
#define EB_HANDOVER_DEBUG_LOG_ERROR     DEBUG_LOG_ERROR
#define EB_HANDOVER_DEBUG_LOG_INFO      DEBUG_LOG_INFO

/* All the marshal type descriptors to be used by Apps P1 (Earbud application)
   marshalling.
   
   Formed as a hierarchy of marshal type descriptors following the layered
   application framework:-
     - Common marshal types
     - Domain marshal types
     - Service marshal types
     - Tws Topology marshal types
     - Application marshal types
 */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *)&marshal_type_descriptor_##type,
const marshal_type_descriptor_t * const  mtd_handover_app[] = {
    /* PLEASE READ -  Make sure ordering of marshal TYPES here shall match 
    with the ordering of marshal TYPES in corresponding layer.
    In other words, if you add/remove marshal TYPES in domain layer then ordering
    of marshal TYPES in domain_marshal_types.h should match for domain marshal TYPES 
    here.

    Marshal TYPES belonging to layer(s) other than you have reordered SHALL NOT 
    BE REORDERED here. */

    /* Start of common marshal types, refer marshal_common.h for types and
    type descriptors.*/
    MARSHAL_COMMON_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    /* Start of domain marshal types, refer domain_marshal_types.h for types 
    and type descriptors.*/
    AV_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    A2DP_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    AVRCP_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    CONNECTION_MANAGER_LIST_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    HFP_PROFILE_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    HFP_PROFILE_STATES_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    BT_DEVICE_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    BT_DEVICE_HANDOVER_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    AUDIO_ROUTER_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    BANDWIDTH_MANAGER_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
#ifdef USE_SYNERGY
    HIDD_PROFILE_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
#endif
#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
    LE_BROADCAST_MANAGER_SELF_SCAN_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
#endif
    /* Start of service marshal types, refer service_marshal_types.h for types
    and type descriptors.*/
    STATE_PROXY_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    MEDIA_PLAYER_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    /* Start of tws topology marshal types, refer tws_topology_marshal_types.h
    for types and type descriptors.*/
    TWS_TOPOLOGY_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    /* Start of Apps marshal types, refer earbud_sm_handover.h for types and
    type descriptors.*/
    EARBUD_HANDOVER_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};

typedef enum
{
    MARSHAL_STATE_UNINITIALIZED,
    MARSHAL_STATE_INITIALIZED,
    MARSHAL_STATE_MARSHALING,
    MARSHAL_STATE_UNMARSHALING
} eb_marshal_state_t;

COMPILE_TIME_ASSERT(ARRAY_DIM(mtd_handover_app) == NUMBER_OF_EARBUD_APP_MARSHAL_OBJECT_TYPES,
marshal_type_descriptors_out_of_sync_with_marshal_types); 

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/

/* handover interface static function declarations */
static bool earbudHandover_Veto(void);
static bool earbudHandover_VetoLink(const tp_bdaddr *tp_bd_addr);
static bool earbudHandover_Marshal(const tp_bdaddr *addr, uint8 *buffer, uint16 buffer_size, uint16 *written);
static bool earbudHandover_Unmarshal(const tp_bdaddr *addr, const uint8 *buffer, uint16 buffer_size, uint16 *consumed);
static void earbudHandover_Commit(const tp_bdaddr *tp_bd_addr, const bool role);
static void earbudHandover_Complete(const bool role);
static void earbudHandover_Abort(void);
static const registered_handover_interface_t * getNextInterface(uint8 interface_type);

/******************************************************************************
 * Local Structure Definitions
 ******************************************************************************/
/*! \brief Stores unmarshalled data received from Primary earbud */
typedef struct {
    /* Pointer to unmarshalled data */
    void * data;
    /* Marshal Type of unmarshalled data */
    uint8 type;
    /* Result of unmarshalling */
    app_unmarshal_status_t unmarshalling_status;
} handover_app_unmarshal_data_t;

/*! \brief Structure to store device instance specific information */
typedef struct handover_app_device_tag
{
    struct handover_app_device_tag *next;
    /* Marshaling State */
    eb_marshal_state_t marshal_state;
    /* Device being handed-over */
    tp_bdaddr tp_bd_addr;
    /* Indicate if generic marshal/unmarshal data type to be considered for this device or not */
    bool allow_generic_data_type;
    union {
        struct {
            marshaller_t marshaller;
        } marshal;

        struct {
            unmarshaller_t unmarshaller;
            /* List of Unmarshalled objects received from Primary earbud */
            handover_app_unmarshal_data_t *data_list;
            /* Index to the next free entry in unmarshalled data list */
            uint8 list_free_index;
            /* Size of unmarshal data list. This can be less than interfaces_len since not all clients will marshal data */
            uint8 list_size;
        }unmarshal;
    }u;
}handover_app_device_t;

/*! \brief Handover context maintains the handover state for the application */
typedef struct {
    handover_app_device_t *device_list;
    /* List of handover interfaces registered by application components */
    const registered_handover_interface_t *interfaces;
    /* Size of registered handover interface list */
    uint8 interfaces_len;
    /* Pointer to the interface currently in use for marshaling or unmarshaling */
    const registered_handover_interface_t *curr_interface;
    /* Index of current marshal type undergoing marshalling */
    const marshal_type_info_t *curr_type;
} handover_app_context_t;

/******************************************************************************
 * Local Declarations
 ******************************************************************************/

/* Handover interface */
const handover_interface application_handover_interface =
        MAKE_HANDOVER_IF_VPL(
            &earbudHandover_Veto,
            &earbudHandover_VetoLink,
            &earbudHandover_Marshal,
            &earbudHandover_Unmarshal,
            &earbudHandover_Commit,
            &earbudHandover_Complete,
            &earbudHandover_Abort);

#ifdef USE_SYNERGY
extern const handover_interface csr_bt_cm_handover_if;
extern const handover_interface csr_bt_hf_handover_if;
extern const handover_interface csr_bt_av_handover_if;
extern const handover_interface csr_bt_avrcp_handover_if;
#ifdef INCLUDE_PBAP_CLIENT
extern const handover_interface pac_handover_if;
#endif

#ifdef INCLUDE_HIDD_PROFILE
extern const handover_interface csr_bt_hidd_handover_if;
#endif

#ifdef ENABLE_LE_HANDOVER
extern const handover_interface csr_bt_gatt_sd_handover_if;
extern const handover_interface csr_bt_gatt_handover_if;
extern const handover_interface gatt_connect_handover_if;
extern const handover_interface gatt_battery_server_handover_if;
extern const handover_interface gatt_server_handover_if;
#ifdef INCLUDE_QSS
extern const handover_interface gatt_qss_server_handover_if;
#endif

#ifdef INCLUDE_GBSS
extern const handover_interface generic_broadcast_scan_server_handover_if;
#endif

#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)
extern const handover_interface bap_handover_if;
extern const handover_interface vcs_service_handover_if;

#ifdef ENABLE_TMAP_PROFILE
extern const handover_interface tmap_client_handover_if;
#endif /* ENABLE_TMAP_PROFILE */

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST) */

#ifdef INCLUDE_LE_AUDIO_UNICAST
extern const handover_interface unicast_manager_handover_if;
extern const handover_interface media_control_client_handover_if;
extern const handover_interface call_control_client_handover_if;
extern const handover_interface micp_server_handover_if;

#endif /* INCLUDE_LE_AUDIO_UNICAST*/

#endif /* ENABLE_LE_HANDOVER */


/* Handover interfaces for all P1 components */
const handover_interface * handover_clients[] = {
    &application_handover_interface,
    &csr_bt_cm_handover_if,
    &csr_bt_hf_handover_if,
    &csr_bt_av_handover_if,
#ifdef INCLUDE_HIDD_PROFILE
    &csr_bt_hidd_handover_if,
#endif
    &csr_bt_avrcp_handover_if,
#ifdef INCLUDE_PBAP_CLIENT
    &pac_handover_if,
#endif
    &mirror_handover_if,
    &kymera_a2dp_mirror_handover_if,
#ifdef INCLUDE_DFU_PEER
    &dfu_peer_handover_if,
#endif
#if defined(INCLUDE_MIRRORING) && defined (INCLUDE_GAIA)
    &gaia_handover_if,
#endif
#ifdef INCLUDE_FAST_PAIR
    &fast_pair_handover_if,
#endif
#ifdef INCLUDE_BTDBG
    &btdbg_profile_handover_if,
#endif

#ifdef ENABLE_LE_HANDOVER
    &gatt_connect_handover_if,
    &csr_bt_gatt_sd_handover_if,
    &csr_bt_gatt_handover_if,

#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)
    &bap_handover_if,
#endif

#ifdef INCLUDE_LE_AUDIO_UNICAST
    &unicast_manager_handover_if,
    &media_control_client_handover_if,
    &call_control_client_handover_if,
    &micp_server_handover_if,
#endif

#ifdef ENABLE_TMAP_PROFILE
    &tmap_client_handover_if,
#endif

#ifdef INCLUDE_GBSS
    &generic_broadcast_scan_server_handover_if,
#endif

#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)
    &vcs_service_handover_if,
#endif
    &gatt_battery_server_handover_if,
    &gatt_server_handover_if,

#ifdef INCLUDE_QSS
    &gatt_qss_server_handover_if,
#endif

#endif /* ENABLE_LE_HANDOVER */

#ifdef ENABLE_ACCESSORY_HANDOVER
    &accessory_handover_if,
#endif
    NULL
};
#else
/* NULL terminated list of handover interfaces for all P1 components */
const handover_interface * handover_clients[] = {
    &connection_handover_if,
    &a2dp_handover_if,
    &avrcp_handover,
    &hfp_handover_if,
    &application_handover_interface,
    &mirror_handover_if,
    &kymera_a2dp_mirror_handover_if,
#ifdef INCLUDE_DFU_PEER
    &dfu_peer_handover_if,
#endif
#ifdef INCLUDE_GAIA
    &gaia_handover_if,
#endif
#ifdef INCLUDE_FAST_PAIR
    &fast_pair_handover_if,
#endif
#ifdef ENABLE_ACCESSORY_HANDOVER
    &accessory_handover_if,
#endif
    NULL
};
#endif

/* Application context instance */
static handover_app_context_t handover_app;
#define earbudHandover_Get() (&handover_app)

#define FOR_EACH_REGISTERED_INTERFACE_CONDITION(interface, condition) \
    for (const registered_handover_interface_t *interface = handover_app.interfaces; \
        (condition) && interface < (handover_app.interfaces + handover_app.interfaces_len); \
        interface++ )

#define FOR_EACH_REGISTERED_INTERFACE(interface) FOR_EACH_REGISTERED_INTERFACE_CONDITION(interface, TRUE)

#define FOR_EACH_REGISTERED_INTERFACE_FROM_CURRENT(interface, from) \
    for (const registered_handover_interface_t *interface = from; \
        interface < (handover_app.interfaces + handover_app.interfaces_len); \
        interface++ )

#define FOR_EACH_MARSHAL_TYPE_OF_INTERFACE(interface, type_info) \
    for (const marshal_type_info_t *type_info = interface->type_list->types_info_list; \
        type_info < (interface->type_list->types_info_list + interface->type_list->list_size); \
        type_info++)

#define FOR_EACH_MARSHAL_TYPE_OF_INTERFACE_FROM_CURRENT(interface, type_info, curr_type_info) \
    for (const marshal_type_info_t *type_info = curr_type_info; \
        type_info < (interface->type_list->types_info_list + interface->type_list->list_size); \
        type_info++)

#define FOR_EACH_APP_DEVICE(device) \
    for (const handover_app_device_t *device = handover_app.device_list; \
        device != NULL; device = device->next )

#define FOR_EACH_UNMARSHAL_DATA_OF_DEVICE(device, unmarshal_data) \
    for (const handover_app_unmarshal_data_t *unmarshal_data = device->u.unmarshal.data_list; \
        unmarshal_data < (device->u.unmarshal.data_list + device->u.unmarshal.list_free_index); \
        unmarshal_data++)

#define earbudHandover_IsGenericDataAllowedForDevice(device)   (device->allow_generic_data_type == TRUE)
#define earbudHandover_IsMarshalTypeCategoryPerInstance(category) (category == MARSHAL_TYPE_CATEGORY_PER_INSTANCE)
#define earbudHandover_IsMarshalTypeCategoryGeneric(category) (category == MARSHAL_TYPE_CATEGORY_GENERIC)

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/
/*! \brief Checks if any interface, having valid marshal types, has registered or not and also the matching interface type
 *
 * \param type Type of handover interface clients that are intereseted
 * \returns TRUE if any valid interface registered, FALSE otherwise
 */
static bool earbudHandover_IsAnyValidInterfaceRegistered(uint8 interface_type)
{
    FOR_EACH_REGISTERED_INTERFACE(curr_inf)
    {
        /* Check if this interface has valid marshal types */
        if ((curr_inf->type_list) && (interface_type == curr_inf->interface_type))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/*! \brief Get or Create instance of device
 *
 * \param[in] addr Pointer to bluetooth transport address
 * \returns Pointer to instance of device
 */
static handover_app_device_t* earbudHandover_GetOrCreateDevice(const tp_bdaddr* addr)
{
    handover_app_device_t *new_device = NULL;
    handover_app_context_t *app_data = earbudHandover_Get();
    bool is_same_transport = FALSE;
    tp_bdaddr resolved_addr;
    tp_bdaddr resolved_device_addr;

    /* Make device instance store reference of resolved public address with retaining transport of address */
    if (!ConManagerResolveTpaddr(addr, &resolved_addr))
    {
        /* RPA resolution is failed use the RANDOM address */
        EB_HANDOVER_DEBUG_LOG_ERROR("earbudHandover_GetOrCreateDevice, ConManagerResolveTpaddr failed");
    }

    FOR_EACH_APP_DEVICE(device)
    {
        /* The device may contain a random address. Since we are resolving the supplied tp_bdaddr, the 
           device address also needs to be resolved before comparison.
        */
        (void) ConManagerResolveTpaddr(&device->tp_bd_addr, &resolved_device_addr);

        if (BdaddrTpIsSame(&resolved_addr, &resolved_device_addr))
        {
            new_device = (handover_app_device_t*)device;
            break;
        }
        /* mark up that there is another device connected with similar transport */
        else if (addr->transport == device->tp_bd_addr.transport)
        {
            is_same_transport = TRUE;
        }
    }
    if(new_device == NULL)
    {
        new_device = PanicUnlessNew(handover_app_device_t);
        memset(new_device, 0, sizeof(handover_app_device_t));
        new_device->marshal_state = MARSHAL_STATE_INITIALIZED;
        new_device->tp_bd_addr = resolved_addr;
        /* Allow generic data marshaling/unmarshaling only once for each transport specific devices(like BREDR, BLE)*/
        new_device->allow_generic_data_type = !is_same_transport;

        new_device->next = app_data->device_list;
        app_data->device_list = new_device;
    }

    return new_device;
}

/*! \brief cleanup the unmarsalled data list for a devic

    \param[in] device Pointer to device instance holds unmarshalled data list allocated memory
*/
static void earbudHandover_CleanupUnmarshalDataListForDevice(handover_app_device_t *device)
{
    /* Free up each unmarshalled data corresponding a marshal type */
    FOR_EACH_UNMARSHAL_DATA_OF_DEVICE(device, unmarshal_data)
    {
        /* If we have null data, free_index is probably incorrect. Panic!! */
        PanicNull(unmarshal_data->data);

        if (unmarshal_data->unmarshalling_status != UNMARSHAL_SUCCESS_DONT_FREE_OBJECT)
        {            
            free(unmarshal_data->data);
        }
    }
    free(device->u.unmarshal.data_list);
    device->u.unmarshal.data_list = NULL;
    device->u.unmarshal.list_size = 0;
    device->u.unmarshal.list_free_index = 0;
}

/*! \brief Create and initialize list of unmarshalled data for a device

    \param[in] device Pointer to device which maintains unmarshalled data

    \note Size of list is equal to the number of clients which have data to be marshalled.
    List is expected to be initialized during Unmarshalling and cleaned up on
    Complete or Abort.
*/
static void earbudHandover_InitUnmarshalDataListForDevice(handover_app_device_t *device)
{
    uint8 number_of_marshal_types = 0;

    /* Iterate through each interface for total marshal types to be marshalled */
    FOR_EACH_REGISTERED_INTERFACE(curr_inf)
    {
        /* Should consider only interfaces that have valid marshal type and matching ACL transport */
        if ((curr_inf->type_list) && (curr_inf->interface_type == device->tp_bd_addr.transport))
        {
            FOR_EACH_MARSHAL_TYPE_OF_INTERFACE(curr_inf, type_info)
            {
                /* Generic marshal types only be considered for Focused/Mirroing device */
                if (earbudHandover_IsMarshalTypeCategoryPerInstance(type_info->category) || earbudHandover_IsGenericDataAllowedForDevice(device))
                {
                    number_of_marshal_types++;
                }
            }
        }
    }
    EB_HANDOVER_DEBUG_LOG_DEBUG("earbudHandover_InitUnmarshalDataListForDevice: number_of_marshal_types:%d, device:0x%6lx, transport:%d",
                                number_of_marshal_types,
                                device->tp_bd_addr.taddr.addr.lap,
                                device->tp_bd_addr.transport);
    device->u.unmarshal.data_list = (handover_app_unmarshal_data_t *)PanicNull(calloc(number_of_marshal_types, sizeof(handover_app_unmarshal_data_t)));
    device->u.unmarshal.list_size = number_of_marshal_types;
    device->u.unmarshal.list_free_index = 0;
}

/*! \brief  Get category of marshal type in requested registered interface
 *          Only used during unmarshalling procedure.
 * \param[in] interface Pointer to registered interface which handles the given marshal type
 * \param[in] type  Marshal type of which category to be retrieved
 * \returns Marshal type category of requested marshal type in an interface.
 *          Refer \ref marshal_type_category_t.
 */
static uint8 earbudHandover_getMarshalTypeCategoryFromInterface(const registered_handover_interface_t* interface, uint8 type)
{
    /* For all types in current interface's type_list */
    FOR_EACH_MARSHAL_TYPE_OF_INTERFACE(interface, type_info)
    {
        if (type_info->type == type)
        {
            return type_info->category;
        }
    }
    EB_HANDOVER_DEBUG_LOG_ERROR("earbudHandover_getMarshalTypeCategoryFromInterface:failed for type:%d", type);
    /* Shouldn't end up with unidentified type for an interface */
    Panic();
    return MARSHAL_TYPE_CATEGORY_GENERIC;
}

/*! \brief Find the registered interface which handles a given marshal type.
    Only used during unmarshaling procedure.
    \param[in] marshal_type Marshal type for which handover interface is to be searched.
    \param[in] interface_type Type of interface(BLE/BREDR) shall be considered for marshal_type look-up match.
    \returns Pointer to handover interface (if found), or NULL.
             Refer \ref registered_handover_interface_t.
*/
static const registered_handover_interface_t * earbudHandover_GetInterfaceForType(uint8 marshal_type, uint8 interface_type)
{
    /* For all registered interfaces */
    FOR_EACH_REGISTERED_INTERFACE(curr_inf)
    {
        /* Specific interface type only shall be considered for matching marshal type */
        if ((curr_inf->type_list) && (curr_inf->interface_type == interface_type))
        {
            /* For all types in current interface's type_list */
            FOR_EACH_MARSHAL_TYPE_OF_INTERFACE(curr_inf, type_info)
            {
                if (type_info->type == marshal_type)
                {
                    return curr_inf;
                }
            }
        }
    }

    return NULL;
}

/*! \brief  Destroy marshaller/unmarshaller instances of device instance, if any.
 *          This can be used during Complete, Abort stages of handover.
 *  \param[in] device Pointer to device instance which holds marshaller/unmarshaller instance.
 */
static void earbudHandover_CleanupMarshallerForDevice(handover_app_device_t *device)
{
    EB_HANDOVER_DEBUG_LOG_VERBOSE("earbudHandover_CleanupMarshallerForDevice");

    if (device->marshal_state == MARSHAL_STATE_MARSHALING)
    {
        if (device->u.marshal.marshaller)
        {
            MarshalDestroy(device->u.marshal.marshaller, FALSE);
        }
        device->u.marshal.marshaller = NULL;
    }
    else if (device->marshal_state == MARSHAL_STATE_UNMARSHALING)
    {
        if (device->u.unmarshal.unmarshaller)
        {
            UnmarshalDestroy(device->u.unmarshal.unmarshaller, FALSE);
        }
        device->u.unmarshal.unmarshaller = NULL;

        /* Clear and free up unmarshalled data for this device */
        earbudHandover_CleanupUnmarshalDataListForDevice(device);
    }
    device->marshal_state = MARSHAL_STATE_INITIALIZED;
}

/*! \brief  Clean up the all devices, if any, were created for application handover procedure*/
static void earbudHandover_CleanupDeviceList(void)
{
    handover_app_device_t **current_dev = &earbudHandover_Get()->device_list;
    while (*current_dev)
    {
        handover_app_device_t *next_dev = (*current_dev)->next;
        /* Should free-up all the marshaller/unmarshaller instances and corresponding data */
        earbudHandover_CleanupMarshallerForDevice(*current_dev);
        free(*current_dev);
        *current_dev = next_dev;
    }
}

/*! \brief  Clean up earbud application context.
 *          This can be used during EarbudHandover_Init, Complete and Abort stage.
 */
static void earbudHandover_CleanupAppContext(void)
{
    handover_app_context_t *app_data = earbudHandover_Get();
    earbudHandover_CleanupDeviceList();
    app_data->curr_interface = NULL;
    app_data->curr_type = NULL;
    appSmSetMarshalledState(FALSE);
}

/*! \brief Perform veto on all registered interfaces.
    \returns TRUE if any registerd component vetos, FALSE otherwise.
*/
static bool earbudHandover_Veto(void)
{
    bool veto = FALSE;

    FOR_EACH_REGISTERED_INTERFACE_CONDITION(curr_inf, !veto)
    {
        if (curr_inf->Veto)
        {
            veto = curr_inf->Veto();
        }
    }

    if (veto)
    {
        EB_HANDOVER_DEBUG_LOG_DEBUG("earbudHandover_Veto vetoed");
    }

    return veto;
}

/*! \brief Perform veto on all registered interfaces for a specific link.
    \returns TRUE if any registerd component vetos, FALSE otherwise.
*/
static bool earbudHandover_VetoLink(const tp_bdaddr *tp_bd_addr)
{
    bool veto = FALSE;

    FOR_EACH_REGISTERED_INTERFACE_CONDITION(curr_inf, !veto)
    {
        union app_handover_veto_u vl = curr_inf->VetoLink;

        if (vl.le && tp_bd_addr->transport == curr_inf->interface_type)
        {
            switch (tp_bd_addr->transport)
            {
                case INTERFACE_TYPE_BREDR:
                    veto = vl.bredr(&tp_bd_addr->taddr.addr);
                break;
                case INTERFACE_TYPE_BLE:
                    veto = vl.le(&tp_bd_addr->taddr);
                break;
                default:
                break;
            }
        }
    }

    return veto;
}

/*! \brief Get the next registered interface which has valid Marshal TypeList
 *
 * \returns Pointer to registered interface which handles valid marshal type.
*/
static const registered_handover_interface_t * getNextInterface(uint8 interface_type)
{
    handover_app_context_t * app_data = earbudHandover_Get();
    const registered_handover_interface_t *start_inf = app_data->interfaces;

    if(app_data->curr_interface)
    {
        /*Start from next to current interface*/
        start_inf = app_data->curr_interface + 1;
    }


    FOR_EACH_REGISTERED_INTERFACE_FROM_CURRENT(curr_inf, start_inf)
    {
        /*We only need interfaces which have valid Marshal Types and matching interface type(BLE/BREDR).*/
        if ((curr_inf->type_list) && (curr_inf->interface_type == interface_type))
            return curr_inf;
    }

    return NULL;
}

/*! \brief Commit the roles on all registered components.
    \param[in] tp_bd_addr Bluetooth address of the connected device / handset.
    \param[in] role TRUE if primary, FALSE otherwise
*/
static void earbudHandover_Commit(const tp_bdaddr *tp_bd_addr, const bool role)
{
    UNUSED(role);

    /* If any valid interfaces are registerd, definitely there must be device created.
     * The device's marshal state must be in MARSHALLING/UNMARSHALLING, otherwise it's fault */
    if (earbudHandover_IsAnyValidInterfaceRegistered(tp_bd_addr->transport))
    {
        handover_app_device_t* device = earbudHandover_GetOrCreateDevice(tp_bd_addr);
        EB_HANDOVER_DEBUG_LOG_DEBUG("earbudHandover_Commit: device:0x%06lx, state:enum:eb_marshal_state_t:%d",
                                                                            tp_bd_addr->taddr.addr.lap,
                                                                            device->marshal_state);
        PanicFalse(device->marshal_state == MARSHAL_STATE_MARSHALING || device->marshal_state == MARSHAL_STATE_UNMARSHALING);
    }
    else
    {
        EB_HANDOVER_DEBUG_LOG_WARN("earbudHandover_Commit:earbudHandover_IsAnyValidInterfaceRegistered failed, transport:%d", tp_bd_addr->transport);
    }
}

/*! \brief calls Unmarshal handlers of registered interface for each unmarshalled type
 *
 * \param[in] device Pointer to device for which unmarshalled data belong to
 */
static void earbudHandover_ClientUnmarshalForDevice(const handover_app_device_t *device)
{
    const registered_handover_interface_t * curr_inf = NULL;
    app_unmarshal_status_t result = UNMARSHAL_FAILURE;

    PanicFalse(device->marshal_state == MARSHAL_STATE_UNMARSHALING);
    EB_HANDOVER_DEBUG_LOG_INFO("earbudHandover_ClientUnmarshalForDevice: device:0x%06lx, transport:%d", device->tp_bd_addr.taddr.addr.lap, device->tp_bd_addr.transport);

    FOR_EACH_UNMARSHAL_DATA_OF_DEVICE(device, unmarshal_data)
    {
        /* Call unmarshal and store the result */
        curr_inf = earbudHandover_GetInterfaceForType(unmarshal_data->type, device->tp_bd_addr.transport);
        PanicNull((void *)curr_inf);
        uint8 type_category = earbudHandover_getMarshalTypeCategoryFromInterface(curr_inf, unmarshal_data->type);
        /* Unmarshal generic marshal type data only for Focused/Mirroring device */
        if (earbudHandover_IsMarshalTypeCategoryPerInstance(type_category) || earbudHandover_IsGenericDataAllowedForDevice(device))
        {
            if (curr_inf->interface_type == INTERFACE_TYPE_BREDR)
            {
                /* Unmarshal interface for BREDR clients will always provide plain bluetooth address of remote device */
                 result = curr_inf->Unmarshal.bredr(&(device->tp_bd_addr.taddr.addr), unmarshal_data->type, unmarshal_data->data);
            }
            else if (curr_inf->interface_type == INTERFACE_TYPE_BLE)
            {
                /* Unmarshal interface for LE clients will provide typed bluetooth address.
                 * This address shall represents resolved public address  in case of bonded LE device or
                 * unresolved random address in case of non bonded LE device */
                 result = curr_inf->Unmarshal.le(&(device->tp_bd_addr.taddr), unmarshal_data->type, unmarshal_data->data);
            }
            PanicFalse(result > UNMARSHAL_FAILURE);
            ((handover_app_unmarshal_data_t*)unmarshal_data)->unmarshalling_status = result;
            EB_HANDOVER_DEBUG_LOG_VERBOSE("earbudHandover_ClientUnmarshalForDevice:Client unmarshal complete for type: %d", unmarshal_data->type);
        }
        else
        {
            /* This type is Generic and Device is not focused/mirroring */
            EB_HANDOVER_DEBUG_LOG_VERBOSE("earbudHandover_ClientUnmarshalForDevice:Client unmarshal skipped for type: %d", unmarshal_data->type);
        }
    }
}

/*! \brief Commit the roles on all registered components.
    \param[in] primary TRUE if primary, FALSE otherwise
*/
static void earbudHandover_Complete(const bool primary)
{
    EB_HANDOVER_DEBUG_LOG_INFO("earbudHandover_Complete: primary:%d", primary);

    if (primary)
    {
        FOR_EACH_APP_DEVICE(device)
        {
            earbudHandover_ClientUnmarshalForDevice(device);
        }
    }
    FOR_EACH_REGISTERED_INTERFACE(curr_inf)
    {
        curr_inf->Commit(primary);
    }

    if (primary)
    {
        FOR_EACH_APP_DEVICE(device)
        {
            appLinkPolicyHandoverForceUpdateHandsetLinkPolicy(&(device->tp_bd_addr.taddr.addr));
        }
    }

    /* Complete is the final interface invoked during handover. Cleanup now. */
    earbudHandover_CleanupAppContext();
}

/*! \brief Handover application's marshaling interface.

    \note Possible cases:
    1. written is not incremented and return value is FALSE: this means buffer is insufficient.
    2. written < buffer_size and return value is FALSE. This means buffer is insufficient.
    3. written <= buffer_size and return value is TRUE. This mean marshaling is complete.
    4. written is not incremented and return value is TRUE. This means marshaling not required.

    \param[in] addr address of handset.
    \param[in] buffer input buffer with data to be marshalled.
    \param[in] buffer_size size of input buffer.
    \param[out] written number of bytes consumed during marshalling.
    \returns TRUE, if marshal types from all registered interfaces have been marshalled
                  successfully. In this case we destory the marshaling instance and set
                  the state back to MARSHAL_STATE_INITIALIZED.
             FALSE, if the buffer is insufficient for marshalling.
 */
static bool earbudHandover_Marshal(const tp_bdaddr *addr, uint8 *buffer, uint16 buffer_size, uint16 *written)
{
    handover_app_context_t * app_data = earbudHandover_Get();
    handover_app_device_t *device;
    bool marshalled = TRUE;
    void *data = NULL;

    PanicNull((void*)addr);
    PanicNull((void*)buffer);
    PanicNull((void*)written);


    /* It's not required to create device, if there is no valid interface is registered for this handover interface type  */
    if(earbudHandover_IsAnyValidInterfaceRegistered(addr->transport))
    {
        /* Creates new device instance if not exist one */
        device = earbudHandover_GetOrCreateDevice(addr);
        PanicFalse(device->marshal_state == MARSHAL_STATE_INITIALIZED ||
                   device->marshal_state == MARSHAL_STATE_MARSHALING);
    }
    else
    {
        /* No marshal types registered, return success */
        EB_HANDOVER_DEBUG_LOG_WARN("earbudHandover_Marshal: No valid interface is registered");
        return TRUE;
    }

    EB_HANDOVER_DEBUG_LOG_INFO("earbudHandover_Marshal: device:0x%06lx, transport:%d, state:enum:eb_marshal_state_t:%d",
                                                                        device->tp_bd_addr.taddr.addr.lap,
                                                                        device->tp_bd_addr.transport,
                                                                        device->marshal_state);

    /* Create marshaller if not done yet */
    if (device->marshal_state == MARSHAL_STATE_INITIALIZED)
    {
        app_data->curr_interface = NULL;
        /* Should filter all the client interfaces that are not related to this device transport(BLE/BREDR) */
        app_data->curr_interface = getNextInterface(addr->transport);
        /* There is already verified that valid interfaces are registered */
        PanicNull((void*)app_data->curr_interface);
        device->marshal_state = MARSHAL_STATE_MARSHALING;
        device->u.marshal.marshaller = MarshalInit(mtd_handover_app, NUMBER_OF_EARBUD_APP_MARSHAL_OBJECT_TYPES);
        PanicNull(device->u.marshal.marshaller);
        app_data->curr_type = app_data->curr_interface->type_list->types_info_list;
    }

    MarshalSetBuffer(device->u.marshal.marshaller, buffer, buffer_size);
    /* For all remaining registered interfaces. */
    while (marshalled && (app_data->curr_interface))
    {
        /* Iterate from current marshal type */
        FOR_EACH_MARSHAL_TYPE_OF_INTERFACE_FROM_CURRENT(app_data->curr_interface, type_info, app_data->curr_type)
        {
            /* Generic marshal types are marshalled only for Focused/Mirroring device */
            if (earbudHandover_IsMarshalTypeCategoryPerInstance(type_info->category) || earbudHandover_IsGenericDataAllowedForDevice(device))
            {
                bool client_marshalled = FALSE;

                if (app_data->curr_interface->interface_type == INTERFACE_TYPE_BREDR)
                {
                    /* Marshal interface for BREDR clients will always provide plain bluetooth address of remote device */
                    client_marshalled = app_data->curr_interface->Marshal.bredr(&(device->tp_bd_addr.taddr.addr), type_info->type, &data);
                }
                else if (app_data->curr_interface->interface_type == INTERFACE_TYPE_BLE)
                {
                    /* Marshal interface for LE clients will provide typed bluetooth address.
                     * This address shall represents resolved public address  in case of bonded LE device or
                     * unresolved random address in case of non bonded LE device */
                    client_marshalled = app_data->curr_interface->Marshal.le(&(device->tp_bd_addr.taddr), type_info->type, &data);
                }

                if (client_marshalled)
                {
                    if (Marshal(device->u.marshal.marshaller, data, type_info->type))
                    {
                        EB_HANDOVER_DEBUG_LOG_VERBOSE("earbudHandover_Marshal - Marshalling successfull for type: %d", type_info->type);
                    }
                    else
                    {
                        /* Insufficient buffer for marshalling. */
                        EB_HANDOVER_DEBUG_LOG_WARN("earbudHandover_Marshal - Insufficient buffer for type: %d!", type_info->type);
                        marshalled = FALSE;
                        /* Save the current marshal type */
                        app_data->curr_type = type_info;
                        break;
                    }
                }
                else
                {
                    /* Nothing to marshal for this type. Continue ahead. */
                }
            }
            else
            {
                EB_HANDOVER_DEBUG_LOG_VERBOSE("earbudHandover_Marshal - Marshalling skipped for type: %d", type_info->type);
            }
        }

        if (marshalled)
        {
            /* Should filter all the client interfaces that are not related to this device transport(BLE/BREDR) */
            app_data->curr_interface = getNextInterface(addr->transport);
            if (app_data->curr_interface)
            {
                /* All types for current interface marshalled.
                 * Re-initialize app_data->curr_type, from next interface.
                 */
                app_data->curr_type = app_data->curr_interface->type_list->types_info_list;
            }
        }
    }

    *written = MarshalProduced(device->u.marshal.marshaller);
    return marshalled;
}

/*! \brief Handover application's unmarshaling interface.

    \note Possible cases,
    1. consumed is not incremented and return value is FALSE. This means buffer is
    insufficient to unmarshal.
    2. consumed < buffer_size and return value is FALSE. Need more data from caller.
    3. consumed == buffer_size and return value is TRUE. This means all data
      unmarshalled successfully. There could still be more data with caller
      in which case this function is invoked again.

    \param[in] addr address of handset.
    \param[in] buffer input buffer with data to be unmarshalled.
    \param[in] buffer_size size of input buffer.
    \param[out] consumed number of bytes consumed during unmarshalling.
    \returns TRUE, if all types have been successfully unmarshalled.
             FALSE, if buffer is insufficient for unmarshalling.
 */
static bool earbudHandover_Unmarshal(const tp_bdaddr *addr, const uint8 *buffer, uint16 buffer_size, uint16 *consumed)
{
    /* We don't have check for earbudHandover_IsAnyValidInterfaceRegistered to create device instance,
     * because handover profile will not invoke earbud interface for Unmarshalling if nothing was
     * marshalled from old Primary earbud and sent to old Secondary earbud */
    handover_app_device_t *device = earbudHandover_GetOrCreateDevice(addr);
    bool unmarshalled = TRUE;
    handover_app_unmarshal_data_t * data = NULL;
    
    PanicFalse(buffer);
    PanicFalse(buffer_size);
    PanicFalse(consumed);
    PanicFalse(device->marshal_state == MARSHAL_STATE_INITIALIZED ||
               device->marshal_state == MARSHAL_STATE_UNMARSHALING);

    EB_HANDOVER_DEBUG_LOG_INFO("earbudHandover_Unmarshal: device:0x%06lx, state:enum:eb_marshal_state_t:%d",
                                                                        device->tp_bd_addr.taddr.addr.lap,
                                                                        device->marshal_state);
    /* Create unmarshaller on first call */
    if (device->marshal_state == MARSHAL_STATE_INITIALIZED)
    {
        /* Unexpected unmarshal call, as no type handlers exist for this handover interface type */
        PanicNull((void*)getNextInterface(addr->transport));

        /*Initialize list of unmarshalled data*/
        earbudHandover_InitUnmarshalDataListForDevice(device);

        device->marshal_state = MARSHAL_STATE_UNMARSHALING;
        device->u.unmarshal.unmarshaller = UnmarshalInit(mtd_handover_app, NUMBER_OF_EARBUD_APP_MARSHAL_OBJECT_TYPES);
        PanicNull(device->u.unmarshal.unmarshaller);
    }

    UnmarshalSetBuffer(device->u.unmarshal.unmarshaller, buffer, buffer_size);

    /* Loop until all types are extracted from buffer */
    while (unmarshalled && (*consumed < buffer_size))
    {
        data = &device->u.unmarshal.data_list[device->u.unmarshal.list_free_index];
        PanicFalse(device->u.unmarshal.list_free_index < device->u.unmarshal.list_size);

        if (Unmarshal(device->u.unmarshal.unmarshaller, &data->data, &data->type))
        {
            /* Could not find interface for marshal_type */
            if (!earbudHandover_GetInterfaceForType(data->type, addr->transport))
            {
                EB_HANDOVER_DEBUG_LOG_ERROR("earbudHandover_Unmarshal - Could not find interface for type: %d", data->type);
                Panic();
            }
                       
            device->u.unmarshal.list_free_index++;
            *consumed = UnmarshalConsumed(device->u.unmarshal.unmarshaller);
            EB_HANDOVER_DEBUG_LOG_VERBOSE("earbudHandover_Unmarshal - Unmarshalling successfull for type: %d, Index: %d, Consumed: %d",
                                          data->type,
                                          device->u.unmarshal.list_free_index - 1,
                                          *consumed);
        }
        else
        {
            /* No types found. Buffer has incomplete data, ask for remaining. */
            EB_HANDOVER_DEBUG_LOG_WARN("earbudHandover_Unmarshal - Incomplete data for unmarshaling!");
            unmarshalled = FALSE;
        }
    }

    return unmarshalled;
}

/*! \brief Handover application's Abort interface
 *
 *  Cleans up the marshaller instance.
 */
static void earbudHandover_Abort(void)
{
    earbudHandover_CleanupAppContext();
}


bool EarbudHandover_Init(Task init_task)
{
    UNUSED(init_task);
    handover_app_context_t * app_data = earbudHandover_Get();
    unsigned handover_registrations_array_dim;
    EB_HANDOVER_DEBUG_LOG_VERBOSE("EarbudHandover_Init");

    memset(app_data, 0, sizeof(*app_data));

    /* Register handover interfaces with earbud handover application. */
    handover_registrations_array_dim = (unsigned)handover_interface_registrations_end -
                                       (unsigned)handover_interface_registrations_begin;
    PanicFalse((handover_registrations_array_dim % sizeof(registered_handover_interface_t)) == 0);
    handover_registrations_array_dim /= sizeof(registered_handover_interface_t);

    if (handover_registrations_array_dim)
    {
        app_data->interfaces = handover_interface_registrations_begin;
        app_data->interfaces_len = handover_registrations_array_dim;
    }
    return TRUE;
}
#endif
