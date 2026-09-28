/**
 * @file    twi.c
 * @brief   AVR TWI (I2C) Driver implementation for ATmega32
 */
#include "twi.h"
#include <avr/io.h>
#include <util/twi.h>   /* AVR-libc TWI status code macros (optional cross-check) */

/*===========================================================================
 * Internal helpers
 *===========================================================================*/

/** Mask to extract status code bits from TWSR */
#define TWI_STATUS_MASK     0xF8

/** Read masked status register */
#define TWI_GET_STATUS()    (TWSR & TWI_STATUS_MASK)

/**
 * @brief  Send a TWI command and wait for TWINT flag.
 *         TWCR is written with TWEN always set.
 * @param  control_flags  Additional TWCR bits (e.g. TWSTA, TWSTO, TWEA)
 */
static inline void twi_send_cmd(uint8_t control_flags)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | control_flags;
    /* Wait until hardware clears TWINT (operation complete) */
    while (!(TWCR & (1 << TWINT)));
}

/*===========================================================================
 * Initialisation
 *===========================================================================*/

void TWI_init(void)
{
    /* Set SCL prescaler bits in TWSR (bits 1:0) */
    TWSR = (TWI_TWPS_VALUE & 0x03);

    /* Set bit-rate register */
    TWBR = TWI_TWBR_VALUE;

    /* Enable TWI module */
    TWCR = (1 << TWEN);
}

void TWI_deinit(void)
{
    TWCR = 0x00;   /* Disable TWI */
    TWSR = 0x00;
    TWBR = 0x00;
}

/*===========================================================================
 * Low-level primitives
 *===========================================================================*/

uint8_t TWI_get_status(void)
{
    return TWI_GET_STATUS();
}

/* ---- START / STOP ---- */

TWI_Error_t TWI_start(void)
{
    twi_send_cmd((1 << TWSTA));   /* Send START */

    if (TWI_GET_STATUS() != TWI_STATUS_START)
        return TWI_ERR_START;

    return TWI_OK;
}

TWI_Error_t TWI_repeated_start(void)
{
    twi_send_cmd((1 << TWSTA));   /* Send (Re)START */

    uint8_t status = TWI_GET_STATUS();
    if (status != TWI_STATUS_REPEATED_START && status != TWI_STATUS_START)
        return TWI_ERR_START;

    return TWI_OK;
}

void TWI_stop(void)
{
    /* Send STOP — no need to wait for TWINT (STOP clears automatically) */
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

/* ---- ADDRESS PHASE ---- */

TWI_Error_t TWI_write_address(uint8_t slave_addr)
{
    /* SLA+W: shift left and clear R/W bit */
    TWDR = (uint8_t)((slave_addr << 1) & 0xFE);
    twi_send_cmd(0);   /* Clear TWINT to transmit */

    uint8_t status = TWI_GET_STATUS();
    if (status == TWI_STATUS_MT_SLA_NACK)  return TWI_ERR_SLA_NACK;
    if (status == TWI_STATUS_MT_ARB_LOST)  return TWI_ERR_ARB_LOST;
    if (status != TWI_STATUS_MT_SLA_ACK)   return TWI_ERR_BUS;

    return TWI_OK;
}

TWI_Error_t TWI_read_address(uint8_t slave_addr)
{
    /* SLA+R: shift left and set R/W bit */
    TWDR = (uint8_t)((slave_addr << 1) | 0x01);
    twi_send_cmd(0);

    uint8_t status = TWI_GET_STATUS();
    if (status == TWI_STATUS_MR_SLA_NACK)  return TWI_ERR_SLA_NACK;
    if (status == TWI_STATUS_MT_ARB_LOST)  return TWI_ERR_ARB_LOST;
    if (status != TWI_STATUS_MR_SLA_ACK)   return TWI_ERR_BUS;

    return TWI_OK;
}

/* ---- DATA PHASE ---- */

TWI_Error_t TWI_write_byte(uint8_t data)
{
    TWDR = data;
    twi_send_cmd(0);

    uint8_t status = TWI_GET_STATUS();
    if (status == TWI_STATUS_MT_DATA_NACK) return TWI_ERR_DATA_NACK;
    if (status == TWI_STATUS_MT_ARB_LOST)  return TWI_ERR_ARB_LOST;
    if (status != TWI_STATUS_MT_DATA_ACK)  return TWI_ERR_BUS;

    return TWI_OK;
}

TWI_Error_t TWI_read_byte_ack(uint8_t *data)
{
    /* Enable ACK so slave continues sending */
    twi_send_cmd((1 << TWEA));

    if (TWI_GET_STATUS() != TWI_STATUS_MR_DATA_ACK)
        return TWI_ERR_BUS;

    *data = TWDR;
    return TWI_OK;
}

TWI_Error_t TWI_read_byte_nack(uint8_t *data)
{
    /* No TWEA — send NACK to signal last byte */
    twi_send_cmd(0);

    if (TWI_GET_STATUS() != TWI_STATUS_MR_DATA_NACK)
        return TWI_ERR_BUS;

    *data = TWDR;
    return TWI_OK;
}

/*===========================================================================
 * High-level convenience functions
 *===========================================================================*/

TWI_Error_t TWI_master_transmit(uint8_t slave_addr, const uint8_t *buf, uint8_t len)
{
    TWI_Error_t err;

    err = TWI_start();
    if (err != TWI_OK) return err;

    err = TWI_write_address(slave_addr);
    if (err != TWI_OK) { TWI_stop(); return err; }

    for (uint8_t i = 0; i < len; i++) {
        err = TWI_write_byte(buf[i]);
        if (err != TWI_OK) { TWI_stop(); return err; }
    }

    TWI_stop();
    return TWI_OK;
}

TWI_Error_t TWI_master_receive(uint8_t slave_addr, uint8_t *buf, uint8_t len)
{
    TWI_Error_t err;

    if (len == 0) return TWI_OK;

    err = TWI_start();
    if (err != TWI_OK) return err;

    err = TWI_read_address(slave_addr);
    if (err != TWI_OK) { TWI_stop(); return err; }

    for (uint8_t i = 0; i < len; i++) {
        if (i < (len - 1)) {
            err = TWI_read_byte_ack(&buf[i]);   /* ACK all but last */
        } else {
            err = TWI_read_byte_nack(&buf[i]);  /* NACK last byte   */
        }
        if (err != TWI_OK) { TWI_stop(); return err; }
    }

    TWI_stop();
    return TWI_OK;
}

TWI_Error_t TWI_master_write_read(uint8_t  slave_addr,
                                  const uint8_t *write_buf, uint8_t write_len,
                                  uint8_t       *read_buf,  uint8_t read_len)
{
    TWI_Error_t err;

    /* ---- Write phase ---- */
    err = TWI_start();
    if (err != TWI_OK) return err;

    err = TWI_write_address(slave_addr);
    if (err != TWI_OK) { TWI_stop(); return err; }

    for (uint8_t i = 0; i < write_len; i++) {
        err = TWI_write_byte(write_buf[i]);
        if (err != TWI_OK) { TWI_stop(); return err; }
    }

    /* ---- Repeated START then read phase ---- */
    if (read_len > 0) {
        err = TWI_repeated_start();
        if (err != TWI_OK) { TWI_stop(); return err; }

        err = TWI_read_address(slave_addr);
        if (err != TWI_OK) { TWI_stop(); return err; }

        for (uint8_t i = 0; i < read_len; i++) {
            if (i < (read_len - 1)) {
                err = TWI_read_byte_ack(&read_buf[i]);
            } else {
                err = TWI_read_byte_nack(&read_buf[i]);
            }
            if (err != TWI_OK) { TWI_stop(); return err; }
        }
    }

    TWI_stop();
    return TWI_OK;
}
