/*******************************************************************************

Copyright (C) 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief buffer iterator - used to iterator over buffers during serialisation and de-serialisation.
 */

#ifndef BUFF_ITERATOR_H_
#define BUFF_ITERATOR_H_

typedef struct BUFF_ITERATOR
{
    uint8_t* data;
} BUFF_ITERATOR;

#define buff_iterator_initialise(iter, buffer)   \
    (iter)->data = (buffer)

#define buff_iterator_get_octet(iter) (*(iter)->data++)


#endif
