/*!
   \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                           All Rights Reserved.
               Qualcomm Technologies International, Ltd. Confidential and Proprietary.
   \file
   \addtogroup ama
   @{
   \brief      Implementation for AMA On-Head/In-Ear Detection for Accessories.
*/
#ifndef _AMA_OHD_H_
#define _AMA_OHD_H_

/*! \brief Check is the accessory is in-ear(earbuds) or on-head(headset)
 *  \return TRUE if it is in-ear/on-head, otherwise FALSE. 
 */
bool AmaOhd_IsAccessoryInEarOrOnHead(void);

/*! \brief Initialise the AMA Over head component.
*/
void AmaOhd_Init(void);

#endif /* _AMA_OHD_H_ */
/*! @} */