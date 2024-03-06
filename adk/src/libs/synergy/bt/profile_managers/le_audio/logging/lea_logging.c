/*******************************************************************************

Copyright (C) 2021-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#include <stdio.h>
#include "lea_logging.h"

CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtPacsLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtAscsLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtBassLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtBapLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtCcpLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtMcpLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtMcsLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtTbsLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtVcsLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtCsipLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtVcpLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtAicsLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtCasLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtCsisLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtMicsLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtVocsLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtTmapLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtTmasLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtCapLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtAudioAnnouncementParserLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtMicpLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtPbpLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtGmasLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtGmapLto);
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtHidsLto);

void LeaLoggingRegister(void)
{
    CSR_LOG_TEXT_REGISTER(&CsrBtPacsLto, "PACS", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtAscsLto, "ASCS", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtBassLto, "BASS", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtBapLto, "BAP", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtCcpLto, "CCP", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtMcpLto, "MCP", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtMcsLto, "MCS", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtTbsLto, "TBS", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtVcsLto, "VCS", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtCsipLto, "CSIP", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtVcpLto, "VCP", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtAicsLto, "AICS", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtCasLto, "CAS", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtCsisLto, "CSIS", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtMicsLto, "MICS", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtVocsLto, "VOCS", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtTmapLto, "TMAP", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtTmasLto, "TMAS", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtCapLto, "CAP", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtAudioAnnouncementParserLto, "Audio Announcement Parser", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtMicpLto, "MICP", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtGmasLto, "GMAS", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtGmapLto, "GMAP", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtHidsLto, "HIDS", 0, NULL);

    CSR_LOG_TEXT_REGISTER(&CsrBtPbpLto, "PBP", 0, NULL);
}
