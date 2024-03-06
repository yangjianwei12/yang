/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      Kymera QSS Lossless Audio Data Manager
*/
#ifndef KYMERA_QSS_H
#define KYMERA_QSS_H

#ifdef INCLUDE_QSS

/*! \brief Read the QSS lossless audio data from respective kymera plugin 

    \param lossless data.

    \return TRUE if fetch was was successful, else FALSE
*/
bool KymeraQss_ReadAptxAdaptiveLosslesssInfo(uint32 *lossless_data);
#endif /*INCLUDE_QSS */

#endif /* KYMERA_QSS_H */
