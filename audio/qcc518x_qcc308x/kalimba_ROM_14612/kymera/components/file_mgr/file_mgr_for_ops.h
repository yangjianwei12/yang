/****************************************************************************
 * Copyright 2017 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file file_mgr_for_ops.h
 * \ingroup file_mgr
 *
 * Public definitions for file_mgr
 */

#ifndef _FILE_MGR_FOR_OPS_H_
#define _FILE_MGR_FOR_OPS_H_

/****************************************************************************
Include Files
*/

#include "buffer/cbuffer_c.h"
#ifdef INSTALL_EXTERNAL_MEM
#include "ext_buffer/ext_buffer.h"
#endif

/* Struct with information about the data file */
typedef struct
{
    uint16    type;
    bool      valid;
    union
    {
        tCbuffer      *file_data;
#ifdef INSTALL_EXTERNAL_MEM
        EXT_BUFFER    *ext_file_data;
#endif
    } u;
} DATA_FILE;

/* The callback function definition for transfer complete */
typedef void (*FILE_MGR_EOF_CBACK)(bool);


extern void file_mgr_task_init(void **data);

/**
 *
 * \brief Finds and returns a file of a given ID
 *
 * \param file_id  ID of the file
 *
 * \return pointer to a file
 */
extern DATA_FILE *file_mgr_get_file(uint16 file_id);

/**
 *
 * \brief release the file back for auto free files 
 *
 * \param file_id  ID of the file
 *
 * \return TRUE if successfully released file from
 *         'stored_files', FALSE if not (file_id is
 *         not a valid entry in 'stored_files').
 */
extern bool file_mgr_release_file(uint16 file_id);

/**
 * \brief The audio data service calls this API once transfer is done.
 *
 * \param file    The file transfered.
 * \param success True if the transfer succeeded.
 * \param cback   callback
 **/
extern void file_mgr_transfer_done(DATA_FILE *file,
                                   bool success,
                                   FILE_MGR_EOF_CBACK cback);

#endif /*_FILE_MGR_FOR_OPS_H_*/

