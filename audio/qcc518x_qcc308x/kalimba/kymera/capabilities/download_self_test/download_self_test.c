/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  download_self_test.c
 * \ingroup  capabilities
 *
 *  Basic download self test capability
 *
 */
/****************************************************************************
Include Files
*/
#include "download_self_test_private.h"
#include "capabilities.h"
#include <string.h>
/****************************************************************************
Private Function Definitions
*/
/* Message handlers */
static bool download_self_test_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/* Op msg handlers */
static bool download_self_test_opmsg_start_self_test(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

/* Data processing function */
static void download_self_test_process_data_c(OPERATOR_DATA *, TOUCHED_TERMINALS*);
/*****************************************************************************
Private Constant Declarations
*/
/** The download self-test capability function handler table */
const handler_lookup_struct download_self_test_handler_table =
{
    download_self_test_create,  /* OPCMD_CREATE */
    base_op_destroy,            /* OPCMD_DESTROY */
    base_op_start,              /* OPCMD_START */
    base_op_stop,               /* OPCMD_STOP */
    NULL,                       /* OPCMD_RESET */
    NULL,                       /* OPCMD_CONNECT */
    NULL,                       /* OPCMD_DISCONNECT */
    NULL,                       /* OPCMD_BUFFER_DETAILS */
    NULL,                       /* OPCMD_DATA_FORMAT */
    NULL                        /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table */
const opmsg_handler_lookup_table_entry download_self_test_opmsg_handler_table[] =
    {{OPMSG_DOWNLOAD_SELF_TEST_ID_START_SELF_TEST,     download_self_test_opmsg_start_self_test},
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION,           base_op_opmsg_get_capability_version},
    {0, NULL}};

const CAPABILITY_DATA download_self_test_cap_data =
{
    CAP_ID_DOWNLOAD_SELF_TEST,             /* Capability ID */
    0, 1,                                  /* Version information - hi and lo parts */
    1, 1,                                  /* Max number of sinks/inputs and sources/outputs */
    &download_self_test_handler_table,     /* Pointer to message handler function table */
    download_self_test_opmsg_handler_table,/* Pointer to operator message handler function table */
    download_self_test_process_data_c,     /* Pointer to data processing function */
    0,                                     /* Reserved */
    sizeof(DOWNLOAD_SELF_TEST_OP_DATA)     /* Size of capability-specific per-instance data */
};
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_SELF_TEST, DOWNLOAD_SELF_TEST_OP_DATA)

/****************************************************************************
Private Function Definitions
*/
static inline DOWNLOAD_SELF_TEST_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (DOWNLOAD_SELF_TEST_OP_DATA *) base_op_get_instance_data(op_data);
}

static void test_debug_logging_pm(unsigned loop_count)
{
    int i=0;
    AUDIO_LOG_STRING(string1, "Hello");
    AUDIO_LOG_STRING(string2, "Hello there");

    /* Test debug logging*/
    while(i < loop_count)
    {
        L0_DBG_MSG( "debug log test: Test normal logging. \n");
        L0_DBG_MSG1("debug log test: i = %d \n", i);
        L0_DBG_MSG2("debug log test: %d %d \n", 1, 2);
        L0_DBG_MSG3("debug log test: %d %d %d \n", 1, 2, 3);
        L0_DBG_MSG4("debug log test: %d %d %d %d \n", 1, 2, 3, 4);
        L0_DBG_MSG5("debug log test: %d %d %d %d %d \n", 1, 2, 3, 4, 5);

        L0_DBG_MSG( "debug log test: Test log level. Log 4 should be missing\n");
        L4_DBG_MSG1("debug log test: %d \n", 1);

        L0_DBG_MSG( "debug log test: Test string prints! \n");
        L0_DBG_MSG1("debug log test: %s \n", string1);
        L0_DBG_MSG2("debug log test: %s %d \n", string1, 1);
        L0_DBG_MSG2("debug log test: %s %s \n", string1, string2);
        L0_DBG_MSG4("debug log test: %d %s %s %d \n", 0, string1, string2, 1);

        L0_DBG_MSG( "debug log test: From here come some errors! \n");
        L0_DBG_MSG1("debug log test: %d \n", string1);
        L0_DBG_MSG4("debug log test: %d %d \n", 1, 2, 3, 4);
        L0_DBG_MSG3("debug log test: %d %d %d %d %d \n", 1, 2, 3);
        L0_DBG_MSG( "debug log test: End of the errors! \n");

        i++;
    }
}

#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
#pragma codesection DM_AS_PM
static void test_debug_logging_dm_as_pm(void)
{
    int i=0;
    AUDIO_LOG_STRING(string1, "Hello");
    AUDIO_LOG_STRING(string2, "Hello there");
    /* Test calls from DM as PM to PM. */
    test_debug_logging_pm(10);
    /* Test calls from DM as PM to ROM. */
    while(i < 100)
    {
        L0_DBG_MSG( "DM as PM debug log test: Test normal logging. \n");
        L0_DBG_MSG1("DM as PM debug log test: i = %d \n", i);
        L0_DBG_MSG2("DM as PM debug log test: %d %d \n", 1, 2);
        L0_DBG_MSG3("DM as PM debug log test: %d %d %d \n", 1, 2, 3);
        L0_DBG_MSG4("DM as PM debug log test: %d %d %d %d \n", 1, 2, 3, 4);
        L0_DBG_MSG5("DM as PM debug log test: %d %d %d %d %d \n", 1, 2, 3, 4, 5);

        L0_DBG_MSG( "DM as PM debug log test: Test string prints! \n");
        L0_DBG_MSG1("DM as PM debug log test: %s \n", string1);
        L0_DBG_MSG2("DM as PM debug log test: %s %d \n", string1, 1);
        L0_DBG_MSG2("DM as PM debug log test: %s %s \n", string1, string2);
        L0_DBG_MSG4("DM as PM debug log test: %d %s %s %d \n", 0, string1, string2, 1);

        i++;
    }
}
#endif /* CHIP_HAS_NVRAM_ACCESS_TO_DM */

/* ********************************** API functions ************************************* */

static bool download_self_test_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    DOWNLOAD_SELF_TEST_OP_DATA *self_test_data = get_instance_data(op_data);

    test_debug_logging_pm(1000);
#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
    test_debug_logging_dm_as_pm();
#endif /* CHIP_HAS_NVRAM_ACCESS_TO_DM */

    /* call base_op create, which also allocates and fills response message */
    if (!base_op_create_lite(op_data, response_data))
    {
        return FALSE;
    }
    self_test_data->ip_format = AUDIO_DATA_FORMAT_FIXP;
    self_test_data->op_format = AUDIO_DATA_FORMAT_FIXP;
    return TRUE;
}

/* ************************************* Data processing-related functions and wrappers **********************************/

static void download_self_test_process_data_c(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    /* Does nothing, it's not expected to be called as we don't run as a normal operator */
    return ;
}
#if !defined(USE_DOWNLOAD_SELF_TEST_LIB) && !defined(USE_DOWNLOAD_SELF_TEST_LIB2)
int return_basebinding_test(void);

volatile unsigned char firstByte = 0xC;
volatile unsigned firstWord = 0x87654321UL;
volatile uint16 firstHalf = 0xABCD;
volatile unsigned char secondByte = 0x2A;
volatile unsigned char thirdByte = 0x2B;
volatile unsigned char fourthByte = 0x2C;
volatile unsigned char fifthByte = 0x2D;
volatile unsigned char ByteArray[3] = {0x2E, 0x2F, 0x30};
volatile unsigned secondWord = 0x63;
volatile unsigned char sixthByte = 0x31;
volatile unsigned secondHalf[3] = {0xBEEF,0xDEAD, 0xABCD};
volatile uint16 secondHalf1 = 0xBEEF;

int Matrix_x[] = {0x60};
int Vector_y[] = {0xBA0D};
int computed_output[1] = {0x0BAD};
int my_global_test_var;

int return_basebinding_test(void)
{
    return my_global_test_var;
}

static int MatrixVectorMultiplyByteByShort(int* x, int xrows, int xcols, const int* y, int yrows, int ycols, int* output)
{
    output[0] = x[0]<<8;
    output[0] |= (y[0] & 0xFF);
    return output[0];
}

static unsigned int download_self_test_variable_size_vars_test_c(void)
{
    if (firstByte != 0xC)
    {
        return 0xBAD;
    }
    if (firstWord != 0x87654321UL)
    {
        return 0xBAD;
    }
    if (firstHalf != 0xABCD)
    {
        return 0xBAD;
    }
    if (secondByte != 0x2A)
    {
        return 0xBAD;
    }
    if (thirdByte != 0x2B)
    {
        return 0xBAD;
    }
    if (fourthByte != 0x2C)
    {
        return 0xBAD;
    }
    if (fifthByte != 0x2D)
    {
        return 0xBAD;
    }
    if (ByteArray[2] != 0x30)
    {
        return 0xBAD;
    }
    if (secondWord != 0x63)
    {
        return 0xBAD;
    }
    if (sixthByte != 0x31)
    {
        return 0xBAD;
    }
    if (secondHalf[1] != 0xDEAD)
    {
        return 0xBAD;
    }
    if (secondHalf1 != 0xBEEF)
    {
        return 0xBAD;
    }
    return 0x600D;
}

#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
#pragma codesection DM_AS_PM
static int MatrixVectorMultiplyByteByShort_dm_as_pm(int* x, int xrows, int xcols, const int* y, int yrows, int ycols, int* output)
{
    output[0] = x[0]<<8;
    output[0] |= (y[0] & 0xFF);
    return output[0];
}
#pragma codesection DM_AS_PM
static unsigned int download_self_test_variable_size_vars_test_c_dm_as_pm(void)
{
    if (firstByte != 0xC)
    {
        return 0xBAD;
    }
    if (firstWord != 0x87654321UL)
    {
        return 0xBAD;
    }
    if (firstHalf != 0xABCD)
    {
        return 0xBAD;
    }
    if (secondByte != 0x2A)
    {
        return 0xBAD;
    }
    if (thirdByte != 0x2B)
    {
        return 0xBAD;
    }
    if (fourthByte != 0x2C)
    {
        return 0xBAD;
    }
    if (fifthByte != 0x2D)
    {
        return 0xBAD;
    }
    if (ByteArray[2] != 0x30)
    {
        return 0xBAD;
    }
    if (secondWord != 0x63)
    {
        return 0xBAD;
    }
    if (sixthByte != 0x31)
    {
        return 0xBAD;
    }
    if (secondHalf[1] != 0xDEAD)
    {
        return 0xBAD;
    }
    if (secondHalf1 != 0xBEEF)
    {
        return 0xBAD;
    }
    return 0x600D;
}
#endif /* CHIP_HAS_NVRAM_ACCESS_TO_DM */

static int32 dummy_static = 0x43000000;
static int32* dummy_static_pointer = &dummy_static;

static unsigned int download_self_test_symbol_not_found_in_varLut(void)
{
    if (dummy_static_pointer == NULL)
    {
        return 0xBAD;
    }
    return 0x600D;
}

#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
#pragma codesection DM_AS_PM
static unsigned int download_self_test_symbol_not_found_in_varLut_dm_as_pm(void)
{
    if (dummy_static_pointer == NULL)
    {
        return 0xBAD;
    }
    return 0x600D;
}
#endif /* CHIP_HAS_NVRAM_ACCESS_TO_DM */

static unsigned int download_self_test_variables_arrangement(void)
{
    static char dict_1[] = "sil";
    static char dict_2[] = "speech";
    static char dict_3[] = "ni";
    static char dict_4[] = "hao";
    static char dict_5[] = "xiao";
    static char dict_6[] = "le";
    int sil = strlen(dict_1);
    int speech = strlen(dict_2);
    int ni = strlen(dict_3);
    int hao = strlen(dict_4);
    int xiao = strlen(dict_5);
    int le = strlen(dict_6);
    if ((sil == 3) && (speech == 6) && (ni == 2) && (hao == 3) && (xiao == 4) && (le == 2))
    {
        if ((strcmp(dict_1,"sil") == 0) && (strcmp(dict_2,"speech")==0) &&
                (strcmp(dict_3,"ni") == 0) && (strcmp(dict_4,"hao") == 0) &&
                (strcmp(dict_5,"xiao") == 0) && (strcmp(dict_6,"le") == 0))
        {
            return 0x600D;
        }
    }
    return 0xBAD;
}
#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
#pragma codesection DM_AS_PM
static unsigned int download_self_test_variables_arrangement_dm_as_pm(void)
{
    static char dict_1[] = "sil";
    static char dict_2[] = "speech";
    static char dict_3[] = "ni";
    static char dict_4[] = "hao";
    static char dict_5[] = "xiao";
    static char dict_6[] = "le";
    int sil = strlen(dict_1);
    int speech = strlen(dict_2);
    int ni = strlen(dict_3);
    int hao = strlen(dict_4);
    int xiao = strlen(dict_5);
    int le = strlen(dict_6);
    if ((sil == 3) && (speech == 6) && (ni == 2) && (hao == 3) && (xiao == 4) && (le == 2))
    {
        if ((strcmp(dict_1,"sil") == 0) && (strcmp(dict_2,"speech")==0) &&
                (strcmp(dict_3,"ni") == 0) && (strcmp(dict_4,"hao") == 0) &&
                (strcmp(dict_5,"xiao") == 0) && (strcmp(dict_6,"le") == 0))
        {
            return 0x600D;
        }
    }
    return 0xBAD;
}

#pragma codesection DM_AS_PM
static bool tests_for_dm_as_pm(void)
{
    unsigned int test_result = 0xBAD;
    unsigned int (*func_ptr_dm_as_pm[])(void) = {&download_self_test_branch_test_dm_as_pm, &download_self_test_var_ref_test_minim_dm_as_pm, &download_self_test_var_ref_test_maxim_dm_as_pm};
    unsigned int i;

    test_result = download_self_test_call_from_maxim_to_minim_test_maxim();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_call_from_maxim_to_minim_test_maxim_dm_as_pm();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_call_from_maxim_to_maxim_test_maxim_dm_as_pm();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_call_from_maxim_to_maxim_test_maxim();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_call_from_minim_to_maxim_test_minim();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_call_from_minim_to_maxim_test_minim_dm_as_pm();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_call_from_minim_to_minim_test_minim_dm_as_pm();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    /* Check multiple variable sizes referencing */
    test_result = download_self_test_variable_size_vars_test_maxim_asm_dm_as_pm();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_variable_size_vars_test_minim_asm_dm_as_pm();
    if (test_result != 0x600D)
    {
        return FALSE;
    }

    /* Call assembly functions. */
    for (i = 0; i < ARRAY_SIZE(func_ptr_dm_as_pm); i++)
    {
        test_result = func_ptr_dm_as_pm[i]();
        if (test_result != 0x600D)
        {
            return FALSE;
        }
    }

    test_result = download_self_test_variable_size_vars_test_c_dm_as_pm();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_symbol_not_found_in_varLut_dm_as_pm();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_array_offset_access_dm_as_pm();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_array_offset_access_check_dm_as_pm();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_variables_arrangement_dm_as_pm();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = MatrixVectorMultiplyByteByShort_dm_as_pm(Matrix_x, 3, 400, Vector_y, 400, 3, computed_output);
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    return TRUE;
}
#endif /* CHIP_HAS_NVRAM_ACCESS_TO_DM */

static int unitnobasebinding_test(void)
{
    if (my_global_test_var == 0 || my_global_test_var == 0xABCD)
    {
        my_global_test_var = 0x600D;
    }

    return my_global_test_var;
}

/* **************************** Operator message handlers ******************************** */

/* Duplicated in download_self_test_util.c */
static unsigned int my_static_array[] = {0, 1, 2};

static bool download_self_test_opmsg_start_self_test(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    unsigned int test_result = 0xBAD;
    /* Compiler will convert func_ptr into an anonymous symbol L_lcx, and pull in C-runtime __CRT_wcpy function
     * Build system must deal with this
     * */
    unsigned int (*func_ptr[])(void) = {&download_self_test_branch_test, &download_self_test_var_ref_test_minim, &download_self_test_var_ref_test_maxim};
    unsigned int* my_local_array[] = {&my_static_array[0], &my_static_array[1], &my_static_array[2]};
    unsigned int i;
    L4_DBG_MSG("download_self_test_opmsg_start_self_test  \n");
    set_a(1);
    /* Set up the a default success response information */
    /* length is 1 word, msgID */
    *resp_length = OPMSG_RSP_PAYLOAD_SIZE_RAW_DATA(0);
    *resp_data = (OP_OPMSG_RSP_PAYLOAD *)xpnewn(*resp_length, unsigned);
    if (*resp_data == NULL)
    {
        return FALSE;
    }

    /* Test that global variables are not grouped. */
    test_result = unitnobasebinding_test();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    /* Check calls between Minim/Maxim code sections */
    test_result = download_self_test_call_from_maxim_to_minim_test_maxim();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    /* Check multiple variable sizes referencing */
    test_result = download_self_test_variable_size_vars_test_maxim_asm();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_variable_size_vars_test_minim_asm();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_variable_size_vars_test_c();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_symbol_not_found_in_varLut();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_array_offset_access();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_array_offset_access_check();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_AddSub_B_instruction(0, -1);
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_call_from_maxim_to_insert32_instr_test_maxim();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_call_from_minim_to_insert32_instr_test_minim();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_variables_arrangement();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = download_self_test_extra_memory();
    if (test_result != 0x600D)
    {
        return FALSE;
    }
    test_result = MatrixVectorMultiplyByteByShort(Matrix_x, 3, 400, Vector_y, 400, 3, computed_output);
    if (test_result != 0x600D)
    {
        return FALSE;
    }

#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
    if(!tests_for_dm_as_pm())
    {
        return FALSE;
    }
#endif /* CHIP_HAS_NVRAM_ACCESS_TO_DM */

    /* Call assembly functions and change the already allocated response to the result of the test */
    for (i = 0; i < ARRAY_SIZE(func_ptr); i++)
    {
        test_result = func_ptr[i]();
        if (test_result != 0x600D)
        {
            /* Return BAD (0xBAD) */
            (*resp_data)->msg_id = test_result;
            return TRUE;
        }
    }
    /* This loop forces the creation of anonymous initialisation tables L_lcx */
    for (i = 0; i < ARRAY_SIZE(my_static_array); i++)
    {
        my_static_array[i] = *my_local_array[i];
    }

    /* Force linking of another C module for this capability */
    my_static_array[0] = download_self_test_util_helper();
    /* return GOOD (0x600D) or BAD (0xBAD)*/
    (*resp_data)->msg_id = my_static_array[0];
    return TRUE;
}
#else /* USE_DOWNLOAD_SELF_TEST_LIB || USE_DOWNLOAD_SELF_TEST_LIB2 */
static bool download_self_test_opmsg_start_self_test(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* Set up the a default success response information */
    /* length is 1 word, msgID */
    *resp_length = OPMSG_RSP_PAYLOAD_SIZE_RAW_DATA(0);
	*resp_data = (OP_OPMSG_RSP_PAYLOAD *)xpnewn(*resp_length, unsigned);
    if (*resp_data == NULL)
    {
        return FALSE;
    }

    (*resp_data)->msg_id = 0xBAD;

#if defined(USE_DOWNLOAD_SELF_TEST_LIB)
    if (!download_self_test_lib_start())
    {
        return FALSE;
    }
    if (dm2_v5 == 0)
    {
        return FALSE;
    }
#endif

#if defined(USE_DOWNLOAD_SELF_TEST_LIB2)
    if (!download_self_test_lib2_start())
    {
        return FALSE;
    }
#endif
    (*resp_data)->msg_id = 0x600D;
    return TRUE;
}
#endif /* !USE_DOWNLOAD_SELF_TEST_LIB && !USE_DOWNLOAD_SELF_TEST_LIB2 */

