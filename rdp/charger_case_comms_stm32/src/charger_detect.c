/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief     USB Charger detection for STM32F0xx devices 
*/

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

#include "main.h"
#include "charger_detect.h"
#include "usb.h"
#include "timer.h"
#ifdef TEST
#include "test_st.h"
#endif

/*-----------------------------------------------------------------------------
------------------ PREPROCESSOR DEFINITIONS -----------------------------------
-----------------------------------------------------------------------------*/

#define CHARGER_DETECT_DCD_TIMEOUT_US (600 * 1000) 
#define CHARGER_DETECT_DCD_REREAD_US (20 * 1000)
#define CHARGER_DETECT_MODE_CHANGE_US (50 * 1000)

/*-----------------------------------------------------------------------------
------------------ TYPE DEFINITIONS -------------------------------------------
-----------------------------------------------------------------------------*/

typedef enum
{
    CHARGER_DETECT_IDLE,
    CHARGER_DETECT_START,
    CHARGER_DETECT_DCD,
    CHARGER_DETECT_DCD_REREAD,
    CHARGER_DETECT_START_PRIMARY_DETECTION,
    CHARGER_DETECT_PRIMARY_DETECTION,
    CHARGER_DETECT_START_SECONDARY_DETECTION,
    CHARGER_DETECT_SECONDARY_DETECTION,
    CHARGER_DETECT_FINISH,
}
CHARGER_DETECT_STATE;

/*-----------------------------------------------------------------------------
------------------ VARIABLES --------------------------------------------------
-----------------------------------------------------------------------------*/

/** The current state of the charger detection state machine */
static CHARGER_DETECT_STATE charger_detect_state;            

/** Time to wait till we should enter next state */
static uint64_t charger_detect_delay_time; 

/** The last charger detection result */
static CHARGER_DETECT_TYPE charger_detect_type;

/*-----------------------------------------------------------------------------
------------------ FUNCTIONS --------------------------------------------------
-----------------------------------------------------------------------------*/

/**
 * \brief Helper function to check if a charger detection delay has finished
 * \return True if the delay has elapsed, false if it is still ongoing
 */
static bool charger_detect_delay_finished(void)
{
    return global_time_us >= charger_detect_delay_time ;
}

/**
 * \brief Enter the next state in charger detection
 * \param state The state to advance to.
 * \param us_to_wait Number of microseconds to wait till next state.
 * advancing to the next state.
 */
static void charger_detect_next_state(CHARGER_DETECT_STATE state,
                                      uint64_t us_to_wait)
{
    charger_detect_state = state;
    charger_detect_delay_time = global_time_us + us_to_wait; 
}

void charger_detect_start(void)
{
    usb_activate_bcd();
    charger_detect_next_state(CHARGER_DETECT_START, 0);
}

void charger_detect_cancel(void)
{
    usb_deactivate_bcd();
    charger_detect_next_state(CHARGER_DETECT_IDLE, 0);
}

void charger_detect_periodic(void)
{
    switch (charger_detect_state)
    {
        case CHARGER_DETECT_IDLE:
            break;

        case CHARGER_DETECT_START:
            charger_detect_type = CHARGER_DETECT_TYPE_PENDING;
            charger_detect_next_state(CHARGER_DETECT_DCD,
                                      CHARGER_DETECT_DCD_TIMEOUT_US);
            break;

        case CHARGER_DETECT_DCD:
            /* Wait for DCD to complete or timeout */
            if (charger_detect_delay_finished() || usb_dcd())
            {
                charger_detect_next_state(CHARGER_DETECT_DCD_REREAD,
                                          CHARGER_DETECT_DCD_REREAD_US);
            }
            break;
        case CHARGER_DETECT_DCD_REREAD:
            if (charger_detect_delay_finished())
            {
                /* Reread the DCD result just in case it changed */
                if (usb_dcd())
                {
                    charger_detect_next_state(CHARGER_DETECT_START_PRIMARY_DETECTION,
                                              CHARGER_DETECT_MODE_CHANGE_US);
                }
                else
                {
                    /* DCD failed, we're conneted to a wall charger */
                    charger_detect_type = CHARGER_DETECT_TYPE_FLOATING;
                    charger_detect_next_state(CHARGER_DETECT_FINISH, 0);
                }
                usb_dcd_disable();
            }
            break;
        case CHARGER_DETECT_START_PRIMARY_DETECTION:
            /* Start primary detection and wait */
            if (charger_detect_delay_finished())
            {
                usb_primary_detection_enable();
                charger_detect_next_state(CHARGER_DETECT_PRIMARY_DETECTION,
                                          CHARGER_DETECT_MODE_CHANGE_US);
            }
            break;
        case CHARGER_DETECT_PRIMARY_DETECTION:
            if (charger_detect_delay_finished())
            {
              /* Check the result of primary detection. If it succeeded, move
               * onto secondary detection, otherwise we have a SDP.
               */
              if (usb_pdet())
              {
                  charger_detect_next_state(CHARGER_DETECT_START_SECONDARY_DETECTION,
                                            CHARGER_DETECT_MODE_CHANGE_US);
              }
              else
              {
                  /* SDP detected */
                  charger_detect_type = CHARGER_DETECT_TYPE_SDP;
                  charger_detect_next_state(CHARGER_DETECT_FINISH, 0);
              }
              usb_primary_detection_disable();
            }
            break;
        case CHARGER_DETECT_START_SECONDARY_DETECTION:
            /* Start secondary detection and wait */
            if (charger_detect_delay_finished())
            {
                usb_secondary_detection_enable();
                charger_detect_next_state(CHARGER_DETECT_SECONDARY_DETECTION,
                                          CHARGER_DETECT_MODE_CHANGE_US);

            }
            break;
        case CHARGER_DETECT_SECONDARY_DETECTION:
            if (charger_detect_delay_finished())
            {
                if (usb_sdet())
                {
                    /* DCP detected */
                    charger_detect_type = CHARGER_DETECT_TYPE_DCP;
                }
                else
                {
                    /* We have a CDP? */
                    charger_detect_type = CHARGER_DETECT_TYPE_CDP;
                }
                charger_detect_next_state(CHARGER_DETECT_FINISH, 0);
            }
            break;
        case CHARGER_DETECT_FINISH:
            usb_deactivate_bcd();
            charger_detect_next_state(CHARGER_DETECT_IDLE, 0);
            usb_start();
            break;
        default:
            break;
    }
}

CHARGER_DETECT_TYPE charger_detect_get_type(void)
{
    return charger_detect_type;
}
