#ifndef CSR_BT_SMLCOMMONTESTDEF_H__
#define CSR_BT_SMLCOMMONTESTDEF_H__
/******************************************************************************
 Copyright (c) 2008-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"

#ifdef __cplusplus
extern "C" {
#endif

/*BEGIN Defines for APPL_SYNCML_SERVER_SIDE*/
#define APP_SMLS_REGISTER_STATE                          0
#define APP_SMLS_ACTIVE_STATE                            1
#define APP_SMLS_CONNECT_STATE                           2
#define APP_SMLS_CONNECTFAILRESP_STATE                   3
#define APP_SMLS_CONNECTED_STATE                         4
#define APP_SMLS_DISCONNECT_STATE                        5
#define APP_SMLS_PUT_SMLSMSG_OBJ_STATE                   6
#define APP_SMLS_PUT_NEXTCHUNK_SMLMSG_OBJ_STATE          7
#define APP_SMLS_PUT_SMLSMSG_OBJ_FAILRESP_STATE          8
#define APP_SMLS_PUT_NEXTCHUNK_SMLMSG_OBJ_FAILRESP_STATE 9
#define APP_SMLS_GET_OBJ_STATE                           10
#define APP_SMLS_GET_OBJ_NEXT_STATE                      11
#define APP_SMLS_GET_OBJ_FAILRESP_STATE                  12
#define APP_SMLS_GET_OBJ_FAILRESP_NEXT_STATE             13
#define APP_SMLS_DEACTIVATE_STATE                        14
#define APP_SMLS_TEST_CASE_CLOSE_DOWN_STATE              15

#define APP_SMLS_RXDATA_MAX_PUT_PACKET_COUNT             10
#define APP_SMLS_TXDATA_MAX_GET_PACKET_COUNT             10

/*used in smls_put-session./ smls_get_session-splitatobex..*/
/*taken from the smls get sessions earlier made..*/
#define A_SMLS_PASSWD_SERVER    "8765"
#define A_SMLS_PASSWD_SERVER_LENGTH    4
#define A_SMLS_PASSWD_CLIENT    "8765"
#define A_SMLS_PASSWD_CLIENT_LENGTH    4
#define B_SMLS_PASSWD_CLIENT    "1234"
#define B_SMLS_PASSWD_CLIENT_LENGTH    4
#define SMLS_CLIENT_USER        "smlc_user\0"
#define B_SMLS_PASSWD_SERVER    "4321"
#define B_SMLS_PASSWD_SERVER_LENGTH    4




/*BEGIN OBEX_MAX_PACKET_SIZES and connected files on the syncml-server-side.....*/

#define APP_SMLS_MAX_OBEX_PACKET_SIZE_1    10000

#define APP_SMLS_MAX_OBEX_PACKET_SIZE_2    10000

#define APP_SMLS_MAX_OBEX_PACKET_SIZE_3    25000

/*END OBEX_MAX_PACKET_SIZES and connected files on the syncml-server-side.....*/


/*END defines used in help-/util-functions used in the different testcases*/


/*END Defines for APPL_SYNCML_SERVER_SIDE*/
/**************************************************************************/




/**************************************************************************/
/*BEGIN Defines for APPL_SYNCML_CLIENT_SIDE*/
/*General TimeOut Values*/
#define APP_SMLC_TESTCASE_RECONNECT_TIMEOUT      (300000)  /*equals 300 msecs*/

/*state*/
#define TC_APPL_SMLC_ACTIVE_STATE                           0
#define TC_APPL_SMLC_ACTIVE1_STATE                          1
#define TC_APPL_SMLC_SEARCH_STATE                           2
#define TC_APPL_SMLC_CONNECT_STATE                          3
#define TC_APPL_SMLC_CONNECTFAILRESP_STATE                  4
#define TC_APPL_SMLC_DISCONNECT_STATE                       5
#define TC_APPL_SMLC_TEST_CASE_CLOSE_DOWN_STATE             6
#define TC_APPL_SMLC_PUT_SMLMSG_OBJ_STATE                   7
#define TC_APPL_SMLC_PUT_NEXTCHUNK_SMLMSG_OBJ_STATE         8
#define TC_APPL_SMLC_PUT_SMLMSG_OBJ_FAILRESP_STATE          9
#define TC_APPL_SMLC_PUT_NEXTCHUNK_SMLMSG_OBJ_FAILRESP_STATE 10
#define TC_APPL_SMLC_GET_SMLMSG_OBJ_STATE                   11
#define TC_APPL_SMLC_GET_NEXTCHUNK_SMLMSG_OBJ_STATE         12
#define TC_APPL_SMLC_GET_SMLMSG_OBJ_FAILRESP_STATE          13
#define TC_APPL_SMLC_GET_NEXTCHUNK_SMLMSG_OBJ_FAILRESP_STATE 14
#define TC_APPL_SMLC_DEACTIVATE_STATE                       14
#define TC_APPL_SMLC_ABORT_STATE                            16

/*defines the maximum recv packets before the testcase is forced closed down with error!*/
/*ATT refers to a single get-session/get-resp OR put-session*/
#define TC_SMLC_MAX_NO_OF_RX_GET_PACKETS    40
#define TC_SMLC_MAX_NO_OF_TX_PUT_PACKETS    40


#define SMLS_PASSWD_SERVER         "8765"
#define SMLS_PASSWD_SERVER_LENGTH    4


#define SMLC_CLIENT_USER        "smlc_user\0"
#define SMLC_SERVER_USER        "smls_user\0"

#define SMLC_PASSWD_SERVER        "8765"
#define SMLC_PASSWD_SERVER_LENGTH    4
#define SMLC_PASSWD_SERVER_1    "4321"

#define SMLC_PASSWD_CLIENT_1    "1234"
#define SMLC_PASSWD_CLIENT_1_LENGTH    4

/*for searching tests*/
#define APPL_SYNCMLSERVER_UUID_128 {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x02, 0xEE, 0x00, 0x00, 0x02}
/*used with codetype CsrBtUuid128*/

/*BEGIN OBEX_MAX_PACKET_SIZES and connected files on the syncml-client-side.....*/
#define APPL_SMLC_MAX_OBEX_PACKET_SIZE_1    1000
#define APPL_SMLC_MAX_OBEX_PACKET_SIZE_2    1000
#define TC_SMLC_MAX_OBEX_PACKET_SIZE_3      1000
#define TC_SMLC_MAX_OBEX_PACKET_SIZE_4      CSR_BT_MAX_OBEX_SIGNAL_LENGTH
/*END OBEX_MAX_PACKET_SIZES and connected files on the syncml-client-side.....*/

#ifdef __cplusplus
}
#endif

#endif
