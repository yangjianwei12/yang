/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      DCF framework for v1 format.
*/

#ifndef DCF_V1_FORMAT_H_
#define DCF_V1_FORMAT_H_

/*! \brief DCF data element constructor function callback */
typedef uint8 (* dcf_data_element_constructor_function) (uint8 * data, uint8 max_data_size);

/*! \brief DCF data element constructor */
typedef struct
{
    uint32 type;
    dcf_data_element_constructor_function constructor_function;
} dcf_data_element_constructor_t;

/*! \brief DCF advertising data set identities */
typedef enum
{
    dcf_v1_public_identity,
    dcf_v1_private_identity,
    dcf_v1_trusted_identity,
    dcf_v1_trusted_plus_private_identity,
} dcf_v1_identity_t;

/*! \brief Data element set definition */
typedef struct 
{
    dcf_data_element_constructor_t * constructors;
    uint8 number_of_constructors;
    dcf_v1_identity_t identity;
} dcf_data_element_set_t;

/*! \brief Constructs the supplied data element set

    \param data pointer to a data buffer where the data element set will be constructed
    \param max_data_size size of data buffer
    \param data_element_set the data element set to be constructed

    \returns total size of the constructed data element set
*/
uint8 Dcfv1format_ConstructDataElementSet(uint8 * data, uint8 max_data_size, const dcf_data_element_set_t * data_element_set);

#endif
