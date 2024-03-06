/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      UART
*/

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "stm32f0xx_usart.h"
#include "stm32f0xx_rcc.h"
#include "main.h"
#include "cli.h"
#include "power.h"
#include "uart.h"
#ifdef TEST
#include "test_st.h"
#endif
#ifdef SCHEME_B
#include "charger_comms.h"
#ifdef CHARGER_COMMS_FAKE_U
#include "fake_earbud.h"
#endif
#endif

/*-----------------------------------------------------------------------------
------------------ PREPROCESSOR DEFINITIONS -----------------------------------
-----------------------------------------------------------------------------*/

#define UART_BIT_RATE 115200

/*
* These should always be powers of 2 in order to simplify calculations and
* save memory.
*/
#define UART_RX_BUFFER_SIZE 1024
#define UART_TX_BUFFER_SIZE 512
#ifdef SCHEME_B
#define UART_CC_RX_BUFFER_SIZE 256
#define UART_CC_TX_BUFFER_SIZE 256
#endif

/*-----------------------------------------------------------------------------
------------------ TYPE DEFINITIONS -------------------------------------------
-----------------------------------------------------------------------------*/

typedef struct
{
    uint8_t *rx_buf;
    uint8_t *tx_buf;
    uint16_t rx_buf_head;
    uint16_t dma_rx_idx;
    uint16_t rx_buf_tail;
    uint16_t tx_buf_head;
    uint16_t tx_buf_tail;
    bool in_progress;
}
UART_INFO;

typedef struct
{
    USART_TypeDef *uart;
    bool use_dma;
    DMA_Channel_TypeDef *rx_dma;
    DMA_Channel_TypeDef *tx_dma;
    uint16_t rx_buf_size;
    uint16_t tx_buf_size;
    uint32_t rr_rx;
    uint32_t rr_tx;
    void (*rx_byte)(uint8_t data);
    void (*tx_done)(void);
}
UART_CONFIG;

/*-----------------------------------------------------------------------------
------------------ PROTOTYPES -------------------------------------------------
-----------------------------------------------------------------------------*/
void uart_charger_comms_tx_done(void);

/*-----------------------------------------------------------------------------
------------------ VARIABLES --------------------------------------------------
-----------------------------------------------------------------------------*/

static uint8_t uart_rx_buf_cli[UART_RX_BUFFER_SIZE];
static uint8_t uart_tx_buf_cli[UART_TX_BUFFER_SIZE];
#ifdef SCHEME_B
static uint8_t uart_rx_buf_dock[UART_CC_RX_BUFFER_SIZE];
static uint8_t uart_tx_buf_dock[UART_CC_TX_BUFFER_SIZE];
#ifdef CHARGER_COMMS_FAKE_U
static uint8_t uart_rx_buf_earbud[UART_CC_RX_BUFFER_SIZE];
static uint8_t uart_tx_buf_earbud[UART_CC_TX_BUFFER_SIZE];
#endif
#endif

UART_CONFIG uart_config[NO_OF_UARTS] =
{
    {
        NULL,
        false,
        NULL,
        NULL,
        UART_RX_BUFFER_SIZE,
        UART_TX_BUFFER_SIZE,
        POWER_RUN_UART_RX,
        POWER_RUN_UART_TX,
        cli_uart_rx,
        NULL
    },

#ifdef SCHEME_B
    {
        NULL,
        true,
        NULL,
        NULL,
        UART_CC_RX_BUFFER_SIZE,
        UART_CC_TX_BUFFER_SIZE,
        POWER_RUN_UART_CC_RX,
        POWER_RUN_UART_CC_TX,
        charger_comms_receive,
        uart_charger_comms_tx_done
    },

#ifdef CHARGER_COMMS_FAKE_U
    {
        NULL,
        false,
        NULL,
        NULL,
        UART_CC_RX_BUFFER_SIZE,
        UART_CC_TX_BUFFER_SIZE,
        POWER_RUN_UART_EB_RX,
        POWER_RUN_UART_EB_TX,
        earbud_rxc,
        NULL
    },
#endif

#endif
};

static UART_INFO uart_info[NO_OF_UARTS];

/*-----------------------------------------------------------------------------
------------------ FUNCTIONS --------------------------------------------------
-----------------------------------------------------------------------------*/

void uart_init(void)
{
    /* Set each UART.
     * This should be static, however some compilers refuse to initialise 
     * the uart structure pointer so do it here. */
    uart_config[UART_CLI].uart = USART1;
#ifdef SCHEME_B
    uart_config[UART_DOCK].uart = USART3;
#endif
#ifdef CHARGER_COMMS_FAKE_U
    uart_config[UART_EARBUD].uart = USART4;
#endif

    /* Enable clock for USART1. */
    RCC->APB2ENR |= RCC_APB2Periph_USART1;

    /* Set USART1 bit rate. */
    USART1->BRR = (SystemCoreClock + (UART_BIT_RATE / 2)) / UART_BIT_RATE;

    /* Enable USART1, enable transmit and receive, enable the interrupts. */
    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE |
                   USART_CR1_UE | USART_CR1_RXNEIE | USART_CR1_TCIE;

    uart_info[UART_CLI].rx_buf = uart_rx_buf_cli;
    uart_info[UART_CLI].tx_buf = uart_tx_buf_cli;

#ifdef SCHEME_B

    /* Enable clock for USART3 and DMA1. */
    RCC->APB1ENR |= RCC_APB1Periph_USART3;
    RCC->AHBENR  |= RCC_AHBPeriph_DMA1;

    /* Set USART3 bit rate. */
    USART3->BRR = (SystemCoreClock + (UART_CC_BIT_RATE / 2)) / UART_CC_BIT_RATE;

    /* Set both TX and RX to be inverted. */
    USART3->CR2 |= USART_CR2_TXINV | USART_CR2_RXINV;

    /* Setup DMA for receive */
    DMA1_Channel6->CPAR = (uint32_t)(&USART3->RDR);
    DMA1_Channel6->CMAR = (uint32_t)uart_rx_buf_dock;
    DMA1_Channel6->CNDTR = UART_CC_RX_BUFFER_SIZE;
    DMA1_Channel6->CCR =  DMA_Mode_Circular | DMA_MemoryInc_Enable |
                          DMA_MemoryDataSize_Byte | DMA_PeripheralDataSize_Byte | DMA_CCR_EN;

    /* Setup DMA for transmit */
    DMA1_Channel7->CPAR = (uint32_t)(&USART3->TDR);
    DMA1_Channel7->CMAR = (uint32_t)uart_tx_buf_dock;
    DMA1_Channel7->CNDTR = UART_CC_TX_BUFFER_SIZE;
    DMA1_Channel7->CCR =  DMA_DIR_PeripheralDST | DMA_Mode_Normal | DMA_MemoryInc_Enable |
                          DMA_MemoryDataSize_Byte | DMA_PeripheralDataSize_Byte;

    /* Setup UART to use DMA for both receive and transmit. */
    USART3->CR3 |= USART_CR3_DMAR | USART_CR3_DMAT;

    /* Enable interrupts and the UART. */
    USART3->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_IDLEIE |
                   USART_CR1_UE | USART_CR1_TCIE;

    uart_config[UART_DOCK].rx_dma = DMA1_Channel6;
    uart_config[UART_DOCK].tx_dma = DMA1_Channel7;

    uart_info[UART_DOCK].rx_buf = uart_rx_buf_dock;
    uart_info[UART_DOCK].tx_buf = uart_tx_buf_dock;

#ifdef CHARGER_COMMS_FAKE_U

    /*
    * Enable clock for USART4.
    */
    RCC->APB1ENR |= RCC_APB1Periph_USART4;

    /*
    * Set USART4 bit rate.
    */
    USART4->BRR = (SystemCoreClock + (UART_CC_BIT_RATE / 2)) / UART_CC_BIT_RATE;

    /*
     * Set both TX and RX to be inverted.
     */
    USART4->CR2 |= USART_CR2_TXINV | USART_CR2_RXINV;

    /*
    * Enable USART3, enable transmit and receive, enable the interrupts.
    */
    USART4->CR1 |= USART_CR1_TE | USART_CR1_RE |
        USART_CR1_UE | USART_CR1_RXNEIE | USART_CR1_TCIE;

    uart_info[UART_EARBUD].rx_buf = uart_rx_buf_earbud;
    uart_info[UART_EARBUD].tx_buf = uart_tx_buf_earbud;

#endif

#endif
}

void uart_clock_disable(void)
{
    uint8_t uart_no;

    /*
    * Before we disable the clock, wait for any transmit in progress to
    * complete.
    */
    for (uart_no=0; uart_no<NO_OF_UARTS; uart_no++)
    {
        UART_INFO *info = &uart_info[uart_no];
        const UART_CONFIG *cfg = &uart_config[uart_no];

        if (info->in_progress)
        {
            while (!(cfg->uart->ISR & USART_ISR_TC));
            info->in_progress = false;
            info->tx_buf_tail = (info->tx_buf_tail + 1) & (cfg->tx_buf_size - 1);
        }
    }

    RCC->APB2ENR &= ~RCC_APB2Periph_USART1;
#ifdef SCHEME_B
    RCC->APB1ENR &= ~RCC_APB1Periph_USART3;
#ifdef CHARGER_COMMS_FAKE_U
    RCC->APB1ENR &= ~RCC_APB1Periph_USART4;
#endif
#endif
}

#ifdef SCHEME_B
void uart_charger_comms_tx_done(void)
{
    /* Re-enable UART receive after transmitting */
    USART3->CR1 |= USART_CR1_RE;
    charger_comms_transmit_done();
}
#endif

void uart_tx(UART_ID uart_no, uint8_t *data, uint16_t len)
{
    UART_INFO *info = &uart_info[uart_no];
    const UART_CONFIG *cfg = &uart_config[uart_no];

    if (cfg->use_dma)
    {
        if (!info->in_progress)
        {
            info->in_progress = true;

#ifdef SCHEME_B
            /* Before transmitting on the one-wire UART, disable UART receive so
             * we don't receive the same the data we're transmitting */
            if (uart_no == UART_DOCK)
            {
                cfg->uart->CR1 &= ~USART_CR1_RE;
            }
#endif
            /* Copy the data and set-up a DMA transfer */
            memcpy(info->tx_buf, data, len);
            cfg->tx_dma->CMAR = (uint32_t)info->tx_buf;
            cfg->tx_dma->CNDTR = len;
            cfg->tx_dma->CCR |= DMA_CCR_EN;

            power_set_run_reason(cfg->rr_tx);
        }
        return;
    }

    /*
    * Put all the data in the buffer to be sent.
    */
    while (len--)
    {
        uint16_t next_head =
            (uint16_t)((info->tx_buf_head + 1) & (cfg->tx_buf_size - 1));

        if (next_head == info->tx_buf_tail)
        {
            /*
            * Not enough room in the buffer, so give up. Data output will be
            * truncated.
            */
            break;
        }

        info->tx_buf[info->tx_buf_head] = (uint8_t)*data++;
        info->tx_buf_head = next_head;
    }

    if (!info->in_progress)
    {
        info->in_progress = true;
        
#ifdef SCHEME_B
        /* Before transmitting on the one-wire UART, disable UART receive so
         * we don't receive the same the data we're transmitting */
        if (uart_no == UART_DOCK)
        {
            USART3->CR1 &= ~USART_CR1_RE;
        }
#endif
        cfg->uart->TDR = info->tx_buf[info->tx_buf_tail];
    }

    power_set_run_reason(cfg->rr_tx);
}

/*
* Force out everything in the TX buffer, to be used in the event of a fault.
*/
void uart_dump(void)
{
    uint8_t uart_no;

    for (uart_no=0; uart_no<NO_OF_UARTS; uart_no++)
    {
        UART_INFO *info = &uart_info[uart_no];
        const UART_CONFIG *cfg = &uart_config[uart_no];

        while (info->tx_buf_head != info->tx_buf_tail)
        {
            cfg->uart->TDR = info->tx_buf[info->tx_buf_tail];
            while (!(cfg->uart->ISR & USART_ISR_TC));
            cfg->uart->ICR |= USART_ISR_TC;
            info->tx_buf_tail = (info->tx_buf_tail + 1) & (cfg->tx_buf_size - 1);
        }
    }
}

void uart_tx_periodic(void)
{
    uint8_t uart_no;

    for (uart_no=0; uart_no<NO_OF_UARTS; uart_no++)
    {
        UART_INFO *info = &uart_info[uart_no];
        const UART_CONFIG *cfg = &uart_config[uart_no];

        /*
        * Kick off any data to be sent.
        */
        if (!info->in_progress)
        {
            if (info->tx_buf_head != info->tx_buf_tail)
            {
                info->in_progress = true;
                cfg->uart->TDR = info->tx_buf[info->tx_buf_tail];
            }
        }
    }
}

void uart_rx_periodic(void)
{
    uint8_t uart_no;

    for (uart_no=0; uart_no<NO_OF_UARTS; uart_no++)
    {
        UART_INFO *info = &uart_info[uart_no];
        const UART_CONFIG *cfg = &uart_config[uart_no];

        /*
        * Handle received data.
        */
        while (info->rx_buf_head != info->rx_buf_tail)
        {
            cfg->rx_byte(info->rx_buf[info->rx_buf_tail]);
            info->rx_buf_tail = (info->rx_buf_tail + 1) & (cfg->rx_buf_size - 1);
        }
        power_clear_run_reason(cfg->rr_rx);
    }
    power_clear_run_reason(POWER_RUN_UART_RX);
}

inline static void uart_irq(uint8_t uart_no)
{
    UART_INFO *info = &uart_info[uart_no];
    const UART_CONFIG *cfg = &uart_config[uart_no];
    uint16_t isr = cfg->uart->ISR;

    cfg->uart->ICR |= isr;

    /* New character received */
    if (isr & USART_ISR_RXNE)
    {
        power_set_run_reason(cfg->rr_rx);
        info->rx_buf[info->rx_buf_head] = (uint8_t)(cfg->uart->RDR & 0xFF);
        info->rx_buf_head = (info->rx_buf_head + 1) & (cfg->rx_buf_size - 1);
    }

    /* Line is idle */
    if (cfg->use_dma && isr & USART_ISR_IDLE)
    {
        /* Get index where the DMA has written up to. */
        uint16_t pos = cfg->rx_buf_size - cfg->rx_dma->CNDTR;

        /* If there is new data, update the rx buffer head so we can process in
         * the background. */
        if (pos != info->dma_rx_idx)
        {
            power_set_run_reason(cfg->rr_rx);
            info->rx_buf_head = (info->rx_buf_head + (pos - info->dma_rx_idx)) & (cfg->rx_buf_size - 1);
        }
        info->dma_rx_idx = pos;
    }

    /* Transmission complete */
    if (isr & USART_ISR_TC)
    {
        if (info->in_progress)
        {
            bool tx_complete = false;

            if (cfg->use_dma)
            {
                /* End the DMA transfer */
                cfg->tx_dma->CCR &= ~DMA_CCR_EN;
                tx_complete = true;
            }
            else
            {
                info->tx_buf_tail = (info->tx_buf_tail + 1) & (cfg->tx_buf_size - 1);

                if (info->tx_buf_head != info->tx_buf_tail)
                {
                    cfg->uart->TDR = info->tx_buf[info->tx_buf_tail];
                }
                else
                {
                    tx_complete = true;
                }
            }
            
            if (tx_complete)
            {
                if (cfg->tx_done)
                {
                    cfg->tx_done();
                }
                power_clear_run_reason(cfg->rr_tx);
                info->in_progress = false;
            }
        }
    }
}

void USART1_IRQHandler(void)
{
    uart_irq(UART_CLI);
}

#ifdef SCHEME_B
void USART3_4_IRQHandler(void)
{
    uart_irq(UART_DOCK);
#ifdef CHARGER_COMMS_FAKE_U
    uart_irq(UART_EARBUD);
#endif
}
#endif

void uart_set_baud_rate(UART_ID uart_no, uint32_t rate)
{
    const UART_CONFIG *cfg = &uart_config[uart_no];
    cfg->uart->BRR = (SystemCoreClock + (rate / 2)) / rate;
}

CLI_RESULT ats_baud_rate(uint8_t cmd_source __attribute__((unused)))
{
    bool ret = CLI_ERROR;

    long uart_no;
    long baud_rate;

    if (cli_get_next_parameter(&uart_no, 10) &&
        cli_get_next_parameter(&baud_rate, 10))
    {
        if (uart_no < NO_OF_UARTS && baud_rate != 0)
        {
            uart_set_baud_rate((UART_ID)uart_no, baud_rate);
            ret = CLI_OK;
        }
    }

    return ret;
}
