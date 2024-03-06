/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  download_ml_engine_lib_cap.h
 * \ingroup capabilities
 *
 * Download MLEngine Lib Capability header file. <br>
 *
 */
#ifndef _DOWNLOAD_ML_ENGINE_LIB_CAP_H
#define _DOWNLOAD_ML_ENGINE_LIB_CAP_H

/*****************************************************************************
Include Files
*/

#include "capabilities.h"
#include "dsl_ml_engine_lib_if.h"

/*****************************************************************************
Constants
*/

#define DOWNLOAD_ML_ENGINE_LIB_MAOR2_VERSION_MAJOR         1
#define DOWNLOAD_ML_ENGINE_LIB_MAOR2_VERSION_MINOR         0
#define DOWNLOAD_ML_ENGINE_LIB_MAX_OUTPUT_TERMINALS        1
#define DOWNLOAD_ML_ENGINE_LIB_MAX_INPUT_TERMINALS         1

/****************************************************************************
Public Type Declarations
*/

/* capability-specific extra operator data */
typedef struct DOWNLOAD_ML_ENGINE_LIB_OP_DATA
{
  DOWNLOAD_ML_ENGINE_LIB_TABLE ml_engine_lib_table;
} DOWNLOAD_ML_ENGINE_LIB_OP_DATA;

/** The capability data structure for stub capability */
extern const CAPABILITY_DATA download_ml_engine_lib_cap_data;

#endif /* _DOWNLOAD_ML_ENGINE_LIB_CAP_H */
