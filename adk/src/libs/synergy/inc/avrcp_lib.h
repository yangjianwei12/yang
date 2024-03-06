/******************************************************************************
 Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 DESCRIPTION
    Header file providing mapping for Synergy AVRCP profile's public interfaces.
    Refer to csr_bt_avrcp_lib.h for APIs descriptions.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef COMMON_SYNERGY_INC_AVRCP_LIB_H_
#define COMMON_SYNERGY_INC_AVRCP_LIB_H_

#include "synergy.h"
#include "csr_bt_avrcp_lib.h"

/*! Default AVRCP supported features */
#define AVRCP_CONFIG_SUPPORTED_FEATURES    (CSR_BT_AVRCP_CONFIG_SR_FEAT_CAT1_PLAY_REC)

/*! Default AVRCP MTU size */
#define AVRCP_CONFIG_DEFAULT_MTU            (672)

/*! Default AVRCP Provider Name */
#define AVRCP_CONFIG_PROVIDER_NAME         "CAA-AV-AVRCP"

#define AVRCP_PRIM             (SYNERGY_EVENT_BASE + CSR_BT_AVRCP_PRIM)

#define AvrcpConfigReqSend(_task,                           \
                           _config,                         \
                           _mtu,                            \
                           _tgConfig,                       \
                           _ctConfig)                       \
    CsrBtAvrcpConfigReqSend(TrapToOxygenTask(_task),        \
                            _config,                        \
                            _mtu,                           \
                            _tgConfig,                      \
                            _ctConfig,                      \
                            0)

#define AvrcpConfigRoleSupport(_details,                    \
                               _roleConfig,                 \
                               _version,                    \
                               _featureMask,                \
                               _providerName,               \
                               _serviceName)                \
    CsrBtAvrcpConfigRoleSupport(_details,                   \
                               _roleConfig,                 \
                               _version,                    \
                               _featureMask,                \
                               _providerName,               \
                               _serviceName)

#define AvrcpConfigRoleNoSupport(_details)                  \
    CsrBtAvrcpConfigRoleNoSupport(_details)

#define AvrcpActivateReqSend(_maxIncoming)                  \
    CsrBtAvrcpActivateReqSend(_maxIncoming)

#define AvrcpConnectReqSend(_deviceAddr)                    \
    CsrBtAvrcpConnectReqSend(_deviceAddr)

#define AvrcpDisconnectReqSend(_connectionId)               \
    CsrBtAvrcpDisconnectReqSend(_connectionId)

#define AvrcpCtPassThroughReqSend(_task,                            \
                                  _connId,                          \
                                  _opId,                            \
                                  _state)                           \
    CsrBtAvrcpCtPassThroughReqSend(TrapToOxygenTask(_task),         \
                                  _connId,                          \
                                  _opId,                            \
                                  _state)

#define AvrcpTgPassThroughResSend(_connId,                          \
                                  _msgId,                           \
                                  _status)                          \
    CsrBtAvrcpTgPassThroughResSend(_connId,                         \
                                   _msgId,                          \
                                   _status)

#define AvrcpCtNotiRegisterReqSend(_task,                           \
                                   _connId,                         \
                                   _notiMask,                       \
                                   _playbackInterval,               \
                                   _configMask)                     \
    CsrBtAvrcpCtNotiRegisterReqSend(TrapToOxygenTask(_task),        \
                                    _connId,                        \
                                    _notiMask,                      \
                                    _playbackInterval,              \
                                    _configMask)

#define AvrcpCtSetVolumeReqSend(_task,                              \
                                _connId,                            \
                                _volume)                            \
    CsrBtAvrcpCtSetVolumeReqSend(TrapToOxygenTask(_task),           \
                                 _connId,                           \
                                 _volume)

#define AvrcpTgNotiVolumeRes(_connId,                               \
                             _status,                               \
                             _msgId,                                \
                             _volume)                               \
    CsrBtAvrcpTgNotiVolumeRes(_connId,                              \
                              _status,                              \
                              _msgId,                               \
                              _volume)

#define AvrcpTgNotiVolumeReq(_playerId, _volume)                    \
    CsrBtAvrcpTgNotiVolumeReq(_playerId, _volume)

#define AvrcpTgSetVolumeResSend(_connId,                            \
                                _volume,                            \
                                _msgId,                             \
                                _tlabel,                            \
                                _status)                            \
    CsrBtAvrcpTgSetVolumeExtResSend(_connId,                        \
                                    _volume,                        \
                                    _msgId,                         \
                                    _status,                        \
                                    _tlabel)

#define AvrcpTgNotiPlaybackStatusRes(_connId,                       \
                                      _status,                      \
                                      _msgId,                       \
                                      _playbackStatus)              \
     CsrBtAvrcpTgNotiPlaybackStatusRes(_connId,                     \
                                      _status,                      \
                                      _msgId,                       \
                                      _playbackStatus)

#define AvrcpTgNotiPlaybackStatusReq(_playerId, _playbackStatus)    \
    CsrBtAvrcpTgNotiPlaybackStatusReq(_playerId, _playbackStatus)

#define AvrcpTgMpRegisterReqSend(_task,                             \
                                 _notificationMask,                 \
                                 _configMask,                       \
                                 _pasLen,                           \
                                 _pas,                              \
                                 _majorType,                        \
                                 _subType,                          \
                                 _featureMask,                      \
                                 _playerName)                       \
    CsrBtAvrcpTgMpRegisterReqSend(TrapToOxygenTask(_task),          \
                                  _notificationMask,                \
                                  _configMask,                      \
                                  _pasLen,                          \
                                  _pas,                             \
                                  _majorType,                       \
                                  _subType,                         \
                                  _featureMask,                     \
                                  _playerName)

#define AvrcpTgNotiReqSend(_playerId,                               \
                           _notiId,                                 \
                           _notiData)                               \
     CsrBtAvrcpTgNotiReqSend(_playerId,                             \
                             _notiId,                               \
                             _notiData)

#define AvrcpTgNotiResSend(_connId,                                 \
                           _notiId,                                 \
                           _notiData,                               \
                           _status,                                 \
                           _msgId)                                  \
     CsrBtAvrcpTgNotiResSend(_connId,                               \
                             _notiId,                               \
                             _notiData,                             \
                             _status,                               \
                             _msgId)

#ifdef INSTALL_AVRCP_MEDIA_PLAYER_SELECTION
#define AvrcpCtSetAddressedPlayerReqSend(_task,                     \
                                         _connId,                   \
                                         _playerId)                 \
    CsrBtAvrcpCtSetAddressedPlayerReqSend(TrapToOxygenTask(_task),  \
                                          _connId,                  \
                                          _playerId)

#define AvrcpTgSetAddressedPlayerReqSend(_connId,                   \
                                         _playerId,                 \
                                         _uidCounter)               \
     CsrBtAvrcpTgSetAddressedPlayerReqSend(_connId,                 \
                                           _playerId,               \
                                           _uidCounter)

#define AvrcpTgSetAddressedPlayerResSend(_connId,                   \
                                         _playerId,                 \
                                         _uidCounter,               \
                                         _msgId,                    \
                                         _status)                   \
     CsrBtAvrcpTgSetAddressedPlayerResSend(_connId,                 \
                                           _playerId,               \
                                           _uidCounter,             \
                                           _msgId,                  \
                                           _status)
#endif /* INSTALL_AVRCP_MEDIA_PLAYER_SELECTION */

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
/*! Controller APIs for Browsing */
#define AvrcpCtSetBrowsedPlayerReqSend(_task,                       \
                                       _connId,                     \
                                       _playerId)                   \
        CsrBtAvrcpCtSetBrowsedPlayerReqSend(TrapToOxygenTask(_task),\
                                            _connId,                \
                                            _playerId)

#define AvrcpCtGetFolderItemsReqSend(_task,                         \
                                     _connId,                       \
                                     _scope,                        \
                                     _startItem,                    \
                                     _endItem,                      \
                                     _attributeMask)                \
        CsrBtAvrcpCtGetFolderItemsReqSend(TrapToOxygenTask(_task),  \
                                          _connId,                  \
                                          _scope,                   \
                                          _startItem,               \
                                          _endItem,                 \
                                          _attributeMask)

#define AvrcpCtChangePathReqSend(_task,                             \
                                 _connId,                           \
                                 _uidCounter,                       \
                                 _folderDir,                        \
                                 _folderUid)                        \
    CsrBtAvrcpCtChangePathReqSend(TrapToOxygenTask(_task),          \
                                  _connId,                          \
                                  _uidCounter,                      \
                                  _folderDir,                       \
                                  _folderUid)

#define AvrcpCtPlayReqSend(_task,                                   \
                           _connId,                                 \
                           _scope,                                  \
                           _uidCounter,                             \
                           _uid)                                    \
    CsrBtAvrcpCtPlayReqSend(TrapToOxygenTask(_task),                \
                            _connId,                                \
                            _scope,                                 \
                            _uidCounter,                            \
                            _uid)

#define AvrcpCtAddToNowPlayingReqSend(_task,                        \
                                      _connId,                      \
                                      _scope,                       \
                                      _uidCounter,                  \
                                      _uid)                         \
    CsrBtAvrcpCtAddToNowPlayingReqSend(TrapToOxygenTask(_task),     \
                                       _connId,                     \
                                       _scope,                      \
                                       _uidCounter,                 \
                                       _uid)

#define AvrcpCtSearchReqSend(_task,                                 \
                             _connId,                               \
                             _text,                                 \
                             _charsetId)                            \
    CsrBtAvrcpCtSearchReqSend(TrapToOxygenTask(_task),              \
                              _connId,                              \
                              _text,                                \
                              _charsetId)

#define AvrcpCtGetTotalNumberOfItemsReqSend(_task,                  \
                                            _connId,                \
                                            _scope)                 \
    CsrBtAvrcpCtGetTotalNumberOfItemsReqSend(TrapToOxygenTask(_task), \
                                             _connId,               \
                                             _scope)

/*! Target APIs for Browsing */
#define AvrcpTgSetBrowsedPlayerResSend(_connId,                     \
                                       _playerId,                   \
                                       _uidCounter,                 \
                                       _itemsCount,                 \
                                       _folderDepth,                \
                                       _folderNamesLen,             \
                                       _folderNames,                \
                                       _msgId,                      \
                                       _status)                     \
    CsrBtAvrcpTgSetBrowsedPlayerResSend(_connId,                    \
                                        _playerId,                  \
                                        _uidCounter,                \
                                        _itemsCount,                \
                                        _folderDepth,               \
                                        _folderNamesLen,            \
                                        _folderNames,               \
                                        _msgId,                     \
                                        _status)

#define AvrcpTgGetTotalNumberOfItemsResSend(_connId,                \
                                            _noOfItems,             \
                                            _uidCounter,            \
                                            _msgId,                 \
                                            _status)                \
    CsrBtAvrcpTgGetTotalNumberOfItemsResSend(_connId,               \
                                             _noOfItems,            \
                                             _uidCounter,           \
                                             _msgId,                \
                                             _status)

#define AvrcpTgGetFolderItemsResSend(_connId,                       \
                                     _itemCount,                    \
                                     _uidCounter,                   \
                                     _itemsLen,                     \
                                     _items,                        \
                                     _msgId,                        \
                                     _status)                       \
    CsrBtAvrcpTgGetFolderItemsResSend(_connId,                      \
                                      _itemCount,                   \
                                      _uidCounter,                  \
                                      _itemsLen,                    \
                                      _items,                       \
                                      _msgId,                       \
                                      _status)

#define AvrcpTgChangePathResSend(_connId,                           \
                                 _itemsCount,                       \
                                 _msgId,                            \
                                 _status)                           \
    CsrBtAvrcpTgChangePathResSend(_connId,                          \
                                  _itemsCount,                      \
                                  _msgId,                           \
                                  _status)

#define AvrcpTgPlayResSend(_connId,                                 \
                           _uid,                                    \
                           _scope,                                  \
                           _msgId,                                  \
                           _status)                                 \
    CsrBtAvrcpTgPlayResSend(_connId,                                \
                            _uid,                                   \
                            _scope,                                 \
                            _msgId,                                 \
                            _status)

#define AvrcpTgAddToNowPlayingResSend(_connId,                      \
                                      _msgId,                       \
                                      _status)                      \
    CsrBtAvrcpTgAddToNowPlayingResSend(_connId,                     \
                                       _msgId,                      \
                                       _status)

#define AvrcpTgSearchResSend(_connId,                               \
                             _uidCounter,                           \
                             _numberOfItems,                        \
                             _msgId,                                \
                             _status)                               \
    CsrBtAvrcpTgSearchResSend(_connId,                              \
                              _uidCounter,                          \
                              _numberOfItems,                       \
                              _msgId,                               \
                              _status)

#define AvrcpTgNotiUidsReq(_playerId, _uidCounter)                  \
    CsrBtAvrcpTgNotiUidsReq(_playerId, _uidCounter)
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

#if defined (CSR_BT_INSTALL_AVRCP_BROWSING) || defined (INSTALL_AVRCP_METADATA_ATTRIBUTES)
#define AvrcpCtGetAttributesReqSend(_task,                          \
                                    _connId,                        \
                                    _scope,                         \
                                    _uid,                           \
                                    _uidCounter,                    \
                                    _attributeMask)                 \
    CsrBtAvrcpCtGetAttributesReqSend(TrapToOxygenTask(_task),       \
                                     _connId,                       \
                                     _scope,                        \
                                     _uid,                          \
                                     _uidCounter,                   \
                                     _attributeMask)

#define AvrcpCtGetAttributesResSend(_connId,                        \
                                    _proceed)                       \
    CsrBtAvrcpCtGetAttributesResSend(_connId,                       \
                                     _proceed)

#define AvrcpTgGetAttributesResSend(_connId,                        \
                                    _attribCount,                   \
                                    _attribDataLen,                 \
                                    _attribData,                    \
                                    _msgId,                         \
                                    _status)                        \
    CsrBtAvrcpTgGetAttributesResSend(_connId,                       \
                                    _attribCount,                   \
                                    _attribDataLen,                 \
                                    _attribData,                    \
                                    _msgId,                         \
                                    _status)
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING || INSTALL_AVRCP_METADATA_ATTRIBUTES */

#if defined(INSTALL_AVRCP_METADATA_ATTRIBUTES) || defined(INSTALL_AVRCP_NOTIFICATIONS)
#define AvrcpCtGetPlayStatusReqSend(_task,                          \
                                    _connId)                        \
    CsrBtAvrcpCtGetPlayStatusReqSend(TrapToOxygenTask(_task),       \
                                     _connId)

#define AvrcpTgGetPlayStatusResSend(_connId,                        \
                                    _songLength,                    \
                                    _songPosition,                  \
                                    _playStatus,                    \
                                    _msgId,                         \
                                    _status)                        \
    CsrBtAvrcpTgGetPlayStatusResSend(_connId,                       \
                                    _songLength,                    \
                                    _songPosition,                  \
                                    _playStatus,                    \
                                    _msgId,                         \
                                    _status)

#define AvrcpTgNotiTrackRes(_connId,                                \
                            _status,                                \
                            _msgId,                                 \
                            _uuid)                                  \
    CsrBtAvrcpTgNotiTrackRes(_connId, _status, _msgId, _uuid)

#define AvrcpTgNotiTrackReq(_playerId, _uid)                        \
    CsrBtAvrcpTgNotiTrackReq(_playerId, _uid)
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES || INSTALL_AVRCP_NOTIFICATIONS */

#ifdef CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS
#define AvrcpCtPasAttIdReqSend(_task,                               \
                               _connId)                             \
    CsrBtAvrcpCtPasAttIdReqSend(TrapToOxygenTask(_task),            \
                                _connId)

#define AvrcpCtPasAttTxtReqSend(_task,                              \
                                _connId,                            \
                                _attribIdCount,                     \
                                _attribId)                          \
    CsrBtAvrcpCtPasAttTxtReqSend(TrapToOxygenTask(_task),           \
                                 _connId,                           \
                                 _attribIdCount,                    \
                                 _attribId)

#define AvrcpCtPasAttTxtResSend(_connId,                            \
                                _proceed)                           \
    CsrBtAvrcpCtPasAttTxtResSend(_connId,                           \
                                 _proceed)

#define AvrcpCtPasValIdReqSend(_task,                               \
                               _connId,                             \
                               _attribId)                           \
    CsrBtAvrcpCtPasValIdReqSend(TrapToOxygenTask(_task),            \
                                _connId,                            \
                                _attribId)

#define AvrcpCtPasValTxtReqSend(_task,                              \
                               _connId,                             \
                               _attribId,                           \
                               _valIdCount,                         \
                               _valId)                              \
    CsrBtAvrcpCtPasValTxtReqSend(TrapToOxygenTask(_task),           \
                                 _connId,                           \
                                 _attribId,                         \
                                 _valIdCount,                       \
                                 _valId)

#define AvrcpCtPasValTxtResSend(_connId,                            \
                                _proceed)                           \
    CsrBtAvrcpCtPasValTxtResSend(_connId,                           \
                                 _proceed)

#define AvrcpCtPasCurrentReqSend(_task,                             \
                                _connId,                            \
                                _attribIdCount,                     \
                                _attribId)                          \
    CsrBtAvrcpCtPasCurrentReqSend(TrapToOxygenTask(_task),          \
                                 _connId,                           \
                                 _attribIdCount,                    \
                                 _attribId)

#define AvrcpCtPasSetReqSend(_task,                                 \
                             _connId,                               \
                             _attValPairCount,                      \
                             _attValPair)                           \
    CsrBtAvrcpCtPasSetReqSend(TrapToOxygenTask(_task),              \
                              _connId,                              \
                              _attValPairCount,                     \
                              _attValPair)

#define AvrcpCtInformBatteryStatusReqSend(_task,                    \
                                          _connId,                  \
                                          _batStatus)               \
    CsrBtAvrcpCtInformBatteryStatusReqSend(TrapToOxygenTask(_task), \
                                           _connId,                 \
                                           _batStatus)

#define AvrcpCtInformDispCharSetReqSend(_task,                      \
                                        _connId,                    \
                                        _charsetCount,              \
                                        _charset)                   \
    CsrBtAvrcpCtInformDispCharSetReqSend(TrapToOxygenTask(_task),   \
                                         _connId,                   \
                                         _charsetCount,             \
                                         _charset)

#define AvrcpTgPasCurrentResSend(_connId,                           \
                                 _msgId,                            \
                                 _pasCount,                         \
                                 _pas,                              \
                                 _status)                           \
    CsrBtAvrcpTgPasCurrentResSend(_connId,                          \
                                  _msgId,                           \
                                  _pasCount,                        \
                                  _pas,                             \
                                  _status)

#define AvrcpTgPasSetReq(_task,                                     \
                         _playerId,                                 \
                         _changedPasCount,                          \
                         _changedPas)                               \
    CsrBtAvrcpTgPasSetReqSend(TrapToOxygenTask(_task),              \
                              _playerId,                            \
                              _changedPasCount,                     \
                              _changedPas)

#define AvrcpTgPasSetResSend(_connId,                               \
                             _msgId,                                \
                             _status)                               \
    CsrBtAvrcpTgPasSetResSend(_connId,                              \
                              _msgId,                               \
                              _status)

#endif /* CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS */

#ifdef INSTALL_AVRCP_UNIT_COMMANDS
#define AvrcpCtUnitInfoCmdReqSend(_task,                            \
                                  _connId,                          \
                                  _pDatalen,                        \
                                  _pData)                           \
    CsrBtAvrcpCtUnitInfoCmdReqSend(TrapToOxygenTask(_task),         \
                                   _connId,                         \
                                   _pDatalen,                       \
                                   _pData)

#define AvrcpCtSubUnitInfoCmdReqSend(_task,                         \
                                     _connId,                       \
                                     _pDatalen,                     \
                                     _pData)                        \
    CsrBtAvrcpCtSubUnitInfoCmdReqSend(TrapToOxygenTask(_task),      \
                                      _connId,                      \
                                      _pDatalen,                    \
                                      _pData)

#endif /* INSTALL_AVRCP_UNIT_COMMANDS */

#define AvrcpCtLibItemsAttributeGet(maxData,                        \
                                    attIndex,                       \
                                    itemsLen,                       \
                                    items,                          \
                                    attribId,                       \
                                    charset,                        \
                                    attLen,                         \
                                    att)                            \
    CsrBtAvrcpCtLibItemsAttributeGet(maxData,                       \
                                     attIndex,                      \
                                     itemsLen,                      \
                                     items,                         \
                                     attribId,                      \
                                     charset,                       \
                                     attLen,                        \
                                     att)

#define AvrcpFreeUpstreamMessageContents(_message)                  \
    CsrBtAvrcpFreeUpstreamMessageContents(CSR_BT_AVRCP_PRIM, _message)

#endif /* COMMON_SYNERGY_INC_AVRCP_LIB_H_ */
