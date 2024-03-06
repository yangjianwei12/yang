/**
  ******************************************************************************
  * @file    usbd_qcom_usb_debug.c
  * @brief   This file provides the high layer firmware functions to manage the
  *          following functionalities of the USB CDC Class and Qualcomm USB Debug Interface:
  *           - Initialization and Configuration of high and low layer
  *           - Enumeration as CDC Device (and enumeration for each implemented memory interface)
  *           - Enumeration as Qualcomm USB Debug Interface for left and right earbuds
  *           - OUT/IN data transfer
  *           - Command IN transfer (class requests management)
  *           - Error management
  *
  *  @verbatim
  *
  *          ===================================================================
  *                                CDC Class Driver Description
  *          ===================================================================
  *           This driver manages the "Universal Serial Bus Class Definitions for Communications Devices
  *           Revision 1.2 November 16, 2007" and the sub-protocol specification of "Universal Serial Bus
  *           Communications Class Subclass Specification for PSTN Devices Revision 1.2 February 9, 2007"
  *           This driver implements the following aspects of the specification:
  *             - Device descriptor management
  *             - Configuration descriptor management
  *             - Enumeration as CDC device with 2 data endpoints (IN and OUT) and 1 command endpoint (IN)
  *             - Requests management (as described in section 6.2 in specification)
  *             - Abstract Control Model compliant
  *             - Union Functional collection (using 1 IN endpoint for control)
  *             - Data interface class
  *
  *           These aspects may be enriched or modified for a specific user application.
  *
  *            This driver doesn't implement the following aspects of the specification
  *            (but it is possible to manage these features with some modifications on this driver):
  *             - Any class-specific aspect relative to communication classes should be managed by user application.
  *             - All communication classes other than PSTN are not managed
  *
  *  @endverbatim
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                      www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* BSPDependencies
- "stm32xxxxx_{eval}{discovery}{nucleo_144}.c"
- "stm32xxxxx_{eval}{discovery}_io.c"
EndBSPDependencies */

/* Includes ------------------------------------------------------------------*/
#include "usbd_qcom_usb_debug.h"
#include "usbd_ctlreq.h"

#include "stm32f0xx_hal_pcd.h"

#include "main.h"
#include "earbud.h"
#include "usb.h"

#ifdef USB_QCOM_DEBUG_DEVICE

/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_CDC
  * @brief usbd core module
  * @{
  */

/** @defgroup USBD_Qcom_Debug_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_Qcom_Debug_Private_Defines
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_Qcom_Debug_Private_Macros
  * @{
  */

/**
  * @}
  */


/** @defgroup USBD_Qcom_Debug_Private_FunctionPrototypes
  * @{
  */


static uint8_t  USBD_Qcom_Debug_Init(USBD_HandleTypeDef *pdev,
                              uint8_t cfgidx);

static uint8_t  USBD_Qcom_Debug_DeInit(USBD_HandleTypeDef *pdev,
                                uint8_t cfgidx);

static uint8_t  USBD_Qcom_Debug_Setup(USBD_HandleTypeDef *pdev,
                               USBD_SetupReqTypedef *req);

static uint8_t  USBD_Qcom_Debug_DataIn(USBD_HandleTypeDef *pdev,
                                uint8_t epnum);

static uint8_t  USBD_Qcom_Debug_DataOut(USBD_HandleTypeDef *pdev,
                                 uint8_t epnum);

static uint8_t  USBD_Qcom_Debug_EP0_RxReady(USBD_HandleTypeDef *pdev);

static uint8_t  *USBD_Qcom_Debug_GetFSCfgDesc(uint16_t *length);

static uint8_t  *USBD_Qcom_Debug_GetOtherSpeedCfgDesc(uint16_t *length);

static uint8_t  *USBD_Qcom_Debug_GetOtherSpeedCfgDesc(uint16_t *length);

uint8_t  *USBD_Qcom_Debug_GetDeviceQualifierDescriptor(uint16_t *length);

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_Qcom_Debug_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};

/**
  * @}
  */

/** @defgroup USBD_Qcom_Debug_Private_Variables
  * @{
  */

const USBD_ClassTypeDef  USBD_CDC =
{
  USBD_Qcom_Debug_Init,
  USBD_Qcom_Debug_DeInit,
  USBD_Qcom_Debug_Setup,
  NULL,                 /* EP0_TxSent, */
  USBD_Qcom_Debug_EP0_RxReady,
  USBD_Qcom_Debug_DataIn,
  USBD_Qcom_Debug_DataOut,
  NULL,
  NULL,
  NULL,
  NULL,
  USBD_Qcom_Debug_GetFSCfgDesc,
  USBD_Qcom_Debug_GetOtherSpeedCfgDesc,
  USBD_Qcom_Debug_GetDeviceQualifierDescriptor,
};

uint8_t left_debug_rx[USB_QCOM_USB_DEBUG_TRANSFER_SIZE];
uint8_t right_debug_rx[USB_QCOM_USB_DEBUG_TRANSFER_SIZE];
extern USBD_HandleTypeDef hUsbDeviceFS;
uint16_t saved_bmRequest = 0xFF;
uint16_t saved_wValue = 0xFF;
uint16_t saved_wIndex = 0xFF;

/* USB CDC and Qualcomm debug composite device Configuration Descriptor */
__ALIGN_BEGIN uint8_t USBD_Qcom_Debug_CfgFSDesc[USB_TOTAL_CONFIG_DESC_SIZ] __ALIGN_END =
{
  /*Configuration Descriptor*/
  0x09,   /* bLength: Configuration Descriptor size */
  USB_DESC_TYPE_CONFIGURATION,      /* bDescriptorType: Configuration */
  USB_TOTAL_CONFIG_DESC_SIZ,                /* wTotalLength:no of returned bytes. */
  0x00,
  0x04,   /* bNumInterfaces: 2 interface */
  0x01,   /* bConfigurationValue: Configuration value */
  0x00,   /* iConfiguration: Index of string descriptor describing the configuration */
  0xC0,   /* bmAttributes: self powered */
  0xFA,   /* MaxPower 500 mA */

  /*---------------------------------------------------------------------------*/

  /* IAD Descriptor */

  0x08, /* bLength */
  0x0B, /* bDescriptorType - IAD Descriptor*/
  0x00, /* bFirstInterface */
  0x02, /* bInterfaceCount */
  0x02, /* bFunctionClass - Communications USB Device Interface Class*/
  0x02, /* bFunctionSubClass */
  0x01, /* bFunctionProtocol */
  0x05, /* iFunction */

  /*---------------------------------------------------------------------------*/

  /*Interface Descriptor */
  0x09,   /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,  /* bDescriptorType: Interface */
  /* Interface descriptor type */
  0x00,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x01,   /* bNumEndpoints: One endpoints used */
  0x02,   /* bInterfaceClass: Communication Interface Class */
  0x02,   /* bInterfaceSubClass: Abstract Control Model */
  0x01,   /* bInterfaceProtocol: Common AT commands */
  0x00,   /* iInterface: */

  /*Header Functional Descriptor*/
  0x05,   /* bLength: Endpoint Descriptor size */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x00,   /* bDescriptorSubtype: Header Func Desc */
  0x10,   /* bcdCDC: spec release number */
  0x01,

  /*Call Management Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x01,   /* bDescriptorSubtype: Call Management Func Desc */
  0x00,   /* bmCapabilities: D0+D1 */
  0x01,   /* bDataInterface: 1 */

  /*ACM Functional Descriptor*/
  0x04,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
  0x02,   /* bmCapabilities */

  /*Union Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x06,   /* bDescriptorSubtype: Union func desc */
  0x00,   /* bMasterInterface: Communication class interface */
  0x01,   /* bSlaveInterface0: Data Class Interface */

  /*Endpoint 2 Descriptor*/
  0x07,                           /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,   /* bDescriptorType: Endpoint */
  CDC_CMD_EP,                     /* bEndpointAddress */
  0x03,                           /* bmAttributes: Interrupt */
  LOBYTE(CDC_CMD_PACKET_SIZE),     /* wMaxPacketSize: */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  CDC_FS_BINTERVAL,                           /* bInterval: */
  /*---------------------------------------------------------------------------*/

  /*Data class interface descriptor*/
  0x09,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_INTERFACE,  /* bDescriptorType: */
  0x01,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x02,   /* bNumEndpoints: Two endpoints used */
  0x0A,   /* bInterfaceClass: CDC */
  0x00,   /* bInterfaceSubClass: */
  0x00,   /* bInterfaceProtocol: */
  0x00,   /* iInterface: */

  /*Endpoint OUT Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
  CDC_OUT_EP,                        /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                              /* bInterval: ignore for Bulk transfer */

  /*Endpoint IN Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
  CDC_IN_EP,                         /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                               /* bInterval: ignore for Bulk transfer */


  /* Qualcomm USB Debug Interface - Left Earbud */

  0x09,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_INTERFACE,  /* bDescriptorType: */
  0x02,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x02,   /* bNumEndpoints: Two endpoints used */
  0xFF,   /* bInterfaceClass: Vendor Specific */
  0x00,   /* bInterfaceSubClass: */
  0x00,   /* bInterfaceProtocol: */
  0x00,   /* iInterface: */

  /*Endpoint OUT Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
  LEFT_EARBUD_OUT_EP,                        /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(USB_DEBUG_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(USB_DEBUG_PACKET_SIZE),
  0x00,                              /* bInterval: ignore for Bulk transfer */

  /*Endpoint IN Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
  LEFT_EARBUD_IN_EP,                         /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(USB_DEBUG_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(USB_DEBUG_PACKET_SIZE),
  0x00,                               /* bInterval: ignore for Bulk transfer */


  /* Qualcomm USB Debug Interface - Right Earbud */

  0x09,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_INTERFACE,  /* bDescriptorType: */
  0x03,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x02,   /* bNumEndpoints: Two endpoints used */
  0xFF,   /* bInterfaceClass: Vendor Specific */
  0x00,   /* bInterfaceSubClass: */
  0x00,   /* bInterfaceProtocol: */
  0x00,   /* iInterface: */

  /*Endpoint OUT Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
  RIGHT_EARBUD_OUT_EP,                        /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(USB_DEBUG_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(USB_DEBUG_PACKET_SIZE),
  0x00,                              /* bInterval: ignore for Bulk transfer */

  /*Endpoint IN Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
  RIGHT_EARBUD_IN_EP,                         /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(USB_DEBUG_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(USB_DEBUG_PACKET_SIZE),
  0x00                               /* bInterval: ignore for Bulk transfer */
} ;

__ALIGN_BEGIN uint8_t USBD_Qcom_Debug_OtherSpeedCfgDesc[USB_CDC_CONFIG_DESC_SIZ] __ALIGN_END =
{
  0x09,   /* bLength: Configuation Descriptor size */
  USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION,
  USB_CDC_CONFIG_DESC_SIZ,
  0x00,
  0x02,   /* bNumInterfaces: 2 interfaces */
  0x01,   /* bConfigurationValue: */
  0x04,   /* iConfiguration: */
  0xC0,   /* bmAttributes: */
  0x32,   /* MaxPower 100 mA */

  /*Interface Descriptor */
  0x09,   /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,  /* bDescriptorType: Interface */
  /* Interface descriptor type */
  0x00,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x01,   /* bNumEndpoints: One endpoints used */
  0x02,   /* bInterfaceClass: Communication Interface Class */
  0x02,   /* bInterfaceSubClass: Abstract Control Model */
  0x01,   /* bInterfaceProtocol: Common AT commands */
  0x00,   /* iInterface: */

  /*Header Functional Descriptor*/
  0x05,   /* bLength: Endpoint Descriptor size */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x00,   /* bDescriptorSubtype: Header Func Desc */
  0x10,   /* bcdCDC: spec release number */
  0x01,

  /*Call Management Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x01,   /* bDescriptorSubtype: Call Management Func Desc */
  0x00,   /* bmCapabilities: D0+D1 */
  0x01,   /* bDataInterface: 1 */

  /*ACM Functional Descriptor*/
  0x04,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
  0x02,   /* bmCapabilities */

  /*Union Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x06,   /* bDescriptorSubtype: Union func desc */
  0x00,   /* bMasterInterface: Communication class interface */
  0x01,   /* bSlaveInterface0: Data Class Interface */

  /*Endpoint 2 Descriptor*/
  0x07,                           /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,         /* bDescriptorType: Endpoint */
  CDC_CMD_EP,                     /* bEndpointAddress */
  0x03,                           /* bmAttributes: Interrupt */
  LOBYTE(CDC_CMD_PACKET_SIZE),     /* wMaxPacketSize: */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  CDC_FS_BINTERVAL,                           /* bInterval: */

  /*---------------------------------------------------------------------------*/

  /*Data class interface descriptor*/
  0x09,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_INTERFACE,  /* bDescriptorType: */
  0x01,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x02,   /* bNumEndpoints: Two endpoints used */
  0x0A,   /* bInterfaceClass: CDC */
  0x00,   /* bInterfaceSubClass: */
  0x00,   /* bInterfaceProtocol: */
  0x00,   /* iInterface: */

  /*Endpoint OUT Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
  CDC_OUT_EP,                        /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  0x40,                              /* wMaxPacketSize: */
  0x00,
  0x00,                              /* bInterval: ignore for Bulk transfer */

  /*Endpoint IN Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,     /* bDescriptorType: Endpoint */
  CDC_IN_EP,                        /* bEndpointAddress */
  0x02,                             /* bmAttributes: Bulk */
  0x40,                             /* wMaxPacketSize: */
  0x00,
  0x00                              /* bInterval */
};

/**
  * @}
  */

/** @defgroup USBD_Qcom_Debug_Private_Functions
  * @{
  */

/**
  * @brief  USBD_Qcom_Debug_Init
  *         Initialize the CDC/Qualcomm Debug interfaces
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_Qcom_Debug_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx __attribute__((unused)))
{
  uint8_t ret = 0U;
  USBD_Qcom_Debug_HandleTypeDef   *hcdc;

  {
    /* Open EP IN */
    USBD_LL_OpenEP(pdev, CDC_IN_EP, USBD_EP_TYPE_BULK,
                   CDC_DATA_FS_IN_PACKET_SIZE);

    pdev->ep_in[CDC_IN_EP & 0xFU].is_used = 1U;

    /* Open EP OUT */
    USBD_LL_OpenEP(pdev, CDC_OUT_EP, USBD_EP_TYPE_BULK,
                   CDC_DATA_FS_OUT_PACKET_SIZE);

    pdev->ep_out[CDC_OUT_EP & 0xFU].is_used = 1U;

    /* Open Qualcomm USB debug endpoints */
    USBD_LL_OpenEP(pdev, RIGHT_EARBUD_OUT_EP, USBD_EP_TYPE_BULK, USB_DEBUG_PACKET_SIZE);
    pdev->ep_out[RIGHT_EARBUD_OUT_EP & 0xFU].is_used = 1U;

    USBD_LL_OpenEP(pdev, RIGHT_EARBUD_IN_EP, USBD_EP_TYPE_BULK, USB_DEBUG_PACKET_SIZE);
    pdev->ep_in[RIGHT_EARBUD_IN_EP & 0xFU].is_used = 1U;

    USBD_LL_OpenEP(pdev, LEFT_EARBUD_OUT_EP, USBD_EP_TYPE_BULK, USB_DEBUG_PACKET_SIZE);
    pdev->ep_out[LEFT_EARBUD_OUT_EP & 0xFU].is_used = 1U;

    USBD_LL_OpenEP(pdev, LEFT_EARBUD_IN_EP, USBD_EP_TYPE_BULK, USB_DEBUG_PACKET_SIZE);
    pdev->ep_in[LEFT_EARBUD_IN_EP & 0xFU].is_used = 1U;
  }

  pdev->pClassData = USBD_malloc(sizeof(USBD_Qcom_Debug_HandleTypeDef));

  if (pdev->pClassData == NULL)
  {
    ret = 1U;
  }
  else
  {
    hcdc = (USBD_Qcom_Debug_HandleTypeDef *) pdev->pClassData;

    /* Init  physical Interface components */
    ((USBD_Qcom_Debug_ItfTypeDef *)pdev->pUserData)->Init();

    /* Init Xfer states */
    hcdc->TxState = 0U;
    hcdc->RxState = 0U;

    /* Prepare Out endpoint to receive next packet */
    USBD_LL_PrepareReceive(pdev, 
            CDC_OUT_EP,
            hcdc->RxBuffer,
            CDC_DATA_FS_OUT_PACKET_SIZE);

    /* Prepare Qualcomm USB debug data */
    USBD_LL_PrepareReceive(pdev,
            RIGHT_EARBUD_OUT_EP,
            right_debug_rx,
            USB_DEBUG_PACKET_SIZE);
    USBD_LL_PrepareReceive(pdev,
            LEFT_EARBUD_OUT_EP,
            left_debug_rx,
            USB_DEBUG_PACKET_SIZE);

  }
  return ret;
}

/**
  * @brief  USBD_Qcom_Debug_Init
  *         DeInitialize the CDC/Qualcomm Debug layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_Qcom_Debug_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx __attribute__((unused)))
{
  uint8_t ret = 0U;

  /* Close EP IN */
  USBD_LL_CloseEP(pdev, CDC_IN_EP);
  pdev->ep_in[CDC_IN_EP & 0xFU].is_used = 0U;

  /* Close EP OUT */
  USBD_LL_CloseEP(pdev, CDC_OUT_EP);
  pdev->ep_out[CDC_OUT_EP & 0xFU].is_used = 0U;
 
  /* Close USB debug endpoints */
  USBD_LL_CloseEP(pdev, LEFT_EARBUD_OUT_EP);
  pdev->ep_out[LEFT_EARBUD_OUT_EP & 0xFU].is_used = 0U;

  USBD_LL_CloseEP(pdev, LEFT_EARBUD_IN_EP);
  pdev->ep_in[LEFT_EARBUD_IN_EP & 0xFU].is_used = 0U;

  USBD_LL_CloseEP(pdev, RIGHT_EARBUD_OUT_EP);
  pdev->ep_out[RIGHT_EARBUD_OUT_EP & 0xFU].is_used = 0U;

  USBD_LL_CloseEP(pdev, RIGHT_EARBUD_IN_EP);
  pdev->ep_in[RIGHT_EARBUD_IN_EP & 0xFU].is_used = 0U;

  /* DeInit  physical Interface components */
  if (pdev->pClassData != NULL)
  {
    ((USBD_Qcom_Debug_ItfTypeDef *)pdev->pUserData)->DeInit();
    USBD_free(pdev->pClassData);
    pdev->pClassData = NULL;
  }

  return ret;
}

/**
  * @brief  USBD_Qcom_Debug_Setup
  *         Handle the CDC/Qualcomm Debug specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  USBD_Qcom_Debug_Setup(USBD_HandleTypeDef *pdev,
                               USBD_SetupReqTypedef *req)
{
  USBD_Qcom_Debug_HandleTypeDef   *hcdc = (USBD_Qcom_Debug_HandleTypeDef *) pdev->pClassData;
  uint8_t ifalt = 0U;
  uint16_t status_info = 0U;
  uint8_t ret = USBD_OK;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
    case USB_REQ_TYPE_CLASS :
      if (req->wLength)
      {
        if (req->bmRequest & 0x80U)
        {
          ((USBD_Qcom_Debug_ItfTypeDef *)pdev->pUserData)->Control(req->bRequest,
                                                            (uint8_t *)(void *)hcdc->data,
                                                            req->wLength);

          USBD_CtlSendData(pdev, (uint8_t *)(void *)hcdc->data, req->wLength);
        }
        else
        {
          saved_bmRequest = req->bmRequest;
          hcdc->CmdOpCode = req->bRequest;
          hcdc->CmdLength = (uint8_t)req->wLength;

          USBD_CtlPrepareRx(pdev, (uint8_t *)(void *)hcdc->data, req->wLength);
        }
      }
      else
      {
        ((USBD_Qcom_Debug_ItfTypeDef *)pdev->pUserData)->Control(req->bRequest,
                                                          (uint8_t *)(void *)req, 0U);
      }
      break;

    case USB_REQ_TYPE_STANDARD:
      switch (req->bRequest)
      {
        case USB_REQ_GET_STATUS:
          if (pdev->dev_state == USBD_STATE_CONFIGURED)
          {
            USBD_CtlSendData(pdev, (uint8_t *)(void *)&status_info, 2U);
          }
          else
          {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        case USB_REQ_GET_INTERFACE:
          if (pdev->dev_state == USBD_STATE_CONFIGURED)
          {
            USBD_CtlSendData(pdev, &ifalt, 1U);
          }
          else
          {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        case USB_REQ_SET_INTERFACE:
          if (pdev->dev_state != USBD_STATE_CONFIGURED)
          {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        default:
          USBD_CtlError(pdev, req);
          ret = USBD_FAIL;
          break;
      }
      break;

    case USB_REQ_TYPE_VENDOR:

        /* Store some information from this request - we may need it later */
        saved_bmRequest = req->bmRequest;
        hcdc->CmdOpCode = req->bRequest;
        saved_wValue = req->wValue;
        saved_wIndex = req->wIndex;
        hcdc->CmdLength = (uint8_t)req->wLength;

        if (req->bmRequest & 0x80U)
        {
          if (usb_debug_vendor(req->bmRequest,
                               req->bRequest,
                               req->wValue,
                               req->wIndex,
                               req->wLength,
                               (void *)hcdc->data))
          {
              USBD_CtlSendData(pdev, (uint8_t *)(void *)hcdc->data, req->wLength);
          }

        }
        else
        {

          USBD_CtlPrepareRx(pdev, (uint8_t *)(void *)hcdc->data, req->wLength);
        }
      break;

    default:
      USBD_CtlError(pdev, req);
      ret = USBD_FAIL;
      break;
  }

  return ret;
}

void USB_Qcom_Debug_Send_Vendor_Response(USBD_HandleTypeDef *pdev,
                                         uint16_t wIndex,
                                         uint8_t *data,
                                         uint16_t len)
{
    USBD_Qcom_Debug_HandleTypeDef   *hcdc = (USBD_Qcom_Debug_HandleTypeDef *) pdev->pClassData;

    if (len == hcdc->CmdLength &&
        wIndex == saved_wIndex)
    {
        USBD_CtlSendData(pdev, data, len);
    }
}

/**
  * @brief  USBD_Qcom_Debug_DataIn
  *         Data sent on non-control IN endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  USBD_Qcom_Debug_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_Qcom_Debug_HandleTypeDef *hcdc = (USBD_Qcom_Debug_HandleTypeDef *)pdev->pClassData;
  PCD_HandleTypeDef *hpcd = pdev->pData;

  if (pdev->pClassData != NULL)
  {
    if ((pdev->ep_in[epnum].total_length > 0U) && ((pdev->ep_in[epnum].total_length % hpcd->IN_ep[epnum].maxpacket) == 0U))
    {
      /* Update the packet total length */
      pdev->ep_in[epnum].total_length = 0U;

      /* Send ZLP */
      USBD_LL_Transmit(pdev, epnum, NULL, 0U);
    }
    else
    {
        usb_tx_complete();
        hcdc->TxState = 0U;
    }
    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}

/**
  * @brief  USBD_Qcom_Debug_ReceivePacket
  *         prepare OUT Endpoint for reception
  * @param  pdev: device instance
  * @retval status
  */
uint8_t Prepare_Debug_Receive(USBD_HandleTypeDef *pdev, uint8_t earbud)
{

    if (earbud == EARBUD_RIGHT)
    {
      /* Prepare USB debug OUT endpoints*/
      USBD_LL_PrepareReceive(pdev,
                             RIGHT_EARBUD_OUT_EP,
                             right_debug_rx,
                             USB_DEBUG_PACKET_SIZE);
    }
    else
    {
      USBD_LL_PrepareReceive(pdev,
                             LEFT_EARBUD_OUT_EP,
                             left_debug_rx,
                             USB_DEBUG_PACKET_SIZE);
    }
    return USBD_OK;
}

static void prepare_left_debug_rx(void)
{
    (void)Prepare_Debug_Receive(&hUsbDeviceFS, EARBUD_LEFT);
}

static void prepare_right_debug_rx(void)
{
    (void)Prepare_Debug_Receive(&hUsbDeviceFS, EARBUD_RIGHT);
}

/**
  * @brief  USBD_Qcom_Debug_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  USBD_Qcom_Debug_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_Qcom_Debug_HandleTypeDef   *hcdc = (USBD_Qcom_Debug_HandleTypeDef *) pdev->pClassData;

  /* Get the received data length */
  hcdc->RxLength = USBD_LL_GetRxDataSize(pdev, epnum);

  /* USB data will be immediately processed, this allow next USB traffic being
  NAKed till the end of the application Xfer */
  if (pdev->pClassData != NULL)
  {
    switch (epnum)
    {
    case CDC_OUT_EP:
        ((USBD_Qcom_Debug_ItfTypeDef*)pdev->pUserData)->Receive(hcdc->RxBuffer, &hcdc->RxLength);
        break;
    case LEFT_EARBUD_OUT_EP:
        usb_debug_rx(EARBUD_LEFT, left_debug_rx, hcdc->RxLength, prepare_left_debug_rx);
        break;
    case RIGHT_EARBUD_OUT_EP:
        usb_debug_rx(EARBUD_RIGHT, right_debug_rx, hcdc->RxLength, prepare_right_debug_rx);
        break;
    }

    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}

/**
  * @brief  USBD_Qcom_Debug_EP0_RxReady
  *         Handle EP0 Rx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_Qcom_Debug_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
  USBD_Qcom_Debug_HandleTypeDef   *hcdc = (USBD_Qcom_Debug_HandleTypeDef *) pdev->pClassData;

  if ((pdev->pUserData != NULL) && (hcdc->CmdOpCode != 0xFFU))
  {
    ((USBD_Qcom_Debug_ItfTypeDef *)pdev->pUserData)->Control(hcdc->CmdOpCode,
                                                      (uint8_t *)(void *)hcdc->data,
                                                      (uint16_t)hcdc->CmdLength);

    if (saved_bmRequest & USB_REQ_TYPE_VENDOR)
    {
        (void)usb_debug_vendor(saved_bmRequest, hcdc->CmdOpCode, saved_wValue,
                               saved_wIndex, (uint16_t)hcdc->CmdLength, (void *)hcdc->data);
    }
    hcdc->CmdOpCode = 0xFFU;

  }
  return USBD_OK;
}

/**
  * @brief  USBD_Qcom_Debug_GetFSCfgDesc
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_Qcom_Debug_GetFSCfgDesc(uint16_t *length)
{
  *length = sizeof(USBD_Qcom_Debug_CfgFSDesc);
  return USBD_Qcom_Debug_CfgFSDesc;
}

/**
  * @brief  USBD_Qcom_Debug_GetCfgDesc
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_Qcom_Debug_GetOtherSpeedCfgDesc(uint16_t *length)
{
  *length = sizeof(USBD_Qcom_Debug_OtherSpeedCfgDesc);
  return USBD_Qcom_Debug_OtherSpeedCfgDesc;
}

/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
uint8_t  *USBD_Qcom_Debug_GetDeviceQualifierDescriptor(uint16_t *length)
{
  *length = sizeof(USBD_Qcom_Debug_DeviceQualifierDesc);
  return USBD_Qcom_Debug_DeviceQualifierDesc;
}

/**
* @brief  USBD_Qcom_Debug_RegisterInterface
  * @param  pdev: device instance
  * @param  fops: CD  Interface callback
  * @retval status
  */
uint8_t  USBD_Qcom_Debug_RegisterInterface(USBD_HandleTypeDef   *pdev,
                                    USBD_Qcom_Debug_ItfTypeDef *fops)
{
  uint8_t  ret = USBD_FAIL;

  if (fops != NULL)
  {
    pdev->pUserData = fops;
    ret = USBD_OK;
  }

  return ret;
}

/**
  * @brief  USBD_Qcom_Debug_SetTxBuffer
  * @param  pdev: device instance
  * @param  pbuff: Tx Buffer
  * @retval status
  */
uint8_t  USBD_Qcom_Debug_SetTxBuffer(USBD_HandleTypeDef   *pdev,
                              uint8_t  *pbuff,
                              uint16_t length)
{
  USBD_Qcom_Debug_HandleTypeDef   *hcdc = (USBD_Qcom_Debug_HandleTypeDef *) pdev->pClassData;

  hcdc->TxBuffer = pbuff;
  hcdc->TxLength = length;

  return USBD_OK;
}


/**
  * @brief  USBD_Qcom_Debug_SetRxBuffer
  * @param  pdev: device instance
  * @param  pbuff: Rx Buffer
  * @retval status
  */
uint8_t  USBD_Qcom_Debug_SetRxBuffer(USBD_HandleTypeDef   *pdev,
                              uint8_t  *pbuff)
{
  USBD_Qcom_Debug_HandleTypeDef   *hcdc = (USBD_Qcom_Debug_HandleTypeDef *) pdev->pClassData;

  hcdc->RxBuffer = pbuff;

  return USBD_OK;
}

/**
  * @brief  USBD_Qcom_Debug_TransmitPacket
  *         Transmit packet on IN endpoint
  * @param  pdev: device instance
  * @retval status
  */
uint8_t  USBD_Qcom_Debug_TransmitPacket(USBD_HandleTypeDef *pdev)
{
  USBD_Qcom_Debug_HandleTypeDef   *hcdc = (USBD_Qcom_Debug_HandleTypeDef *) pdev->pClassData;

  if (pdev->pClassData != NULL)
  {
    if (hcdc->TxState == 0U)
    {
      /* Tx Transfer in progress */
      hcdc->TxState = 1U;

      /* Update the packet total length */
      pdev->ep_in[CDC_IN_EP & 0xFU].total_length = hcdc->TxLength;

      /* Transmit next packet */
      USBD_LL_Transmit(pdev, CDC_IN_EP, hcdc->TxBuffer,
                       (uint16_t)hcdc->TxLength);

      return USBD_OK;
    }
    else
    {
      return USBD_BUSY;
    }
  }
  else
  {
    return USBD_FAIL;
  }
}

uint8_t usb_debug_tx(uint8_t earbud, uint8_t* Buf, uint16_t Len)
{
    USBD_HandleTypeDef *pdev = &hUsbDeviceFS;
    USBD_Qcom_Debug_HandleTypeDef *hcdc = (USBD_Qcom_Debug_HandleTypeDef *) pdev->pClassData;
    uint8_t ep = earbud == EARBUD_RIGHT ? RIGHT_EARBUD_IN_EP : LEFT_EARBUD_IN_EP;

    if (pdev->pClassData != NULL)
    {
        if (hcdc->TxState == 0U)
        {
            /* Tx Transfer in progress */
            hcdc->TxState = 1U;

            /* Update the packet total length */
            pdev->ep_in[ep & 0xFU].total_length = Len;

            /* Transmit next packet */
            return USBD_LL_Transmit(pdev, ep, Buf,
                           (uint16_t)Len);
        }
        else
        {
            return USBD_BUSY;
        }
    }
    else
    {
        return USBD_FAIL;
    }
}

/**
  * @brief  USBD_Qcom_Debug_ReceivePacket
  *         prepare OUT Endpoint for reception
  * @param  pdev: device instance
  * @retval status
  */
uint8_t  USBD_Qcom_Debug_ReceivePacket(USBD_HandleTypeDef *pdev)
{
  USBD_Qcom_Debug_HandleTypeDef   *hcdc = (USBD_Qcom_Debug_HandleTypeDef *) pdev->pClassData;

  /* Suspend or Resume USB Out process */
  if (pdev->pClassData != NULL)
  {
    {
      /* Prepare Out endpoint to receive next packet */
      USBD_LL_PrepareReceive(pdev,
                             CDC_OUT_EP,
                             hcdc->RxBuffer,
                             CDC_DATA_FS_OUT_PACKET_SIZE);
    }
    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}


#endif /* USB_QCOM_DEBUG_DEVICE */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
