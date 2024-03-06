/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_qcom_usb_debug_if.c
  * @brief          : USB device for Virtual Com Port and Qualcomm USB Debug
                      composite device.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_qcom_usb_debug_if.h"
#include "main.h"
#include "usb.h"

#ifdef USB_QCOM_DEBUG_DEVICE

/* USER CODE BEGIN INCLUDE */

/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device library.
  * @{
  */

/** @addtogroup USBD_Qcom_Debug_IF
  * @{
  */

/** @defgroup USBD_Qcom_Debug_IF_Private_TypesDefinitions USBD_Qcom_Debug_IF_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_Qcom_Debug_IF_Private_Defines USBD_Qcom_Debug_IF_Private_Defines
  * @brief Private defines.
  * @{
  */

/* USER CODE BEGIN PRIVATE_DEFINES */
/* Define size for the receive and transmit buffer over CDC */
/* It's up to user to redefine and/or remove those define */
#define APP_RX_DATA_SIZE  64
#define APP_TX_DATA_SIZE  64
/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_Qcom_Debug_IF_Private_Macros USBD_Qcom_Debug_IF_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

/** @defgroup USBD_Qcom_Debug_IF_Private_Variables USBD_Qcom_Debug_IF_Private_Variables
  * @brief Private variables.
  * @{
  */
/* Create buffer for reception and transmission           */
/* It's up to user to redefine and/or remove those define */
/** Received data over USB are stored in this buffer      */
uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];

/** Data to send over USB CDC are stored in this buffer   */
uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];

/** USB CDC linecoding data */
static uint8_t usbd_qcom_usb_debug_cdc_linecoding_data[CDC_LINE_CODING_LENGTH];

/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_Qcom_Debug_IF_Exported_Variables USBD_Qcom_Debug_IF_Exported_Variables
  * @brief Public variables.
  * @{
  */

extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_Qcom_Debug_IF_Private_FunctionPrototypes USBD_Qcom_Debug_IF_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t CDC_Init_FS(void);
static int8_t CDC_DeInit_FS(void);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t* pbuf, uint32_t *Len);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */

/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
  * @}
  */

USBD_Qcom_Debug_ItfTypeDef USBD_Interface_fops_FS =
{
  CDC_Init_FS,
  CDC_DeInit_FS,
  CDC_Control_FS,
  CDC_Receive_FS
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Initializes the CDC media low layer over the FS USB IP
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Init_FS(void)
{
  /* USER CODE BEGIN 3 */
  /* Set Application Buffers */
  USBD_Qcom_Debug_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, 0);
  USBD_Qcom_Debug_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
  return (USBD_OK);
  /* USER CODE END 3 */
}

/**
  * @brief  DeInitializes the CDC media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DeInit_FS(void)
{
  /* USER CODE BEGIN 4 */
  return (USBD_OK);
  /* USER CODE END 4 */
}

/**
  * @brief  Manage the CDC class requests
  * @param  cmd: Command code
  * @param  pbuf: Buffer containing command data (request parameters)
  * @param  length: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf __attribute__((unused)), uint16_t length __attribute__((unused)))
{
    size_t i;

    switch (cmd)
    {
    case CDC_SET_LINE_CODING:
        if (length == CDC_LINE_CODING_LENGTH)
        {
            for (i = 0; i < CDC_LINE_CODING_LENGTH; i++)
            {
                usbd_qcom_usb_debug_cdc_linecoding_data[i] = pbuf[i];
            }
        } 
        break;

    case CDC_GET_LINE_CODING:
        if (length == CDC_LINE_CODING_LENGTH)
        {
            for (i = 0; i < CDC_LINE_CODING_LENGTH; i++)
            {
                pbuf[i] = usbd_qcom_usb_debug_cdc_linecoding_data[i];
            }
        } 
        break;

    case CDC_SET_CONTROL_LINE_STATE:
        {
            if (((USBD_SetupReqTypedef *)pbuf)->wValue & USB_FEATURE_REMOTE_WAKEUP)
            {
                usb_ready();
            }
            else
            {
                usb_not_ready();
            }
        }
        break;

    default:
        break;
    }

  return (USBD_OK);
  /* USER CODE END 5 */
}

/**
  * @brief  Data received over USB OUT endpoint are sent over CDC interface
  *         through this function.
  *
  *         @note
  *         This function will block any OUT packet reception on USB endpoint
  *         untill exiting this function. If you exit this function before transfer
  *         is complete on CDC interface (ie. using DMA controller) it will result
  *         in receiving more data while previous ones are still not sent.
  *
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
{
  /* USER CODE BEGIN 6 */
  usb_rx(Buf, *Len);

  USBD_Qcom_Debug_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
  USBD_Qcom_Debug_ReceivePacket(&hUsbDeviceFS);
  return (USBD_OK);
  /* USER CODE END 6 */
}

/**
  * @brief  Qcom_Usb_Debug_Transmit_FS
  *         Data to send over USB IN endpoint are sent over CDC interface
  *         through this function.
  *         @note
  *
  *
  * @param  Buf: Buffer of data to be sent
  * @param  Len: Number of data to be sent (in bytes)
  * @retval USBD_OK if all operations are OK else USBD_FAIL or USBD_BUSY
  */
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len)
{
  uint8_t result = USBD_OK;
  /* USER CODE BEGIN 7 */
  USBD_Qcom_Debug_HandleTypeDef *hcdc = (USBD_Qcom_Debug_HandleTypeDef*)hUsbDeviceFS.pClassData;
  if (hcdc->TxState != 0){
    return USBD_BUSY;
  }
  USBD_Qcom_Debug_SetTxBuffer(&hUsbDeviceFS, Buf, Len);
  result = USBD_Qcom_Debug_TransmitPacket(&hUsbDeviceFS);

  /* USER CODE END 7 */
  return result;
}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
  * @}
  */

/**
  * @}
  */

#endif /* USB_QCOM_DEBUG_DEVICE */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
