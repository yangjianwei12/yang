/******************************************************************************
 Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

/****************************************************************************
FILE
    aes.c

CONTAINS
    CsrBtAesEncrypt  -  encrypts input data
*/

#include "csr_synergy.h"

#include "csr_pmem.h"
#include "csr_util.h"
#include <string.h>
#include "csr_bt_aes_private.h"

#define XTIME(a) (0xff & (  ((a) & 0x80) ? (((a) << 1) ^ 0x1b) : ((a) << 1)  )  )
#define SBOX(a) (0xff & (((a) & 1) ? (aes_sbox[(a)/2] >> 8) : (aes_sbox[(a)/2])))
#define RCON(index) ((index)&1?(rcon[(index)/2]>>8):(rcon[(index)/2]&0xff))

/* This copy of reverse_array() uses CsrUint8 pointers
   instead of CsrUint16 pointers and is required because CsrUint8s
   and CsrUint16s aren't the same thing everywhere. */
void reverse_array(void * array, CsrUint16 len);
void reverse_array(void * array, CsrUint16 len)
{
    CsrUint8 *src = (CsrUint8*)array;
    CsrUint8 *dst = src + len;
    CsrUint8 tmp;

    while (len > 1)
    {
        dst--;
        tmp = *dst;
        *dst = *src;
        *src = tmp;
        src++;
        len -= 2;
    }
}

static const CsrUint16 rcon[] =
{
    0x018d, 0x0402, 0x1008, 0x4020, 0x1b80, 0x6c36
};

static const CsrUint16 aes_sbox[] = {
    0x7c63, 0x7b77, 0x6bf2, 0xc56f, 0x0130, 0x2b67, 0xd7fe, 0x76ab,
    0x82ca, 0x7dc9, 0x59fa, 0xf047, 0xd4ad, 0xafa2, 0xa49c, 0xc072,
    0xfdb7, 0x2693, 0x3f36, 0xccf7, 0xa534, 0xf1e5, 0xd871, 0x1531,
    0xc704, 0xc323, 0x9618, 0x9a05, 0x1207, 0xe280, 0x27eb, 0x75b2,
    0x8309, 0x1a2c, 0x6e1b, 0xa05a, 0x3b52, 0xb3d6, 0xe329, 0x842f,
    0xd153, 0xed00, 0xfc20, 0x5bb1, 0xcb6a, 0x39be, 0x4c4a, 0xcf58,
    0xefd0, 0xfbaa, 0x4d43, 0x8533, 0xf945, 0x7f02, 0x3c50, 0xa89f,
    0xa351, 0x8f40, 0x9d92, 0xf538, 0xb6bc, 0x21da, 0xff10, 0xd2f3,
    0x0ccd, 0xec13, 0x975f, 0x1744, 0xa7c4, 0x3d7e, 0x5d64, 0x7319,
    0x8160, 0xdc4f, 0x2a22, 0x8890, 0xee46, 0x14b8, 0x5ede, 0xdb0b,
    0x32e0, 0x0a3a, 0x0649, 0x5c24, 0xd3c2, 0x62ac, 0x9591, 0x79e4,
    0xc8e7, 0x6d37, 0xd58d, 0xa94e, 0x566c, 0xeaf4, 0x7a65, 0x08ae,
    0x78ba, 0x2e25, 0xa61c, 0xc6b4, 0xdde8, 0x1f74, 0xbd4b, 0x8a8b,
    0x3e70, 0x66b5, 0x0348, 0x0ef6, 0x3561, 0xb957, 0xc186, 0x9e1d,
    0xf8e1, 0x1198, 0xd969, 0x948e, 0x1e9b, 0xe987, 0x55ce, 0xdf28,
    0xa18c, 0x0d89, 0xe6bf, 0x6842, 0x9941, 0x0f2d, 0x54b0, 0x16bb
};

/****************************************************************************
NAME
  CsrBtAesEncrypt - encrypts input data

FUNCTION
    Encrypts input data using a 128 byte key. Input and output are
    little-endian. (The algorithm is actually big-endian, but the
    data_in and key arrays are reversed before it starts and data_out
    is reversed after it finishes.)
*/
void CsrBtAesEncrypt(const CsrUint8 *data_in, const CsrUint8* key, CsrUint8 *data_out)
{
    CsrUint16 i, j, k;
    CsrUint8 row, col ;
    CsrUint8 break_counter;
    CsrUint8 state_new[BLOCK_SIZE];
    CsrUint8 *ptr;
    CsrUint8 temp[INT_SIZE_AES];
    CsrUint8 w[EXPANDED_KEY_SIZE];

    /* Transfer key to w[] and data_in to data_out... */
    (void)SynMemCpyS(w, EXPANDED_KEY_SIZE, key, BLOCK_SIZE);
    if (data_out != data_in)
        (void)SynMemCpyS(data_out, BLOCK_SIZE, data_in, BLOCK_SIZE);

    /* ...and then convert from little-endian to big-endian */
    /*reverse_array(data_out, BLOCK_SIZE);
    reverse_array(w, BLOCK_SIZE);*/

    /* Key Expansion */
    for (i = Nk * INT_SIZE_AES; i < EXPANDED_KEY_SIZE; i+=4)
    {
        SynMemCpyS(&temp, INT_SIZE_AES, &w[i-4], INT_SIZE_AES);
        if ((i % (Nk * INT_SIZE_AES)) == 0)
        {
            /* rot_word */
            row     = temp[0];
            temp[0] = temp[1];
            temp[1] = temp[2];
            temp[2] = temp[3];
            temp[3] = row;

            /* sub_word */
            for (j = 0; j < 4; j++)
            {
                row     = temp[j] >> 4;
                col     = temp[j] & 0xf;
                temp[j] = SBOX(row*BLOCK_SIZE+col);
            }

            temp[0] ^= RCON(i/(Nk*INT_SIZE_AES));
        }

        for(j = 0; j < INT_SIZE_AES; j++)
            w[j+i] = w[j + i - Nk * INT_SIZE_AES] ^ temp[j] ;
    }

    /* Actual AES Encrypt */
    /* add round key */
    for(i=0; i < BLOCK_SIZE; i++)
    {
        data_out[i] ^= w[i];
    }

    for(i=0; i < 10; i++)          /* fixed at 10 for 128 bit cipher*/
    {
        /* subBytes */
        for(j = 0; j < BLOCK_SIZE; j++)
        {
            row = data_out[j] >> 4 ;
            col = data_out[j] & 0xf;
            data_out[j] = SBOX(row*BLOCK_SIZE + col);
        }

        /* shift rows */
        for(k = 1, break_counter = 1; k < 4; k++, break_counter++)
        {
            for(j = 0; j < break_counter; j++)
            {
                row            = data_out[k];
                data_out[k]    = data_out[k+4];
                data_out[k+4]  = data_out[k+8];
                data_out[k+8]  = data_out[k+12];
                data_out[k+12] = row;
            }
        }

        /* Mix Columns */
        if (i == 9)
        {
            /* don't do a mix columns for last round */
            SynMemCpyS(state_new, BLOCK_SIZE, data_out, BLOCK_SIZE) ;
        }
        else
        {
            /*
             * Since we only use powers of 2 & 3, we can replace the galois multiply with it's
             * equivelant expressions involving xtime. Original code:
             * state_new[j]= gm(2,state[j]) ^ gm(3,state[j+1]) ^ state[j+2] ^ state[j+3]
             * state_new[j+1]= state[j] ^ gm(2,state[j+1]) ^ gm(3,state[j+2]) ^ state[j+3]
             * state_new[j+2]= state[j] ^ state[j+1] ^ gm(2,state[j+2]) ^ gm(3,state[j+3])
             * state_new[j+3]= gm(3,state[j]) ^ state[j+1] ^ state[j+2] ^ gm(2,state[j+3])
             */

            for(j = 0; j < 16; j += 4)
            {
                state_new[j]  = XTIME(data_out[j]) ^ XTIME(data_out[j+1]) ^
                    data_out[j+1] ^ data_out[j+2] ^ data_out[j+3];

                state_new[j+1]= data_out[j]^ XTIME(data_out[j+1]) ^
                    XTIME(data_out[j+2]) ^ data_out[j+2] ^ data_out[j+3];

                state_new[j+2]= data_out[j] ^ data_out[j+1] ^ XTIME(data_out[j+2]) ^
                    XTIME(data_out[j+3]) ^ data_out[j+3];

                state_new[j+3]= XTIME(data_out[j]) ^ data_out[j] ^ data_out[j+1] ^
                    data_out[j+2] ^ XTIME(data_out[j+3]) ;
            }
        }

        /* add round key */
        ptr = &w[(i+1) * BLOCK_SIZE] ;
        for(j=0; j < 16; j ++)
        {
            /* state[j] = state_new[j] ^ w[(i+1)*BLOCK_SIZE+j] ; */
            data_out[j] = state_new[j] ^ *(ptr+j) ;
        }
    } /* next */

    /* Convert output from big-endian to little-endian */
    /*reverse_array(data_out, BLOCK_SIZE);*/
}

/****************************************************************************
NAME
  CsrBtAesCmac - generates AES based Message authentication code
                 for variable length input data

FUNCTION
    AES-CMAC uses the Advanced Encryption Standard [NIST-AES] as a
    building block.  To generate a MAC, AES-CMAC takes a secret key, a
    message of variable length, and the length of the message in octets
    as inputs and returns a fixed-bit string called a MAC(16 bytes in this case).

*/


/* Rotation function - Shift Left by one bit */

static CsrUint8* rotateByOneBit(CsrUint8* value)
{
    CsrUint8 *rotated_value = CsrPmemZalloc(BLOCK_SIZE);
    CsrUint8 residue = 0;
    CsrUint16 temp = 0, index;

    for (index = BLOCK_SIZE - 1; index >= 0 && index < BLOCK_SIZE; index--)
    {
        temp = (value[index] << 1) | residue;
        rotated_value[index] = (CsrUint8)(temp);
        residue = (CsrUint8)((temp & 0xff00) >> 8);
    }

    return rotated_value;
}

/* Get Key Function */

static CsrUint8* getKey(CsrUint8* initialKey)
{
    CsrUint8* tempKey = CsrPmemZalloc(BLOCK_SIZE);
    CsrUint8 index;
    CsrUint8 const_Rb[] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00 , 0x00, 0x87 };

    /*   if MSB(input) is equal to 0               +
     +   then    tempKey : = input << 1;             +
     +   else    tempKey : = (input << 1) XOR const_Rb;
 */

    if ((initialKey[0] & 0x80) == 0)
    {
        /* If MSB of the Output is Zero shift left by 1 bit to get Key1*/

        tempKey = rotateByOneBit(initialKey);
    }
    else
    {
        /* Other Wise rotate the Output and Xor it with Const_Rb*/

        tempKey = rotateByOneBit(initialKey);
        for (index = 0; index < BLOCK_SIZE; index++)
            tempKey[index] ^= const_Rb[index];
    }

    return tempKey;
}

static void GenerateSubkey(const CsrUint8* Key, CsrUint8* Key1, CsrUint8* Key2)
{

    /* Key Generation Algorithm */
    CsrUint8 const_zero[BLOCK_SIZE] = { 0 };
    CsrUint8 *output = CsrPmemZalloc(BLOCK_SIZE);

    /*   Step 1.  L := AES-128(K, const_Zero);    */

    CsrBtAesEncrypt(const_zero, Key, output);

    /* Generating K1*/

   /*  Step 2:     if MSB(L) is equal to 0               +
        +   then    K1 : = L << 1;             +
        +   else    K1 : = (L << 1) XOR const_Rb;
    */

    SynMemCpyS(Key1, BLOCK_SIZE, getKey(output), BLOCK_SIZE);

    /* Generating K2*/

    /* +Step 3:  if MSB(K1) is equal to 0 +
       + then    K2 : = K1 << 1;          +
       + else    K2 : = (K1 << 1) XOR const_Rb; */

    SynMemCpyS(Key2, BLOCK_SIZE, getKey(Key1), BLOCK_SIZE);

}

/* padding function */
static CsrUint8* padInputMessage(const CsrUint8* input, CsrUint16 length)
{
    /*!
       For an input string x of r-octets, where 0 <= r < 16, the padding
       function, padding(x), is defined as follows:

   -   padding(x) = x || 10^i      where i is 128-8*r-1
    !*/
    CsrUint8 index;
    CsrUint8* padded_message = CsrPmemZalloc(BLOCK_SIZE);

    for (index = 0; index < BLOCK_SIZE - 1; index++)
    {
        if (index < length)
        {
            /* Copy the input message */

            padded_message[index] = input[index];
        }
        else
        {
            /* pad remaining bytes  first byte with 0x10 the
            following bytes by 00s
            */
            padded_message[index + 1] = 0x00;

        }
    }

    /* pad the byte immediately after the input message
       with 0x10*/
    if(length < BLOCK_SIZE)
        padded_message[length] = 0x80;

    /* return the padded message*/
    return padded_message;
}



static CsrUint8 calCulateCeilingValue16(CsrUint16 dividend)
{
    /* Calculating the Ceiling value for given input
   That is divide by BLOCK_SIZE i.e 16 and add 1
   to quotient if there is residue for output
   */

    CsrUint16 temp;
    temp = dividend >> 4;

    if ((dividend & 0x000f))
        temp += 1;

    return ((CsrUint8)temp);
}

/* Block xoring and block size is always 16 */

static void block_xor(CsrUint8* first_in, CsrUint8* second_in, CsrUint8* out)
{
    CsrUint8 index;
    for (index = 0; index < BLOCK_SIZE; index++)
        out[index] = first_in[index] ^ second_in[index];
}

void CsrBtAesCmac(const CsrUint8* data_in, CsrUint16 data_length, CsrUint8* key, CsrUint8* data_out)
{
    /* Reference: RFC4493 */

    CsrUint8* sub_key1, * sub_key2;
    CsrUint8 Message[BLOCK_SIZE][BLOCK_SIZE] = {0};
    CsrUint8* message_last = CsrPmemZalloc(BLOCK_SIZE);
    CsrUint8 x[16];
    CsrUint8 y[16] = { 0 };
    CsrUint8* padded;
    CsrBool flag;
    CsrUint16 length, n, i, j;

    /* initialize memory for keys*/

    sub_key1 = CsrPmemZalloc(BLOCK_SIZE);
    sub_key2 = CsrPmemZalloc(BLOCK_SIZE);

    /* Copy Input to the Message */


    length = data_length;
    for (i = 0; i < calCulateCeilingValue16(data_length); i++)
    {
        for (j = 0; j < BLOCK_SIZE; j++)
        {
            /* break out of the loop if length
               is less than zero */
            if (length <= 0)
                break;
            if (j < length)
                Message[i][j] = data_in[(i * BLOCK_SIZE) + j];
        }
        length = length - BLOCK_SIZE;
    }


    /* Generate Subkeys */
    /*    +   Step 1.  (K1,K2) := Generate_Subkey(K);                         +*/
    GenerateSubkey(key, sub_key1, sub_key2);

    /* Step 2.  n := ceil(len/BLOCK_SIZE);*/
    n = calCulateCeilingValue16(data_length);

    /*     +   Step 3.  if n = 0                                               +
    +            then                                                   +
    +                 n := 1;                                           +
    +                 flag := false;                                    +
    +            else                                                   +
    +                 if len mod const_Bsize is 0                       +
    +                 then flag := true;                                +
    +                 else flag := false;                               +          */

    if (n == 0)
    {
        n = 1;
        flag = FALSE;
    }
    else
    {
        flag = (data_length % BLOCK_SIZE == 0);
    }

    /*  +   Step 4.  if flag is true                                        +
   +            then M_last := M_n XOR K1;                             +
   +            else M_last := padding(M_n) XOR K2;                    +   */

    if (flag)
    {
        /* if flag is true , Xor with Sub Key 1*/

         /*for (i = 0; i < BLOCK_SIZE; i++)
             message_last[i] = Message[n][i] ^ sub_key1[i];*/

        block_xor(&(Message[n-1][0]), sub_key1, message_last);

    }
    else
    {
        /* otherwise, pad the message and then XOR padded message with Sub Key 2*/

        padded = padInputMessage(&(Message[n-1][0]), (data_length % BLOCK_SIZE));

        /* for (i = 0; i < BLOCK_SIZE; i++)
             message_last[i] = padded[i] ^ sub_key2[i];*/

        block_xor(padded, sub_key2, message_last);
    }

    /*    +   Step 5.  X := const_Zero;                                + */

    for (i = 0; i < BLOCK_SIZE; i++)
        x[i] = 0;

    /*    +   Step 6.  for i := 1 to n-1 do                            +
   +                begin                                              +
   +                  Y := X XOR M_i;                                  +
   +                  X := AES-128(K,Y);                               +
   +                end                                                +
   +            Y := M_last XOR X;                                     +
   +            T := AES-128(K,Y);                                     +*/

    for (i = 0; i < n - 1; i++)
    {
        /* perform MAC'ng of 1...n-1 blocks of message*/

    /*    for (j = 0; j < BLOCK_SIZE; j++)
        {
            y[j] = x[j] ^ Message[i][j];
        }*/

        block_xor(x, &(Message[i][0]), y);
        CsrBtAesEncrypt(y, key, x);
    }

    /* MAC the last padded block
           Y := M_last XOR X;  */

    block_xor(x, message_last, y);

    /* T := AES-128(K,Y); */
    CsrBtAesEncrypt(y, key, data_out);
}
