/**
  ******************************************************************************
  * @file    usbd_qcom_usb_debug.h
  * @brief   header file for the usbd_qcom_usb_debug.c file.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_QCOM_USB_DEBUG_H
#define __USB_QCOM_USB_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include  "usbd_ioreq.h"


/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */

/** @defgroup usbd_cdc
  * @brief This file is the Header file for usbd_cdc.c
  * @{
  */


/** @defgroup usbd_cdc_Exported_Defines
  * @{
  */
#define CDC_IN_EP                                   0x81U  /* EP1 for data IN */
#define CDC_OUT_EP                                  0x01U  /* EP1 for data OUT */
#define CDC_CMD_EP                                  0x82U  /* EP2 for CDC commands */

/* Qualcomm USB Debug endpoints */
#define LEFT_EARBUD_OUT_EP                          0x06
#define LEFT_EARBUD_IN_EP                           0x86
#define RIGHT_EARBUD_OUT_EP                         0x07
#define RIGHT_EARBUD_IN_EP                          0x87


#ifndef CDC_HS_BINTERVAL
#define CDC_HS_BINTERVAL                          0x10U
#endif /* CDC_HS_BINTERVAL */

#ifndef CDC_FS_BINTERVAL
#define CDC_FS_BINTERVAL                          0x10U
#endif /* CDC_FS_BINTERVAL */

/* CDC Endpoints parameters: you can fine tune these values depending on the needed baudrates and performance. */
#define CDC_DATA_HS_MAX_PACKET_SIZE                 512U  /* Endpoint IN & OUT Packet size */
#define CDC_DATA_FS_MAX_PACKET_SIZE                 64U  /* Endpoint IN & OUT Packet size */
#define CDC_CMD_PACKET_SIZE                         8U  /* Control Endpoint Packet size */

#define USB_CDC_CONFIG_DESC_SIZ                     75U
#define CDC_DATA_HS_IN_PACKET_SIZE                  CDC_DATA_HS_MAX_PACKET_SIZE
#define CDC_DATA_HS_OUT_PACKET_SIZE                 CDC_DATA_HS_MAX_PACKET_SIZE

#define CDC_DATA_FS_IN_PACKET_SIZE                  CDC_DATA_FS_MAX_PACKET_SIZE
#define CDC_DATA_FS_OUT_PACKET_SIZE                 CDC_DATA_FS_MAX_PACKET_SIZE

#define USB_QCOM_USB_DEBUG_CONFIG_DESC_SIZ          0x17
#define USB_DEBUG_PACKET_SIZE                       64
#define USB_QCOM_USB_DEBUG_TRANSFER_SIZE            512

#define USB_TOTAL_CONFIG_DESC_SIZ                   (USB_CDC_CONFIG_DESC_SIZ + USB_QCOM_USB_DEBUG_CONFIG_DESC_SIZ + USB_QCOM_USB_DEBUG_CONFIG_DESC_SIZ)

/*---------------------------------------------------------------------*/
/*  CDC definitions                                                    */
/*---------------------------------------------------------------------*/
#define CDC_SEND_ENCAPSULATED_COMMAND               0x00U
#define CDC_GET_ENCAPSULATED_RESPONSE               0x01U
#define CDC_SET_COMM_FEATURE                        0x02U
#define CDC_GET_COMM_FEATURE                        0x03U
#define CDC_CLEAR_COMM_FEATURE                      0x04U
#define CDC_SET_LINE_CODING                         0x20U
#define CDC_GET_LINE_CODING                         0x21U
#define CDC_SET_CONTROL_LINE_STATE                  0x22U
#define CDC_SEND_BREAK                              0x23U
#define CDC_LINE_CODING_LENGTH                      7

/**
  * @}
  */


/** @defgroup USBD_CORE_Exported_TypesDefinitions
  * @{
  */

typedef struct _USBD_Qcom_Debug_Itf
{
  int8_t (* Init)(void);
  int8_t (* DeInit)(void);
  int8_t (* Control)(uint8_t cmd, uint8_t *pbuf, uint16_t length);
  int8_t (* Receive)(uint8_t *Buf, uint32_t *Len);

} USBD_Qcom_Debug_ItfTypeDef;


typedef struct
{
  uint32_t data[CDC_DATA_HS_MAX_PACKET_SIZE / 4U];      /* Force 32bits alignment */
  uint8_t  CmdOpCode;
  uint8_t  CmdLength;
  uint8_t  *RxBuffer;
  uint8_t  *TxBuffer;
  uint32_t RxLength;
  uint32_t TxLength;

  __IO uint32_t TxState;
  __IO uint32_t RxState;
}
USBD_Qcom_Debug_HandleTypeDef;



/** @defgroup USBD_CORE_Exported_Macros
  * @{
  */

/**
  * @}
  */

/** @defgroup USBD_CORE_Exported_Variables
  * @{
  */

extern const USBD_ClassTypeDef  USBD_CDC;
#define USBD_Qcom_Debug_CLASS    &USBD_CDC
/**
  * @}
  */

/** @defgroup USB_CORE_Exported_Functions
  * @{
  */
uint8_t  USBD_Qcom_Debug_RegisterInterface(USBD_HandleTypeDef   *pdev,
                                    USBD_Qcom_Debug_ItfTypeDef *fops);

uint8_t  USBD_Qcom_Debug_SetTxBuffer(USBD_HandleTypeDef   *pdev,
                              uint8_t  *pbuff,
                              uint16_t length);

uint8_t  USBD_Qcom_Debug_SetRxBuffer(USBD_HandleTypeDef   *pdev,
                              uint8_t  *pbuff);

uint8_t  USBD_Qcom_Debug_ReceivePacket(USBD_HandleTypeDef *pdev);

uint8_t  USBD_Qcom_Debug_TransmitPacket(USBD_HandleTypeDef *pdev);


uint8_t usb_debug_tx(uint8_t earbud, uint8_t* Buf, uint16_t Len);

void usb_debug_rx(uint8_t earbud, uint8_t *pkt, uint32_t len, void (*cb)(void));

/**
 * \brief Handler for device vendor requests for Qualcomm USB debug
 *
 * \param bmRequest The bmRequestType value
 * \param bRequest The bRequest value
 * \param wValue The wValue value
 * \param wIndex The wIndex value
 * \param wLength The wLength value
 * \param data Pointer to any additional data as part of the vendor request
 * \return True if a response was sent to this vendor request, false otherwise.
 */
bool usb_debug_vendor(uint8_t bmRequest, uint8_t bRequest, uint16_t wValue,
                      uint16_t wIndex, uint16_t wLength, void *data);

/**
 * \brief Send a vendor request response to the USB host.
 *
 * \param pdev The device to send this vendor request on.
 * \param wIndex The wIndex of the vendor request we are responding to.
 *               This selects the earbud
 * \param data Pointer to the response data
 * \param len The length of \a data in number of octets.
 */
void USB_Qcom_Debug_Send_Vendor_Response(USBD_HandleTypeDef *pdev,
                                         uint16_t wIndex,
                                         uint8_t *data,
                                         uint16_t len);

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif  /* __USB_QCOM_USB_DEBUG_H */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
