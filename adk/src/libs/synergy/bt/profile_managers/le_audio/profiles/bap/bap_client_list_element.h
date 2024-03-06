/*******************************************************************************

Copyright (C) 2018-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef BAP_CLIENT_LIST_ELEMENT_H_
#define BAP_CLIENT_LIST_ELEMENT_H_

#include "qbl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BapClientListElement
{
    struct BapClientListElement *next;
} BapClientListElement;

/*
 * Public API for BapClientListElement
 */
#define bapClientListElementInitialise(bapClientListElement)  ((bapClientListElement)->next = NULL)
#define bapClientListElementGetNext(bapClientListElement)    ((bapClientListElement)->next)
#define bapClientListElementSetNext(this, other)         ((this)->next = (other))


#ifdef __cplusplus
}
#endif

#endif /* BAP_CLIENT_LIST_ELEMENT_H_ */
