/*!
\copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Support for Azoteq IQS269A
*/
#ifdef INCLUDE_CAPSENSE
#ifdef HAVE_TOUCHPAD_IQS269A
#include <bitserial_api.h>
#include <panic.h>
#include <pio.h>
#include <pio_monitor.h>
#include <pio_common.h>
#include <stdlib.h>
#include <logging.h>
#include <pl_timers/pl_timers.h>

#include "adk_log.h"
#include "touchpad_iqs269a.h"
#include "touch.h"
#include "touch_config.h"
#include "proximity.h"

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(touch_sensor_messages)
ASSERT_MESSAGE_GROUP_NOT_OVERFLOWED(TOUCH, TOUCH_SENSOR_MESSAGE_END)

const struct __touch_config_t touch_config = {
    .i2c_addr = IQS269A_I2C_ADDRESS,
    .i2c_clock_khz = 100,
    .pios = {
        /* The touch PIO definitions are defined in the platform x2p file */
        .i2c_scl = RDP_PIO_I2C_SCL,
        .i2c_sda = RDP_PIO_I2C_SDA,
        .rdy = RDP_IQS269A_PIO_RDY
    },
};

/*!< Task information for touch pad */
touchTaskData app_touch;

#ifdef INCLUDE_PROXIMITY
#ifdef HAVE_PROXIMITY_IQS269A

LOGGING_PRESERVE_MESSAGE_ENUM(proximity_messages)
/*!< Task information for proximity sensor */
proximityTaskData app_proximity;

#endif
#endif

TouchFlags tf = {
    .touchFlag = FALSE,
    .bsHandleCloseNeeded = TRUE,
    .dumpFlag = FALSE,
    .suppressSlide = FALSE,
    .lastEventTouch = FALSE,
    .wearFlag = FALSE,
    .proximityRegisted = FALSE,
    .enableWearProxEvent = TRUE,
    .enableWearTouchEvent = TRUE,
    .suppressEvents = FALSE,
	.resetAfterInit = TRUE
};


TouchVariables tv = {
    .bs_handle = BITSERIAL_HANDLE_ERROR,
    /* Thresholds of channels for wearing detection points */
    .wearProxThres = IQS269A_PROX_THRESHOLD_CH4,
    .wearTouchThres = IQS269A_TOUCH_THRESHOLD_CH4
};

static TouchPadVariable tpv[4] = {
    {IQS269A_REGISTER_CH0, 0x00},
    {IQS269A_REGISTER_CH1, 0x00},
    {IQS269A_REGISTER_CH2, 0x00},
    {IQS269A_REGISTER_CH3, 0x00},
};

/*! \brief Cancel all timers in use; used when resetting or in error recovery */
static void touchIqs269a_CancelTimers(void){
    touchTaskData *touch = &app_touch;
    /* cancel the held timer if receiving any touch event*/
    MessageCancelAll(&touch->task, TOUCH_INTERNAL_HELD_CANCEL_TIMER);
    MessageCancelAll(&touch->task, TOUCH_INTERNAL_HELD_TIMER);
    MessageCancelAll(&touch->task, TOUCH_INTERNAL_CLICK_TIMER);
}

/*! \brief Send action to anyone registered

    Check for registrants first to save message allocation if not needed.
*/
static void touchIqs269a_SendActionMessage(touch_action_t action)
{
    DEBUG_LOG_INFO("IQS269A Send Action enum:touch_action_t:0x%04x", action);
    if (TaskList_Size(TaskList_GetFlexibleBaseTaskList(TouchSensor_GetActionClientTasks())))
    {
        MESSAGE_MAKE(message, TOUCH_SENSOR_ACTION_T);

        message->action = action;
        TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(TouchSensor_GetActionClientTasks()),
                             TOUCH_SENSOR_ACTION, message);
    }
}

static bool touchIqs269a_MapAndSendEvents(touch_action_t action, bool send_raw_regardless)
{
    touchTaskData *touch = &app_touch;
    unsigned i;

    for (i=0; i < touch->action_table_size; i++)
    {
        if (action == touch->action_table[i].action)
        {
            MessageId id = touch->action_table[i].message;

            DEBUG_LOG_VERBOSE("touchIqs269a_MapAndSendEvents action enum:touch_action_t:%d message: 0x%x", action, id);

            touchIqs269a_SendActionMessage(action);
            TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(TouchSensor_GetUiClientTasks()), id);
            return TRUE;
        }
    }

    if (send_raw_regardless)
    {
        touchIqs269a_SendActionMessage(action);
    }
    return FALSE;
}

static bool touchIqs269a_IfEventAvailable(void)
{
    touchTaskData *touch = &app_touch;
    uint16 count = 0;

    while(PioCommonGetPio(touch->config->pios.rdy))
    {
        if(count == touchConfigCheckAvailableCounts())
            return FALSE;
        count++;
        timer_n_us_delay(touchConfigWaitRdyUs());
    }
    return TRUE;
}

static void touchIqs269a_SetupI2c(const touchConfig *config, bool set_io, bool set_fn, bool set_tr)
{
    int i;

    pio_func i2c_pios[] = {{config->pios.i2c_scl, BITSERIAL_1_CLOCK_OUT},
                           {config->pios.i2c_scl, BITSERIAL_1_CLOCK_IN},
                           {config->pios.i2c_sda, BITSERIAL_1_DATA_OUT},
                           {config->pios.i2c_sda, BITSERIAL_1_DATA_IN}};
    int i2c_pio_size = ARRAY_DIM(i2c_pios);

    for (i = 0; i < i2c_pio_size; i++)
    {
        uint16 pio = i2c_pios[i].pio;
        uint16 bank = PioCommonPioBank(pio);
        uint32 mask = PioCommonPioMask(pio);

        if (set_io)
        {
            /* Setup I2C PIOs with strong pull-up */
            PioSetMapPins32Bank(bank, mask, 0);
            PioSetDir32Bank(bank, mask, 0);
            PioSet32Bank(bank, mask, mask);
            PioSetStrongBias32Bank(bank, mask, mask);
        }
        if (set_fn)
        {
            PioSetMapPins32Bank(bank, mask, 0);
            PioSetFunction(pio, i2c_pios[i].func);
        }
        if (set_tr)
        {
            PioSetMapPins32Bank(bank, mask, 0);
            PioSetFunction(pio, OTHER);
        }
    }
}

static void touchIqs269a_Enable(const touchConfig *config)
{
    uint16 bank = PioCommonPioBank(config->pios.rdy);
    uint16 mask = PioCommonPioMask(config->pios.rdy);

    DEBUG_LOG_VERBOSE("touchIqs269aEnable");

    /* Setup Interrupt PIO as input with weak pull up */
    PanicNotZero(PioSetMapPins32Bank(bank, mask, mask));
    PanicNotZero(PioSetDir32Bank(bank, mask, 0));

    touchIqs269a_SetupI2c(config, TRUE, FALSE, FALSE);
}

static bitserial_handle touchIqs269a_BitserialOpen(bitserial_block_index bs_index, const touchConfig *config)
{
    bitserial_config bsconfig;
    bitserial_handle handle;

    memset(&bsconfig, 0, sizeof(bsconfig));
    bsconfig.mode = BITSERIAL_MODE_I2C_MASTER;
    bsconfig.clock_frequency_khz = config->i2c_clock_khz;
    bsconfig.u.i2c_cfg.i2c_address = config->i2c_addr;

    handle = BitserialOpen(bs_index, &bsconfig);

    if(handle == BITSERIAL_HANDLE_ERROR)
    {
        DEBUG_LOG_INFO("touchIqs269aBitserial Open Error, Panic!");
        PanicFalse((handle != BITSERIAL_HANDLE_ERROR));
    }

    /* Set up the PIOs function after BitserialOpen to avoid glitches */
    touchIqs269a_SetupI2c(config, FALSE, TRUE, FALSE);

    return handle;
}

static void touchIqs269a_BitserialCloseIfNeeded(const touchConfig *config)
{
    if(tf.bsHandleCloseNeeded && tv.bs_handle != BITSERIAL_HANDLE_ERROR)
    {
        touchIqs269a_SetupI2c(config, FALSE, FALSE, TRUE);
        BitserialClose(tv.bs_handle);
        tv.bs_handle = BITSERIAL_HANDLE_ERROR;
    }
}

static bitserial_handle touchIqs269a_BitserialOpenIfNeeded(bitserial_block_index bs_index, const touchConfig *config)
{
    if(tv.bs_handle == BITSERIAL_HANDLE_ERROR)
    {
        tv.bs_handle = touchIqs269a_BitserialOpen(bs_index, config);
    }

    return tv.bs_handle;
}

static bool touchIqs269a_WriteData(uint8 reg, uint8 tx1, uint8 tx2)
{
    bitserial_result result = BITSERIAL_RESULT_INVAL;
    bitserial_handle handle;
    const touchConfig *config = &touch_config;
    uint8 tx_data[3];

    tx_data[0] = reg;
    tx_data[1] = tx1;
    tx_data[2] = tx2;

    do
    {
        handle = touchIqs269a_BitserialOpenIfNeeded(TOUCH_BITSERIAL_BLOCK, config);

        result = BitserialWrite(handle,
                                BITSERIAL_NO_MSG,
                                tx_data, 3,
                                BITSERIAL_FLAG_BLOCK);

        touchIqs269a_BitserialCloseIfNeeded(config);
    }while(result != BITSERIAL_RESULT_SUCCESS);

    if(result != BITSERIAL_RESULT_SUCCESS)
    {
        DEBUG_LOG_INFO("IQS269A Write %02x Failed %d", reg, result);
    }

    return (result == BITSERIAL_RESULT_SUCCESS);
}

static bitserial_result touchIqs269a_ReadData(uint8 reg, uint8 *val, uint16 size)
{
    bitserial_result result = BITSERIAL_RESULT_INVAL;
    bitserial_handle handle;
    const touchConfig *config = &touch_config;

    memset(val, 0, size);

    handle = touchIqs269a_BitserialOpenIfNeeded(TOUCH_BITSERIAL_BLOCK, config);

    result = BitserialTransfer(handle,
                               BITSERIAL_NO_MSG,
                               &reg,
                               1,
                               val,
                               size);
    touchIqs269a_BitserialCloseIfNeeded(config);

    return result;
}

static void touchIqs269a_SendResetCmd(void)
{
    bitserial_result result = BITSERIAL_RESULT_INVAL;

    result = touchIqs269a_WriteData(IQS269A_REGISTER_PMU, 0x00, IQS269A_GENERAL_I2C_RESET);

    if(!result)
    {
        DEBUG_LOG_INFO("touchIqs269a_SendResetCmd Failed");
    }
}

static void touchIqs269a_SetPowerSwitchingAndEventMode(bool enable, bool ackReset)
{
    uint8 gen = IQS269A_GENERAL_I2C_ONLY_SWIPE;
    uint8 pmu = enable ? IQS269A_GENERAL_PMU_SET: IQS269A_GENERAL_PMU_NO_SWITCH;
    if(ackReset)
    {
        gen |= IQS269A_GENERAL_I2C_ACK_RESET;
    }
    if (enable)
    {
        gen |= IQS269A_GENERAL_ENABLE_EVENTS;
    }
    /* Configure Auto Power Switching and Event Mode */
    touchIqs269a_WriteData(IQS269A_REGISTER_PMU, pmu, gen);
}

static bool touchIqs269a_SetFilterStrength(uint8 filter_str)
{
    /* Activate Channels */
    touchIqs269a_WriteData(IQS269A_REGISTER_CHS, IQS269A_ACTIVE_CHS, filter_str);
    return TRUE;
}

static bool touchIqs269a_SetSlideStrength(uint8 level)
{
    if(level > 3)
    {
        DEBUG_LOG_WARN("touchIqs269a_SetSlideStrength: Level cannot larger than 3");
        return FALSE;
    }

    /* Enable Tracking UI and Set Slide Strength */
    touchIqs269a_WriteData(IQS269A_REGISTER_REF, 0x00, IQS269A_TRACKING_UI | level);

    return TRUE;
}

static bool touchIqs269a_SetSlideTimeout(uint8 to)
{
    if(to == 0)
    {
        DEBUG_LOG_WARN("touchIqs269a_SetSlideTimeout: Timtout Cannot be 0");
        return FALSE;
    }
    touchIqs269a_WriteData(IQS269A_REGISTER_TAP, IQS269A_TAP_TIMEOUT, to);

    return TRUE;
}

static bool touchIqs269a_SetSlideCoordinateThreshold(uint8 co)
{
    if(co == 0)
    {
        DEBUG_LOG_WARN("touchIqs269a_SetSlideCoordinateThreshold: Timtout Cannot be 0");
        return FALSE;
    }
    touchIqs269a_WriteData(IQS269A_REGISTER_SWIPE, co, 0x00);

    return TRUE;
}

static bool touchIqs269a_SetTouchThresholdCH(uint8 channel, uint8 thres)
{
    uint8 reg;

    if(channel > MAX_TOUCH_CHANNEL)
    {
        DEBUG_LOG_VERBOSE("touchIqs269a_SetTouchThresholdCH: Channel cannot be larger than %d", MAX_TOUCH_CHANNEL);
        return FALSE;
    }

    if(thres == 0)
    {
        DEBUG_LOG_VERBOSE("touchIqs269a_SetTouchThresholdCH: Threshold cannot be 0");
        return FALSE;
    }

    if(tpv[channel].touchPadThres == thres)
    {
        return TRUE;
    }

    tpv[channel].touchPadThres = thres;
    reg = tpv[channel].reg;

    touchIqs269a_WriteData(reg+4, 0xFF, thres);

    return TRUE;
}

#ifdef INCLUDE_PROXIMITY
#ifdef HAVE_PROXIMITY_IQS269A
static void touchIqs269a_HandleProximity(uint8 proxChs)
{
    uint8 maskChs = proxChs & IQS269A_WEAR_CHANNEL_MASK;

    if(!tf.wearFlag && maskChs)
    {
        DEBUG_LOG_INFO("touchIqs269a_CheckWearStatus: IQS269A earbud is worn!");

        /* Inform clients */
        if(tf.proximityRegisted)
        {
            proximityTaskData *prox = ProximityGetTaskData();
            prox->state->proximity = proximity_state_in_proximity;
            TaskList_MessageSendId(prox->clients, PROXIMITY_MESSAGE_IN_PROXIMITY);
        }

        tf.wearFlag = TRUE;
    }
    if(tf.wearFlag && (maskChs == 0))
    {
        DEBUG_LOG_INFO("touchIqs269a_CheckWearStatus: IQS269A earbud isn't worn!");

        /* Inform clients */
        if(tf.proximityRegisted)
        {
            proximityTaskData *prox = ProximityGetTaskData();
            prox->state->proximity = proximity_state_not_in_proximity;
            TaskList_MessageSendId(prox->clients, PROXIMITY_MESSAGE_NOT_IN_PROXIMITY);
        }

        tf.wearFlag = FALSE;
    }
}

static bool touchIqs269a_ChangeWearProxThreshold(uint8 thres)
{
    uint8 reg;

    if(thres == 0)
    {
        DEBUG_LOG_VERBOSE("touchIqs269a_ChangeWearProxThreshold: Threshold cannot be 0");
        return FALSE;
    }

    if(tv.wearProxThres == thres)
    {
        return TRUE;
    }

    tv.wearProxThres = thres;

    reg = IQS269A_REGISTER_CH4;
    touchIqs269a_WriteData(reg+4, tv.wearProxThres, tv.wearTouchThres);

    reg = IQS269A_REGISTER_CH5;
    touchIqs269a_WriteData(reg+4, tv.wearProxThres, tv.wearTouchThres);

    return TRUE;
}
#endif
#endif

static void touchIqs269a_SetReportEvents(uint8 emask)
{
    touchIqs269a_WriteData(IQS269A_REGISTER_CHRESEED, 0x00, emask);
}

static void touchIqs269a_SetIfCloseBitserialHandler(bool flag)
{
    tf.bsHandleCloseNeeded = flag;
}

static void touchIqs269a_Initialize(uint8 sysflags)
{
    uint8 reg = 0;

    DEBUG_LOG_VERBOSE("touchIqs269aInitializing");
    touchIqs269a_SetIfCloseBitserialHandler(FALSE);
    /* If Reset Flag is 1, clear it */
    touchIqs269a_SetPowerSwitchingAndEventMode(FALSE, sysflags & IQS269A_SYS_RESET_MASK);

    touchIqs269a_SetFilterStrength(IQS269A_FILTER_STRENGTH);

    /* If the touchpad is "disabled", simply don't enable the event interrupts */
    if (tf.suppressEvents)
    {
        touchIqs269a_SetReportEvents(IQS269A_NO_EVENT_MASK);
    }
    else
    {
        touchIqs269a_SetReportEvents(IQS269A_EVENT_MASK);
    }

    /* Set timeouts */
    touchIqs269a_WriteData(IQS269A_REGISTER_REPORT, IQS269A_NP_REPORT_RATE, IQS269A_LP_REPORT_RATE);
    touchIqs269a_WriteData(IQS269A_REGISTER_ULP_REPORT, IQS269A_ULP_REPORT_RATE, IQS269A_POWER_MODE_TIMEOUT);
    touchIqs269a_WriteData(IQS269A_REGISTER_TIMEOUT, IQS269A_RDY_TIMEOUT, IQS269A_LTA_TIMEOUT);

    touchIqs269a_SetSlideStrength(IQS269A_SLIDE_STRENGTH);

    /* Set block ref channel */
    /* This ref channel blocks slider channels from reporting proximity and touch events*/
    touchIqs269a_WriteData(IQS269A_REGISTER_EVENT , IQS269A_REF_BLOCK_CHANNEL, 0x00);

    /* Settings for individual channels */
    /* CH0 */
    reg = IQS269A_REGISTER_CH0;
    touchIqs269a_WriteData(reg, IQS269A_CRX_RDP_CH0, IQS269A_TX_CH0);
    touchIqs269a_WriteData(reg+1, IQS269A_SENSING_SET1_CH0, IQS269A_SENSING_SET2_CH0);
    touchIqs269a_WriteData(reg+2, IQS269A_SENSING_SET3_CH0, IQS269A_ATI_SET_CH0);
    touchIqs269a_WriteData(reg+5, IQS269A_DEEP_TOUCH_THRES_CH0, IQS269A_HYSTERESIS_CH0);
    touchIqs269a_SetTouchThresholdCH(0, IQS269A_TOUCH_THRESHOLD_CH0);
    touchIqs269a_WriteData(reg+6, IQS269A_REF_CHANNEL_CH0, IQS269A_ASSOCIATE_WEIGHT_CH0);

    /* CH1 */
    reg = IQS269A_REGISTER_CH1;
    touchIqs269a_WriteData(reg, IQS269A_CRX_RDP_CH1, IQS269A_TX_CH1);
    touchIqs269a_WriteData(reg+1, IQS269A_SENSING_SET1_CH1, IQS269A_SENSING_SET2_CH1);
    touchIqs269a_WriteData(reg+2, IQS269A_SENSING_SET3_CH1, IQS269A_ATI_SET_CH1);
    touchIqs269a_WriteData(reg+5, IQS269A_DEEP_TOUCH_THRES_CH1, IQS269A_HYSTERESIS_CH1);
    touchIqs269a_SetTouchThresholdCH(1, IQS269A_TOUCH_THRESHOLD_CH1);
    touchIqs269a_WriteData(reg+6, IQS269A_REF_CHANNEL_CH1, IQS269A_ASSOCIATE_WEIGHT_CH1);

    /* CH2 */
    reg = IQS269A_REGISTER_CH2;
    touchIqs269a_WriteData(reg, IQS269A_CRX_RDP_CH2, IQS269A_TX_CH2);
    touchIqs269a_WriteData(reg+1, IQS269A_SENSING_SET1_CH2, IQS269A_SENSING_SET2_CH2);
    touchIqs269a_WriteData(reg+2, IQS269A_SENSING_SET3_CH2, IQS269A_ATI_SET_CH2);
    touchIqs269a_WriteData(reg+5, IQS269A_DEEP_TOUCH_THRES_CH2, IQS269A_HYSTERESIS_CH2);
    touchIqs269a_SetTouchThresholdCH(2, IQS269A_TOUCH_THRESHOLD_CH2);
    touchIqs269a_WriteData(reg+6, IQS269A_REF_CHANNEL_CH2, IQS269A_ASSOCIATE_WEIGHT_CH2);

    /* CH3 */
    reg = IQS269A_REGISTER_CH3;
    touchIqs269a_WriteData(reg, IQS269A_CRX_RDP_CH3, IQS269A_TX_CH3);
    touchIqs269a_WriteData(reg+1, IQS269A_SENSING_SET1_CH3, IQS269A_SENSING_SET2_CH3);
    touchIqs269a_WriteData(reg+2, IQS269A_SENSING_SET3_CH3, IQS269A_ATI_SET_CH3);
    touchIqs269a_WriteData(reg+5, IQS269A_DEEP_TOUCH_THRES_CH3, IQS269A_HYSTERESIS_CH3);
    touchIqs269a_SetTouchThresholdCH(3, IQS269A_TOUCH_THRESHOLD_CH3);
    touchIqs269a_WriteData(reg+6, IQS269A_REF_CHANNEL_CH3, IQS269A_ASSOCIATE_WEIGHT_CH3);

#ifdef INCLUDE_PROXIMITY
#ifdef HAVE_PROXIMITY_IQS269A
    /* CH4 */
    reg = IQS269A_REGISTER_CH4;
    touchIqs269a_WriteData(reg, IQS269A_CRX_RDP_CH4, IQS269A_TX_CH4);
    touchIqs269a_WriteData(reg+1, IQS269A_SENSING_SET1_CH4, IQS269A_SENSING_SET2_CH4);
    touchIqs269a_WriteData(reg+2, IQS269A_SENSING_SET3_CH4, IQS269A_ATI_SET_CH4);
    touchIqs269a_WriteData(reg+4, tv.wearProxThres, tv.wearTouchThres);
    touchIqs269a_WriteData(reg+5, IQS269A_DEEP_TOUCH_THRES_CH4, IQS269A_HYSTERESIS_CH4);
    touchIqs269a_WriteData(reg+6, IQS269A_REF_CHANNEL_CH4, IQS269A_ASSOCIATE_WEIGHT_CH4);

    /* CH5 */
    reg = IQS269A_REGISTER_CH5;
    touchIqs269a_WriteData(reg, IQS269A_CRX_RDP_CH5, IQS269A_TX_CH5);
    touchIqs269a_WriteData(reg+1, IQS269A_SENSING_SET1_CH5, IQS269A_SENSING_SET2_CH5);
    touchIqs269a_WriteData(reg+2, IQS269A_SENSING_SET3_CH5, IQS269A_ATI_SET_CH5);
    touchIqs269a_WriteData(reg+4, tv.wearProxThres, tv.wearTouchThres);
    touchIqs269a_WriteData(reg+5, IQS269A_DEEP_TOUCH_THRES_CH5, IQS269A_HYSTERESIS_CH5);
    touchIqs269a_WriteData(reg+6, IQS269A_REF_CHANNEL_CH5, IQS269A_ASSOCIATE_WEIGHT_CH5);

    touchIqs269a_ChangeWearProxThreshold(IQS269A_PROX_THRESHOLD_CH4);

    /* CH6 */
    reg = IQS269A_REGISTER_CH6;
    touchIqs269a_WriteData(reg, IQS269A_CRX_RDP_CH6, IQS269A_TX_CH6);
    touchIqs269a_WriteData(reg+1, IQS269A_SENSING_SET1_CH6, IQS269A_SENSING_SET2_CH6);
    touchIqs269a_WriteData(reg+2, IQS269A_SENSING_SET3_CH6, IQS269A_ATI_SET_CH6);
    touchIqs269a_WriteData(reg+4, IQS269A_PROX_THRESHOLD_CH6, IQS269A_TOUCH_THRESHOLD_CH6);
    touchIqs269a_WriteData(reg+5, IQS269A_DEEP_TOUCH_THRES_CH6, IQS269A_HYSTERESIS_CH6);
    touchIqs269a_WriteData(reg+6, IQS269A_REF_CHANNEL_CH6, IQS269A_ASSOCIATE_WEIGHT_CH6);
#endif
#endif

    /* CH7 */
    reg = IQS269A_REGISTER_CH7;
    touchIqs269a_WriteData(reg, IQS269A_CRX_RDP_CH7, IQS269A_TX_CH7);
    touchIqs269a_WriteData(reg+1, IQS269A_SENSING_SET1_CH7, IQS269A_SENSING_SET2_CH7);
    touchIqs269a_WriteData(reg+2, IQS269A_SENSING_SET3_CH7, IQS269A_ATI_SET_CH7);
    touchIqs269a_WriteData(reg+5, IQS269A_DEEP_TOUCH_THRES_CH7, IQS269A_HYSTERESIS_CH7);
    touchIqs269a_WriteData(reg+4, IQS269A_PROX_THRESHOLD_CH7, IQS269A_TOUCH_THRESHOLD_CH7);
    touchIqs269a_WriteData(reg+6, IQS269A_REF_CHANNEL_CH7, IQS269A_ASSOCIATE_WEIGHT_CH7);

    /* Set slider config */
    /* Add Ch 1->3 to slider 0 */
    touchIqs269a_WriteData(IQS269A_REGISTER_SLIDERS, IQS269A_SLIDER0_CHS, IQS269A_SLIDER1_CHS);
    touchIqs269a_SetSlideTimeout(IQS269A_SWIPE_TIMEOUT);
    touchIqs269a_SetSlideCoordinateThreshold(IQS269A_SWIPE_CO_THRESHOLD);

    /* Let channels do ATI */
    touchIqs269a_WriteData(IQS269A_REGISTER_PMU, 0x00, IQS269A_GENERAL_I2C_REDO_ATI);
    touchIqs269a_WriteData(IQS269A_REGISTER_SWIPE, IQS269A_SWIPE_CO_THRESHOLD, IQS269A_RE_ATI_CHANNELS);

    /* We need to set the "Close" flag here so that the *next* bitserial transaction will close it after the transfer. */
    touchIqs269a_SetIfCloseBitserialHandler(TRUE);

    /* Enable sensor fully */
    touchIqs269a_SetPowerSwitchingAndEventMode(TRUE, FALSE);
}

static void touchIqs269a_HandleSwipe(uint8 swipeFlag)
{
    touchTaskData *touch = &app_touch;

    /*! Only Handle Swipe Up and Down */
    if(tf.touchFlag && (swipeFlag & IQS269A_SWIPE_UP_DOWN_MASK))
    {
        touchIqs269a_CancelTimers();
        DEBUG_LOG_INFO("IQS269A Slider Flag %02x", swipeFlag);

        if(touch->number_of_press > 1)
        {
            DEBUG_LOG_INFO("IQS269A HandleSwipe: Tap larger than 1");
        }
        else
        {
            touch_action_t action;

            if(touch->number_of_press == 1)
                action = TAP_SLIDE_UP;
            else
                action = SLIDE_UP;

            if(swipeFlag & IQS269A_SWIPE_DOWN_MASK)
            {
                if(touch->number_of_press == 1)
                    action = TAP_SLIDE_DOWN;
                else
                    action = SLIDE_DOWN;
            }
            touchIqs269a_MapAndSendEvents(action, TRUE);
        }
        touch->number_of_press = 0;
        touch->number_of_seconds_held = 0;
        tf.lastEventTouch = FALSE;
        tf.suppressSlide = TRUE;
    }
}

static void touchIqs269a_SendTouchEvent(touch_action_t action)
{
    touchTaskData *touch = &app_touch;

    touchIqs269a_CancelTimers();

    touchIqs269a_MapAndSendEvents(action, TRUE);

    if(action == HAND_COVER)
    {
        MessageSendLater(&touch->task, TOUCH_INTERNAL_HELD_CANCEL_TIMER, NULL, touchConfigPressCancelMs());
        tf.touchFlag = TRUE;
    }
    else
    {
        if(tf.lastEventTouch)
        {
            MessageSend(&touch->task, TOUCH_INTERNAL_HELD_RELEASE, NULL);
        }
        tf.touchFlag = FALSE;
    }
    tf.lastEventTouch = TRUE;
    tf.suppressSlide = FALSE;
}

static void touchIqs269a_HandleTouches(uint8 touchChs)
{
    DEBUG_LOG_INFO("IQS269A Touch %02x", touchChs);

    if(!tf.touchFlag && (touchChs & IQS269A_TOUCH_CHANNEL_MASK))
    {
        touchIqs269a_SendTouchEvent(HAND_COVER);
    }
    if(tf.touchFlag && ((touchChs & IQS269A_TOUCH_CHANNEL_MASK) == 0))
    {
        touchIqs269a_SendTouchEvent(HAND_COVER_RELEASE);
    }
}

static void touchIqs269a_DumpMemoryMap(void)
{
    uint8 Reg[2] = {0};
    uint8 i = 0;

    i = 0x80;
    while(i<0xC4)
    {
        touchIqs269a_ReadData(i, Reg, 2);
        DEBUG_LOG_INFO("RW Reg 0x%02x, val[0:1] %02x:%02x", i, Reg[0], Reg[1]);
        i = i + 1;
    }

    i = 0x36;
    while(i>0x01)
    {
        touchIqs269a_ReadData(i, Reg, 2);
        DEBUG_LOG_INFO("RO Reg 0x%02x, val[0:1] %02x:%02x", i, Reg[0], Reg[1]);
        i = i - 1;
    }

    PanicZero(0);
}

static void touchIqs269a_DoDumpMemoryMap(bool flag)
{
    tf.dumpFlag = flag;
}

static void touchIqs269a_HandlePowerMode(uint8 flags)
{
    uint8 pm = (flags & IQS269A_POWER_MODE_CHANGE_MASK) >> 3;
    switch(pm)
    {
        case NORMAL_POWER:
            DEBUG_LOG_INFO("IQS269A Power Mode Normal Power");
            break;
        case LOW_POWER:
            DEBUG_LOG_INFO("IQS269A Power Mode Low Power");
            break;
        case ULOW_POWER:
            DEBUG_LOG_INFO("IQS269A Power Mode Ultra Low Power");
            break;
        default:
            break;
    }
}

static void touchIqs269a_ReadEvent(bool ifPio)
{
    touchTaskData *touch = &app_touch;
    bitserial_result result = BITSERIAL_RESULT_INVAL;
    uint8 val[9] = {0};

    if(!touchIqs269a_IfEventAvailable())
    {
        /* Terminate reading event function when there is no event, no matter what power mode it is */
        /* Let interrupt handler work */
        return;
    }

    if(ifPio)
    {
        DEBUG_LOG_VERBOSE("touchIqs269a_ReadEvent triggered by PIO");
    }

    if(tf.dumpFlag)
        touchIqs269a_DumpMemoryMap();

    result = touchIqs269a_ReadData(IQS269A_REGISTER_SYS, val, 9);

    if(result == BITSERIAL_RESULT_SUCCESS)
    {
        uint8 sysflags = val[0];
        uint8 flags = val[1];
        uint8 swipeflags = val[2];
        uint8 touchChannels = val[6];

        DEBUG_LOG_VERBOSE("IQS269A ReadEvent: 0:0x%02x, 1:0x%02x, 2:0x%02x, 3:0x%02x, 4:0x%02x, 5:0x%02x, 6:0x%02x, 7:0x%02x, 8:0x%02x",
                          val[0],val[1],val[2],val[3],val[4],val[5],val[6],val[7],val[8]);

        /* When sensor sends reset flag, it's the highest priority flag */
        if(sysflags & IQS269A_SYS_RESET_MASK)
        {
            /* Cancel everything before doing reset */
            touchIqs269a_CancelTimers();
            touch->number_of_seconds_held = 0;
            touch->number_of_press = 0;

            DEBUG_LOG_INFO("IQS269A Sensor Reset");
            touchIqs269a_Initialize(sysflags);
            if(tf.resetAfterInit)
            {
                MessageSendLater(&touch->task, TOUCH_RESET_SENSOR, NULL, 0);
                tf.resetAfterInit = FALSE;
            }
        }
        else if (sysflags & IQS269A_SYS_EVENT_MASK)
        {
            DEBUG_LOG_DEBUG("IQS269A sysflags %02x flags %02x swipe %02x touch %02x", sysflags, flags, swipeflags, touchChannels);
            /* One event report can invole many events */
            /* Check for gesture */
            if((flags & IQS269A_EVENT_GESTURE_MASK))
            {
                /* Only handle one swipe between each touch and release */
                if(!tf.suppressSlide)
                    touchIqs269a_HandleSwipe(swipeflags);
            }
            /* Check for Touch */
            if((flags & IQS269A_EVENT_TOUCH_MASK))
            {
                touchIqs269a_HandleTouches(touchChannels);
            }
#ifdef INCLUDE_PROXIMITY
#ifdef HAVE_PROXIMITY_IQS269A
            /* Prox */
            if((flags & IQS269A_EVENT_PROX_MASK))
            {
                uint8 proxChannels = val[4];
                touchIqs269a_HandleProximity(proxChannels);
            }
#endif
#endif
            /* Power switching mode might be impacted when settings are changed */
            /* Leave here for debugging */
            if(flags & IQS269A_EVENT_POWER_MODE_MASK)
            {
                touchIqs269a_HandlePowerMode(sysflags);
            }
        }
    }
    else
    {
        DEBUG_LOG_WARN("IQS269A Read Failed %d", result);
    }
}

static void touchIqs269a_Reset(bool hold)
{
    touchTaskData *touch = &app_touch;

    DEBUG_LOG_INFO("touchIqs269a_Reset: Do reset with flag %d", hold);

    touchIqs269a_CancelTimers();

    touch->number_of_press = 0;
    touch->number_of_seconds_held = 0;

    tf.touchFlag = FALSE;
    tf.suppressEvents = hold;

    /* Reset the sensor hardware */
    touchIqs269a_SendResetCmd();
    
    /* The sensor will tell us when it has completed its reset. */
}

static void touchIqs269a_MapTouchTimerToLogicalInput(MessageId id, uint8 press, uint8 data)
{
    touch_action_t touch_ui_input = MAX_ACTION;
    bool found = FALSE;

    /* convert timer count to touch event map*/
    if (id == TOUCH_INTERNAL_HELD_TIMER)
    {
        /* press hold handling */
        switch (press)
        {
            case SINGLE_PRESS:
                touch_ui_input = data + TOUCH_PRESS_HOLD_OFFSET;
                break;
            case DOUBLE_PRESS:
                touch_ui_input = data + TOUCH_DOUBLE_PRESS_HOLD_OFFSET;
                break;
            /* only handle up to double press hold for now */
            default:
                return;
        }

    }
    else if (id == TOUCH_INTERNAL_HELD_RELEASE)
    {
        /* hold release handling */
        switch (press)
        {
            case SINGLE_PRESS:
                touch_ui_input = (touch_action_t) data + TOUCH_PRESS_RELEASE_OFFSET;
                break;
            case DOUBLE_PRESS:
                touch_ui_input = (touch_action_t) data + TOUCH_DOUBLE_PRESS_HOLD_RELEASE_OFFSET;
                break;
            /* hold release only handle up to double press for now */
            default:
                return;
        }
    }
    else if (id == TOUCH_INTERNAL_CLICK_TIMER)
    {
        /* quick press handling */
        if (press >= MAX_PRESS_SUPPORT)
        {
            return;
        }
        /* cast up the type */
        touch_ui_input = (touch_action_t) press;
    }

    if (touch_ui_input != MAX_ACTION)
    {
        /* try to match input action with UI message to be broadcasted*/
        DEBUG_LOG_DEBUG("Send Event 0x%02x", touch_ui_input);
        found = touchIqs269a_MapAndSendEvents(touch_ui_input, TRUE);

        /* for release event, find the closest release UI event, if not found the exact release timer UI event  */
        if (id == TOUCH_INTERNAL_HELD_RELEASE)
        {
            while (touch_ui_input && touch_ui_input > TOUCH_PRESS_RELEASE_OFFSET && !found)
            {
                touch_ui_input --;

                /* This is a generated action. Only send the action if we send a UI event */
                found = touchIqs269a_MapAndSendEvents(touch_ui_input, FALSE);
            }
        }
    }
}

static void touchIqs269a_MessageHandler(Task task, MessageId id, Message msg)
{
    touchTaskData *touch = (touchTaskData *) task;
    const MessagePioChanged *mpc = (const MessagePioChanged *)msg;
    bool pio_set;
    const touchConfig *config = &touch_config;

    UNUSED(msg);
    UNUSED(touch);

    switch(id)
    {
        case MESSAGE_PIO_CHANGED:
            if (PioMonitorIsPioInMessage(mpc, config->pios.rdy, &pio_set))
            {
                if (!pio_set)
                {
                    touchIqs269a_ReadEvent(TRUE);
                }
            }
            break;
        case TOUCH_INTERNAL_HELD_CANCEL_TIMER:
            /* This time out is to prevent accidental very quick touch, if this timer expired then a touch is counted */
            touch->number_of_seconds_held = 0;
            MessageSendLater(&touch->task, TOUCH_INTERNAL_HELD_TIMER, NULL, D_SEC(1) - touchConfigPressCancelMs());
            break;
        case TOUCH_INTERNAL_HELD_TIMER:
            /* send notification if we have button held subscription before increasing the counter*/
            touch->number_of_seconds_held++;
            DEBUG_LOG_DEBUG("Touch %u held %u seconds", touch->number_of_press+1, touch->number_of_seconds_held);
            touchIqs269a_MapTouchTimerToLogicalInput(id, touch->number_of_press + 1, touch->number_of_seconds_held);
            /* to recover from a loss of release event */
            if (touch->number_of_seconds_held <= touchConfigMaximumHeldTimeSeconds())
            {
                MessageSendLater(&touch->task, TOUCH_INTERNAL_HELD_TIMER, NULL, D_SEC(1));
            }
            else
            {
                touch->number_of_seconds_held = 0;
                touchIqs269a_Reset(tf.suppressEvents);
            }
            break;
        case TOUCH_INTERNAL_HELD_RELEASE:
            /* send notification if we have held release subscription then reset the counter*/
            MessageSendLater(&touch->task, TOUCH_INTERNAL_CLICK_TIMER, NULL, touchConfigClickTimeoutlMs());
            if (touch->number_of_seconds_held > 0)
            {
                /* long press release */
                DEBUG_LOG_VERBOSE("Touch %u held release %u seconds", touch->number_of_press, touch->number_of_seconds_held);
                touchIqs269a_MapTouchTimerToLogicalInput(id, touch->number_of_press + 1, touch->number_of_seconds_held);
                touch->number_of_press = 0;
            }
            else
            {
                /* quick press release*/
                touch->number_of_press++;
                DEBUG_LOG_VERBOSE("Quick press %u", touch->number_of_press);
            }
            touch->number_of_seconds_held = 0;
            break;
        case TOUCH_INTERNAL_CLICK_TIMER:
            /* if this is expired, meaning the quick click hasn't been cancelled, can send multi click event*/
            if (touch->number_of_press)
            {
                DEBUG_LOG_VERBOSE("Quick press release %u", touch->number_of_press);
                touchIqs269a_MapTouchTimerToLogicalInput(TOUCH_INTERNAL_CLICK_TIMER, touch->number_of_press, 0);
                touch->number_of_press = 0;
            }
            break;
        case TOUCH_RESET_SENSOR:
            touchIqs269a_SendResetCmd();
            break;
        default:
            break;
    }
}

static bool touchIqs269a_GetIfCloseBitserialHandler(void)
{
    return tf.bsHandleCloseNeeded;
}

static void touchIqs269a_StartIfNeeded(void)
{
    touchTaskData *touch = &app_touch;
    const touchConfig *config = &touch_config;

    if (!touch->config)
    {
        tf.resetAfterInit = TRUE;
        touchIqs269a_DoDumpMemoryMap(FALSE);
        touchIqs269a_Enable(config);
        touchIqs269a_SetIfCloseBitserialHandler(TRUE);
        if(touchIqs269a_GetIfCloseBitserialHandler())
        {
            DEBUG_LOG_VERBOSE("IQS269A close bitserial handler TRUE");
        }
        else
        {
            DEBUG_LOG_VERBOSE("IQS269A close bitserial handler FALSE");
        }

        touch->task.handler = touchIqs269a_MessageHandler;

        touch->config = config;
        PioMonitorRegisterTask(&touch->task, touch->config->pios.rdy);

        touch->number_of_press = 0;
        touch->number_of_seconds_held = 0;
        touchIqs269a_Reset(TRUE);
    }
}


static void touchIqs269a_StopIfNeeded(void)
{
    touchTaskData *touch = &app_touch;

    if (touch->config)
    {
        uint16 ui_clients = TaskList_Size(TaskList_GetFlexibleBaseTaskList(TouchSensor_GetUiClientTasks()));
        uint16 action_clients = TaskList_Size(TaskList_GetFlexibleBaseTaskList(TouchSensor_GetActionClientTasks()));

        if (!ui_clients && !action_clients)
        {
            PioMonitorUnregisterTask(&touch->task, touch->config->pios.rdy);
            touch->config = NULL;

            if (!ui_clients)
            {
                touch->action_table = NULL;
                touch->action_table_size = 0;
            }
        }
    }
}

bool TouchSensor_Init(Task init_task)
{
    UNUSED(init_task);

    TaskList_InitialiseWithCapacity(TouchSensor_GetUiClientTasks(),
                                    TOUCH_CLIENTS_INITIAL_CAPACITY);
    TaskList_InitialiseWithCapacity(TouchSensor_GetActionClientTasks(),
                                    TOUCH_CLIENTS_INITIAL_CAPACITY);

    return TRUE;
}

bool TouchSensorClientRegister(Task task, uint32 size_action_table, const touch_event_config_t *action_table)
{
    touchTaskData *touch = &app_touch;
    bool ret = FALSE;

    ret =  TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(TouchSensor_GetUiClientTasks()), task);

    if(!ret)
        return ret;

    /* update action table*/
    if (size_action_table && action_table != NULL)
    {
        touch->action_table = action_table;
        touch->action_table_size = size_action_table;
    }

    touchIqs269a_StartIfNeeded();

    return ret;
}

void TouchSensorClientUnRegister(Task task)
{
    TaskList_RemoveTask(TaskList_GetFlexibleBaseTaskList(TouchSensor_GetUiClientTasks()), task);

    touchIqs269a_StopIfNeeded();
}

bool TouchSensorActionClientRegister(Task task)
{
    bool ret = FALSE;

    ret = TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(TouchSensor_GetActionClientTasks()), task);

    if(!ret)
        return ret;

    touchIqs269a_StartIfNeeded();

    return ret;
}

void TouchSensorActionClientUnRegister(Task task)
{
    TaskList_RemoveTask(TaskList_GetFlexibleBaseTaskList(TouchSensor_GetActionClientTasks()), task);

    touchIqs269a_StopIfNeeded();
}

/* if hold = TRUE, then reset the sensor but do not allow events to be generated */
bool TouchSensor_Reset(bool hold)
{
    touchTaskData *touch = &app_touch;
    DEBUG_LOG_INFO("TouchSensor_Reset(); config? 0x%p, disable events? %d", touch->config, hold);
    if (touch->config) {
        touchIqs269a_Reset(hold);
        return TRUE;
    }
    return FALSE;
}

bool AppTouchSensorGetDormantConfigureKeyValue(dormant_config_key *key, uint32* value)
{
    /* The rdy PIO will already be a strong pull input when there is no touch event.
     * When we enter dormant we need to be woken when this line goes low.
     * The touchpad uses active low interrupt logic */

    *key = PIO_WAKE_INVERT_MASK;
    *value = 1 << touch_config.pios.rdy;

    return TRUE;
}

#ifdef INCLUDE_PROXIMITY
#ifdef HAVE_PROXIMITY_IQS269A
bool appProximityClientRegister(Task task)
{
    proximityTaskData *prox = ProximityGetTaskData();

    if (NULL == prox->clients)
    {
        prox->config = NULL;
        prox->state = PanicUnlessNew(proximityState);
        prox->state->proximity = proximity_state_unknown;
        prox->clients = TaskList_Create();

        tf.proximityRegisted = TRUE;
    }

    if(tf.wearFlag)
    {
        DEBUG_LOG_INFO("IQS269A Proximity In");
        prox->state->proximity = proximity_state_in_proximity;
        MessageSend(task, PROXIMITY_MESSAGE_IN_PROXIMITY, NULL);
    }
    else
    {
        DEBUG_LOG_INFO("IQS269A Proximity Not In");
        prox->state->proximity = proximity_state_not_in_proximity;
        MessageSend(task, PROXIMITY_MESSAGE_NOT_IN_PROXIMITY, NULL);
    }

    DEBUG_LOG_VERBOSE("Proximity Client registered");

    return TaskList_AddTask(prox->clients, task);
}

void appProximityClientUnregister(Task task)
{
    proximityTaskData *prox = ProximityGetTaskData();
    TaskList_RemoveTask(prox->clients, task);
    if (0 == TaskList_Size(prox->clients))
    {
        TaskList_Destroy(prox->clients);
        prox->clients = NULL;
        free(prox->state);
        prox->state = NULL;

        tf.proximityRegisted = FALSE;
    }
}
/* This function is to switch on/off sensor for power saving*/
void appProximityEnableSensor(Task task, bool enable)
{
    UNUSED(task);
    UNUSED(enable);
}
#endif /* HAVE_PROXIMITY_IQS269A */
#endif /* INCLUDE_PROXIMITY */

#endif /* HAVE_TOUCHPAD_IQS269A */
#endif /* INCLUDE_CAPSENSE*/
