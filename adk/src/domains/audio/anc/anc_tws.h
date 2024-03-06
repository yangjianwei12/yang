/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       anc_tws.h
\brief      Provides TWS support for ANC use cases. 
*/


#ifndef ANC_TWS_H
#define ANC_TWS_H

/*! \brief Check if primary device
*/
#ifdef ENABLE_ADAPTIVE_ANC
bool AncTws_IsPrimary(void);
#else
#define AncTws_IsPrimary() (FALSE)
#endif

#endif /* ANC_TWS_H */
