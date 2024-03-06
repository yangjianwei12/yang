/******************************************************************************
 Copyright (c) 2008-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_bt_util.h"
#include "csr_bt_platform.h"
#include "csr_bt_obex_extraction_lib.h"
#include "csr_bt_obex.h"
#include "csr_bt_obex_auth.h"
#include "csr_bt_pb_common.h"
#include "csr_bt_pb_util.h"

#ifndef EXCLUDE_CSR_BT_PAC_MODULE
static const CsrCharString *_pbFolderName[] =
{
 CSR_BT_PB_FOLDER_PB_STR,
 CSR_BT_PB_FOLDER_ICH_STR,
 CSR_BT_PB_FOLDER_OCH_STR,
 CSR_BT_PB_FOLDER_MCH_STR,
 CSR_BT_PB_FOLDER_CCH_STR,
 CSR_BT_PB_FOLDER_FAV_STR,
 CSR_BT_PB_FOLDER_SPD_STR,
};

static const CsrUint8 _pbHeaderValSize[] =
{
 CSR_BT_OBEX_PB_ORDER_VAL_LEN,
 CSR_BT_OBEX_PB_SEARCH_VAL_LEN,
 CSR_BT_OBEX_PB_SEARCH_ATT_VAL_LEN,
 CSR_BT_OBEX_PB_MAX_LST_CNT_VAL_LEN,
 CSR_BT_OBEX_PB_LST_START_OFF_VAL_LEN,
 CSR_BT_OBEX_PB_PROP_SEL_VAL_LEN,
 CSR_BT_OBEX_PB_FORMAT_VAL_LEN,
 CSR_BT_OBEX_PB_PHONEBOOK_SIZE_VAL_LEN,
 CSR_BT_OBEX_PB_MISSED_CALLS_VAL_LEN,
 CSR_BT_OBEX_PB_PRIM_VER_VAL_LEN,
 CSR_BT_OBEX_PB_SEC_VER_VAL_LEN,
 CSR_BT_OBEX_PB_VCARD_SEL_VAL_LEN,
 CSR_BT_OBEX_PB_DATABASE_ID_VAL_LEN,
 CSR_BT_OBEX_PB_VCARD_SEL_OP_VAL_LEN,
 CSR_BT_OBEX_PB_RST_MISSED_CALL_VAL_LEN,
 CSR_BT_OBEX_PB_SUPP_FEATURES_VAL_LEN
};

void CsrBtPbReverseCopy(CsrUint8* dest, CsrUint8* src, CsrUint8 size)
{
    CsrUint8 i;
    for (i = 0; i < size; i++)
    {
        dest[i] = src[size - i - 1];
    }
}

CsrUint8 CsrBtPbHeaderValSize(CsrUint8 header)
{
    CsrUint8 size;
    CsrUint8 index = header - 1;

    if (index < CSR_BT_OBEX_PB_LAST_ID)
    {
        size = _pbHeaderValSize[index];
    }
    else
    {
        size = 0;
    }
    return size;
}

CsrUint16 CsrBtPbApplicationHeaderLen(PbAppParFlag flag)
{
    CsrUint16 nLen = CSR_BT_OBEX_APP_PAR_HEADER_SIZE;
    CsrUint8 i;
    for(i = 0; i <= CSR_BT_OBEX_PB_LAST_ID; i++)
    {
        if(flag & CSR_BT_OBEX_PB_ID_TO_FLAG(i))
        {
            nLen += CsrBtPbHeaderValSize(i) + CSR_BT_OBEX_PB_TAG_SIZE;
        }
    }
    return nLen;
}

CsrUint16 CsrBtPbPutHeaderParam(CsrUint8 *buf,
                                CsrUint8 header,
                                CsrUint8 *pHeaderVal)
{
    CsrUint8 size;

    if (header == CSR_BT_OBEX_PB_SEARCH_VAL_ID)
    {
        size = (CsrUint8)CsrStrLen((CsrCharString *) pHeaderVal) + 1;
        CsrMemCpy(&buf[CSR_BT_OBEX_PB_TAG_SIZE], pHeaderVal, size);
    }
    else
    {
        size = CsrBtPbHeaderValSize(header);
        CsrBtPbReverseCopy(&buf[CSR_BT_OBEX_PB_TAG_SIZE], pHeaderVal, size);
    }

    if (size)
    {
        buf[0] = header;
        buf[1] = size;
        return (size + CSR_BT_OBEX_PB_TAG_SIZE);
    }
    else
    {
        return (0);
    }
}

/* Common folders between telecom and sim/telecom are handled here */
static CsrUint16 getFolderFromCommonTelecom(CsrUint16 currentFolder,
                                            CsrCharString **pStr)
{
    if (!CsrStrNCmp(*pStr,
                    CSR_BT_PB_FOLDER_PB_STR,
                    CsrStrLen(CSR_BT_PB_FOLDER_PB_STR)))
    {
        *pStr += CsrStrLen(CSR_BT_PB_FOLDER_PB_STR);
        currentFolder |= CSR_BT_PB_PB_ID << 8;
    }
    else if (!CsrStrNCmp(*pStr,
                         CSR_BT_PB_FOLDER_ICH_STR,
                         CsrStrLen(CSR_BT_PB_FOLDER_ICH_STR)))
    {
        *pStr += CsrStrLen(CSR_BT_PB_FOLDER_ICH_STR);
        currentFolder |= CSR_BT_PB_ICH_ID << 8;
    }
    else if (!CsrStrNCmp(*pStr,
                         CSR_BT_PB_FOLDER_OCH_STR,
                         CsrStrLen(CSR_BT_PB_FOLDER_OCH_STR)))
    {
        *pStr += CsrStrLen(CSR_BT_PB_FOLDER_OCH_STR);
        currentFolder |= CSR_BT_PB_OCH_ID << 8;
    }
    else if (!CsrStrNCmp(*pStr,
                         CSR_BT_PB_FOLDER_MCH_STR,
                         CsrStrLen(CSR_BT_PB_FOLDER_MCH_STR)))
    {
        *pStr += CsrStrLen(CSR_BT_PB_FOLDER_MCH_STR);
        currentFolder |= CSR_BT_PB_MCH_ID << 8;
    }
    else if (!CsrStrNCmp(*pStr,
                         CSR_BT_PB_FOLDER_CCH_STR,
                         CsrStrLen(CSR_BT_PB_FOLDER_CCH_STR)))
    {
        *pStr += CsrStrLen(CSR_BT_PB_FOLDER_CCH_STR);
        currentFolder |= CSR_BT_PB_CCH_ID << 8;
    }
    else if (!CsrStrNCmp(*pStr,
                         CSR_BT_PB_FOLDER_UP_STR,
                         CsrStrLen(CSR_BT_PB_FOLDER_UP_STR)))
    {
        *pStr += CsrStrLen(CSR_BT_PB_FOLDER_UP_STR);
        /* Go up */
        currentFolder &= ~CSR_BT_PB_FOLDER_TELECOM_ID;
    }
    else
    {
        /* No other folder possible */
        currentFolder = CSR_BT_PB_FOLDER_INVALID_ID;
    }

    return (currentFolder);
}

static CsrUint16 getFolderFromPhoneTelecom(CsrUint16 currentFolder,
                                           CsrCharString **pStr)
{
    /* telecom has "fav" and "spd" folder besides the common folders */
    if (!CsrStrNCmp(*pStr,
                    CSR_BT_PB_FOLDER_FAV_STR,
                    CsrStrLen(CSR_BT_PB_FOLDER_FAV_STR)))
    {
        *pStr += CsrStrLen(CSR_BT_PB_FOLDER_FAV_STR);
        currentFolder |= CSR_BT_PB_FAV_ID << 8;
    }
    else if (!CsrStrNCmp(*pStr,
                         CSR_BT_PB_FOLDER_SPD_STR,
                         CsrStrLen(CSR_BT_PB_FOLDER_SPD_STR)))
    {
        *pStr += CsrStrLen(CSR_BT_PB_FOLDER_SPD_STR);
        currentFolder |= CSR_BT_PB_SPD_ID << 8;
    }
    else
    {
        currentFolder = getFolderFromCommonTelecom(currentFolder, pStr);
    }

    return (currentFolder);
}

static CsrUint16 getFolderFromSim(CsrUint16 currentFolder,
                                  CsrCharString **pStr)
{
    if (!CsrStrNCmp(*pStr,
                    CSR_BT_PB_FOLDER_TELECOM_STR,
                    CsrStrLen(CSR_BT_PB_FOLDER_TELECOM_STR)))
    {
        *pStr += CsrStrLen(CSR_BT_PB_FOLDER_TELECOM_STR);
        currentFolder |= CSR_BT_PB_FOLDER_TELECOM_ID;
    }
    else if (!CsrStrNCmp(*pStr,
                         CSR_BT_PB_FOLDER_UP_STR,
                         CsrStrLen(CSR_BT_PB_FOLDER_UP_STR)))
    {
        *pStr += CsrStrLen(CSR_BT_PB_FOLDER_UP_STR);
        /* Go up */
        currentFolder &= ~CSR_BT_PB_FOLDER_SIM1_ID;
    }
    else
    {
        /* No other source folder possible */
        currentFolder = CSR_BT_PB_FOLDER_INVALID_ID;
    }

    return (currentFolder);
}

static CsrUint16 getFolderFromRoot(CsrUint16 currentFolder,
                                   CsrCharString **pStr)
{
    if (!CsrStrNCmp(*pStr,
                    CSR_BT_PB_FOLDER_SIM1_STR,
                    CsrStrLen(CSR_BT_PB_FOLDER_SIM1_STR)))
    {
        *pStr += CsrStrLen(CSR_BT_PB_FOLDER_SIM1_STR);
        currentFolder = CSR_BT_PB_FOLDER_SIM1_ID;
    }
    else if (!CsrStrNCmp(*pStr,
                         CSR_BT_PB_FOLDER_UP_STR,
                         CsrStrLen(CSR_BT_PB_FOLDER_UP_STR)))
    {
        /* Cannot go up from root */
        currentFolder = CSR_BT_PB_FOLDER_INVALID_ID;
    }
    else
    {
        currentFolder = getFolderFromSim(currentFolder, pStr);
    }

    return (currentFolder);
}

static CsrUint16 getFolderFromUnknownPath(CsrUint16 currentFolder,
                                          CsrCharString **pStr)
{
    if (currentFolder >> 8)
    {
        /* We are in one of phonebook/sim folders */
        if (!CsrStrNCmp(*pStr,
                        CSR_BT_PB_FOLDER_UP_STR,
                        CsrStrLen(CSR_BT_PB_FOLDER_UP_STR)))
        {
            *pStr += CsrStrLen(CSR_BT_PB_FOLDER_UP_STR);
            /* Go up */
            currentFolder &= 0xFF;
        }
        else
        {
            /* Cannot go any further down */
            currentFolder = CSR_BT_PB_FOLDER_INVALID_ID;
        }
    }
    else
    {
        /* Unknown folder */
        currentFolder = CSR_BT_PB_FOLDER_INVALID_ID;
    }
    return (currentFolder);
}

CsrUint16 CsrBtPbGetFolderId(CsrUint16 currentFolder,
                             CsrUcs2String *relativePath)
{
    /* Start with current folder. Parse relativePath in parts.
     * Change current folder as relativePath is parsed.
     * Return CSR_BT_PB_INVALID_ID if illegal folder path is arrived at.
     * Consider ".vcf" also as end of string */

    if (relativePath != NULL)
    {
        CsrCharString *str = (CsrCharString *) CsrUcs2ByteString2Utf8(relativePath);
        CsrCharString *strTmp = str;

        while (CsrStrLen(strTmp) != 0)
        {
            switch (currentFolder)
            {
                case CSR_BT_PB_FOLDER_ROOT_ID:
                currentFolder = getFolderFromRoot(currentFolder, &strTmp);
                    break;

                case CSR_BT_PB_FOLDER_SIM1_ID:
                currentFolder = getFolderFromSim(currentFolder, &strTmp);
                    break;

                case CSR_BT_PB_FOLDER_TELECOM_ID:
                /* telecom has "fav" and "spd" folder besides the common folders */
                currentFolder = getFolderFromPhoneTelecom(currentFolder, &strTmp);
                    break;

                case CSR_BT_PB_FOLDER_SIM1_TELECOM_ID:
                currentFolder = getFolderFromCommonTelecom(currentFolder, &strTmp);
                    break;

                default:
                currentFolder = getFolderFromUnknownPath(currentFolder, &strTmp);
                    break;
            }

            if (currentFolder == CSR_BT_PB_FOLDER_INVALID_ID)
            {
                /* Invalid path. Something wrong with input */
                break;
            }
            /* Skip "/" (path separator) */
            else if (!CsrStrNCmp(strTmp,
                                 CSR_BT_PB_PATH_SEPARATOR,
                                 CsrStrLen(CSR_BT_PB_PATH_SEPARATOR)))
            {
                strTmp += CsrStrLen(CSR_BT_PB_PATH_SEPARATOR);
            }
            else if (!CsrStrNCmp(strTmp,
                                 CSR_BT_PB_VCF_EXT,
                                 CsrStrLen(CSR_BT_PB_VCF_EXT)))
            {
                /* End of path */
                break;
            }
        }
        CsrPmemFree(str);
    }
    return (currentFolder);
}

CsrUcs2String *CsrBtPbGetFolderStr(CsrUint16 current,
                                   CsrUint16 target,
                                   CsrBool vcf)
{
    /* Folder path consists of two part - "source" and "folder"
     * "source" represents origin of folder
     * "folder" are phonebook and call history folders */

    CsrCharString path[CSR_BT_MAX_PATH_LENGTH];
    CsrUint8 targetFolder = target >> 8;
    CsrUint8 targetSource = target & 0xFF;
    CsrUint8 *pCurrentSource = (CsrUint8 *)&current;
    CsrUint8 *pCurrentFolder = ((CsrUint8 *)&current) + 1;

    /* Start with empty string */
    path[0] = '\0';
    while(target != current)
    {
        if(targetSource == *pCurrentSource)
        {
            /* Same source folder */
            if(targetFolder != *pCurrentFolder)
            {
                /* Different phonebook folders */
                if(*pCurrentFolder == 0)
                {
                    /* We are at telecom. Just jump to target folder */
                    current = target;
                    CsrStrLCat(path,
                               CSR_BT_PB_PATH_SEPARATOR,
                               sizeof(path));
                    CsrStrLCat(path,
                               _pbFolderName[targetFolder - 1],
                               sizeof(path));
                }
                else
                {
                    /* Different folders. Jump up */
                    *pCurrentFolder = 0;
                    CsrStrLCat(path,
                               CSR_BT_PB_PATH_SEPARATOR,
                               sizeof(path));
                    CsrStrLCat(path,
                               CSR_BT_PB_FOLDER_UP_STR,
                               sizeof(path));
                }
            }
            else
            {
                /* This should not happen. Something wrong with input */
                return (NULL);
            }
        }
        /* Different source folders. We need to move to right source first.
         * Are we at one of phonebook folders? */
        else if(*pCurrentFolder)
        {
            /* Go one step up to telecom */
            *pCurrentFolder = 0;
            CsrStrLCat(path,
                       CSR_BT_PB_PATH_SEPARATOR,
                       sizeof(path));
            CsrStrLCat(path,
                       CSR_BT_PB_FOLDER_UP_STR,
                       sizeof(path));
        }
        /* We are not phonebook folders.
         * Are we at one of telecom folders */
        else if(*pCurrentSource & CSR_BT_PB_FOLDER_TELECOM_ID)
        {
            /* Go one step up */
            *pCurrentSource &= ~CSR_BT_PB_FOLDER_TELECOM_ID;
            CsrStrLCat(path,
                       CSR_BT_PB_PATH_SEPARATOR,
                       sizeof(path));
            CsrStrLCat(path,
                       CSR_BT_PB_FOLDER_UP_STR,
                       sizeof(path));
        }
        /* We are above telecom folder.
         * Are we at SIM1 folder? */
        else if(*pCurrentSource & CSR_BT_PB_FOLDER_SIM1_ID)
        {
            if (targetSource & CSR_BT_PB_FOLDER_SIM1_ID)
            {
                /* Target is also in SIM1.
                 * Go one step down to telecom */
                *pCurrentSource |= CSR_BT_PB_FOLDER_TELECOM_ID;
                CsrStrLCat(path,
                           CSR_BT_PB_PATH_SEPARATOR,
                           sizeof(path));
                CsrStrLCat(path,
                           CSR_BT_PB_FOLDER_TELECOM_STR,
                           sizeof(path));
            }
            else
            {
                /* Target has different source
                 * Go up to root */
                *pCurrentSource = CSR_BT_PB_FOLDER_ROOT_ID;
                CsrStrLCat(path,
                           CSR_BT_PB_PATH_SEPARATOR,
                           sizeof(path));
                CsrStrLCat(path,
                           CSR_BT_PB_FOLDER_UP_STR,
                           sizeof(path));
            }
        }
        /* Check if we are at root */
        else if(*pCurrentSource == CSR_BT_PB_FOLDER_ROOT_ID)
        {
            /* Check target is in SIM or phonebook */
            if(targetSource & CSR_BT_PB_FOLDER_SIM1_ID)
            {
                *pCurrentSource |= CSR_BT_PB_FOLDER_SIM1_ID;
                CsrStrLCat(path,
                           CSR_BT_PB_PATH_SEPARATOR,
                           sizeof(path));
                CsrStrLCat(path,
                           CSR_BT_PB_FOLDER_SIM1_STR,
                           sizeof(path));
            }
            else
            {
                *pCurrentSource |= CSR_BT_PB_FOLDER_TELECOM_ID;
                CsrStrLCat(path,
                           CSR_BT_PB_PATH_SEPARATOR,
                           sizeof(path));
                CsrStrLCat(path,
                           CSR_BT_PB_FOLDER_TELECOM_STR,
                           sizeof(path));
            }
        }
        else
        {
            /* This should not happen. Something wrong with input */
            return (NULL);
        }
    }

    if(CsrStrLen(path))
    {
        CsrUcs2String *ucs2Path;

        if ((vcf != FALSE) && (target & 0xFF00))
        {
            CsrStrLCat(path,
                       CSR_BT_PB_VCF_EXT,
                       sizeof(path));
        }

        /* Remove path separator ("/") from string */
        ucs2Path = CsrUtf82Ucs2ByteString((CsrUint8 *) (path
                        + CsrStrLen(CSR_BT_PB_PATH_SEPARATOR)));

        return (ucs2Path);
    }
    else
    {
        return (NULL);
    }
}
#endif /*EXCLUDE_CSR_BT_PAC_MODULE*/
