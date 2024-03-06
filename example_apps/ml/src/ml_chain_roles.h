/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\ingroup    ml_example
\brief      Chain roles definitions.
 
*/

#ifndef ML_CHAIN_ROLES_H_
#define ML_CHAIN_ROLES_H_


/*@{*/


/*! These names may be used in chain operator definitions.
*/
typedef enum chain_operator_roles
{
    /*! Role identifier used for machine learning example SVAD capability */
    OPR_ML_EXAMPLE_SVAD = 0x1000,

    /*! Role identifier used for download ML engine lib capability */
    OPR_DNLD_ML_ENGINE_LIB,

    /*! Role identifier used for input buffer */
    OPR_INPUT_BUFFER,

} chain_operator_role_t;

/*! These names may be used in chain endpoint definitions.
*/
typedef enum chain_endpoint_roles
{
    /*! Input of audio data */
    EPR_PCM_IN = 0x2000,

} chain_endpoint_role_t;

/*@}*/

#endif /* ML_CHAIN_ROLES_H_ */
