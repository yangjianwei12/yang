/*!
\copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       touchpad_psoc4000s.h
\brief      Header file for Cypress Touch Pad PSoc 4000S
*/

#ifndef TOUCHPAD_IQS269A_PRIVATE_H
#define TOUCHPAD_IQS269A_H

#if (defined HAVE_TOUCHPAD_IQS269A) || (defined HAVE_PROXIMITY_IQS269A)

#include <types.h>
#include "touch.h"

#define DEV_PIO_I2C_SCL                (36)
#define DEV_PIO_I2C_SDA                (37)
#define DEV_IQS269A_PIO_RDY            (35)

#define RDP_IQS269A_PIO_RDY            (2)

#define IQS269A_I2C_ADDRESS            (0x44)

/* Device Info */
#define IQS269A_REGISTER_INFO          (0x00)

/* System Flag */
/* ---------------------------------------------------------------------------------------------
 * |                 Byte 0                      |                 Byte 1                      |
 * ---------------------------------------------------------------------------------------------
 * |  7  |  6 |  5 |  4  |  3  |  2  |  1  |  0  |  7  |  6 |  5 |  4  |  3  |  2  |  1  |  0  |
 * ---------------------------------------------------------------------------------------------
 * |Reset| Reserved| Power Mode| ATI |Event|ULP  |Power|Sys-|Ref |Resv |Gest-|Deep |TOUCH|PROX |
 * |Flag |         |           | In  |     |Upda-|Mode |tem |Cha-|     |ture |Touch|     |     |
 * |     |         |           | Prog|     |te   |Chan-|    |nnel|     |     |     |     |     |
 * |     |         |           |     |     |     |ge   |    |    |     |     |     |     |     |
 * ---------------------------------------------------------------------------------------------
 */

#define IQS269A_REGISTER_HW_INFO       (0x01)
#define IQS269A_REGISTER_SYS           (0x02)

/* Slider Flag */
/* -------------------------------------------------------------------------------------------------
 * |                  Byte 0                       |                 Byte 1                        |
 * -------------------------------------------------------------------------------------------------
 * |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |  7  |  6 |  5  |  4  |  3  |  2  |  1  |  0   |
 * -------------------------------------------------------------------------------------------------
 * |FLIC-|FLIC-|HOLD_|TAP_1|FLIC-|FLIC-|HOLD_|TAP_0|                Reserved                       |
 * |K_NEG|K_POS|1    |     |K_NEG|K_POS|0    |     |                                               |
 * |1    |1    |     |     |0    |0    |     |     |                                               |
 * -------------------------------------------------------------------------------------------------
 */
#define IQS269A_REGISTER_SLIDER_FLAG   (0x03)

/* Channel Flags */
/* ------------------------------------------------------------------------------------------------
 * |                 Byte 0                       |                 Byte 1                        |
 * ------------------------------------------------------------------------------------------------
 * |  7  |  6 |  5  |  4  |  3  |  2  |  1  |  0  |  7  |  6 |  5  |  4  |  3  |  2  |  1  |  0   |
 * ------------------------------------------------------------------------------------------------
 * | Channel Proximity State (CH7 <- CH0)         | Channel Proximity Direction State             |
 * ------------------------------------------------------------------------------------------------
 * | Channel Touch State                          | Channel Deep Touch State                      |
 * ------------------------------------------------------------------------------------------------
 * | Reference Channels Actively Used             | Reserved                                      |
 * ------------------------------------------------------------------------------------------------
 *
 */
#define IQS269A_REGISTER_CH_PROX       (0x04)
#define IQS269A_REGISTER_CH_TOUCH      (0x05)
#define IQS269A_REGISTER_CH_REF        (0x06)

/* Slider Coordinates */
/* ------------------------------------------------------------------------------------------------
 * |                 Byte 0                       |                 Byte 1                        |
 * ------------------------------------------------------------------------------------------------
 * |  7  |  6 |  5  |  4  |  3  |  2  |  1  |  0  |  7  |  6 |  5  |  4  |  3  |  2  |  1  |  0   |
 * ------------------------------------------------------------------------------------------------
 * | Slider 0 Coordinate                          | Slider 1 Coordinate                           |
 * ------------------------------------------------------------------------------------------------
 */
#define IQS269A_REGISTER_SLIDER_CO     (0x30)

/* Power Mode Settings */
/* ----------------------------------------------------------------------------------------------
 * |                 Byte 0                       |                 Byte 1                       |
 * ----------------------------------------------------------------------------------------------
 * |  7  |  6 |  5  |  4  |  3  |  2  |  1  |  0  |  7  |  6 |  5  |  4  |  3  |  2  |  1  |  0  |
 * ----------------------------------------------------------------------------------------------
 * |Main |CH0 |Auto |Power Mode |  ULP Update Rate|Slid-|    |Event|     |     | CMD | CMD | CMD |
 * |Oscl-|ULP |Power| 00 - NP   |                 |er UI|    |Mode |     |     |REDO-|Soft-|ACK  |
 * |llat-|Mode|Chan-| 01 - LP   |                 |     |    |     |     |     |ATI  |Reset|Reset|
 * |or   |    |ge   | 10 - ULP  |                 |     |    |     |     |     |     |     |     |
 * ----------------------------------------------------------------------------------------------
 */
#define IQS269A_REGISTER_PMU           (0x80)

/* Channel Settings */
/* ------------------------------------------------------------------------------------------------
 * |                 Byte 0                       |                 Byte 1                        |
 * ------------------------------------------------------------------------------------------------
 * |  7  |  6 |  5  |  4  |  3  |  2  |  1  |  0  |  7  |  6 |  5  |  4  |  3  |  2  |  1  |  0   |
 * ------------------------------------------------------------------------------------------------
 * |       Active CHS(CH7<-CH0)                   |LTA Filter|Count Filt-| LTA Filter|Count Filter|
 * |                                              |Strenth LP|er Str LP  | Str NP    | Str NP     |
 * ------------------------------------------------------------------------------------------------
 *
 * Filters
 * 00 weak & Fast
 * 01
 * 10
 * 11 Strong & Slow
 */
/* ------------------------------------------------------------------------------------------------
 * |                 Byte 0                       |                  Byte 1                       |
 * ------------------------------------------------------------------------------------------------
 * |  7  |  6 |  5  |  4  |  3  |  2  |  1  |  0  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
 * ------------------------------------------------------------------------------------------------
 * |       Reseed CHS(CH7<-CH0)                   |Power|Sys- |Ref  |Resv |Gest-|Deep |TOUCH|PROX |
 * |                                              |Mode |tem  |Cha- |     |ture |Touch|     |     |
 * |                                              |Chan-|     |nnel |     |     |     |     |     |
 * |                                              |ge   |     |     |     |     |     |     |     |
 * ------------------------------------------------------------------------------------------------
 *
 * For Byte 1, if bit is set to 1, the event won't be reported
 */
#define IQS269A_REGISTER_CHS           (0x81)
#define IQS269A_REGISTER_CHRESEED      (0x82)

/* Report Rate Settings */
/* ------------------------------------------------------------------------------------------------
 * |                 Byte 0                       |                 Byte 1                        |
 * ------------------------------------------------------------------------------------------------
 * |  7  |  6 |  5  |  4  |  3  |  2  |  1  |  0  |  7  |  6 |  5  |  4  |  3  |  2  |  1  |  0   |
 * ------------------------------------------------------------------------------------------------
 * | Normal Power Mode Report Rate (0~255 ms)     | Low Power Mode Report Rate (0~255 ms)         |
 * ------------------------------------------------------------------------------------------------
 * | Ultra LP Mode Report Rate (0~255)*16 ms      | Power Mode Timer  (0~255)*512 ms              |
 * ------------------------------------------------------------------------------------------------
 * | Rdy time-out (0~255)*0.5 ms                  | LTA Halt Timeout (0~255)*512 ms               |
 * ------------------------------------------------------------------------------------------------
 *
 * For LTA Halt Timeout, if set it to 0xFF, it never timeout
 */
#define IQS269A_REGISTER_REPORT        (0x83)
#define IQS269A_REGISTER_ULP_REPORT    (0x84)

/* Timeout Settings */
#define IQS269A_REGISTER_TIMEOUT       (0x85)

/* General Settings */
/* ---------------------------------------------------------------------------------------------------
 * |                 Byte 0                         |                 Byte 1                         |
 * ---------------------------------------------------------------------------------------------------
 * |  7  |  6   |  5  |  4  |  3  |  2  |  1  |  0  |  7  |  6 |  5  |  4  |  3  |  2  |  1  |  0    |
 * ---------------------------------------------------------------------------------------------------
 * |     |ATI_LP|ATI- |Disa-|Resev|    GPIO3 Output |Resv |Bi- |           |    Reserved     |Global |
 * |     |      |Band |ble  |     |000-Channel 0    |set 0|dir-|           |                 |CAL-   |
 * |     |      |     |Count|     | .               |     |ect-|           |                 |cap    |
 * |     |      |     |Filt-|     | .               |     |ion |           |                 |       |
 * |     |      |     |er   |     |111-Channel 7    |     |    |           |                 |       |
 * ---------------------------------------------------------------------------------------------------
 * |                   Reserved                     |Reference |Resv |Enab-|  Reserved |Slider filter|
 * |                                                |Channel   |     |le   |   set 00  |     str     |
 * |                                                |default UI|     |ref  |           |00 0 (Raw)   |
 * |                                                |00-NoEvent|     |trac-|           |01 1         |
 * |                                                |01-Prox   |     |king |           |10 2         |
 * |                                                |10-Touch  |     |UI   |           |11 3 (Slow)  |
 * |                                                |11-All    |     |     |           |             |
 * ---------------------------------------------------------------------------------------------------
 * |         Blocking Channel(CH7 <- CH0)           |                Reserved                        |
 * ---------------------------------------------------------------------------------------------------
 */
#define IQS269A_REGISTER_GENERAL       (0x86)
#define IQS269A_REGISTER_REF           (0x87)
#define IQS269A_REGISTER_EVENT         (0x88)

/* Gesture Settings */
/* ------------------------------------------------------------------------------------------------
 * |                 Byte 0                       |                 Byte 1                        |
 * ------------------------------------------------------------------------------------------------
 * |  7  |  6 |  5  |  4  |  3  |  2  |  1  |  0  |  7  |  6 |  5  |  4  |  3  |  2  |  1  |  0   |
 * ------------------------------------------------------------------------------------------------
 * | Channel for Slider 0 (CH7 <- CH0)            | Channel for Slider 1 (CH7 <- CH0)             |
 * ------------------------------------------------------------------------------------------------
 * | Slider TAP timeout (0~255)*16 ms             | Slider SWIPE Timeout  (0~255)*16 ms           |
 * ------------------------------------------------------------------------------------------------
 * | Slider Swipe Threshold (0~255) coordidates   |                                               |
 * ------------------------------------------------------------------------------------------------
 *
 */
#define IQS269A_REGISTER_SLIDERS       (0x89)
#define IQS269A_REGISTER_TAP           (0x8A)
#define IQS269A_REGISTER_SWIPE         (0x8B)

/* Individual Channels */
/*
 * Registers for CHx
 *
 * ------------------------------------------------------------------------------------------------
 * |                 Byte 0                       |                 Byte 1                        |
 * ------------------------------------------------------------------------------------------------
 * |  7  |  6 |  5  |  4  |  3  |  2  |  1  |  0  |  7  |  6 |  5  |  4  |  3  |  2  |  1  |  0   |
 * ------------------------------------------------------------------------------------------------
 * |             CRX (7 <- 0)                     |                  TX (7 <- 0)                  |
 * -----------------------------------------------------------------------------------------------
 * |     |Res-|     |Inte-| Reserved  |  ATI Mode |     | Projected|Resv |        Sensor Mode     |
 * |     |erv-|     |rnal |           |11 Full    |     | Mode Bias|     |0000: Surface           |
 * |     |ed  |     |Cap  |           |10 Partial |     |00: 2.5uA |     |0001: Projected         |
 * |     |    |     |Size |           |01 Semi-Pa-|     |01: 5uA   |     |1001: Self & Mutial ind-|
 * |     |    |     |0:0pF|           |   tial    |     |10: 10uA  |     |uctance 1110: HALL      |
 * |     |    |     |1:0.5|           |00 Disable |     |11: 20uA  |     |1111: Temperature       |
 * ------------------------------------------------------------------------------------------------
 * | Reserved |Enab-|Rese-|Rese-| Sensing   |stat-| ATI Base |           ATI target (x32)         |
 * |          |le   |rved |rved |Frequency  |ic   | value    |                                    |
 * |          |Inte-|     |     |Selection  |fine |          |                                    |
 * |          |nal  |     |     |00:4/1 MHz |mult-|  00: 75  |                                    |
 * |          |Cap  |     |     |01:2MHz/500|ipli-|  01: 100 |                                    |
 * |          |     |     |     |kHz        |ers  |  10: 150 |                                    |
 * |          |     |     |     |10:1MHz/250|HALL |  11: 200 |                                    |
 * |          |     |     |     |kHz        |0:Off|          |                                    |
 * |          |     |     |     |11:500kHz/ |1:On |          |                                    |
 * |          |     |     |     | 125kHz    |     |          |                                    |
 * ------------------------------------------------------------------------------------------------
 * |                               Nornal Use is Read-Only                                        |
 * |----------------------------------------------------------------------------------------------|
 * |Compensat-|Coarse ope-| Fine Operating Point  | Compensation (LSB)                            |
 * |ion (MSB) |ating point| (ATI)                 |                                               |
 * |          |(ATI)      |                       |                                               |
 * ------------------------------------------------------------------------------------------------
 * | Channel Proximity Threshold (0~255) Counts   | Channel Touch Treshold (0~255)*LTA value/256  |
 * ------------------------------------------------------------------------------------------------
 * | Channel Deep Touch Treshold (0~255)*LTA/256  |Channel Hysteresis for| Channel Hysteresis for |
 * |                                              | Deep Touch           | Touch                  |
 * ------------------------------------------------------------------------------------------------
 * |       Reference Channel Association          | Associated sensing channel impact weight      |
 * ------------------------------------------------------------------------------------------------
 *
 */
#define IQS269A_REGISTER_CH0           (0x8C)
#define IQS269A_REGISTER_CH1           (0x93)
#define IQS269A_REGISTER_CH2           (0x9A)
#define IQS269A_REGISTER_CH3           (0xA1)
#define IQS269A_REGISTER_CH4           (0xA8)
#define IQS269A_REGISTER_CH5           (0xAF)
#define IQS269A_REGISTER_CH6           (0xB6)
#define IQS269A_REGISTER_CH7           (0xBD)

#define IQS269A_REGISTER_I2C_CONTROLL  (0xF2)
#define IQS269A_REGISTER_HALL          (0xF5)

#define TOUCH_BITSERIAL_BLOCK          (BITSERIAL_BLOCK_1)

#define IQS269A_GENERAL_PMU_SET        (0x40)
#define IQS269A_GENERAL_PMU_NO_SWITCH  (0x20)
#define IQS269A_GENERAL_I2C_SET        (0xA0)
#define IQS269A_GENERAL_I2C_ACK_RESET  (0x01)
#define IQS269A_GENERAL_I2C_RESET      (0x02)
#define IQS269A_GENERAL_I2C_REDO_ATI   (0x04)
#define IQS269A_GENERAL_ENABLE_EVENTS  (0x20)
#define IQS269A_GENERAL_I2C_ONLY_SWIPE (0x80)
#define IQS269A_FILTER_STRENGTH        (0x00)
#define IQS269A_RESEED_CHS             (0x00)
#define IQS269A_RESEED_CHS_TOUCH       (0x00)
#define IQS269A_NP_REPORT_RATE         (0x0A)
#define IQS269A_LP_REPORT_RATE         (0x30)
#define IQS269A_ULP_REPORT_RATE        (0x03)
#define IQS269A_POWER_MODE_TIMEOUT     (0x03)
#define IQS269A_RDY_TIMEOUT            (0x64)
#define IQS269A_LTA_TIMEOUT            (0x01)
#define IQS269A_TRACKING_UI            (0x10)
#define IQS269A_SLIDE_STRENGTH         (0x00)
#define IQS269A_SLIDER0_CHS            (0x0E)
#define IQS269A_SLIDER1_CHS            (0x00)
#define IQS269A_TAP_TIMEOUT            (0x00)
#define IQS269A_SWIPE_TIMEOUT          (0x36)
#define IQS269A_SWIPE_CO_THRESHOLD     (0x30)

#define IQS269A_CRX_RDP_CH0            (0x80)
#define IQS269A_TX_CH0                 (0x00)
#define IQS269A_SENSING_SET1_CH0       (0xA3)
#define IQS269A_SENSING_SET2_CH0       (0x00)
#define IQS269A_SENSING_SET3_CH0       (0x24)
#define IQS269A_ATI_SET_CH0            (0x10)
#define IQS269A_PROX_THRESHOLD_CH0     (0xFF)
#define IQS269A_TOUCH_THRESHOLD_CH0    (0x03)
#define IQS269A_DEEP_TOUCH_THRES_CH0   (0xFF)
#define IQS269A_HYSTERESIS_CH0         (0x00)
#define IQS269A_REF_CHANNEL_CH0        (0x00)
#define IQS269A_ASSOCIATE_WEIGHT_CH0   (0x00)

#define IQS269A_CRX_RDP_CH1            (0x40)
#define IQS269A_TX_CH1                 (0x00)
#define IQS269A_SENSING_SET1_CH1       (0xA3)
#define IQS269A_SENSING_SET2_CH1       (0x00)
#define IQS269A_SENSING_SET3_CH1       (0x24)
#define IQS269A_ATI_SET_CH1            (0x10)
#define IQS269A_PROX_THRESHOLD_CH1     (0xFF)
#define IQS269A_TOUCH_THRESHOLD_CH1    (0x01)
#define IQS269A_DEEP_TOUCH_THRES_CH1   (0xFF)
#define IQS269A_HYSTERESIS_CH1         (0x00)
#define IQS269A_REF_CHANNEL_CH1        (0x00)
#define IQS269A_ASSOCIATE_WEIGHT_CH1   (0x00)

#define IQS269A_CRX_RDP_CH2            (0x20)
#define IQS269A_TX_CH2                 (0x00)
#define IQS269A_SENSING_SET1_CH2       (0xA3)
#define IQS269A_SENSING_SET2_CH2       (0x00)
#define IQS269A_SENSING_SET3_CH2       (0x24)
#define IQS269A_ATI_SET_CH2            (0x10)
#define IQS269A_PROX_THRESHOLD_CH2     (0xFF)
#define IQS269A_TOUCH_THRESHOLD_CH2    (0x03)
#define IQS269A_DEEP_TOUCH_THRES_CH2   (0xFF)
#define IQS269A_HYSTERESIS_CH2         (0x00)
#define IQS269A_REF_CHANNEL_CH2        (0x00)
#define IQS269A_ASSOCIATE_WEIGHT_CH2   (0x00)

#define IQS269A_CRX_RDP_CH3            (0x10)
#define IQS269A_TX_CH3                 (0x00)
#define IQS269A_SENSING_SET1_CH3       (0xA3)
#define IQS269A_SENSING_SET2_CH3       (0x00)
#define IQS269A_SENSING_SET3_CH3       (0x24)
#define IQS269A_ATI_SET_CH3            (0x10)
#define IQS269A_PROX_THRESHOLD_CH3     (0xFF)
#define IQS269A_TOUCH_THRESHOLD_CH3    (0x01)
#define IQS269A_DEEP_TOUCH_THRES_CH3   (0xFF)
#define IQS269A_HYSTERESIS_CH3         (0x00)
#define IQS269A_REF_CHANNEL_CH3        (0x00)
#define IQS269A_ASSOCIATE_WEIGHT_CH3   (0x00)

#define IQS269A_CRX_RDP_CH4            (0x01)
#define IQS269A_TX_CH4                 (0x00)
#define IQS269A_SENSING_SET1_CH4       (0xA3)
#define IQS269A_SENSING_SET2_CH4       (0x00)
#define IQS269A_SENSING_SET3_CH4       (0x24)
#define IQS269A_ATI_SET_CH4            (0x18)
#define IQS269A_PROX_THRESHOLD_CH4     (0x10)
#define IQS269A_TOUCH_THRESHOLD_CH4    (0xFF)
#define IQS269A_DEEP_TOUCH_THRES_CH4   (0xFF)
#define IQS269A_HYSTERESIS_CH4         (0x00)
#define IQS269A_REF_CHANNEL_CH4        (0x00)
#define IQS269A_ASSOCIATE_WEIGHT_CH4   (0x00)

#define IQS269A_CRX_RDP_CH5            (0x08)
#define IQS269A_TX_CH5                 (0x00)
#define IQS269A_SENSING_SET1_CH5       (0xA3)
#define IQS269A_SENSING_SET2_CH5       (0x00)
#define IQS269A_SENSING_SET3_CH5       (0x24)
#define IQS269A_ATI_SET_CH5            (0x18)
#define IQS269A_PROX_THRESHOLD_CH5     (0x10)
#define IQS269A_TOUCH_THRESHOLD_CH5    (0xFF)
#define IQS269A_DEEP_TOUCH_THRES_CH5   (0xFF)
#define IQS269A_HYSTERESIS_CH5         (0x00)
#define IQS269A_REF_CHANNEL_CH5        (0x00)
#define IQS269A_ASSOCIATE_WEIGHT_CH5   (0x00)

#define IQS269A_CRX_RDP_CH6            (0x04)
#define IQS269A_TX_CH6                 (0x00)
#define IQS269A_SENSING_SET1_CH6       (0xA3)
#define IQS269A_SENSING_SET2_CH6       (0x00)
#define IQS269A_SENSING_SET3_CH6       (0x24)
#define IQS269A_ATI_SET_CH6            (0x18)
#define IQS269A_PROX_THRESHOLD_CH6     (0xFF)
#define IQS269A_TOUCH_THRESHOLD_CH6    (0xFF)
#define IQS269A_DEEP_TOUCH_THRES_CH6   (0xFF)
#define IQS269A_HYSTERESIS_CH6         (0x00)
#define IQS269A_REF_CHANNEL_CH6        (0x30)
#define IQS269A_ASSOCIATE_WEIGHT_CH6   (0x00)

#define IQS269A_CRX_RDP_CH7            (0x70)
#define IQS269A_TX_CH7                 (0x00)
#define IQS269A_SENSING_SET1_CH7       (0xA3)
#define IQS269A_SENSING_SET2_CH7       (0x00)
#define IQS269A_SENSING_SET3_CH7       (0x24)
#define IQS269A_ATI_SET_CH7            (0x10)
#define IQS269A_PROX_THRESHOLD_CH7     (0x00)
#define IQS269A_TOUCH_THRESHOLD_CH7    (0x00)
#define IQS269A_DEEP_TOUCH_THRES_CH7   (0xFF)
#define IQS269A_HYSTERESIS_CH7         (0x00)
#define IQS269A_REF_CHANNEL_CH7        (0x0E)
#define IQS269A_ASSOCIATE_WEIGHT_CH7   (0x00)

#define IQS269A_SYS_EVENT_MASK         (0x02)
#define IQS269A_SYS_ATI_MASK           (0x04)
#define IQS269A_SYS_RESET_MASK         (0x80)

#define IQS269A_EVENT_PROX_MASK        (0x01)
#define IQS269A_EVENT_TOUCH_MASK       (0x02)
#define IQS269A_EVENT_GESTURE_MASK     (0x08)
#define IQS269A_EVENT_POWER_MODE_MASK  (0x80)

#define IQS269A_TOUCH_CHANNEL_MASK     (0x01)
#define IQS269A_WEAR_CHANNEL_MASK      (0x30)

#define IQS269A_SWIPE_UP_DOWN_MASK     (0xCC)
#define IQS269A_SWIPE_UP_MASK          (0x88)
#define IQS269A_SWIPE_DOWN_MASK        (0x44)

#define IQS269A_POWER_MODE_CHANGE_MASK (0x18)
#define IQS269A_LP_POWER_MASK          (0x08)
#define IQS269A_ULP_POWER_MASK         (0x10)

#define IQS269A_REF_BLOCK_CHANNEL      (0x80)

/* Gather swipe, touch and proximity events */
/* bit set to 1 means event is ignored */
#define IQS269A_EVENT_MASK             (0xF4)
#define IQS269A_NO_EVENT_MASK          (0xFF)

#ifdef HAVE_PROXIMITY_IQS269A
#define IQS269A_ACTIVE_CHS             (0xFF)
#define IQS269A_RE_ATI_CHANNELS        (0xFF)
#else
#define IQS269A_ACTIVE_CHS             (0x8F)
#define IQS269A_RE_ATI_CHANNELS        (0x8F)
#endif

#define IQS269A_I2C_STOP_IGNORE        (0x40)
#define IQS269A_I2C_END_WINDOW_MASK    (0x80)
#define IQS269A_I2C_END_WINDOW_V3      (0xC1)

#define IQS269A_HALL_MASK              (0x80)

#define MAX_TOUCH_CHANNEL              (3)

#define touchConfigPressCancelMs()              (300)
#define touchConfigMaximumHeldTimeSeconds()     (65)
#define touchConfigClickTimeoutlMs()            (400)
#define touchConfigWaitRdyUs()                  (100)
#define touchConfigReadEventIntervalMs()        (10)
#define touchConfigCheckAvailableCounts()       (10)
#define touchConfigSendResetCmdMs()             (1300)

typedef struct _pio_func
{
   uint16 pio;
   pin_function_id func;
} pio_func;

enum
{
    TOUCH_READ_EVENT=1,
    TOUCH_INTERNAL_HELD_CANCEL_TIMER,
    TOUCH_INTERNAL_HELD_TIMER,
    TOUCH_INTERNAL_HELD_RELEASE,
    TOUCH_INTERNAL_CLICK_TIMER,
    TOUCH_RESET_SENSOR
};

enum
{
    SLIDE_STRENGTH_RAW=0,
    SLIDE_STRENGTH_LEVEL_1,
    SLIDE_STRENGTH_LEVEL_2,
    SLIDE_STRENGTH_LEVEL_3
};

enum
{
    NORMAL_POWER=0,
    LOW_POWER,
    ULOW_POWER
};

/*! Internal representation of proximity state */
enum proximity_states
{
    proximity_state_unknown,
    proximity_state_in_proximity,
    proximity_state_not_in_proximity
};

/*! Trivial state for storing in-proximity state */
struct __proximity_state
{
    /*! The sensor proximity state */
    enum proximity_states proximity;
};

/*! The high level configuration of touch sensor */
struct __touch_config_t
{
    /*! The I2C address */
    uint8 i2c_addr;
    /*! The I2C clock frequency */
    uint16 i2c_clock_khz;
    /*! The PIOs used to control/communicate with the sensor */
    struct
    {
        /*! Interrupt PIO driven by the sensor */
        uint8 rdy;
        /*! I2C serial data PIO */
        uint8 i2c_sda;
        /*! I2C serial clock PIO */
        uint8 i2c_scl;
    } pios;
};

typedef struct _touchFlags
{
    bool touchFlag:1;
    bool bsHandleCloseNeeded:1;
    bool dumpFlag:1;
    bool suppressSlide:1;
    bool lastEventTouch:1;
    bool wearFlag:1;
    bool proximityRegisted:1;
    bool enableWearProxEvent:1;
    bool enableWearTouchEvent:1;
    bool suppressEvents:1;
    bool resetAfterInit:1;
}TouchFlags;

typedef struct _touchVariables
{
    bitserial_handle bs_handle;
    /* Thresholds of channels for wearing detection points */
    uint8 wearProxThres;
    uint8 wearTouchThres;
}TouchVariables;

typedef struct _touchPadVariable
{
    uint8 reg;
    uint8 touchPadThres;
}TouchPadVariable;

#endif /* HAVE_TOUCHPAD_IQS269A*/
#endif /* TOUCHPAD_IQS269A_PRIVATE_H */
