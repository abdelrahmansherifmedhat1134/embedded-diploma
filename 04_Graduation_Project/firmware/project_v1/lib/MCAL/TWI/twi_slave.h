/**
 * @file    twi_slave.h
 * @brief   AVR TWI (I2C) Slave Driver for ATmega32
 *
 * Features:
 *  - Interrupt-driven (non-blocking)
 *  - Configurable slave address + optional general call
 *  - User-supplied TX/RX callback hooks
 *  - Internal TX/RX ring buffers
 */

#ifndef TWI_SLAVE_H_
#define TWI_SLAVE_H_

#include <avr/io.h>
#include <stdint.h>

/*===========================================================================
 * Configuration
 *===========================================================================*/

/** Internal RX buffer size (bytes) */
#define TWI_SLAVE_RX_BUFFER_SIZE    32U

/** Internal TX buffer size (bytes) */
#define TWI_SLAVE_TX_BUFFER_SIZE    32U

/** Set to 1 to respond to General Call address (0x00) as well */
#define TWI_SLAVE_GENERAL_CALL      0

/*===========================================================================
 * TWI Slave Status Codes (TWSR & 0xF8)
 *===========================================================================*/

/* Slave Receiver */
#define TWI_SR_SLA_ACK              0x60  /**< SLA+W received, ACK returned        */
#define TWI_SR_ARB_LOST_SLA_ACK     0x68  /**< Arb lost, SLA+W received, ACK ret.  */
#define TWI_SR_GEN_ACK              0x70  /**< General call received, ACK returned  */
#define TWI_SR_ARB_LOST_GEN_ACK     0x78  /**< Arb lost, general call, ACK ret.     */
#define TWI_SR_DATA_ACK             0x80  /**< Data received (SLA), ACK returned    */
#define TWI_SR_DATA_NACK            0x88  /**< Data received (SLA), NACK returned   */
#define TWI_SR_GEN_DATA_ACK         0x90  /**< Data received (gen call), ACK ret.   */
#define TWI_SR_GEN_DATA_NACK        0x98  /**< Data received (gen call), NACK ret.  */
#define TWI_SR_STOP                 0xA0  /**< STOP or repeated START received      */

/* Slave Transmitter */
#define TWI_ST_SLA_ACK              0xA8  /**< SLA+R received, ACK returned         */
#define TWI_ST_ARB_LOST_SLA_ACK     0xB0  /**< Arb lost, SLA+R received, ACK ret.   */
#define TWI_ST_DATA_ACK             0xB8  /**< Data transmitted, ACK received        */
#define TWI_ST_DATA_NACK            0xC0  /**< Data transmitted, NACK received       */
#define TWI_ST_LAST_DATA_ACK        0xC8  /**< Last data transmitted, ACK received   */

/*===========================================================================
 * Callback typedefs
 *===========================================================================*/

/**
 * @brief Called when master has finished writing (STOP/RE-START received).
 *        Read received bytes with TWI_slave_rx_available() / TWI_slave_rx_read().
 */
typedef void (*twi_slave_rx_complete_cb_t)(void);

/**
 * @brief Called when master wants to read from this slave (SLA+R received).
 *        Load reply bytes with TWI_slave_tx_write() before returning.
 */
typedef void (*twi_slave_tx_request_cb_t)(void);

/*===========================================================================
 * Public API
 *===========================================================================*/

/**
 * @brief  Initialise TWI as slave with a 7-bit address.
 * @param  slave_addr   7-bit address (unshifted, 0x00–0x7F)
 * @param  rx_cb        Called after a full master→slave transfer (may be NULL)
 * @param  tx_cb        Called when master requests data (may be NULL)
 */
void TWI_slave_init(uint8_t slave_addr,
                    twi_slave_rx_complete_cb_t rx_cb,
                    twi_slave_tx_request_cb_t  tx_cb);

/**
 * @brief  Disable TWI slave and stop listening.
 */
void TWI_slave_deinit(void);

/*---------------------------------------------------------------------------
 * TX buffer (data to send to master)
 *---------------------------------------------------------------------------*/

/**
 * @brief  Enqueue a byte to transmit to the master.
 *         Call this inside the tx_request callback.
 * @return 1 on success, 0 if TX buffer full
 */
uint8_t TWI_slave_tx_write(uint8_t data);

/**
 * @brief  Enqueue multiple bytes.
 * @return Number of bytes actually queued
 */
uint8_t TWI_slave_tx_write_buf(const uint8_t *buf, uint8_t len);

/**
 * @brief  Clear the TX buffer.
 */
void TWI_slave_tx_flush(void);

/*---------------------------------------------------------------------------
 * RX buffer (data received from master)
 *---------------------------------------------------------------------------*/

/**
 * @brief  Number of bytes available in the RX buffer.
 */
uint8_t TWI_slave_rx_available(void);

/**
 * @brief  Read one byte from the RX buffer.
 * @return Byte value, or 0 if buffer empty
 */
uint8_t TWI_slave_rx_read(void);

/**
 * @brief  Read multiple bytes from the RX buffer.
 * @return Number of bytes actually read
 */
uint8_t TWI_slave_rx_read_buf(uint8_t *buf, uint8_t max_len);

/**
 * @brief  Clear the RX buffer.
 */
void TWI_slave_rx_flush(void);

/**
 * @brief  Returns 1 if a complete RX transaction was received since last check.
 *         Clears the flag on read (polling alternative to callback).
 */
uint8_t TWI_slave_rx_ready(void);

#endif /* TWI_SLAVE_H_ */
