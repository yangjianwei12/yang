###############################################################################
# Copyright (c) 2012-2018 Qualcomm Technologies International, Ltd.
# All Rights Reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
# REVISION:      $Revision: #  $
###############################################################################

include(CMakeLists.txt.dist)

csr_add_quality_checks(csr_bt_hidd
                       SOURCES ${SRCS} ${SRCS_API}
                       HEADERS ${HDRS} ${PUBLIC_HDRS})

csr_add_quality_targets(csr_bt_hidd)

