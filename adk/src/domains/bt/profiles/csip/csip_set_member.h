/*!
   \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
               All Rights Reserved.
               Qualcomm Technologies International, Ltd. Confidential and Proprietary.
   \file
   \addtogroup csip
   \brief      Coordinated Sets Identification Profile (CSIP)

               CSIP enables the discovery of devices that belong to one or more Coordinated Sets.
               A Coordinated Set is defined as a group of devices that are configured to support a
               specific scenario. Examples of Coordinated Sets include a pair of hearing aids, a pair
               of earbuds, or a speaker set that receives multi-channel audio and that reacts to control
               commands in a coordinated way (e.g., volume up and volume down). Other examples of
               Coordinated Sets include a group of sensor nodes (e.g., electrocardiogram (EKG) leads,
               tire pressure sensors, etc.) that trigger a specific measurement when instructed by a client device.
               CSIP is agnostic to the actual features and functions implemented by the members of the Coordinated
               Set. The purpose of CSIP is to specify a mechanism to discover a Coordinated Set and its members,
               and to specify how a device can be discovered as part of one or more Coordinated Sets. CSIP
               also specifies a way to grant exclusive access to the Coordinated Set to a client such that
               race conditions can be avoided when multiple clients want to access the Coordinated Set at the
               same time.

   @{
*/

#ifndef CSIP_SET_MEMBER_H_
#define CSIP_SET_MEMBER_H_

/*! \brief Initialises the CSIP set member.

 */
bool CsipSetMember_Init(Task init_task);

/*! \brief Set Sirk key in the LE advertising data

 */
void CsipSetMember_SetSirkKey(uint8 *sirk_key);

#endif /* CSIP_SET_MEMBER_H_ */
/*! @} */