/*!
\copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interrupts
*/

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

#include "stm32f0xx.h"
#include "stm32f0xx_misc.h"
#include "stm32f0xx_exti.h"
#include "stm32f0xx_syscfg.h"
#include "stm32f0xx_rcc.h"
#include "main.h"
#include "case.h"
#include "gpio.h"
#include "wdog.h"
#include "memory.h"
#include "uart.h"
#include "interrupt.h"
#include "battery.h"

/*-----------------------------------------------------------------------------
------------------ PREPROCESSOR DEFINITIONS -----------------------------------
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
------------------ VARIABLES --------------------------------------------------
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
------------------ FUNCTIONS --------------------------------------------------
-----------------------------------------------------------------------------*/

void EXTI_line_disable(uint16_t pin) {
    EXTI->IMR ^= GPIO_EXTI(pin);
    EXTI->EMR |= GPIO_EXTI(pin);
}

void EXTI_line_enable(uint16_t pin) {
    EXTI->IMR |= GPIO_EXTI(pin);
    EXTI->EMR &= ~GPIO_EXTI(pin);
}

static void interrupt_nvic_init(IRQn_Type irqn, uint8_t priority)
{
    NVIC_InitTypeDef nvicStructure;

    nvicStructure.NVIC_IRQChannel = irqn;
    nvicStructure.NVIC_IRQChannelPriority = priority;
    nvicStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvicStructure);
    NVIC_EnableIRQ(irqn);
}

void interrupt_init(void)
{
    /*
    * Set external interrupts on lid and charger detection pins
    * to trigger on rising and falling edges.
    */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    SYSCFG_EXTILineConfig(GPIO_PORT_NUMBER(GPIO_MAG_SENSOR),
                          GPIO_PIN_NUMBER(GPIO_MAG_SENSOR));
    SYSCFG_EXTILineConfig(GPIO_PORT_NUMBER(GPIO_CHG_SENSE),
                          GPIO_PIN_NUMBER(GPIO_CHG_SENSE));

#ifdef HAVE_LOAD_SWITCH
    SYSCFG_EXTILineConfig(GPIO_PORT_NUMBER(GPIO_LOAD_FAULT_N),
                          GPIO_PIN_NUMBER(GPIO_LOAD_FAULT_N));
#endif

#ifdef FAST_TIMER_INTERRUPT
    interrupt_nvic_init(TIM14_IRQn, 0);
#endif
#ifdef USB_ENABLED
    interrupt_nvic_init(USB_IRQn, 0);
#endif
    interrupt_nvic_init(ADC1_IRQn, 1);
    interrupt_nvic_init(EXTI0_1_IRQn, 1);
#ifdef HAVE_EARBUD_SWITCHES
    interrupt_nvic_init(EXTI2_3_IRQn, 1);
#endif
    interrupt_nvic_init(EXTI4_15_IRQn, 1);
#ifdef SCHEME_A
    interrupt_nvic_init(TIM3_IRQn, 0);
#endif
#ifdef SCHEME_B
    interrupt_nvic_init(USART3_4_IRQn, 3);
#endif
    interrupt_nvic_init(USART1_IRQn, 3);
    interrupt_nvic_init(TIM17_IRQn, 3);
    interrupt_nvic_init(RTC_IRQn, 3);

    uint32_t exti_gpio_pins = (GPIO_EXTI(GPIO_MAG_SENSOR) | GPIO_EXTI(GPIO_CHG_SENSE)
#ifdef HAVE_EARBUD_SWITCHES
                              | GPIO_EXTI(GPIO_SWITCH_L) | GPIO_EXTI(GPIO_SWITCH_R)
#endif
    );

    EXTI->IMR |= exti_gpio_pins;
    EXTI->EMR &= ~(exti_gpio_pins);
    EXTI->RTSR |= exti_gpio_pins;
    EXTI->FTSR |= exti_gpio_pins;

#ifdef HAVE_LOAD_SWITCH
    EXTI->IMR |= GPIO_EXTI(GPIO_LOAD_FAULT_N);
    EXTI->EMR &= ~GPIO_EXTI(GPIO_LOAD_FAULT_N);
    EXTI->FTSR |= GPIO_EXTI(GPIO_LOAD_FAULT_N);

    EXTI->PR |= GPIO_EXTI(GPIO_LOAD_FAULT_N);
#endif

    /*
    * Clear EXTI pending register for select pins.
    */
    EXTI->PR |= exti_gpio_pins;
}

void EXTI0_1_IRQHandler(void)
{
    uint32_t pr = EXTI->PR & 0x00000003; //bits 0 to 1

    EXTI->PR = pr; //clear all bits for this handler

    if (pr & GPIO_EXTI(GPIO_MAG_SENSOR))
    {
        /*
        * Lid open/close detected.
        */
#ifdef HAVE_EARBUD_SWITCHES
        case_lid_intr_h();
#else
        case_event_occurred();
#endif
    }

#ifdef HAVE_LOAD_SWITCH
    if (pr & GPIO_EXTI(GPIO_LOAD_FAULT_N))
    {
        battery_overcurrent();
    }
#endif
}

#ifdef HAVE_EARBUD_SWITCHES
void EXTI2_3_IRQHandler(void)
{
    uint32_t pr = EXTI->PR & 0x0000000c; // bits 2 to 3

    EXTI->PR = pr; //clear all bits for this handler

    if (pr & GPIO_EXTI(GPIO_SWITCH_R))
    {
        case_switch_intr_h(GPIO_SWITCH_R);
    }
}
#endif

void EXTI4_15_IRQHandler(void)
{
    uint32_t pr = EXTI->PR & 0x0000FFF0; // bits 4 to 15

    EXTI->PR = pr; //clear all bits for this handler

    if (pr & GPIO_EXTI(GPIO_CHG_SENSE))
    {
        /*
        * Charger connection/disconnection detected.
        */
#ifdef HAVE_EARBUD_SWITCHES
        case_chg_intr_h();
#else
        case_event_occurred();
#endif
    }
#ifdef HAVE_EARBUD_SWITCHES
    if (pr & GPIO_EXTI(GPIO_SWITCH_L))
    {
        case_switch_intr_h(GPIO_SWITCH_L);
    }
#endif
}

void HardFault_Handler(void)
{
    uint8_t cmd_source = 0;

    /*
    * Kick watchdog so we definitely have enough time to get debug messages
    * out.
    */
    wdog_kick();

    /*
    * Output information of interest.
    */
    PRINT("HARD FAULT");
    mem_stack_dump(cmd_source);

    /*
    * Loop until watchdog reset.
    */
    while (1)
    {
        uart_dump();
    }
}
