/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 * Sensor Hub for P1
*/

#include "sensor_hub/sensor_hub.h"
#include "bitserial/bitserial.h"
#include "pmalloc/pmalloc.h"
#include "panic/panic.h"
#include "int/int.h"
#include "hal/halauxio.h"
#include "FreeRTOS-Kernel/include/FreeRTOS.h"
/*lint -save -e963 */
#include "FreeRTOS-Kernel/include/task.h"
/*lint -restore */
#include "FreeRTOS-Kernel/include/semphr.h"


#include <message.h>
#include <pio.h>
#include <bitserial_api.h>
#include <assert.h>

/*! This is enum of all sensor states.  Note: ordering of enum important */
typedef enum
{
    SENSOR_STATE_DETACHED,      /*!< Sensor isn't attached to a bus */
    SENSOR_STATE_OFF,           /*!< Sensor is attached but off */
    SENSOR_STATE_POWERING_ON,   /*!< Sensor is powering on */
    SENSOR_STATE_ON,            /*!< Sensor is on (from this enum onwards, the sensor is assumed to be on) */
    SENSOR_STATE_POWERING_OFF,  /*!< Sensor is power off */
    SENSOR_STATE_SCHEDULED,     /*!< Sensor read has been scheduled */
    SENSOR_STATE_TRANSFER,      /*!< Sensor transfer has started */
    SENSOR_STATE_PROCESS        /*!< Sensor is processing raw data */
} sensor_state_t;

/*! This is enum of all sensor timing states */
typedef enum
{
    SENSOR_TIME_STATE_INVALID,    /*!< Sensor time is not valid */
    SENSOR_TIME_STATE_VALID,      /*!< Sensor time is a valid time */
    SENSOR_TIME_STATE_TRIGGERED,  /*!< Sensor time has triggered handler */
    SENSOR_TIME_STATE_USED        /*!< Sensor time is a valid time but it has already been used */
} sensor_time_state_t;

/*! This is enum of all sensor timing modes */
typedef enum
{
    SENSOR_TIME_MODE_MANUAL,     /*!< Timing is controlled manually from sensor itself */
    SENSOR_TIME_MODE_PERIODIC,   /*!< Timing is periodic */
    SENSOR_TIME_MODE_RELATIVE    /*!< Timing is relative to another sensor */
} sensor_time_mode_t;

/*! Sensor FIFO structure, used to store processed sensor data */
typedef struct
{
    uint8 index;                /*!< FIFO index. Note: Do not bitfield, atomic read/write required */
    uint8 outdex;               /*!< FIFO outdex. Note: Do not bitfield, atomic read/write required */
    unsigned full:1;            /*!< Set if FIFO is full */
    unsigned size:7;            /*!< Maximum numnber of elements in the FIFO */
    unsigned element_size:8;    /*!< Size in octets of each FIFO element */
    uint8 elements[1];          /*!< Start of FIFO element storage (appropriate additional memory will be allocated to meet storage requirements) */
} sensor_fifo_t;

/*! Sensor timing structure, useds to hold all sensor timing state and information */
typedef struct
{
    sensor_time_state_t state:3;    /*!< Next read time is valid */
    sensor_time_mode_t mode:3;      /*!< Timing mode, periodic, relative, manual */
    unsigned immediate:1;           /*!< Immediate read required */
    uint32 time_us;                 /*!< Period or relative time depending on mode */
    uint32 next_read_time;          /*!< Time of next sensor read */
    Sensor anchor_id;               /*!< Sensor ID for relative timing */
} sensor_timing_t;

/*! Sensor structure, holds completed state of a sensor */
typedef struct sensor
{
    struct sensor *next;            /*!< Pointer to next sensor on the same bus */
    struct sensor *hash_next;       /*!< Pointer to next sensor which has the same hash */
    struct bus *bus;                /*!< Pointer to bus that this sensor is attached to */
    Task task;                      /*!< VM task to send messages to */
    unsigned message_settings:2;    /*!< What kind of message to send to VM */
    unsigned variant:6;             /*!< Data variant to generate */

    Sensor id;                      /*!< ID of this sensor */
    sensor_state_t state;           /*!< State of sensor */
    const sensor_functions_t *functions; /*!< Sensor driver callback functions */

    sensor_timing_t timing;         /*!< Sensor timing state/info */

    uint8 *raw_data;                /*!< Pointer to buffer to store raw sensor data */
    sensor_fifo_t *fifo;            /*!< Pointer to FIFO to store processed sensor data */

    /*! Ad-hoc transfer information */
    struct {
        const void *tx;
        uint16 tx_size;
        void *rx;
        uint16 rx_size;
        bool result;
    } transfer;

    uint32 sub_class_data[1];       /*! Driver instance data (appropriate additional memory will be allocated after this field) */
} sensor_t;

/*! Bus type enum */
typedef enum
{
    BUS_TYPE_I2C,   /*!< Bus is for I2C */
    BUS_TYPE_SPI    /*!< Bus if for SPI (not implemented yet) */
} bus_type_t;

/*! Enum of bus states. Note: ordering of enum important */
typedef enum
{
    BUS_STATE_POWERING_OFF, /*!< Bus is powering off */
    BUS_STATE_IDLE,         /*!< Bus is idle (off) */
    BUS_STATE_POWERING_ON,  /*!< Bus is powering on (from this enum onwards bus is assumed to be active) */
    BUS_STATE_POWERED_ON,   /*!< Bus is on but not doing transfer */
    BUS_STATE_ACTIVE        /*!< Bus is on and doing transfer */
} bus_state_t;

/*! Bus structure, holds completed state of a bus */
typedef struct bus
{
    struct bus *next;       /*!< Pointer to next bus, NULL if last bus in the list */
    bus id;                 /*!< The ID of this bus */
    bus_type_t type:2;      /*!< Type of bus (I2C or SPI) */
    bus_state_t state:3;    /*!< Current state of the bus */

    /*! Bus type specific data */
    union
    {
        /*! I2C specific data */
        struct
        {
            uint8 pio_scl;  /*!< PIO used for SCL */
            uint8 pio_sda;  /*!< PIO used for SDA */
            uint8 address;  /*!< Current I2C address */
        } i2c;

        /* Add SPI fields here when it's implemented */
    } u;

    bitserial_handle handle;    /*!< Handle to bitserial instances, returned from BitserialOpen() */
    bitserial_block_index bitserial_block;  /*!< Bitserial instance number */
    sensor_t *sensor_list;

    tTimerId timer_id;              /*!< Timer ID for next scheduled sensor read */
    sensor_t *active_sensor;        /*!< Pointer to sensor doing scheduled read */
    sensor_t *transfer_sensor;      /*!< Pointer to sensor doing ad-hoc transfer */

    bitserial_transfer_handle transfer_handle;  /*!< Handle for transfer, pass address of this handle
                                                     to bitserial functions, it is passed back in callback */

    uint8_t transfer_count;     /*!< Count of number of transfers queued up */
    uint8_t complete_count;     /*!< Count of number of transfers completed (bit 7 set whilst transfers are
                                     being queued, cleared once all transfers have been queued */

    SemaphoreHandle_t semaphore;    /*!< Semaphore to prevent other operatiosn during power on and off */
} bus_t;


/*! Linked List of buses */
static bus_t *bus_list;

/*! Size of sensor hash table */
#define SENSOR_HASH_TABLE_SIZE (4)

/*! Sensor hash table, used to speed up ID -> structure lookup */
static sensor_t *sensor_table[SENSOR_HASH_TABLE_SIZE];

/*! High priority sensor (hub) task */
static TaskHandle_t sensor_task;

/*! Semaphore used to block caller during ad-hoc transfer */
static SemaphoreHandle_t sensor_semaphore;
static SemaphoreHandle_t sensor_transfer_semaphore;

static bool sensor_default_get_read_time(sensor sensor_id, TIME now, TIME *next_read_time);
static sensor_t *sensor_from_id(Sensor id);
static void bus_schedule_read(bus_t *b);
static void sensor_transfer_handler(void *bus_v);

/*! Macro to convert from bus ID to index */
#define BUS_ID_TO_INDEX(id)  ((id) ^ 0xB0)  /* XOR with magic number to avoid bus ID of 0 */
/*! Macro to convert index to bus ID */
#define BUS_INDEX_TO_ID(idx) ((idx) ^ 0xB0)

/**
 * Allocate a bus ID.
 * @return Allocated bus ID.
 */
static bus bus_alloc_id(void)
{
    static uint8 bus_idx = 0;

    /* Check bus ID doens't already exist */
    bus_t *b = bus_list;
    while (b)
    {
        if (BUS_ID_TO_INDEX(b->id) == bus_idx)
        {
            /* Match found, so increment ID and scan again */
            bus_idx = (bus_idx + 1) % 32;
            b = bus_list;
        }
        else
        {
            b = b->next;
        }
    }

    return BUS_INDEX_TO_ID(bus_idx);
}


/**
 * Find bus from ID.
 * @param id Bus ID
 * @return Pointer to bus structure, or NULL if no match found
 */
static bus_t *bus_from_id(bus id)
{
    bus_t *b = bus_list;
    while (b)
    {
        if (b->id == id)
        {
            return b;
        }

        b = b->next;
    }
    return NULL;
}


/**
 * Create an I2C bus.
 * @param hw_instance Which HW (bitserial) instance should be used
 * @param pio_scl SCL PIO
 * @param pio_sda SDA PIO
 */
bus sensor_hub_bus_i2c(uint8 hw_instance, uint8 pio_scl, uint8 pio_sda)
{
    bitserial_config i2c_config;
    bitserial_handle handle;

    /* Reject I2C bus on non-existent bitserail instance */
    if (hw_instance >= HAVE_NUMBER_OF_BITSERIALS)
    {
        return 0;
    }

    /* Attempt to open Bitserial for I2C with default parameters of 400Khz */
    i2c_config.mode = BITSERIAL_MODE_I2C_MASTER;
    i2c_config.clock_frequency_khz = 400;
    i2c_config.timeout_ms = 5000;
    i2c_config.u.i2c_cfg.flags = BITSERIAL_I2C_ACT_ON_NAK_STOP;
    i2c_config.u.i2c_cfg.i2c_address = 0x00;
    handle = BitserialOpen((bitserial_block_index)((int)P1_BITSERIAL_BLOCK_0 + hw_instance),
                           &i2c_config);

    /* Check handle to determine if open was successful or not */
    if (handle != BITSERIAL_HANDLE_ERROR)
    {
        /* Yay, open was successful so create bus structure */
        bus_t *b = zpnew(bus_t);
        b->type = BUS_TYPE_I2C;
        b->u.i2c.pio_scl = pio_scl;
        b->u.i2c.pio_sda = pio_sda;
        b->u.i2c.address = 0x00;
        b->bitserial_block = (bitserial_block_index)((int)P1_BITSERIAL_BLOCK_0 + hw_instance);
        b->handle = handle;
        b->id = bus_alloc_id();
        b->sensor_list = NULL;
        b->state = BUS_STATE_IDLE;
        b->transfer_sensor = NULL;
        b->semaphore = xSemaphoreCreateBinary();
        (void)xSemaphoreGive(b->semaphore);

        /* Add bus to list */
        b->next = bus_list;
        bus_list = b;

        /* Return ID allocated to this bus */
        return b->id;
    }

    /* If we get here something went wrong, so return 0 to indicate no bus created */
    return 0;
}


/**
 * Bus timer has expired, start the sensor read.
 * @param bus_v Untyped pointer to bus structure.
 */
static void bus_timer_handler(void *bus_v)
{
    bus_t *b;
    sensor_t *s;
    assert(bus_v != NULL);
    b = (bus_t *)bus_v;
    s = b->active_sensor;
    if (s)
    {
        L4_DBG_MSG2("bus_timer_handler, bus %u, sensor %u", b->id, s->id);

        s->timing.immediate = 0;

        assert(s->state == SENSOR_STATE_SCHEDULED);

        assert(s->functions->read_setup != NULL);
        if (s->functions->read_setup(s->id, s->raw_data, s->variant) != BITSERIAL_TRANSFER_HANDLE_NONE)
        {
            s->state = SENSOR_STATE_TRANSFER;

            /* Handler has been triggered, so mark it so that it's not used again */
            if (s->timing.state == SENSOR_TIME_STATE_VALID)
            {
                s->timing.state = SENSOR_TIME_STATE_TRIGGERED;
            }
        }
        else
        {
            L2_DBG_MSG("bus_timer_handler, read not setup");
        }
    }
    else
    {
        L2_DBG_MSG("bus_timer_handler, no active sensor");
    }

}


/**
 * Trigger update of next bus read (to be used from ISR).
 * @param bus_id Bus ID
 */
static void bus_update_from_isr(bus bus_id)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    const uint8 index = BUS_ID_TO_INDEX(bus_id);
    L4_DBG_MSG1("bus_update_from_isr, bus %u", bus_id);

    /* Notify the sensor hub high priority task that the transfer is complete */
    (void)xTaskNotifyFromISR(sensor_task, 1UL << index, eSetBits, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


/**
 * Trigger update of next bus read.
 * @param bus_id Bus ID
 */
static void bus_update(bus bus_id)
{
    L4_DBG_MSG1("bus_update, bus %u", bus_id);

    /* Notify the sensor hub high priority task that the transfer is complete */
    /* Use ISR specific routine if currently running in interrupt handler */
    if (xPortInIsrContext())
    {
        bus_update_from_isr(bus_id);
    }
    else
    {
        const uint8 index = BUS_ID_TO_INDEX(bus_id);
        (void)xTaskNotify(sensor_task, 1UL << index, eSetBits);
    }
}


/**
 * Schdule next sensor read on specified bus.
 * @param b Pointer to bus structure.
 */
static void bus_schedule_read(bus_t *b)
{
    if (b->active_sensor)
    {
        L5_DBG_MSG2("bus_schedule_read, has active sensor %u st %u", b->active_sensor->id, b->active_sensor->state);
        if (b->active_sensor->state == SENSOR_STATE_TRANSFER ||
            b->active_sensor->state == SENSOR_STATE_PROCESS)
        {
            /* If the active sensor is in transfer or process, maybe you are not supposed to do anything here. */
            return;
        }
    }

    switch (b->state)
    {
        /* Powered on or active means we can schedule the next read */
        case BUS_STATE_POWERED_ON:
        case BUS_STATE_ACTIVE:
        {
            int32 b_delta = INT32_MAX;
            sensor_t *a_s = NULL;
            TIME now = hal_get_reg_timer_time();

            /* Iterate through sensors updating read times and keep record of earliest sensor */
            sensor_t *s = b->sensor_list;
            while (s)
            {
                L5_DBG_MSG4("bus_schedule_read, sensor %u imme %u ti_st %u next %u", s->id, s->timing.immediate, s->timing.state, s->timing.next_read_time);

                /* Check if sensor requires immediate reading */
                if (s->timing.immediate)
                {
                    /* Mark timing as valid and use current time */
                    s->timing.state = SENSOR_TIME_STATE_VALID;
                    s->timing.next_read_time = now;
                }
                else
                {
                    /* Attempt to get next read time if current time is invalid or has been used */
                    if ((s->timing.state == SENSOR_TIME_STATE_INVALID) || (s->timing.state == SENSOR_TIME_STATE_USED))
                    {
                        /* Use sensor specific function if available otherwise use default */
                        if (s->functions->get_read_time)
                        {
                            s->timing.state = s->functions->get_read_time(s->id, now, &s->timing.next_read_time) ? SENSOR_TIME_STATE_VALID : SENSOR_TIME_STATE_INVALID;
                        }
                        else
                        {
                            s->timing.state = sensor_default_get_read_time(s->id, now, &s->timing.next_read_time) ? SENSOR_TIME_STATE_VALID : SENSOR_TIME_STATE_INVALID;
                        }
                    }
                }

                L5_DBG_MSG2("bus_schedule_read, ti_st %u next %u", s->timing.state, s->timing.next_read_time);

                /* If this sensor has a valid time, check delta between it's time and now.
                 * if the delta is smaller, remember this sensor */
                if (s->timing.state == SENSOR_TIME_STATE_VALID)
                {
                    int32 s_delta = time_sub(s->timing.next_read_time, now);
                    if (s_delta < b_delta)
                    {
                        a_s = s;
                        b_delta = s_delta;
                    }
                }

                /* Move on to next sensor and loop back */
                s = s->next;
            }

            /* If delta is less than maximum we must have got a valid sensor time */
            if (b_delta < INT32_MAX)
            {
                /* Block the interrupts since we may need to schedule the next read now.
                 * We need to rely on active_sensor state so we must not allow interrupts
                 * to happen */
                block_interrupts();

                /* Firstly, check again if an interrupt has just occurred and changed active_sensor state */
                if (b->active_sensor)
                {
                    L5_DBG_MSG2("bus_schedule_read, int occurred, active sensor %u st %u", b->active_sensor->id, b->active_sensor->state);
                    if (b->active_sensor->state == SENSOR_STATE_TRANSFER ||
                        b->active_sensor->state == SENSOR_STATE_PROCESS)
                    {
                        /* If the active sensor is in transfer or process, it must mean that an interrupt has just occurred.
                         * Just return now and wait for the next sensor task run */
                        unblock_interrupts();
                        return;
                    }
                }

                if (b->active_sensor != a_s)
                {
                    /* NULL is bad at this point */
                    assert(a_s != NULL);
                    /*lint -esym(613,a_s) Match the assert */

                    L4_DBG_MSG2("bus_schedule_read, sensor %u in %d uS", a_s->id, b_delta);

                    /* Cancel any timer currently running as we have a new time */
                    timer_cancel_event_by_function(bus_timer_handler, b);

                    /* Mark current active sensor as on, not active */
                    if (b->active_sensor)
                    {
                        b->active_sensor->state = SENSOR_STATE_ON;
                    }

                    /* Record new active sensor, update it's state to indicate it's scheduled for a read */
                    b->active_sensor = a_s;
                    b->active_sensor->state = SENSOR_STATE_SCHEDULED;

                    /* Bus is now active */
                    b->state = BUS_STATE_ACTIVE;

                    /* Start timer to expire at required read time */
                    if (b_delta < 0)
                    {
                        L5_DBG_MSG("bus_schedule_read, next read in the past");
                        b_delta = 0;
                    }

                    b->timer_id = timer_schedule_event_in(b_delta, bus_timer_handler, b);
                    L4_DBG_MSG1("bus_schedule_read, timer ID %u", b->timer_id);
                }

                /* Next sensor read is scheduled, unblock the interrupts */
                unblock_interrupts();
            }
            else
            {
                /* Bus is not active, just on */
                b->state = BUS_STATE_POWERED_ON;
            }
        }
        break;

        default:
            /* Incorrect state, do nothing */
            break;
    }
}


/**
 * Check if all sensors on the specified bus are now on.
 * If all sensors are now on, initialise I2C bus.
 * @param b Pointer to bus structure.
 */
static void bus_check_sensors_all_on(bus_t *b)
{
    sensor_t *s;
    assert(b->state == BUS_STATE_POWERING_ON);

    /* Check all sensors, exit if one of them is not yet on */
    s = b->sensor_list;
    while (s)
    {
        /* Return if this sensor is not yet on */
        if (s->state != SENSOR_STATE_ON)
        {
            return;
        }

        s = s->next;
    }

    /* Handle bus power on */
    switch (b->type)
    {
        case BUS_TYPE_I2C:
        {
            const uint8 hw_instance = (uint8)((int)b->bitserial_block - (int)P1_BITSERIAL_BLOCK_0);
            static const pin_function_id bs_fid[][4] =
            {
                {BITSERIAL_0_CLOCK_OUT, BITSERIAL_0_CLOCK_IN, BITSERIAL_0_DATA_OUT, BITSERIAL_0_DATA_IN},
                {BITSERIAL_1_CLOCK_OUT, BITSERIAL_1_CLOCK_IN, BITSERIAL_1_DATA_OUT, BITSERIAL_1_DATA_IN},
#if HAVE_NUMBER_OF_BITSERIALS > 2
                {BITSERIAL_2_CLOCK_OUT, BITSERIAL_2_CLOCK_IN, BITSERIAL_2_DATA_OUT, BITSERIAL_2_DATA_IN}
#endif
            };

            const uint32 scl_mask = POFFM(b->u.i2c.pio_scl);
            const uint16 scl_bank = PBANK(b->u.i2c.pio_scl);
            const uint32 sda_mask = POFFM(b->u.i2c.pio_sda);
            const uint16 sda_bank = PBANK(b->u.i2c.pio_sda);
            uint32 invalid_sda, invalid_scl;

            /* Configure PIOs as inputs with pull-ups */
            L4_DBG_MSG4("bus_check_sensors_all_on, configuring I2C PIOs, SCK:%u,%08x SDA:%u,%08x",
                        scl_bank, scl_mask, sda_bank, sda_mask);

            /* Unmap PIOs, set them as inputs with strong pullups */
            invalid_sda  = PioSetMapPins32Bank(sda_bank, sda_mask, sda_mask);
            invalid_scl  = PioSetMapPins32Bank(scl_bank, scl_mask, scl_mask);
            invalid_sda |= PioSetDir32Bank(sda_bank, sda_mask, 0);
            invalid_scl |= PioSetDir32Bank(scl_bank, scl_mask, 0);
            invalid_sda |= PioSet32Bank(sda_bank, sda_mask, sda_mask);
            invalid_scl |= PioSet32Bank(scl_bank, scl_mask, scl_mask);
            invalid_sda |= PioSetStrongBias32Bank(sda_bank, sda_mask, sda_mask);
            invalid_scl |= PioSetStrongBias32Bank(scl_bank, scl_mask, scl_mask);

            /* Attempt to map PIOs to Bitserial block */
            invalid_sda |= PioSetMapPins32Bank(sda_bank, sda_mask, 0);
            invalid_scl |= PioSetMapPins32Bank(scl_bank, scl_mask, 0);
            invalid_sda |= PioSetFunction(b->u.i2c.pio_sda, bs_fid[hw_instance][2]) ? 0 : sda_mask; /* DATA_OUT */
            invalid_sda |= PioSetFunction(b->u.i2c.pio_sda, bs_fid[hw_instance][3]) ? 0 : sda_mask; /* DATA_IN */
            invalid_scl |= PioSetFunction(b->u.i2c.pio_scl, bs_fid[hw_instance][0]) ? 0 : scl_mask; /* CLOCK_OUT */
            invalid_scl |= PioSetFunction(b->u.i2c.pio_scl, bs_fid[hw_instance][1]) ? 0 : scl_mask; /* CLOCK_IN */

            if (invalid_sda)
            {
                L1_DBG_MSG1("bus_check_sensors_all_on, failed to configure SDA PIO %u", b->u.i2c.pio_sda);
                panic(PANIC_SENSOR_HUB_PIO_CONFIGURE_FAILED);
            }
            if (invalid_scl)
            {
                L1_DBG_MSG1("bus_check_sensors_all_on, failed to configure SCL PIO %u", b->u.i2c.pio_scl);
                panic(PANIC_SENSOR_HUB_PIO_CONFIGURE_FAILED);
            }
        }
        break;

        case BUS_TYPE_SPI:
            L1_DBG_MSG("bus_check_sensors_all_on, SPI not supported");
            break;
    }

    /* Iterate through all attached sensors calling power_on function */
    s = b->sensor_list;
    while (s)
    {
        if (s->functions->post_power_on)
        {
            s->functions->post_power_on(b->id, s->id);
        }

        s = s->next;
    }

    /* Bus now power on so update state */
    b->state = BUS_STATE_POWERED_ON;

    /* Bus operation complete, so give semaphore back to allow next blocked operation to run */
    if (xSemaphoreGive(b->semaphore) != pdTRUE)
    {
        panic(PANIC_SENSOR_HUB_SEMAPHORE_FAILED);
    }

    /* Iterate through all attached sensors to ask for next read time */
    bus_update(b->id);
}


/**
 * Power on specified bus.
 * Iterate through all sensors in the bus powering them on.
 * @param bus_id Bus ID
 * @return TRUE if power on initiated, FALSE if request failed
 */
bool sensor_hub_bus_power_on(bus bus_id)
{
    sensor_t *s;
    bus_t *b;
    uint32 i2c_rate = 400;

    /* Return immediately if no ID match or bus is not in the correct state */
    b = bus_from_id(bus_id);
    if ((!b) || (b->state > BUS_STATE_IDLE))
    {
        return FALSE;
    }

    /* Grab semaphore to prevent other bus operations */
    if (xSemaphoreTake(b->semaphore, portMAX_DELAY) != pdTRUE)
    {
        panic(PANIC_SENSOR_HUB_SEMAPHORE_FAILED);
    }

    /* Update state to reflect bus is now powering on */
    b->state = BUS_STATE_POWERING_ON;

    /* Iterate through all attached sensors */
    s = b->sensor_list;
    while (s)
    {
        uint32 raw_data_size = 16;
        uint32 fifo_elements = 2;
        uint32 fifo_type = 0;
        uint32 fifo_size;

        /* At this point there should be no raw data buffer allocated */
        assert(s->raw_data == NULL);

        /* Allocate raw data buffer, ask sensor driver what size it would like but default to
         * 16 bytes if it has no opinion */
        (void)sensor_hub_sensor_get_configuration(s->id, SENSOR_CONFIG_RAW_BUFFER_SIZE, &raw_data_size);
        s->raw_data = pmalloc(raw_data_size);

        /* Free any FIFO that was allocated previously */
        pfree(s->fifo);

        /* Determine size of FIFO by querying sensor driver for number of elements and the
         * element type (from which element size is derived) */
        (void)sensor_hub_sensor_get_configuration(s->id, SENSOR_CONFIG_FIFO_MAX_ELEMENTS, &fifo_elements);
        assert(fifo_elements <= 7);
        (void)sensor_hub_sensor_get_configuration(s->id, SENSOR_CONFIG_FIFO_ELEMENT_TYPE, &fifo_type);
        assert((fifo_type >> 8) <= 127);
        fifo_size = (fifo_type >> 8) * fifo_elements;

        L3_DBG_MSG2("sensor_hub_bus_power_on, sensor %u, fifo_size %u", s->id, fifo_size);

        /* Allocate FIFO memory and initialise the FIFO indices */
        s->fifo = pmalloc(sizeof(sensor_fifo_t) + fifo_size - 1);
        s->fifo->size = fifo_elements;
        s->fifo->index = s->fifo->outdex = 0;
        s->fifo->full = 0;
        s->fifo->element_size = (fifo_type >> 8);

        /* Call pre-power on function if it's specified */
        if (s->functions->pre_power_on)
        {
            s->state = SENSOR_STATE_POWERING_ON;
            s->functions->pre_power_on(b->id, s->id);
        }
        else
        {
            s->state = SENSOR_STATE_ON;
        }

        /* If I2C bus, get lowest clock rate */
        if (b->type == BUS_TYPE_I2C)
        {
            uint32 rate = 100;
            if (!sensor_hub_sensor_get_configuration(s->id, SENSOR_CONFIG_I2C_RATE_KHZ, &rate))
            {
                L2_DBG_MSG1("sensor_hub_bus_power_on, can't get I2C clock rate for sensor %d", s->id);
            }
            if (rate < i2c_rate)
            {
                i2c_rate = rate;
                L3_DBG_MSG1("sensor_hub_bus_power_on, reducing I2C clock rate to %u KHz", rate);
            }
        }

        s = s->next;
    }

    if (b->type == BUS_TYPE_I2C)
    {
       if (BitserialChangeParam(b->handle, BITSERIAL_PARAMS_CLOCK_FREQUENCY_KHZ,
                                 (uint16)i2c_rate, (bitserial_transfer_flags)0) != BITSERIAL_RESULT_SUCCESS)
        {
            L1_DBG_MSG1("sensor_hub_bus_power_on, failed to set I2C clock rate to %u KHz", i2c_rate);
        }
    }

    /* Check if sensors are now powered on */
    bus_check_sensors_all_on(b);

    /* If we got here bus power on has started successfully, so inform caller */
    return TRUE;
}


/**
 * Check if all sensors on the specified bus are now off.
 * If all sensors are now off, shutdown I2C bus.
 * @param b Pointer to bus structure.
 */
static void bus_check_sensors_all_off(bus_t *b)
{
    sensor_t *s;
    L4_DBG_MSG2("bus_check_sensors_all_off, bus %d, state %u", b->id, b->state);

    /* Return if bus is not in powering off state */
    if (b->state != BUS_STATE_POWERING_OFF)
    {
        return;
    }

    s = b->sensor_list;
    while (s)
    {
        if (s->state != SENSOR_STATE_OFF)
        {
            /* This sensor is not off, so return immediately */
            L3_DBG_MSG2("bus_check_sensors_all_off, sensor %d not off on bus %d", s->id, b->id);
            return;
        }

        s = s->next;
    }

    switch (b->type)
    {
        case BUS_TYPE_I2C:
        {
            const uint32 scl_mask = POFFM(b->u.i2c.pio_scl);
            const uint16 scl_bank = PBANK(b->u.i2c.pio_scl);
            const uint32 sda_mask = POFFM(b->u.i2c.pio_sda);
            const uint16 sda_bank = PBANK(b->u.i2c.pio_sda);
            uint32 invalid_sda, invalid_scl;

            invalid_scl  = PioSetMapPins32Bank(scl_bank, scl_mask, scl_mask);
            invalid_sda  = PioSetMapPins32Bank(sda_bank, sda_mask, sda_mask);
            invalid_scl |= PioSetDir32Bank(scl_bank, scl_mask, 0);
            invalid_sda |= PioSetDir32Bank(sda_bank, sda_mask, 0);
            invalid_scl |= PioSet32Bank(scl_bank, scl_mask, scl_mask);
            invalid_sda |= PioSet32Bank(sda_bank, sda_mask, sda_mask);
            invalid_scl |= PioSetStrongBias32Bank(scl_bank, scl_mask, 0);
            invalid_sda |= PioSetStrongBias32Bank(sda_bank, sda_mask, 0);

            if (invalid_sda)
            {
                L1_DBG_MSG1("bus_check_sensors_all_off, failed to configure SDA PIO %u", b->u.i2c.pio_sda);
                panic(PANIC_SENSOR_HUB_PIO_CONFIGURE_FAILED);
            }
            if (invalid_scl)
            {
                L1_DBG_MSG1("bus_check_sensors_all_off, failed to configure SCL PIO %u", b->u.i2c.pio_scl);
                panic(PANIC_SENSOR_HUB_PIO_CONFIGURE_FAILED);
            }
       }
       break;

        case BUS_TYPE_SPI:
            L1_DBG_MSG("bus_check_sensors_all_off, SPI not supported");
            break;
    }

    /* Bus is now off */
    b->state = BUS_STATE_IDLE;

    if (xSemaphoreGive(b->semaphore) != pdTRUE)
    {
        panic(PANIC_SENSOR_HUB_SEMAPHORE_FAILED);
    }
}


/**
 * Sensor powered off indication.  Called from sensor driver to indicate sensor is now powered off.
 * @param sensor_id Sensor ID.
 */
void sensor_hub_bus_sensor_off_indication(sensor sensor_id)
{
    sensor_t *s = sensor_from_id(sensor_id);
    assert(s != NULL);
    /*lint -esym(613,s) Match the assert */

    assert(s->state == SENSOR_STATE_POWERING_OFF);

    s->state = SENSOR_STATE_OFF;
    pfree(s->raw_data);
    s->raw_data = NULL;

    bus_check_sensors_all_off(s->bus);
}


/**
 * Power off specified bus.
 * @param bus_id Bus ID
 * @return TRUE if power off initiated, FALSE if request failed
 */
bool sensor_hub_bus_power_off(bus bus_id)
{
    bus_t *b;
    sensor_t *s;

    b = bus_from_id(bus_id);
    if ((!b) || (b->state < BUS_STATE_POWERING_ON))
    {
        return FALSE;
    }

    if (xSemaphoreTake(b->semaphore, portMAX_DELAY) != pdTRUE)
    {
        panic(PANIC_SENSOR_HUB_SEMAPHORE_FAILED);
    }

    timer_cancel_event_by_function(bus_timer_handler, b);
    timer_cancel_event_by_function(sensor_transfer_handler, b);

    b->state = BUS_STATE_POWERING_OFF;

    /* Iterate through all attached sensors */
    s = b->sensor_list;
    while (s)
    {
        if (s->functions->pre_power_off)
        {
            s->state = SENSOR_STATE_POWERING_OFF;
            s->functions->pre_power_off(b->id, s->id);
        }
        else
        {
            s->state = SENSOR_STATE_OFF;
        }

        s = s->next;
    }

    bus_check_sensors_all_off(b);
    return TRUE;
}


/**
 * Attach sensor to bus.
 * @param bus_id Bus ID
 * @param sensor_id Sensor ID
 * @return TRUE if sensor attached to bus, FALSE is attachment failed
 */
bool sensor_hub_bus_attach_sensor(bus bus_id, sensor sensor_id)
{
    sensor_t *s = sensor_from_id(sensor_id);
    bus_t *b = bus_from_id(bus_id);

    /* Check sensor and bus ID are valid */
    if (!b || !s)
    {
        return FALSE;
    }

    /* Check sensor isn't already attached */
    if (s->bus)
    {
        return FALSE;
    }

    /* Make sensor time as invalid to trigger calculation of next read time */
    s->timing.state = SENSOR_TIME_STATE_INVALID;

    /* Store pointer to bus */
    s->bus = b;

    /* Add sensor to list of attached sensors */
    block_interrupts();
    s->next = b->sensor_list;
    b->sensor_list = s;
    unblock_interrupts();

    return TRUE;
}


/**
 * Allocate sensor ID.
 * @param s Pointer to sensor structure
 * @return Allocated sensor ID
 */
static sensor sensor_alloc_id(sensor_t *s)
{
    static uint8 sensor_id = 0;

    /* Increment sensor ID until no match found */
    do sensor_id += 1; while (!sensor_id || sensor_from_id(sensor_id));

    /* Insert sensor into hash table, link to sensor(s) with same hash */
    block_interrupts();
    {
        const uint8 index = sensor_id % SENSOR_HASH_TABLE_SIZE;
        s->hash_next = sensor_table[index];
        sensor_table[index] = s;
    }
    unblock_interrupts();

    return sensor_id;
}


/**
 * Find sensor from ID.
 * @param id Sensor ID
 * @return Pointer to sensor_t structure for specified ID, or NULL if no match found.
 */
static sensor_t *sensor_from_id(sensor id)
{
    const uint8 index = id % SENSOR_HASH_TABLE_SIZE;
    sensor_t *s = sensor_table[index];
    while (s)
    {
        if (s->id == id)
        {
            break;
        }

        s = s->hash_next;
    }

    return s;
}


/**
 * Manually trigger an immediate sensor read.
 * @param sensor_id Sensor ID
 * @param variant Read variant, pass to driver to select which values to read.
 */
void sensor_hub_sensor_read(sensor sensor_id, uint16 variant)
{
    sensor_t *s = sensor_from_id(sensor_id);
    assert(s != NULL);
    /*lint -esym(613,s) Match the assert */
    if (s->state == SENSOR_STATE_ON)
    {
        block_interrupts();
        s->variant = variant;
        s->timing.immediate = 1;
        unblock_interrupts();
        bus_update(s->bus->id);
    }
}


/**
 * Set sensor timing period
 * @param sensor_id Sensor ID
 * @param period_us New timing period in microseconds.  0 to disable periodic reading.
 */
static void sensor_timing_set_period(sensor sensor_id, uint32 period_us)
{
    sensor_t *s = sensor_from_id(sensor_id);
    assert(s != NULL);
    /*lint -esym(613,s) Match the assert */

    block_interrupts();
    s->timing.time_us = period_us;
    s->timing.mode = (period_us != 0) ? SENSOR_TIME_MODE_PERIODIC : SENSOR_TIME_MODE_MANUAL;
    s->timing.state = SENSOR_TIME_STATE_INVALID;
    unblock_interrupts();

    bus_update(s->bus->id);
}


/**
 * Return sensor timing period
 * @param sensor_id Sensor ID
 * @return Sensor timing period in microseconds
 */
static uint32 sensor_timing_get_period(sensor sensor_id)
{
    sensor_t *s = sensor_from_id(sensor_id);
    assert(s != NULL);
    /*lint -esym(613,s) Match the assert */

    return s->timing.time_us;
}


/**
 * Is sensor reading periodic.
 * @param sensor_id Sensor ID
 * @return TRUE if sensor timing is periodic
 */
static bool sensor_timing_is_periodic(sensor sensor_id)
{
    sensor_t *s = sensor_from_id(sensor_id);
    assert(s != NULL);
    /*lint -esym(613,s) Match the assert */

    return s->timing.mode == SENSOR_TIME_MODE_PERIODIC;
}


/**
 * Specify anchor sensor that this sensor's timing is relative to.
 * @param sensor_id Sensor ID
 * @param anchor_id Sensor ID of anchor sensor.
 */
static void sensor_timing_set_anchor_sensor(sensor sensor_id, sensor anchor_id)
{
    sensor_t *s = sensor_from_id(sensor_id);
    assert(s != NULL);
    /*lint -esym(613,s) Match the assert */

    block_interrupts();
    s->timing.anchor_id = anchor_id;
    unblock_interrupts();
    bus_update(s->bus->id);
}


/**
 * Specify sensor's relative time to anchor sensor.
 * @param sensor_id Sensor ID
 * @param time Relative time
 */
static void sensor_timing_set_relative_time(sensor sensor_id, uint32 time)
{
    sensor_t *s = sensor_from_id(sensor_id);
    assert(s != NULL);
    /*lint -esym(613,s) Match the assert */

    block_interrupts();
    s->timing.mode = SENSOR_TIME_MODE_RELATIVE;
    s->timing.time_us = time;
    unblock_interrupts();
    bus_update(s->bus->id);
}


/**
 * Synchronous transfer has completed, give back semaphore to allow caller to run.
 * @param tfh bitserial transfer handle
 * @param blocking Always FALSE in this case
 * @param result Result of transfer
 */
static void sensor_transfer_complete(bitserial_transfer_handle *tfh, bool blocking, bitserial_result result)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    bus_t *b;

    UNUSED(blocking);

    /* Find bus with match transfer handle */
    b = bus_list;
    while (b)
    {
        if (&b->transfer_handle == tfh)
        {
            sensor_t *s = b->transfer_sensor;
            assert(s != NULL);
            /*lint -esym(613,s) Match the assert */
            s->transfer.result = (result == BITSERIAL_RESULT_SUCCESS);

            /* Clear sensor now that transfer has been completed */
            b->transfer_sensor = NULL;
            break;
        }

        b = b->next;
    }

    /* we should also find a bus that matches tfh */
    assert(b != NULL);

    /* Transfer has completed, give sempahore to caller task to wake it up */
    if (xSemaphoreGiveFromISR(sensor_semaphore, &higher_priority_task_woken) != pdTRUE)
    {
        panic(PANIC_SENSOR_HUB_SEMAPHORE_FAILED);
    }
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

/**
 * Callback from timer, start synchronous sensor transfer
 * @param bus_v Untyped pointer to bus structure
 */
static void sensor_transfer_handler(void *bus_v)
{
    bus_t *b;
    sensor_t *s;

    assert(bus_v != NULL);
    b = (bus_t *)bus_v;

    /* Sanity check that there is a sensor that needs to do ad-hoc transfer */
    s = b->transfer_sensor;
    if (s)
    {
        s->transfer.result = FALSE;
        if (b->type == BUS_TYPE_I2C)
        {
            uint32 i2c_addr = b->u.i2c.address;
            if (!sensor_hub_sensor_get_configuration(s->id, SENSOR_CONFIG_I2C_ADDR, &i2c_addr))
            {
                L1_DBG_MSG1("sensor_transfer_handler, can't get I2C addrss for sensor %d", s->id);
            }
            if (b->u.i2c.address != i2c_addr)
            {
                L5_DBG_MSG2("sensor_transfer_handler, setting bus %d to I2C addresss %02x", b->id, i2c_addr);
                b->u.i2c.address = (uint8)i2c_addr;
                if (BitserialChangeParam(b->handle, BITSERIAL_PARAMS_I2C_DEVICE_ADDRESS,
                                         b->u.i2c.address, (bitserial_transfer_flags)0) != BITSERIAL_RESULT_SUCCESS)
                {
                    /* Failed to set I2C address, return failure to task that called SensorTransfer() */
                    sensor_transfer_complete(&b->transfer_handle, FALSE, BITSERIAL_RESULT_INVAL);
                    return;
                }
            }
        }

        /* Queue up transfer, sensor_transfer_complete will be called when transfer has completed */
        if (!bitserial_add_transfer(b->handle, &b->transfer_handle,
                                   s->transfer.tx, s->transfer.tx_size,
                                   s->transfer.rx, s->transfer.rx_size,
                                   BITSERIAL_ACTION_FLAGS_NONE, sensor_transfer_complete))
        {

            /* Failed to start transfer, return failure to task that called SensorTransfer() */
            sensor_transfer_complete(&b->transfer_handle, FALSE, BITSERIAL_RESULT_INVAL);
        }
    }
}

/**
 * Perform synchronous sensor transfer.
 * If sensor is not active the transfer happend immediately in this context.  If the sensor is active, an immediate
 * time is started, the handler of which will perform the transfer (see sensor_transfer_handler).  In this case this
 * function waits on a binary semaphore until the transfer has completed.
 * @param sensor_id Sensor ID
 * @param tx_data Pointer to data to transmit
 * @param tx_size Number of octets to transmit
 * @param rx_data Pointer to buffer to receive data
 * @param rx_size Number of bytes to receive
 * @return TRUE if transfer occured, FALSE if unable to perform transfer
 */
bool sensor_hub_sensor_transfer(sensor sensor_id, const uint8 *tx_data, uint16 tx_size, uint8 *rx_data, uint16 rx_size)
{
    bus_t *b;
    sensor_t *s;

    s = sensor_from_id(sensor_id);
    assert(s != NULL);
    /*lint -esym(613,s) Match the assert */
    b = s->bus;
    assert(b != NULL);

    switch (b->state)
    {
        /* Bus is active or powered-on, so high priority transfers could happen at any time.
         * To avoid conflicts, schedule an immediate timer to do the transfer at the
         * same priority */
        case BUS_STATE_ACTIVE:
        case BUS_STATE_POWERED_ON:
            /* Use the mutex to protect the transfer sensor */
            if (xSemaphoreTake(sensor_transfer_semaphore, portMAX_DELAY) != pdTRUE)
            {
                panic(PANIC_SENSOR_HUB_SEMAPHORE_FAILED);
            }
            /* Store transfer parameters, schedule read */
            b->transfer_sensor = s;
            s->transfer.tx = tx_data;
            s->transfer.tx_size = tx_size;
            s->transfer.rx = rx_data;
            s->transfer.rx_size = rx_size;

            /* Schedule immediate timer to do the transfer */
            L5_DBG_MSG1("sensor_hub_sensor_transfer, sensor %u transfer pending", s->id);
            (void)timer_schedule_event_in(0, sensor_transfer_handler, b);

            /* Take semaphore, call will return once transfer has completed */
            if (xSemaphoreTake(sensor_semaphore, portMAX_DELAY) != pdTRUE)
            {
                panic(PANIC_SENSOR_HUB_SEMAPHORE_FAILED);
            }
            /* Give back the mutex so other task can start the sensor transfer */
            if (xSemaphoreGive(sensor_transfer_semaphore) != pdTRUE)
            {
                panic(PANIC_SENSOR_HUB_SEMAPHORE_FAILED);
            }
            return s->transfer.result;

        /* If bus powering on or off we can do transfer immediately */
        case BUS_STATE_POWERING_ON:
        case BUS_STATE_POWERING_OFF:
            /* Ensure I2C address is set correctly */
            if (b->type == BUS_TYPE_I2C)
            {
                uint32 i2c_addr = b->u.i2c.address;
                if (!sensor_hub_sensor_get_configuration(sensor_id, SENSOR_CONFIG_I2C_ADDR, &i2c_addr))
                {
                    L1_DBG_MSG1("sensor_hub_sensor_transfer, can't get I2C addrss for sensor %d", sensor_id);
                }
                if (b->u.i2c.address != i2c_addr)
                {
                    L5_DBG_MSG2("sensor_hub_sensor_transfer, setting bus %d to I2C addresss %02x", b->id, i2c_addr);
                    b->u.i2c.address = (uint8)i2c_addr;
                    if (BitserialChangeParam(b->handle, BITSERIAL_PARAMS_I2C_DEVICE_ADDRESS,
                                             b->u.i2c.address, (bitserial_transfer_flags)0) != BITSERIAL_RESULT_SUCCESS)
                    {
                        return FALSE;
                    }
                }
            }
            return BitserialTransfer(b->handle, NULL, tx_data, tx_size, rx_data, rx_size) == BITSERIAL_RESULT_SUCCESS;

        default:
            L1_DBG_MSG2("sensor_hub_sensor_transfer, ignoring, bus %d in state ", b->id, b->state);
            return FALSE;
    }
}


/**
 * Begin asynchronous sensor transfer.  Called from read_setup driver callback.
 * @param sensor_id Sensor ID
 * @return Handle to be passed to read/write functions, or BITSERIAL_TRANSFER_HANDLE_NONE if unable to setup transfer.
 */
bus_transfer_handle sensor_hub_sensor_transfer_begin(sensor sensor_id)
{
    bus_t *b;
    sensor_t *s;
    uint32 i2c_addr;

    s = sensor_from_id(sensor_id);
    assert(s != NULL);
    /*lint -esym(613,s) Match the assert */
    b = s->bus;
    assert(b != NULL);

    if (b->type == BUS_TYPE_I2C)
    {
        i2c_addr = b->u.i2c.address;
        if (!sensor_hub_sensor_get_configuration(sensor_id, SENSOR_CONFIG_I2C_ADDR, &i2c_addr))
        {
            L1_DBG_MSG1("sensor_hub_sensor_transfer_begin, can't get I2C addrss for sensor %d", sensor_id);
        }
        if (b->u.i2c.address != i2c_addr)
        {
            L5_DBG_MSG2("sensor_hub_sensor_transfer_begin, setting bus %d to I2C addresss %02x", b->id, i2c_addr);
            b->u.i2c.address = (uint8)i2c_addr;
            if (BitserialChangeParam(b->handle, BITSERIAL_PARAMS_I2C_DEVICE_ADDRESS,
                                     b->u.i2c.address, (bitserial_transfer_flags)0) != BITSERIAL_RESULT_SUCCESS)
            {
                /* Failed to set I2C address, return null handle */
                return BITSERIAL_TRANSFER_HANDLE_NONE;
            }
        }
    }

    b->transfer_count = 0;
    b->complete_count = 0x80;
    return b->handle;
}


/**
 * Asynchronous bitserial callback.  Called when each of the requested read/writes completes.  When last transfer has completed
 * triggers bus update.
 * @param tfh bitserial transfer handle
 * @param blocking Always FALSE in this case
 * @param result Result of transfer
 */
static void sensor_bitserial_complete(bitserial_transfer_handle *tfh, bool blocking, bitserial_result result)
{
    bus_t *b;

    UNUSED(blocking);
    UNUSED(result);

    /* Find bus with match transfer handle */
    b = bus_list;
    while (b)
    {
        if (&b->transfer_handle == tfh)
        {
            b->complete_count += 1;
            if (b->transfer_count == b->complete_count)
            {
                sensor_t *s = b->active_sensor;
                assert(s != NULL);
                /*lint -esym(613,s) Match the assert */

                assert(s->state == SENSOR_STATE_TRANSFER);
                s->state = SENSOR_STATE_PROCESS;

                L5_DBG_MSG1("sensor_bitserial_complete, result %u", result);

                /* Notify the sensor hub high priority task that the transfer is complete */
                bus_update_from_isr(b->id);
            }
            break;
        }

        b = b->next;
    }
}


/**
 * Add write to asynchronous sensor transfer.  Called from read_setup driver callback.
 * @param sensor_id Sensor ID
 * @param h Value returned from sensor_transfer_begin
 * @param data_ptr Pointer to data to transmit
 * @param data_size Number of octets to transmit
 */
void sensor_hub_sensor_add_write(sensor sensor_id, bus_transfer_handle h, const void *data_ptr, uint16 data_size)
{
    sensor_t *s;
    bus_t *b;

    if (h == BITSERIAL_TRANSFER_HANDLE_NONE)
    {
        return;
    }

    s = sensor_from_id(sensor_id);
    b = s ? s->bus : NULL;
    if (s && b)
    {
        assert(b->transfer_count < 127);
        b->transfer_count += 1;
        if (!bitserial_add_transfer(h, &b->transfer_handle,
                                    data_ptr, data_size,
                                    NULL, 0,
                                    BITSERIAL_ACTION_FLAGS_NONE, sensor_bitserial_complete))
        {
            /* Can't do transfer */
            b->transfer_count -= 1;
        }
    }
}


/**
 * Add read to asynchronous sensor transfer.  Called from read_setup driver callback.
 * @param sensor_id Sensor ID
 * @param h Value returned from sensor_transfer_begin
 * @param data_ptr Pointer to data buffer to receive data
 * @param data_size Number of octets to receive
 */
void sensor_hub_sensor_add_read(sensor sensor_id, bus_transfer_handle h, void *data_ptr, uint16 data_size)
{
    sensor_t *s;
    bus_t *b;

    if (h == BITSERIAL_TRANSFER_HANDLE_NONE)
    {
        return;
    }

    s = sensor_from_id(sensor_id);
    b = s ? s->bus : NULL;
    if (s && b)
    {
        assert(b->transfer_count < 127);
        b->transfer_count += 1;
        if (!bitserial_add_transfer(h, &b->transfer_handle,
                                    NULL, 0,
                                    data_ptr, data_size,
                                    BITSERIAL_ACTION_FLAGS_NONE, sensor_bitserial_complete))
        {
            /* Can't do transfer */
            b->transfer_count -= 1;
        }
    }
}


/**
 * End asynchronous sensor transfer.  Called from read_setup driver callback.
 * @param sensor_id Sensor ID
 * @param h Value returned from sensor_transfer_begin
 */
void sensor_hub_sensor_transfer_end(sensor sensor_id, bus_transfer_handle h)
{
    sensor_t *s;
    bus_t *b;

    if (h == BITSERIAL_TRANSFER_HANDLE_NONE)
    {
        return;
    }

    s = sensor_from_id(sensor_id);
    b = s ? s->bus : NULL;
    if (s && b)
    {
        /* Clear top bit of complete_count, to indicate all transfers queued up */
        block_interrupts();
        b->complete_count &= 0x7F;
        unblock_interrupts();
    }
}


/**
 * Create instance of sensor driver.
 * @param functions Pointer to table of callback functions.
 * @param sub_class_size Size in octets of driver instance data section.
 * @return Sensor ID.
 */
sensor sensor_hub_sensor_create(const sensor_functions_t *functions, uint16 sub_class_size)
{
    const size_t s_size = sizeof(sensor_t) + sub_class_size - sizeof(uint32);
    sensor_t *s = zpmalloc(s_size);

    s->bus = NULL;
    s->functions = functions;
    s->id = sensor_alloc_id(s);
    s->next = NULL;
    s->state = SENSOR_STATE_DETACHED;
    s->message_settings = SENSOR_MESSAGES_SOME;
    s->timing.mode = SENSOR_TIME_MODE_MANUAL;

    return s->id;
}


/**
 * Returns pointer to sensor instance data section.
 * @param sensor_id Sensor ID
 * @return Pointer to instance data section, or NULL if no sensor matching ID
 */
void *sensor_hub_sensor_subclass_data(sensor sensor_id)
{
    sensor_t *s = sensor_from_id(sensor_id);
    return s ? s->sub_class_data : NULL;
}


/**
 * Read sensor configuration parameter/status.
 * @param sensor_id Sensor ID
 * @param key Sensor configuration key
 * @param value Pointer to variable to store value
 * @return TRUE if configuration was read
 */
bool sensor_hub_sensor_get_configuration(sensor sensor_id, sensor_config_key_t key, uint32 *value)
{
    sensor_t *s = sensor_from_id(sensor_id);
    if (s)
    {
        /* Call sensor specific get_config function if it exists */
        if (s->functions->get_config)
        {
            if (s->functions->get_config(sensor_id, key, value))
            {
                return TRUE;
            }
        }

        /* If we get here, either the sensor doesn't have a get_config function or
         * it doesn't recognise the key */
        switch (key)
        {
            /* Get timer period if periodic timing is enabled */
            case SENSOR_CONFIG_PERIOD_US:
                if (sensor_timing_is_periodic(sensor_id))
                {
                    *value = sensor_timing_get_period(sensor_id);
                    return TRUE;
                }
                break;

            default:
                break;
        }
    }
    return FALSE;
}


/**
 * Configure sensor parameters, either a common parameter or a driver specific parameter.
 * @param sensor_id Sensor ID
 * @param key Sensor configuration key
 * @param value Configuration value
 * @return TRUE if configuration was set
 */
bool sensor_hub_sensor_configure(sensor sensor_id, sensor_config_key_t key, uint32 value)
{
    sensor_t *s = sensor_from_id(sensor_id);
    if (s)
    {
        /* Call sensor specific set_config function if it exists */
        if (s->functions->set_config)
        {
            if (s->functions->set_config(sensor_id, key, value))
            {
                return TRUE;
            }
        }

        /* If we get here, either the sensor doesn't have a set_config function or
         * it doesn't recognise the key */
        switch (key)
        {
            /* Set timer period, if period is 0 disable periodic timing */
            case SENSOR_CONFIG_PERIOD_US:
                sensor_timing_set_period(sensor_id, value);
                return TRUE;

            case SENSOR_CONFIG_ANCHOR_SENSOR:
                sensor_timing_set_anchor_sensor(sensor_id, (Sensor)value);
                return TRUE;

            case SENSOR_CONFIG_RELATIVE_US:
                sensor_timing_set_relative_time(sensor_id, value);
                return TRUE;

            case SENSOR_CONFIG_MESSAGES:
                if (value == SENSOR_MESSAGES_SOME)
                {
                    value = SENSOR_MESSAGES_NEXT;
                }
                s->message_settings = value;
                return TRUE;

            default:
                break;
        }
    }
    return FALSE;
}


/**
 * Default calculate sensor read time function.  Handles software driven timing.
 * @param sensor_id Sensor ID
 * @param now Current time
 * @param next_read_time Pointer to time variable that stores the next read time
 * @return TRUE if read time is now valid
 */
static bool sensor_default_get_read_time(sensor sensor_id, TIME now, TIME *next_read_time)
{
    sensor_t *s = sensor_from_id(sensor_id);
    if (s)
    {
        switch (s->timing.mode)
        {
            case SENSOR_TIME_MODE_PERIODIC:
            {
                /* If sensor timing is invalid, calculate from now, if timing has
                 * been used calculate from that time, otherwise just leave the
                 * time as it is */
                switch (s->timing.state)
                {
                    case SENSOR_TIME_STATE_INVALID:
                        *next_read_time = time_add(now, s->timing.time_us);
                        return TRUE;

                    case SENSOR_TIME_STATE_USED:
                        *next_read_time = time_add(s->timing.next_read_time, s->timing.time_us);
                        return TRUE;

                    case SENSOR_TIME_STATE_VALID:
                        return TRUE;

                    default:
                        return FALSE;
                }
            }

            case SENSOR_TIME_MODE_RELATIVE:
            {
                switch (s->timing.state)
                {
                    case SENSOR_TIME_STATE_USED:
                    case SENSOR_TIME_STATE_INVALID:
                    {
                        /* Get anchor sensor from ID */
                        sensor_t *as = sensor_from_id(s->timing.anchor_id);
                        if (as)
                        {
                            /* If anchor sensor time is invalid try and get valid time */
                            if (as->timing.state == SENSOR_TIME_STATE_INVALID)
                            {
                                (void)sensor_default_get_read_time(s->timing.anchor_id, now, &as->timing.next_read_time);
                            }

                            /* If anchor sensor has a valid time (future or past) then calculate our time
                             * relative to it */
                            if (as->timing.state == SENSOR_TIME_STATE_VALID || as->timing.state == SENSOR_TIME_STATE_USED)
                            {
                                *next_read_time = time_add(as->timing.next_read_time, s->timing.time_us);
                                L5_DBG_MSG2("sensor_default_get_read_time, calculated relative time for sensor %u, %u", s->id, *next_read_time);
                                return TRUE;
                            }
                        }
                        else
                            L2_DBG_MSG1("sensor_default_get_read_time, sensor %u invalid for anchor sensor", s->timing.anchor_id);
                    }
                    break;

                    case SENSOR_TIME_STATE_VALID:
                        return TRUE;

                    default:
                        break;
                }
            }
            break;

            default:
                break;
        }
    }

    return FALSE;
}


/**
 * Is FIFO empty?
 * @param f Pointer to FIFO
 * @return TRUE if FIFO is empty, FALSE otherwise
 */
static bool sensor_fifo_is_empty(sensor_fifo_t *f)
{
    return (!f->full && (f->index == f->outdex));
}


/**
 * Is FIFO full?
 * @param f Pointer to FIFO
 * @return TRUE if FIFO is full, FALSE otherwise
 */
static bool sensor_fifo_is_full(sensor_fifo_t *f)
{
    return (f->full && (f->index == f->outdex));
}


/**
 * Get number of elements in FIFO
 * @param f Pointer to FIFO
 * @return Number of elements in the FIFO
 */
static uint8 sensor_fifo_size(sensor_fifo_t *f)
{
    if (!f->full)
    {
        return (f->index >= f->outdex) ?
               (uint8)(f->index - f->outdex) :
               (uint8)(f->size + f->index - f->outdex);
    }

    return f->size;
}


/**
 * Advance index
 * @param f Pointer to FIFO
 */
static void sensor_fifo_advance_index(sensor_fifo_t *f)
{
    f->index += 1;
    if  (f->index == f->size)
    {
        f->index = 0;
    }
    f->full = f->index == f->outdex ? 1 : 0;
}


/**
 * Advance outdex
 * @param f Pointer to FIFO
 */
static void sensor_fifo_advance_outdex(sensor_fifo_t *f)
{
    f->outdex += 1;
    if  (f->outdex == f->size)
    {
        f->outdex = 0;
    }
    f->full = 0;
}


/**
 * Return pointer to item at index
 * @param f Pointer to FIFO
 * @return Pointer to item
 */
static fifo_item_t *sensor_fifo_index_ptr(sensor_fifo_t *f)
{
    uint16_t offset = f->index * f->element_size;
    fifo_item_t *fi = (fifo_item_t *)&f->elements[offset];
    return fi;
}


/**
 * Return pointer to item at outdex
 * @param f Pointer to FIFO
 * @return Pointer to item
 */
static fifo_item_t *sensor_fifo_outdex_ptr(sensor_fifo_t *f)
{
    uint16_t offset = f->outdex * f->element_size;
    fifo_item_t *fi = (fifo_item_t *)&f->elements[offset];
    return fi;
}


/**
 * Returns pointer to item at head of FIFO.
 * @param sensor_id Sensor ID
 * @return Pointer to item, NULL if sensor ID is not valid or FIFO is empty
 */
void *sensor_hub_sensor_data_map(sensor sensor_id)
{
    sensor_t *s = sensor_from_id(sensor_id);
    void *item_ptr = NULL;
    if (s)
    {
        sensor_fifo_t *f = s->fifo;
        block_interrupts();
        if (f && !sensor_fifo_is_empty(f))
        {
            item_ptr = sensor_fifo_outdex_ptr(f);
        }
        unblock_interrupts();
    }

    return item_ptr;

}


/**
 * Returns number of item in FIFO.
 * @param sensor_id Sensor ID
 * @return Number of items in FIFO, 0 if sensor ID is not valid
 */
uint16 sensor_hub_sensor_data_size(sensor sensor_id)
{
    sensor_t *s = sensor_from_id(sensor_id);
    if (s)
    {
        sensor_fifo_t *f = s->fifo;
        if (f)
        {
            uint8 size;
            block_interrupts();
            size = sensor_fifo_size(f);
            unblock_interrupts();
            return size;
        }
    }

    return 0;
}


/**
 * Flush item from FIFO.
 * @param sensor_id Sensor ID
 */
void sensor_hub_sensor_data_flush(sensor sensor_id)
{
    sensor_t *s = sensor_from_id(sensor_id);
    if (s)
    {
        sensor_fifo_t *f = s->fifo;

        block_interrupts();
        if (f && !sensor_fifo_is_empty(f))
        {
            if (s->message_settings == SENSOR_MESSAGES_SOME)
            {
                s->message_settings = SENSOR_MESSAGES_NEXT;
            }

            sensor_fifo_advance_outdex(f);
        }
        unblock_interrupts();
    }
}


/**
 * Set VM task that sensor messages will be sent to.
 * @param sensor_id Sensor ID
 * @param task VM task to send messages to
 * @return Previous VM task or 0 if not previously set.
 */
Task sensor_hub_sensor_set_task(sensor sensor_id, Task task)
{
    Task old_task = NULL;
    sensor_t *s = sensor_from_id(sensor_id);
    if (s)
    {
        old_task = s->task;
        s->task = task;
    }
    return old_task;
}


/**
 * Manually set timestamp of last sensor read.  Intended to be used by interrupt handler where
 * sensor generates interrupts when data is available and the interrupt PIO has timestamping enabled.
 * @param sensor_id Sensor ID
 * @param timestamp Time of sensor read
 */
void sensor_hub_sensor_set_timestamp(sensor sensor_id, uint32 timestamp)
{
    sensor_t *s = sensor_from_id(sensor_id);
    if (s)
    {
        s->timing.next_read_time = timestamp;
    }
}


/**
 * Return timestamp of last sensor read.
 * @param sensor_id Sensor ID
 * @return Last sensor read time, 0 if sensor has not been read or ID is not valid.
 */
uint32 sensor_hub_sensor_get_timestamp(sensor sensor_id)
{
    sensor_t *s = sensor_from_id(sensor_id);
    return s ? s->timing.next_read_time : 0;
}


/**
 * Return boolean indicating if specified sensor is active/on.
 * @param sensor_id Sensor ID
 * @return TRUE if sensor is on, FALSE if sensor is off or ID is not valid.
 */
bool sensor_hub_sensor_is_on(sensor sensor_id)
{
    sensor_t *s = sensor_from_id(sensor_id);
    return s ? s->state >= SENSOR_STATE_ON : FALSE;
}


/**
 * High priority task
 * 1.  Processes received sensor data
 * 2.  Updates next read time
 * @param parameters Unused
 */
static void sensor_hub_task_handler(void *parameters)
{
    UNUSED(parameters);

    for (;;)
    {
        BaseType_t result;
        uint32_t notified_value;

        /* Wait for notification */
        result = xTaskNotifyWait( pdFALSE,    /* Don't clear bits on entry. */
                             ULONG_MAX,        /* Clear all bits on exit. */
                             &notified_value, /* Stores the notified value. */
                             portMAX_DELAY);

        /* Check if we actually got a notification successfully */
        if (result == pdPASS)
        {
            bus_t *b;
            L5_DBG_MSG1("sensor_hub_task_handler, notification %u", notified_value);

            /* Iterate through buses finding match(s) for notifcation(s) */
            b = bus_list;
            while (b)
            {
                const uint8 index = BUS_ID_TO_INDEX(b->id);
                /* Does notication have bit set for this bus ID */
                if (notified_value & (1UL << index))
                {
                    sensor_t *s = b->active_sensor;
                    L5_DBG_MSG1("sensor_hub_task_handler, bus %u notification", b->id);
                    if (s)
                    {
                        L5_DBG_MSG2("sensor_hub_task_handler, active sensor %u, state %u", s->id, s->state);
                        if (s->state == SENSOR_STATE_PROCESS)
                        {
                            /* Call read complete function if we've got a sensor structure,
                             * fifo and there's a read_complete function specified */
                            sensor_fifo_t *f = s->fifo;
                            if (f && s->functions->read_complete)
                            {
                                /* Check if FIFO is full, if so generate warning */
                                if (sensor_fifo_is_full(f))
                                {
                                    L2_DBG_MSG1("sensor_hub_task_handler, sensor %u FIFO full", s->id);
                                }
                                else
                                {
                                    fifo_item_t *fi = sensor_fifo_index_ptr(f);
                                    bool added = s->functions->read_complete(s->id, s->raw_data, fi);

                                    /* If function added data to FIFO advance the index */
                                    if (added)
                                    {
                                        uint8 fifo_size;
                                        sensor_fifo_advance_index(f);
                                        fifo_size = sensor_fifo_size(f);
                                        L5_DBG_MSG2("sensor_hub_task_handler, sensor %u, %u FIFO element(s)", s->id, fifo_size);

                                        if (s->task)
                                        {
                                            if (s->message_settings == SENSOR_MESSAGES_ALL || s->message_settings == SENSOR_MESSAGES_NEXT)
                                            {
                                                MessageSensorData *message = pnew(MessageSensorData);
                                                if (s->message_settings == SENSOR_MESSAGES_NEXT)
                                                {
                                                    s->message_settings = SENSOR_MESSAGES_SOME;
                                                }
                                                message->sensor = s->id;
                                                MessageSend(s->task, MESSAGE_SENSOR_DATA, message);
                                                L5_DBG_MSG1("sensor_hub_task_handler, sending message to task %u", s->task);
                                            }
                                        }
                                    }
                                }
                            }

                            s->state = SENSOR_STATE_ON;

                            /* Sensor data has been processed, so reset timing state so that this sensor can be re-scheduled */
                            if (s->timing.state == SENSOR_TIME_STATE_TRIGGERED)
                            {
                                s->timing.state = SENSOR_TIME_STATE_USED;
                            }

                            /* Processing has completed, so this sensor is no longer 'active' */
                            b->active_sensor = NULL;
                        }
                    }

                    bus_schedule_read(b);
                }

                b = b->next;
            }
        }
    }
}


/*! Sensor task stack size (in octets) */
#define SENSOR_STACK_SIZE   (1024)

/*! Sensor task stack size (in 32 bits words) */
#define SENSOR_STACK_WORDS  (SENSOR_STACK_SIZE / sizeof(StackType_t))

/**
 * Initialise sensor hub
 */
void sensor_hub_init(void)
{
    /* Create high priority sensor hub task for bus updates and sensor data processing */
    static StackType_t sensor_task_stack[SENSOR_STACK_WORDS];
    static StaticTask_t sensor_task_tcb;
    sensor_task = xTaskCreateStatic(
        /*pvTaskCode=*/sensor_hub_task_handler,
        /*pcName=*/"SENSORH",
        /*ulStackDepth=*/SENSOR_STACK_WORDS,
        /*pvParameters=*/NULL,
        /*uxPriority=*/3,
        /*puxStackBuffer=*/sensor_task_stack,
        /*pxTaskBuffer=*/&sensor_task_tcb);

    static StaticSemaphore_t semaphore;
    static StaticSemaphore_t transfer_semaphore;
    sensor_semaphore = xSemaphoreCreateBinaryStatic(&semaphore);
    sensor_transfer_semaphore = xSemaphoreCreateBinaryStatic(&transfer_semaphore);
    (void)xSemaphoreGive(sensor_transfer_semaphore);
}
