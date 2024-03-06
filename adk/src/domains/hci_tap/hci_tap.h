/**!
\copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\defgroup   hci_tap HCI TAP
\ingroup    domains
\brief      This is the interface to HCI TAP Stream that logs the BT HCI Interface
            traffic out off the chip for debugging purpose.
            The traffic logged is
            - HCI Commands and Events
            - ACL Tx and ACL Rx

           !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
           WARNING: calling HciTapStream_ConnectToCDCDevice or
           hciTapStream_ConfigureUart functions in the code by default is a
           security risk as the HCI Interface provides the LINK Keys.

           Therefore it MUST NEVER be called in the code by DEFAULT in a product.
           !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        Example of call in pydbg
          Case 1: Redirection from HCI TAP Stream to USB CDC Virtual UART Port
            QCC5171-AA_DEV-BRD-R3-AA_LEA workspace
            ======================================
            >>>
            apps1.fw.call.HciTapStream_ConnectToCDCDevice()

          Case 2: Redirection from HCI TAP Stream to UART
            QCC5170_AA_LAB_BRD_LEA workspace
            ================================
            >>>
            uart_rate = 8192 # VM_UART_RATE_2000K
            board_uart_rts = 0x16 # PIO 22
            board_uart_cts = 0x17 # PIO 23
            board_uart_tx = 0x14 # PIO 20
            board_uart_rx = 0x15 # PIO 21
            apps1.fw.call.HciTapStream_ConnectToUart(uart_rate, board_uart_rts, board_uart_cts, board_uart_tx, board_uart_rx)
@{
**/
#ifndef HCI_TAP_H_
#define HCI_TAP_H_

#include <stream.h>
#include <source.h>
#include <sink.h>


#if defined(ENABLE_HCI_TAP_STREAM)
 /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  * WARNING: These functions should only be used for testing
  * e.g. called from pydbg
  *
  * The code calling these functions MUST BE REMOVED from the code
  * when the testing is completed so the product remains SECURE.
  * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/*! \brief     Enable HCI TAP Logging: configure the HCI TAP Stream,
               UART Stream and CONNECT the HCI Tap Stream into the UART Stream.
    \return result - TRUE when successfull
*/
extern bool HciTapStream_ConnectToUart(vm_uart_rate uart_rate,
                                       uint8 board_uart_rts,
                                       uint8 board_uart_cts,
                                       uint8 board_uart_tx,
                                       uint8 board_uart_rx);

/*! \brief     Enable HCI TAP Logging: configure the HCI TAP Stream,
               configure the USB CDC Device as a virtual UART COM port Sink
               Stream and CONNECT the HCI TAP Stream as its input.
    \return    result - TRUE when successfull
*/
extern bool HciTapStream_ConnectToCDCDevice(void);

/*! \brief     Disable HCI TAP Logging:
               deconfigure the HCI TAP Stream , deconfigure its connected
               stream(UART Sink Stream or USB CDC Virtual UART Port Stream)
               and free all its resources.
*/
extern void HciTapStream_Disconnect(void);
#endif /* ENABLE_HCI_TAP_STREAM */

/**! @} **/

#endif
