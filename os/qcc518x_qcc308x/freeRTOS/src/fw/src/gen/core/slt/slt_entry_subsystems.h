#ifndef SLT_ENTRY_SUBSYSTEMS_H_
#define SLT_ENTRY_SUBSYSTEMS_H_
/* Autogenerated scheduler task header created by fw/tools/make/header_autogen.py */

#include "core/trap_version/trap_version_slt_entry.h"
#include "core/id/id_slt_entry.h"
#include "core/io/io_slt_entry.h"
#include "core/pmalloc/pmalloc_slt_entry.h"

#define SLT_ENTRY_LIST(m)\
 CORE_TRAP_VERSION_SLT_ENTRY(m) \
 CORE_ID_SLT_ENTRY(m) \
 CORE_IO_SLT_ENTRY(m) \
 CORE_PMALLOC_SLT_ENTRY(m)

#endif /* SLT_ENTRY_SUBSYSTEMS_H_ */
