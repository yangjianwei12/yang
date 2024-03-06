/*!
\copyright  Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      USB
*/

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

#include "stm32f0xx_hal.h"
#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "main.h"
#include "power.h"
#include "clock.h"
#include "cmsis.h"
#include "config.h"
#include "usb.h"
#include "stm32f0xx_hal_pcd.h"
#include "charger_detect.h"

#ifdef USB_QCOM_DEBUG_DEVICE
#include "usbd_qcom_usb_debug.h"
#include "usbd_qcom_usb_debug_if.h"
#include "earbud.h"
#include "qcom_debug_channel.h"
#else
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#endif

/*-----------------------------------------------------------------------------
------------------ PREPROCESSOR DEFINITIONS -----------------------------------
-----------------------------------------------------------------------------*/

/*
* These should always be powers of 2 in order to simplify calculations and
* save memory.
*/
#define USB_RX_BUFFER_SIZE 1024
#define USB_TX_BUFFER_SIZE 512

#ifdef USB_QCOM_DEBUG_DEVICE
#define usb_register_interface USBD_Qcom_Debug_RegisterInterface

static void usb_handle_qcom_debug(void);

static QCOM_DEBUG_CHANNEL_TRANSACTION usb_to_cc_transaction[NO_OF_EARBUDS]  = 
{
    {{0},{0}}
};

static bool usb_forward_left_first = true;

#else
#define usb_register_interface USBD_CDC_RegisterInterface 
#endif

/*-----------------------------------------------------------------------------
------------------ VARIABLES --------------------------------------------------
-----------------------------------------------------------------------------*/

static bool usb_is_ready = false;
static bool usb_data_to_send = false;

static uint8_t usb_rx_buf[USB_RX_BUFFER_SIZE] = {0};
static uint8_t usb_tx_buf[USB_TX_BUFFER_SIZE] = {0};

static uint16_t usb_rx_buf_head = 0;
static uint16_t usb_rx_buf_tail = 0;
static uint16_t usb_tx_buf_head = 0;
static uint16_t usb_tx_buf_tail = 0;

USBD_HandleTypeDef hUsbDeviceFS;
extern PCD_HandleTypeDef hpcd_USB_FS;

/*-----------------------------------------------------------------------------
------------------ PROTOTYPES -------------------------------------------------
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
------------------ FUNCTIONS --------------------------------------------------
-----------------------------------------------------------------------------*/

void SystemClock_Config(void)
{
    /* Configure the USB clock source */
    RCC->CFGR3 |= RCC_CFGR3_USBSW;
}

void usb_init(void)
{
    SystemClock_Config();

    if (USBD_Init(&hUsbDeviceFS, (USBD_DescriptorsTypeDef *)&FS_Desc, DEVICE_FS) == USBD_OK)
    {
        if (USBD_RegisterClass(&hUsbDeviceFS, (USBD_ClassTypeDef *)&USBD_CDC) == USBD_OK)
        {
            if (usb_register_interface(&hUsbDeviceFS, &USBD_Interface_fops_FS) == USBD_OK)
            {
                PRINT_B("USB initialised");
            }
        }
    }
}

void usb_start(void)
{
    if (USBD_Start(&hUsbDeviceFS) == USBD_OK)
    {
        PRINT_B("USB started");
    }
}

void usb_stop(void)
{
    if (USBD_Stop(&hUsbDeviceFS) == USBD_OK)
    {
        if (USBD_DeInit(&hUsbDeviceFS) == USBD_OK)
        {
            usb_not_ready();
            PRINT_B("USB stopped");
        }
    }
    RCC->CFGR3 &= ~RCC_CFGR3_USBSW;
}

void usb_tx(uint8_t *data, uint16_t len)
{
    usb_data_to_send = true;

    /*
    * Put all the data in the buffer to be sent.
    */
    while (len--)
    {
        uint16_t next_head = (uint16_t)((usb_tx_buf_head + 1) & (USB_TX_BUFFER_SIZE-1));

        if (next_head == usb_tx_buf_tail)
        {
            /*
            * Not enough room in the buffer, so give up. Data output will be
            * truncated.
            */
            break;
        }

        usb_tx_buf[usb_tx_buf_head] = *data++;
        usb_tx_buf_head = next_head;
    }

    if (usb_is_ready)
    {
        power_set_run_reason(POWER_RUN_USB_TX);
    }
}

void usb_rx(uint8_t *data, uint32_t len)
{
    uint32_t n;

    power_set_run_reason(POWER_RUN_USB_RX);
    for (n=0; n<len; n++)
    {
        uint16_t next_head = (usb_rx_buf_head + 1) & (USB_RX_BUFFER_SIZE - 1);

        if (next_head == usb_rx_buf_tail)
        {
            /*
            * Not enough room in the buffer, so give up.
            */
            break;
        }
        else
        {
            usb_rx_buf[usb_rx_buf_head] = data[n];
            usb_rx_buf_head = next_head;
        }
    }
}

void usb_tx_complete(void)
{
    usb_data_to_send = false;
    power_clear_run_reason(POWER_RUN_USB_TX);
}

void usb_tx_periodic(void)
{
    uint16_t head = usb_tx_buf_head;
    uint16_t tail = usb_tx_buf_tail;

    /*
    * Kick off any data to be sent.
    */
    if (usb_is_ready && (head != tail))
    {
        uint16_t len;

        if (tail > head)
        {
            len = USB_TX_BUFFER_SIZE - tail;
        }
        else
        {
            len = head - tail;
        }

        if (CDC_Transmit_FS((uint8_t *)&usb_tx_buf[tail], len)==USBD_OK)
        {
            usb_tx_buf_tail = (usb_tx_buf_tail + len) & (USB_TX_BUFFER_SIZE - 1);
        }
    }
}

void usb_rx_periodic(void)
{

#ifdef USB_QCOM_DEBUG_DEVICE
    usb_handle_qcom_debug();
#endif

    /*
    * Handle received data.
    */
    while (usb_rx_buf_head != usb_rx_buf_tail)
    {
        cli_rx(CLI_SOURCE_USB, (char)usb_rx_buf[usb_rx_buf_tail]);
        usb_rx_buf_tail = (usb_rx_buf_tail + 1) & (USB_RX_BUFFER_SIZE - 1);
    }
    power_clear_run_reason(POWER_RUN_USB_RX);
}

void Error_Handler(void)
{
}

void USB_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_FS);
}

void usb_ready(void)
{
    if (!usb_is_ready)
    {
        PRINT_B("USB ready");
        usb_is_ready = true;

        if (usb_data_to_send)
        {
            power_set_run_reason(POWER_RUN_USB_TX);
        }
    }
}

void usb_not_ready(void)
{
    if (usb_is_ready)
    {
        PRINT_B("USB not ready");
        power_clear_run_reason(POWER_RUN_USB_TX);
        usb_is_ready = false;
    }
}

bool usb_has_enumerated(void)
{
    return hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED;
}

void usb_chg_detected(void)
{
    usb_connected();
    /* Start USB charger detection */
    charger_detect_start();
}

void usb_activate_bcd(void)
{
    USB->BCDR = 0;
    (void)HAL_PCDEx_ActivateBCD(&hpcd_USB_FS);
}

void usb_deactivate_bcd(void)
{
    (void)HAL_PCDEx_DeActivateBCD(&hpcd_USB_FS);
}

/*
* Returns true if the USB data line made contact and false otherwise.
*/
bool usb_dcd(void)
{
    return (USB->BCDR & USB_BCDR_DCDET) ? true:false;
}

/*
* Returns true if D- > V_DAT_REF, false otherwise.
*/
bool usb_pdet(void)
{
    return (USB->BCDR & USB_BCDR_PDET) ? true:false;
}

/*
* Returns true if D+ > V_DAT_REF, false otherwise.
*/
bool usb_sdet(void)
{
    return (USB->BCDR & USB_BCDR_SDET) ? true:false;
}

/*
* Disable Data Contact Detection mode.
*/
void usb_dcd_disable(void)
{
    USB->BCDR &= ~USB_BCDR_DCDEN;
}

/*
* Enable Primary Detection mode.
*/
void usb_primary_detection_enable(void)
{
    USB->BCDR |= USB_BCDR_PDEN;
}

/*
* Disable Primary Detection mode.
*/
void usb_primary_detection_disable(void)
{
    USB->BCDR &= ~USB_BCDR_PDEN;
}

/*
* Enable Secondary Detection mode.
*/
void usb_secondary_detection_enable(void)
{
    USB->BCDR |= USB_BCDR_SDEN;
}

/*
* Disable Secondary Detection mode.
*/
void usb_secondary_detection_disable(void)
{
    USB->BCDR &= ~USB_BCDR_SDEN;
}

void usb_connected(void)
{
#if !defined(FORCE_48MHZ_CLOCK)
    DISABLE_IRQ();
    clock_change(CLOCK_48MHZ);
    ENABLE_IRQ();
#endif
    usb_init();
}

void usb_disconnected(void)
{
    usb_stop();
#if !defined(FORCE_48MHZ_CLOCK)
    DISABLE_IRQ();
    clock_change(CLOCK_8MHZ);
    ENABLE_IRQ();
#endif
}

uint64_t usb_serial_num(void)
{
    return config_get_serial();
}

#ifdef USB_QCOM_DEBUG_DEVICE

void usb_debug_rx(uint8_t earbud, uint8_t *pkt, uint32_t len, void (*cb)(void))
{
    PRINTF_B("usb rx earbud %c len=%u", earbud==EARBUD_RIGHT?'R':'L', len);

    /* Copy the data ready for forwarding over charger comms in the background. */
    usb_to_cc_transaction[earbud].tx_data = pkt; 
    usb_to_cc_transaction[earbud].earbud = earbud; 
    usb_to_cc_transaction[earbud].length  = len;
    usb_to_cc_transaction[earbud].cb  = cb;
}

/**
 * \brief Attempt to forward any stored USB debug data to the appropriate earbud
 *        via the Qualcomm USB Debug channel.
 *
 * \return True if there was data to forward and were successful, false otherwise.
 */
static void usb_handle_qcom_debug(void)
{
    uint8_t i;

    for (i = 0; i < NO_OF_EARBUDS; i++)
    {
        /* We alternate the directions we iterate through the earbuds every time
         * this function is called to manage equal priority between the earbuds */
        uint8_t earbud = usb_forward_left_first ? i : NO_OF_EARBUDS - 1 - i;

        if (usb_to_cc_transaction[earbud].length)
        {
            if(qcom_debug_channel_data_tx(usb_to_cc_transaction[earbud].earbud,
                                          usb_to_cc_transaction[earbud].tx_data,
                                          usb_to_cc_transaction[earbud].length))
            {
                PRINTF_B("usb -> comms %c len %u",
                        usb_to_cc_transaction[earbud].earbud == EARBUD_LEFT ? 'L':'R',
                        usb_to_cc_transaction[earbud].length);
                
                usb_to_cc_transaction[earbud].length  = 0;
                usb_to_cc_transaction[earbud].cb();
            }
        }
    }

    usb_forward_left_first = !usb_forward_left_first;
}

/**
 * \brief Transmit a unlock vendor response from an earbud to the USB host.
 * 
 * \param earbud The earbud this response is from. 
 * \param data A pointer to the response data
 * \param len The length of \a data as number of octets.
 */
static void usb_debug_transmit_vendor_response(uint8_t earbud, uint8_t *data, uint16_t len)
{
    uint16_t wIndex;

    /* Validate and convert the earbud back to the appropriate wIndex value */
    switch (earbud)
    {
        case EARBUD_LEFT:
            wIndex = USB_WINDEX_LEFT_EARBUD;
            break;
        case EARBUD_RIGHT:
            wIndex = USB_WINDEX_RIGHT_EARBUD;
            break;
        default:
            return;
    }

    USB_Qcom_Debug_Send_Vendor_Response(&hUsbDeviceFS, wIndex, data, len);
}

bool usb_debug_vendor(uint8_t bmRequest, uint8_t bRequest, uint16_t wValue,
                      uint16_t wIndex, uint16_t wLength, void *data)
{
    bool device_to_host = (bmRequest & USB_BMREQUESTTYPE_DEVICE_TO_HOST);
    bool is_vendor = (bmRequest & USB_BMREQUESTTYPE_VENDOR);
    uint8_t earbud = NO_OF_EARBUDS;

    PRINTF_B("usb vendor %x %x %x %x %x", bmRequest, bRequest, wValue, wIndex, wLength);

    /* Validate that this is a unlock related command */
    if (!is_vendor || wValue != USB_WVALUE_UNLOCK_DEBUGGER)
    {
        return false;
    }

    /* Validate and establish which earbud this unlock request is for */
    switch (wIndex)
    {
        case USB_WINDEX_LEFT_EARBUD:
            earbud = EARBUD_LEFT;
            break;
        case USB_WINDEX_RIGHT_EARBUD:
            earbud = EARBUD_RIGHT;
            break;
        default:
            return false;
    }

    switch (bRequest)
    {
        case USB_BREQUEST_UNLOCK_DEBUGGER:
            if (wLength  != USB_WLENGTH_UNLOCK_DEBUGGER)
            {
                return false;
            }

            if (device_to_host)
            {
                /* Request a random token from an earbud. */
                qcom_debug_channel_request_random_token(earbud, usb_debug_transmit_vendor_response);

                /* No data is sent back now - fetching the random token is asynchronous
                 * so we wait till it's arrived to respond. */
                return false;
            }
            else
            {
                /* Pass the unlock key to the earbud */
                qcom_debug_channel_unlock_key_tx(earbud, (uint8_t*)data, wLength);
            }
            break;

        case USB_BREQUEST_LOCK_DEBUGGER:
            if (wLength != USB_WLENGTH_LOCK_DEBUGGER ||
                device_to_host)
            {
                return false;
            }

            qcom_debug_channel_lock_req(earbud);
            break;

        case USB_BREQUEST_UNLOCK_STATUS:
            if (wLength != USB_WLENGTH_UNLOCK_STATUS ||
                !device_to_host)
            {
                return false;
            }

            /* Request the unlock status  from an earbud. */
            qcom_debug_channel_unlock_status_req(earbud, usb_debug_transmit_vendor_response);

            /* No data is sent back now - fetching the status  is asynchronous
             * so we wait till it's arrived to respond. */
            return false;

        default:
          return false;
    }

    return true;
}

#endif
