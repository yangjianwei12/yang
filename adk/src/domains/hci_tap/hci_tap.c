/*!
\copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\ingroup    hci_tap
\brief      This is the interface to HCI TAP Stream that logs the BT HCI Interface
            traffic out off the chip for debugging purpose.

           !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
           WARNING: calling HciTapStream_ConnectToCDCDevice or
           hciTapStream_ConfigureUart functions in the code by default is a
           security risk as the HCI Interface provides the LINK Keys.

           Therefore it MUST NEVER be called in the code by DEFAULT in a
           final product.
           !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        Example of call in pydbg
           QCC5171-AA_DEV-BRD-R3-AA_LEA workspace
           ======================================
          Case 1: Redirection from HCI TAP Stream to USB CDC Virtual UART Port
            With INCLUDE_USB_DEVICE
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

            QCC5171_AA_DEV_BRD_R3_AA_LEA
            ============================
            Unfortunatly PIO 22 and PIO 23 are not available for cts rts on
            SODIMM 20-18570-4.
            Because any PIO function can be multiplexed to any PIO in 517x data sheet
            it possible to multiplex functions that are not used for a particular
            application, for instance SPI/I²C, SPDIF, and digital microphone.
            CTS and RTS are mapped to pin 16 and pio 17

            >>>
            uart_rate = 8192 # VM_UART_RATE_2000K
            board_uart_rts = 0x10 # PIO 16
            board_uart_cts = 0x11 # PIO 17
            board_uart_tx = 0x14 # PIO 20
            board_uart_rx = 0x15 # PIO 21
            apps1.fw.call.HciTapStream_ConnectToUart(uart_rate, board_uart_rts, board_uart_cts, board_uart_tx, board_uart_rx)
@{
*/
#include <hci_tap.h>
#include <logging.h>
#include <panic.h>
#include <stream.h>

#include <usb_app_cdc.h>
#include <usb_cdc.h>

#include <source.h>
#include <sink.h>

#if defined(ENABLE_HCI_TAP_STREAM)
#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#define HCI_TAPI_STR "HCI-TAP:I: "
#define HCI_TAPW_STR "HCI-TAP:W: "
typedef enum {
    HCI_TAP_DISCONNECTED,
    HCI_TAP_CONNECTED_TO_USB,
    HCI_TAP_CONNECTED_TO_UART
} HciTapState;

typedef struct HciTapPrivateTag
{
    Source src;
    Sink sink;
    HciTapState state;
} HciTapPrivate;

static HciTapPrivate hciTapPrivateData;

#define NB_BITS_IN_MASK ((uint8)32)
static bool hciTapStream_ConfigureUart(vm_uart_rate uart_rate,
    uint8 board_uart_rts,
    uint8 board_uart_cts,
    uint8 board_uart_tx,
    uint8 board_uart_rx)
{
    bool result = FALSE;
    if (board_uart_rts < NB_BITS_IN_MASK &&
        board_uart_cts < NB_BITS_IN_MASK &&
        board_uart_tx < NB_BITS_IN_MASK &&
        board_uart_rx < NB_BITS_IN_MASK)
    {
        /* Assign the appropriate PIOs to be used by UART. */
        uint32 bank = 0;
        uint32 mask = (1<<board_uart_rts) | (1<<board_uart_cts) | (1<<board_uart_tx) | (1<<board_uart_rx);
        DEBUG_LOG_INFO(HCI_TAPI_STR"Pins RTS:%d CTS:%d TX:%d RX:%d" ,
            board_uart_rts, board_uart_cts, board_uart_tx, board_uart_rx);
        PioSetMapPins32Bank(bank, mask, 0);
        PioSetFunction(board_uart_rts, UART_RTS);
        PioSetFunction(board_uart_cts, UART_CTS);
        PioSetFunction(board_uart_tx, UART_TX);
        PioSetFunction(board_uart_rx, UART_RX);
        StreamUartConfigure(uart_rate, VM_UART_STOP_ONE, VM_UART_PARITY_NONE);
        result = TRUE;
    }
    return result;
}

static bool hciTapStream_ConnectStream(Source src, Sink sink)
{
    if(NULL == StreamConnect(src, sink))
    {
        DEBUG_LOG_WARN(HCI_TAPW_STR"FAIL to CONNECT to Source:%x Sink:%x",
                       src, sink);
        return FALSE;
    }
    else
    {
        hciTapPrivateData.src = src;
        hciTapPrivateData.sink = sink;
        return TRUE;
    }
}

static bool hciTapStream_ConnectToUartComplete(void)
{
    Source src = StreamHciTapSource(); /* should be first */
    Sink sink = StreamUartSink();
    bool retVal = hciTapStream_ConnectStream(src, sink);
    if (retVal)
    {
        DEBUG_LOG_INFO(HCI_TAPI_STR"CONNECTED to Uart Stream");
        hciTapPrivateData.state = HCI_TAP_CONNECTED_TO_UART;
    }
    return retVal;
}

static bool hciTapStream_ConnectToCDCDeviceComplete(void)
{
    Source src = StreamHciTapSource(); /* should be first */
    Sink sink = UsbCdc_StreamEpSink();
    bool retVal = hciTapStream_ConnectStream(src, sink);
    if (retVal)
    {
        DEBUG_LOG_INFO(HCI_TAPI_STR"CONNECTED to USB CDC Stream");
        hciTapPrivateData.state = HCI_TAP_CONNECTED_TO_USB;
    }
    return retVal;
}

/* PUBLIC FUNCTIONS **********************************************************/
void HciTapStream_Disconnect(void)
{
    if (hciTapPrivateData.src == NULL)
    {
        DEBUG_LOG_INFO(HCI_TAPI_STR"Already Disconnected");
        return;
    }
    switch(hciTapPrivateData.state)
    {
        case HCI_TAP_CONNECTED_TO_USB:
            StreamDisconnect(hciTapPrivateData.src, hciTapPrivateData.sink);
            (void) SourceClose(hciTapPrivateData.src);
            UsbApplication_Close();
            DEBUG_LOG_INFO(HCI_TAPI_STR"USB CDC and HCI-TAP streams are DISCONNECTED");
        break;
        case HCI_TAP_CONNECTED_TO_UART:
            StreamDisconnect(hciTapPrivateData.src, hciTapPrivateData.sink);
            (void) SourceClose(hciTapPrivateData.src);
            (void) SinkClose(hciTapPrivateData.sink);
            DEBUG_LOG_INFO(HCI_TAPI_STR"UART and HCI-TAP streams are DISCONNECTED");
            break;
        case HCI_TAP_DISCONNECTED: /*falthrough */
        default:
            DEBUG_LOG_INFO(HCI_TAPI_STR"Only source existed. Freeing it!");
            (void) SourceClose(hciTapPrivateData.src);
            break;
    }
    hciTapPrivateData.sink = NULL;
    hciTapPrivateData.src = NULL;
    hciTapPrivateData.state = HCI_TAP_DISCONNECTED;
}

bool HciTapStream_ConnectToCDCDevice(void)
{
    if (HCI_TAP_DISCONNECTED != hciTapPrivateData.state)
    {
        DEBUG_LOG_WARN(HCI_TAPW_STR"ALREADY CONNECTED - call testHciTapStream_Disconnect first");
        return FALSE;
    }
    UsbApplication_Open(&usb_app_cdc); /* see appTestUsbCdcInit*/
    return hciTapStream_ConnectToCDCDeviceComplete();
}

bool HciTapStream_ConnectToUart(vm_uart_rate uart_rate,
                                uint8 board_uart_rts,
                                uint8 board_uart_cts,
                                uint8 board_uart_tx,
                                uint8 board_uart_rx)
{
    if (HCI_TAP_DISCONNECTED != hciTapPrivateData.state)
    {
        DEBUG_LOG_WARN(HCI_TAPW_STR"ALREADY CONNECTED - call testHciTapStream_Disconnect first");
        return FALSE;
    }
    if (hciTapStream_ConfigureUart(uart_rate, board_uart_rts, board_uart_cts,
            board_uart_tx, board_uart_rx))
    {
        return hciTapStream_ConnectToUartComplete();
    }
    return FALSE;
}

#endif /*ENABLE_HCI_TAP_STREAM*/
/*! @} !*/