/**
 * @file    twi.h
 * @brief   AVR TWI (I2C) Driver for ATmega32
 * @author  Driver for ATmega32 @ 8MHz / 16MHz
 *
 * Supports:
 *  - Master Transmitter mode
 *  - Master Receiver mode
 *  - Standard (100kHz) and Fast (400kHz) modes
 *  - Blocking API with status-code error handling
 */

#ifndef TWI_H_
#define TWI_H_

#include <avr/io.h>
#include <stdint.h>

/*===========================================================================
 * Configuration
 *===========================================================================*/

/** CPU frequency in Hz — override via compiler flag if needed */
#ifndef F_CPU
#define F_CPU               16000000UL
#endif

/** Desired SCL frequency (100kHz standard / 400kHz fast mode) */
#define TWI_SCL_FREQ        100000UL

/**
 * TWBR value formula:  TWBR = (F_CPU / SCL_FREQ - 16) / (2 * 4^TWPS)
 * Using prescaler = 0  →  4^0 = 1
 */
#define TWI_TWBR_VALUE      ((uint8_t)(( (F_CPU / TWI_SCL_FREQ) - 16 ) / 2))
#define TWI_TWPS_VALUE      (0x00)   /**< Prescaler bits: 00 → ×1 */

/*===========================================================================
 * TWI Status Codes (TWSR masked with 0xF8)
 *===========================================================================*/

/* General */
#define TWI_STATUS_BUS_ERROR            0x00

/* Master */
#define TWI_STATUS_START                0x08   /**< START transmitted          */
#define TWI_STATUS_REPEATED_START       0x10   /**< Repeated START transmitted  */

/* Master Transmitter */
#define TWI_STATUS_MT_SLA_ACK           0x18   /**< SLA+W sent, ACK received   */
#define TWI_STATUS_MT_SLA_NACK          0x20   /**< SLA+W sent, NACK received  */
#define TWI_STATUS_MT_DATA_ACK          0x28   /**< Data sent, ACK received    */
#define TWI_STATUS_MT_DATA_NACK         0x30   /**< Data sent, NACK received   */
#define TWI_STATUS_MT_ARB_LOST          0x38   /**< Arbitration lost           */

/* Master Receiver */
#define TWI_STATUS_MR_SLA_ACK           0x40   /**< SLA+R sent, ACK received   */
#define TWI_STATUS_MR_SLA_NACK          0x48   /**< SLA+R sent, NACK received  */
#define TWI_STATUS_MR_DATA_ACK          0x50   /**< Data received, ACK sent    */
#define TWI_STATUS_MR_DATA_NACK         0x58   /**< Data received, NACK sent   */

/*===========================================================================
 * Return / Error codes
 *===========================================================================*/

typedef enum {
    TWI_OK              =  0,   /**< Operation successful                    */
    TWI_ERR_START       = -1,   /**< Failed to generate START condition      */
    TWI_ERR_SLA_NACK    = -2,   /**< Slave did not acknowledge address       */
    TWI_ERR_DATA_NACK   = -3,   /**< Slave did not acknowledge data byte     */
    TWI_ERR_ARB_LOST    = -4,   /**< Bus arbitration lost                    */
    TWI_ERR_BUS         = -5,   /**< Generic bus error                       */
} TWI_Error_t;

/*===========================================================================
 * Public API
 *===========================================================================*/

/**
 * @brief  Initialise TWI peripheral (sets TWBR, TWPS, enables TWEN).
 */
void TWI_init(void);

/**
 * @brief  Disable TWI peripheral and release pins.
 */
void TWI_deinit(void);

/**
 * @brief  Generate a START condition and wait for completion.
 * @return TWI_OK or TWI_ERR_START
 */
TWI_Error_t TWI_start(void);

/**
 * @brief  Generate a REPEATED START condition.
 * @return TWI_OK or TWI_ERR_START
 */
TWI_Error_t TWI_repeated_start(void);

/**
 * @brief  Generate a STOP condition.
 */
void TWI_stop(void);

/**
 * @brief  Send slave address + Write bit (SLA+W).
 * @param  slave_addr  7-bit slave address (unshifted)
 * @return TWI_OK, TWI_ERR_SLA_NACK, or TWI_ERR_ARB_LOST
 */
TWI_Error_t TWI_write_address(uint8_t slave_addr);

/**
 * @brief  Send slave address + Read bit (SLA+R).
 * @param  slave_addr  7-bit slave address (unshifted)
 * @return TWI_OK, TWI_ERR_SLA_NACK, or TWI_ERR_ARB_LOST
 */
TWI_Error_t TWI_read_address(uint8_t slave_addr);

/**
 * @brief  Transmit one data byte.
 * @param  data  Byte to transmit
 * @return TWI_OK, TWI_ERR_DATA_NACK, or TWI_ERR_ARB_LOST
 */
TWI_Error_t TWI_write_byte(uint8_t data);

/**
 * @brief  Read one byte and send ACK (more bytes to follow).
 * @param  data  Pointer to store received byte
 * @return TWI_OK
 */
TWI_Error_t TWI_read_byte_ack(uint8_t *data);

/**
 * @brief  Read one byte and send NACK (last byte in transfer).
 * @param  data  Pointer to store received byte
 * @return TWI_OK
 */
TWI_Error_t TWI_read_byte_nack(uint8_t *data);

/*---------------------------------------------------------------------------
 * High-level convenience functions
 *---------------------------------------------------------------------------*/

/**
 * @brief  Write N bytes to a slave device.
 * @param  slave_addr  7-bit slave address
 * @param  buf         Data buffer
 * @param  len         Number of bytes to write
 * @return TWI_OK or error code
 */
TWI_Error_t TWI_master_transmit(uint8_t slave_addr, const uint8_t *buf, uint8_t len);

/**
 * @brief  Read N bytes from a slave device.
 * @param  slave_addr  7-bit slave address
 * @param  buf         Buffer to store received bytes
 * @param  len         Number of bytes to read
 * @return TWI_OK or error code
 */
TWI_Error_t TWI_master_receive(uint8_t slave_addr, uint8_t *buf, uint8_t len);

/**
 * @brief  Write then read (register-read pattern with repeated START).
 * @param  slave_addr   7-bit slave address
 * @param  write_buf    Bytes to write first (e.g. register address)
 * @param  write_len    Number of bytes to write
 * @param  read_buf     Buffer for received bytes
 * @param  read_len     Number of bytes to read
 * @return TWI_OK or error code
 */
TWI_Error_t TWI_master_write_read(uint8_t  slave_addr,
                                  const uint8_t *write_buf, uint8_t write_len,
                                  uint8_t       *read_buf,  uint8_t read_len);

/**
 * @brief  Get the raw TWI status register value (TWSR & 0xF8).
 * @return Last TWI status
 */
uint8_t TWI_get_status(void);

#endif /* TWI_H_ */
