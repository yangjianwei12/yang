/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   qss_manager_domain QSS Manager
\ingroup    domains
\brief      Header file for the QSS Manager
*/

#ifndef QSS_MANAGER_H_
#define QSS_MANAGER_H_

#ifdef INCLUDE_QSS

/*! \brief Initialise the QSS Manager
    \param task
    \return TRUE if init was successful, else FALSE.
*/
bool QssManager_Init(Task task);

/*! \brief Start periodic lossless data poll
*/
void QssManager_StartPeriodicLosslessDataPoll(void);

#endif /* INCLUDE_QSS */
#endif /* QSS_MANAGER_H_ */
