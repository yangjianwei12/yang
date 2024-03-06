/******************************************************************************
 Copyright (c) 2019 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include <stdio.h>

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_util.h"

/*------------------------------------------------------------------*/
/* Bits */
/*------------------------------------------------------------------*/

/* Time proportional with the number of 1's */
CsrUint8 CsrBitCountSparse(CsrUint32 n)
{
    CsrUint8 count = 0;

    while (n)
    {
        count++;
        n &= (n - 1);
    }

    return count;
}

/* Time proportional with the number of 0's */
CsrUint8 CsrBitCountDense(CsrUint32 n)
{
    CsrUint8 count = 8 * sizeof(CsrUint32);

    n ^= (CsrUint32) (-1);

    while (n)
    {
        count--;
        n &= (n - 1);
    }

    return count;
}

/*------------------------------------------------------------------*/
/* Base conversion */
/*------------------------------------------------------------------*/
CsrBool CsrHexStrToUint8(const CsrCharString *string, CsrUint8 *returnValue)
{
    CsrUint16 currentIndex = 0;
    *returnValue = 0;
    if ((string[currentIndex] == '0') && (CSR_TOUPPER(string[currentIndex + 1]) == 'X'))
    {
        string += 2;
    }
    if (((string[currentIndex] >= '0') && (string[currentIndex] <= '9')) || ((CSR_TOUPPER(string[currentIndex]) >= 'A') && (CSR_TOUPPER(string[currentIndex]) <= 'F')))
    {
        while (((string[currentIndex] >= '0') && (string[currentIndex] <= '9')) || ((CSR_TOUPPER(string[currentIndex]) >= 'A') && (CSR_TOUPPER(string[currentIndex]) <= 'F')))
        {
            *returnValue = (CsrUint8) (*returnValue * 16 + (((string[currentIndex] >= '0') && (string[currentIndex] <= '9')) ? string[currentIndex] - '0' : CSR_TOUPPER(string[currentIndex]) - 'A' + 10));
            currentIndex++;
            if (currentIndex >= 2)
            {
                break;
            }
        }
        return TRUE;
    }
    return FALSE;
}

CsrBool CsrHexStrToUint16(const CsrCharString *string, CsrUint16 *returnValue)
{
    CsrUint16 currentIndex = 0;
    *returnValue = 0;
    if ((string[currentIndex] == '0') && (CSR_TOUPPER(string[currentIndex + 1]) == 'X'))
    {
        string += 2;
    }
    if (((string[currentIndex] >= '0') && (string[currentIndex] <= '9')) || ((CSR_TOUPPER(string[currentIndex]) >= 'A') && (CSR_TOUPPER(string[currentIndex]) <= 'F')))
    {
        while (((string[currentIndex] >= '0') && (string[currentIndex] <= '9')) || ((CSR_TOUPPER(string[currentIndex]) >= 'A') && (CSR_TOUPPER(string[currentIndex]) <= 'F')))
        {
            *returnValue = (CsrUint16) (*returnValue * 16 + (((string[currentIndex] >= '0') && (string[currentIndex] <= '9')) ? string[currentIndex] - '0' : CSR_TOUPPER(string[currentIndex]) - 'A' + 10));
            currentIndex++;
            if (currentIndex >= 4)
            {
                break;
            }
        }
        return TRUE;
    }
    return FALSE;
}

CsrBool CsrHexStrToUint32(const CsrCharString *string, CsrUint32 *returnValue)
{
    CsrUint16 currentIndex = 0;
    *returnValue = 0;
    if ((string[currentIndex] == '0') && (CSR_TOUPPER(string[currentIndex + 1]) == 'X'))
    {
        string += 2;
    }
    if (((string[currentIndex] >= '0') && (string[currentIndex] <= '9')) || ((CSR_TOUPPER(string[currentIndex]) >= 'A') && (CSR_TOUPPER(string[currentIndex]) <= 'F')))
    {
        while (((string[currentIndex] >= '0') && (string[currentIndex] <= '9')) || ((CSR_TOUPPER(string[currentIndex]) >= 'A') && (CSR_TOUPPER(string[currentIndex]) <= 'F')))
        {
            *returnValue = *returnValue * 16 + (((string[currentIndex] >= '0') && (string[currentIndex] <= '9')) ? string[currentIndex] - '0' : CSR_TOUPPER(string[currentIndex]) - 'A' + 10);
            currentIndex++;
            if (currentIndex >= 8)
            {
                break;
            }
        }
        return TRUE;
    }
    return FALSE;
}

CsrUint32 CsrPow(CsrUint32 base, CsrUint32 exponent)
{
    if (exponent == 0)
    {
        return 1;
    }
    else
    {
        CsrUint32 i, t = base;

        for (i = 1; i < exponent; i++)
        {
            t = t * base;
        }
        return t;
    }
}

/* Convert signed 32 bit (or less) integer to string */
void CsrIntToBase10(CsrInt32 number, CsrCharString *str)
{
    CsrInt32 digit;
    CsrUint8 index;
    CsrCharString res[I2B10_MAX];
    CsrBool foundDigit = FALSE;

    for (digit = 0; digit < I2B10_MAX; digit++)
    {
        res[digit] = '\0';
    }

    /* Catch sign - and deal with positive numbers only afterwards */
    index = 0;
    if (number < 0)
    {
        res[index++] = '-';
        number = -1 * number;
    }

    digit = 1000000000;
    if (number > 0)
    {
        while ((index < I2B10_MAX - 1) && (digit > 0))
        {
            /* If the foundDigit flag is TRUE, this routine should be proceeded.
            Otherwise the number which has '0' digit cannot be converted correctly */
            if (((number / digit) > 0) || foundDigit)
            {
                foundDigit = TRUE; /* set foundDigit flag to TRUE*/
                res[index++] = (char) ('0' + (number / digit));
                number = number % digit;
            }

            digit = digit / 10;
        }
    }
    else
    {
        res[index] = (char) '0';
    }

    CsrStrLCpy(str, res, I2B10_MAX);
}

void CsrUInt16ToHex(CsrUint16 number, CsrCharString *str)
{
    CsrUint16 index;
    CsrUint16 currentValue;

    for (index = 0; index < 4; index++)
    {
        currentValue = (CsrUint16) (number & 0x000F);
        number >>= 4;
        str[3 - index] = (char) (currentValue > 9 ? currentValue + 55 : currentValue + '0');
    }
    str[4] = '\0';
}

void CsrUInt32ToHex(CsrUint32 number, CsrCharString *str)
{
    CsrUint16 index;
    CsrUint32 currentValue;

    for (index = 0; index < 8; index++)
    {
        currentValue = (CsrUint32) (number & 0x0000000F);
        number >>= 4;
        str[7 - index] = (char) (currentValue > 9 ? currentValue + 55 : currentValue + '0');
    }
    str[8] = '\0';
}

/*------------------------------------------------------------------*/
/*  String */
/*------------------------------------------------------------------*/
#ifndef CSR_USE_STDC_LIB
#include <string.h>
void *CsrMemCpy(void *dest, const void *src, CsrSize count)
{
    return memcpy(dest, src, count);
}

void *CsrMemSet(void *dest, CsrUint8 c, CsrSize count)
{
    return memset(dest, c, count);
}

void *CsrMemMove(void *dest, const void *src, CsrSize count)
{
    return memmove(dest, src, count);
}

CsrInt32 CsrMemCmp(const void *buf1, const void *buf2, CsrSize count)
{
    return memcmp(buf1, buf2, count);
}

#endif

#ifndef CSR_USE_STDC_LIB
CsrCharString *CsrStrCpy(CsrCharString *dest, const CsrCharString *src)
{
    return strcpy(dest, src);
}

CsrCharString *CsrStrNCpy(CsrCharString *dest, const CsrCharString *src, CsrSize count)
{
    return strncpy(dest, src, count);
}

CsrCharString *CsrStrCat(CsrCharString *dest, const CsrCharString *src)
{
    return strcat(dest, src);
}

CsrCharString *CsrStrNCat(CsrCharString *dest, const CsrCharString *src, CsrSize count)
{
    return strncat(dest, src, count);
}

CsrCharString *CsrStrStr(const CsrCharString *string1, const CsrCharString *string2)
{
    return strstr(string1, string2);
}

CsrSize CsrStrLen(const CsrCharString *string)
{
    return strlen(string);
}

CsrInt32 CsrStrCmp(const CsrCharString *string1, const CsrCharString *string2)
{
    return strcmp(string1, string2);
}

CsrInt32 CsrStrNCmp(const CsrCharString *string1, const CsrCharString *string2, CsrSize count)
{
    return strncmp(string1, string2, count);
}

CsrCharString *CsrStrChr(const CsrCharString *string, CsrCharString c)
{
    return strchr(string, c);
}

#endif

CsrSize CsrStrLCpy(CsrCharString *dest, const CsrCharString *src, CsrSize size)
{
    CsrSize retLen = 0;

    if ((dest) && (src) && (size > 0))
    {
        CsrSize srcLen, availableLen;

        srcLen = CsrStrLen(src);
        availableLen = CSRMIN(srcLen, size - 1);

        if (availableLen)
        {
            CsrMemCpy(dest, src, availableLen);
        }
        dest[availableLen] = '\0';

        retLen = srcLen;
    }
    return retLen;
}

CsrSize CsrStrLCat(CsrCharString *dest, const CsrCharString *src, CsrSize size)
{
    CsrSize retLen = 0;

    if ((dest) && (src) && (size > 0))
    {
        CsrSize srcLen, destLen, availableLen;

        srcLen = CsrStrLen(src);
        destLen = CsrStrLen(dest);

        if (destLen + 1 < size)
        {
            availableLen = CSRMIN(srcLen, (size - destLen - 1));

            if (availableLen)
            {
                CsrMemCpy((dest + destLen), src, availableLen);
            }
            dest[destLen + availableLen] = '\0';
        }

        retLen = srcLen + destLen;
    }
    return retLen;
}

CsrInt32 CsrVsnprintf(CsrCharString *string, CsrSize count, const CsrCharString *format, va_list args)
{
    return vsnprintf(string, count, format, args);
}

CsrCharString *CsrStringToken(CsrCharString *strToken,
                              const CsrCharString *delim,
                              CsrCharString **context)
{
    if (delim && context)
    {
        CsrSize strIndex, offset, delimCount, strLen;
        CsrCharString *str;

        if (strToken)
        { /* New string */
            str = strToken;
        }
        else if (*context)
        { /* Use old context */
            str = *context;
        }
        else
        { /* Old context is not valid */
            return NULL;
        }

        strLen = CsrStrLen(str);
        delimCount = CsrStrLen(delim);

        /* Parse through the string for delimiters */
        for (strIndex = 0, offset = 0; strIndex < strLen; strIndex++)
        {
            CsrUint8 delimIndex;
            CsrBool delimFound = FALSE;

            /* Check if we stumbled upon any delimiter */
            for (delimIndex = 0; delimIndex < delimCount; delimIndex++)
            {
                if (str[strIndex] == delim[delimIndex])
                {
                    delimFound = TRUE;
                    break;
                }
            }

            if (delimFound)
            {
                CsrSize resultLen = strIndex - offset;

                if (resultLen > 0)
                {
                    *context = &str[strIndex + 1];      /* Update context */

                    str[strIndex] = '\0';               /* Null terminate the token */

                    return &str[offset];
                }
                else
                { /* Back to back delimiters, skip to next fragment */
                    offset = strIndex + 1;
                }
            }
        }

        *context = NULL;
    }

    return NULL;
}

CsrCharString *CsrStrNCpyZero(CsrCharString       *dest,
                              const CsrCharString *src,
                              CsrSize              count)
{
    CsrStrLCpy(dest, src, count);
    return dest;
}

/* Convert string with base 10 to integer */
CsrUint32 CsrStrToInt(const CsrCharString *str)
{
    CsrInt16 i;
    CsrUint32 res;
    CsrUint32 digit;

    res = 0;
    digit = 1;

    /* Start from the string end */
    for (i = (CsrUint16) (CsrStrLen(str) - 1); i >= 0; i--)
    {
        /* Only convert numbers */
        if ((str[i] >= '0') && (str[i] <= '9'))
        {
            res += digit * (str[i] - '0');
            digit = digit * 10;
        }
    }

    return res;
}

int CsrStrNICmp(const CsrCharString *string1,
                const CsrCharString *string2,
                CsrSize              count)
{
    CsrUint32 index;
    int returnValue = 0;

    for (index = 0; index < count; index++)
    {
        if (CSR_TOUPPER(string1[index]) != CSR_TOUPPER(string2[index]))
        {
            if (CSR_TOUPPER(string1[index]) > CSR_TOUPPER(string2[index]))
            {
                returnValue = 1;
            }
            else
            {
                returnValue = -1;
            }
            break;
        }
        if (string1[index] == '\0')
        {
            break;
        }
    }
    return returnValue;
}

const CsrCharString *CsrGetBaseName(const CsrCharString *file)
{
    const CsrCharString *pch;
    static const CsrCharString dotDir[] = ".";

    if (!file)
    {
        return NULL;
    }

    if (file[0] == '\0')
    {
        return dotDir;
    }

    pch = file + CsrStrLen(file) - 1;

    while (*pch != '\\' && *pch != '/' && *pch != ':')
    {
        if (pch == file)
        {
            return pch;
        }
        --pch;
    }

    return ++pch;
}

/*------------------------------------------------------------------*/
/* Misc */
/*------------------------------------------------------------------*/
CsrBool CsrIsSpace(CsrUint8 c)
{
    switch (c)
    {
        case '\t':
        case '\n':
        case '\f':
        case '\r':
        case ' ':
            return TRUE;
        default:
            return FALSE;
    }
}

CsrSize SynMemCpyS(void *dst, CsrSize dstSize, const void* src, CsrSize copySize)
{
    CsrSize  count = (dstSize <= copySize)? dstSize : copySize;
    memcpy(dst, src, count);
    return count;
}

CsrSize SynMemMoveS(void *dst, CsrSize dstSize, const void* src, CsrSize copySize)
{
    CsrSize  count = (dstSize <= copySize)? dstSize : copySize;
    memmove(dst, src, count);
    return count;
}
