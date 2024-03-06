/*******************************************************************************

Copyright (C) 2021-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef LEA_LOGGING_H_
#define LEA_LOGGING_H_

#include "csr_log_text_2.h"

#ifdef CSR_LOG_ENABLE

#define LEA_LOG_MASK  0x0FFFFFFF

#else
#define LEA_LOG_MASK  0x00000000
#endif

CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtPacsLto);                     /* 0x00000001 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtAscsLto);                     /* 0x00000002 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtBassLto);                     /* 0x00000004 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtBapLto);                      /* 0x00000008 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtCcpLto);                      /* 0x00000010 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtMcpLto);                      /* 0x00000020 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtMcsLto);                      /* 0x00000040 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtTbsLto);                      /* 0x00000080 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtVcsLto);                      /* 0x00000100 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtCsipLto);                     /* 0x00000200 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtVcpLto);                      /* 0x00000400 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtAicsLto);                     /* 0x00000800 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtCasLto);                      /* 0x00001000 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtCsisLto);                     /* 0x00002000 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtMicsLto);                     /* 0x00004000 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtVocsLto);                     /* 0x00008000 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtTmapLto);                     /* 0x00010000 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtTmasLto);                     /* 0x00020000 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtCapLto);                      /* 0x00040000 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtAudioAnnouncementParserLto);  /* 0x00080000 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtMicpLto);                     /* 0x00100000 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtPbpLto);                      /* 0x00200000 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtGmasLto);                     /* 0x00400000 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtGmapLto);                     /* 0x00800000 */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtHidsLto);                     /* 0x01000000 */


void LeaLoggingRegister(void);

#endif
