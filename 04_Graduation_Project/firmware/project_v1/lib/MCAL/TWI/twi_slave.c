/**
 * @file    twi_slave.c
 * @brief   AVR TWI (I2C) Slave Driver implementation for ATmega32
 *          Fully interrupt-driven — no polling inside ISR.
 */

#include "twi_slave.h"
#include <avr/interrupt.h>
#include <string.h>

/*===========================================================================
 * Internal state
 *===========================================================================*/

/* ---- TX ring buffer ---- */
static volatile uint8_t  tx_buf[TWI_SLAVE_TX_BUFFER_SIZE];
static volatile uint8_t  tx_head = 0;   /* write index */
static volatile uint8_t  tx_tail = 0;   /* read index  */
static volatile uint8_t  tx_count = 0;

/* ---- RX ring buffer ---- */
static volatile uint8_t  rx_buf[TWI_SLAVE_RX_BUFFER_SIZE];
static volatile uint8_t  rx_head = 0;
static volatile uint8_t  rx_tail = 0;
static volatile uint8_t  rx_count = 0;

/* ---- Flags & callbacks ---- */
static volatile uint8_t  rx_ready_flag = 0;

static twi_slave_rx_complete_cb_t  rx_complete_cb = 0;
static twi_slave_tx_request_cb_t   tx_request_cb  = 0;

/*===========================================================================
 * Private helpers
 *===========================================================================*/

#define TWI_STATUS_MASK     0xF8
#define TWI_GET_STATUS()    (TWSR & TWI_STATUS_MASK)

/** Reply with ACK and re-enable interrupt */
static inline void twi_ack(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWEA);
}

/** Reply with NACK (last byte or buffer-full) */
static inline void twi_nack(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
}

/** Recover and re-listen (after error / unexpected status) */
static inline void twi_recover(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWEA);
}

/* ---- TX buffer ops ---- */
static uint8_t tx_push(uint8_t byte)
{
    if (tx_count >= TWI_SLAVE_TX_BUFFER_SIZE) return 0;
    tx_buf[tx_head] = byte;
    tx_head = (tx_head + 1) % TWI_SLAVE_TX_BUFFER_SIZE;
    tx_count++;
    return 1;
}

static uint8_t tx_pop(uint8_t *byte)
{
    if (tx_count == 0) return 0;
    *byte = tx_buf[tx_tail];
    tx_tail = (tx_tail + 1) % TWI_SLAVE_TX_BUFFER_SIZE;
    tx_count--;
    return 1;
}

/* ---- RX buffer ops ---- */
static uint8_t rx_push(uint8_t byte)
{
    if (rx_count >= TWI_SLAVE_RX_BUFFER_SIZE) return 0;
    rx_buf[rx_head] = byte;
    rx_head = (rx_head + 1) % TWI_SLAVE_RX_BUFFER_SIZE;
    rx_count++;
    return 1;
}

static uint8_t rx_pop(uint8_t *byte)
{
    if (rx_count == 0) return 0;
    *byte = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % TWI_SLAVE_RX_BUFFER_SIZE;
    rx_count--;
    return 1;
}

/*===========================================================================
 * Initialisation
 *===========================================================================*/

void TWI_slave_init(uint8_t slave_addr,
                    twi_slave_rx_complete_cb_t rx_cb,
                    twi_slave_tx_request_cb_t  tx_cb)
{
    rx_complete_cb = rx_cb;
    tx_request_cb  = tx_cb;

    /* Load 7-bit address into TWAR (bits 7:1); bit 0 = General Call enable */
    TWAR = (uint8_t)((slave_addr << 1) | (TWI_SLAVE_GENERAL_CALL & 0x01));

    /* Clear TWBR — not used in slave mode, but good practice */
    TWBR = 0x00;

    /* Enable TWI + ACK + interrupt */
    TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWEA) | (1 << TWINT);

    sei();   /* Ensure global interrupts are enabled */
}

void TWI_slave_deinit(void)
{
    TWCR = 0x00;
    TWAR = 0x00;
}

/*===========================================================================
 * TX buffer public API
 *===========================================================================*/

uint8_t TWI_slave_tx_write(uint8_t data)
{
    return tx_push(data);
}

uint8_t TWI_slave_tx_write_buf(const uint8_t *buf, uint8_t len)
{
    uint8_t written = 0;
    for (uint8_t i = 0; i < len; i++) {
        if (!tx_push(buf[i])) break;
        written++;
    }
    return written;
}

void TWI_slave_tx_flush(void)
{
    tx_head  = tx_tail = tx_count = 0;
}

/*===========================================================================
 * RX buffer public API
 *===========================================================================*/

uint8_t TWI_slave_rx_available(void)
{
    return rx_count;
}

uint8_t TWI_slave_rx_read(void)
{
    uint8_t byte = 0;
    rx_pop(&byte);
    return byte;
}

uint8_t TWI_slave_rx_read_buf(uint8_t *buf, uint8_t max_len)
{
    uint8_t i = 0;
    while (i < max_len && rx_pop(&buf[i])) i++;
    return i;
}

void TWI_slave_rx_flush(void)
{
    rx_head  = rx_tail = rx_count = 0;
}

uint8_t TWI_slave_rx_ready(void)
{
    if (rx_ready_flag) {
        rx_ready_flag = 0;
        return 1;
    }
    return 0;
}

/*===========================================================================
 * TWI Interrupt Service Routine
 *===========================================================================*/

ISR(TWI_vect)
{
    uint8_t status = TWI_GET_STATUS();
    uint8_t byte;

    switch (status)
    {
        /*-------------------------------------------------------------------
         * SLAVE RECEIVER — master is writing to us
         *-------------------------------------------------------------------*/

        case TWI_SR_SLA_ACK:            /* Own SLA+W received → ACK */
        case TWI_SR_ARB_LOST_SLA_ACK:   /* After arb lost        */
        case TWI_SR_GEN_ACK:            /* General call → ACK    */
        case TWI_SR_ARB_LOST_GEN_ACK:
            /* Ready to receive; ACK if buffer has space */
            if (rx_count < TWI_SLAVE_RX_BUFFER_SIZE)
                twi_ack();
            else
                twi_nack();
            break;

        case TWI_SR_DATA_ACK:           /* Data byte received → ACK */
        case TWI_SR_GEN_DATA_ACK:
            byte = TWDR;
            if (rx_push(byte) && rx_count < TWI_SLAVE_RX_BUFFER_SIZE)
                twi_ack();
            else
                twi_nack();
            break;

        case TWI_SR_DATA_NACK:          /* Data received → we sent NACK */
        case TWI_SR_GEN_DATA_NACK:
            rx_push(TWDR);              /* Store anyway if possible     */
            twi_ack();                  /* Re-enable for next transfer  */
            break;

        case TWI_SR_STOP:               /* STOP or Re-START received — transfer done */
            rx_ready_flag = 1;
            if (rx_complete_cb) rx_complete_cb();
            twi_ack();                  /* Re-enable listening          */
            break;

        /*-------------------------------------------------------------------
         * SLAVE TRANSMITTER — master is reading from us
         *-------------------------------------------------------------------*/

        case TWI_ST_SLA_ACK:            /* SLA+R received → load TX data */
        case TWI_ST_ARB_LOST_SLA_ACK:
            TWI_slave_tx_flush();       /* Fresh buffer for this request */
            if (tx_request_cb) tx_request_cb();
            /* Fall through to send first byte */
            /* intentional fall-through */

        case TWI_ST_DATA_ACK:           /* Previous byte ACK'd → send next */
            if (tx_pop(&byte)) {
                TWDR = byte;
                /* ACK if more data is available, else prepare for NACK */
                if (tx_count > 0)
                    twi_ack();
                else
                    twi_nack();
            } else {
                /* Nothing to send — send 0xFF as dummy */
                TWDR = 0xFF;
                twi_nack();
            }
            break;

        case TWI_ST_DATA_NACK:          /* Master sent NACK → done reading */
        case TWI_ST_LAST_DATA_ACK:      /* Last byte was ACK'd             */
            twi_ack();                  /* Switch back to addressable mode */
            break;

        /*-------------------------------------------------------------------
         * Error / unexpected state — recover
         *-------------------------------------------------------------------*/
        default:
            twi_recover();
            break;
    }
}
