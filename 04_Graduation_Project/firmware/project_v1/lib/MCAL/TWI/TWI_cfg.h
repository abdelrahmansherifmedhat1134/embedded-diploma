/*
 * TWI_cfg.h
 *
 *  Created on: Sep 28, 2026
 */

#ifndef MCAL_TWI_TWI_CFG_H_
#define MCAL_TWI_TWI_CFG_H_

/* F_CPU comes from platformio.ini (board_build.f_cpu) */
#ifndef F_CPU
#error "F_CPU is not defined"
#endif

/* SCL frequency : 100 kHz standard mode */
#define TWI_SCL_FREQ        100000UL

/* Prescaler bits TWPS1:0 = 00 --> prescaler = 1 */
#define TWI_PRESCALER       0

/* TWBR = ((F_CPU / SCL) - 16) / (2 * 4^TWPS)
 * 16 MHz , 100 kHz , TWPS = 0 --> TWBR = (160 - 16) / 2 = 72 */
#define TWI_TWBR_VALUE      ((u8)(((F_CPU / TWI_SCL_FREQ) - 16) / 2))

/* Longest wait for TWINT before the bus is declared stuck (TWI_ERR_TIMEOUT).
 * The wait loop takes about 6 clock cycles per pass : 16 MHz --> 2666 passes = about 1 ms.
 * A normal byte takes 90 us (about 215 passes). */
#define TWI_TIMEOUT_LOOPS   ((u16)((F_CPU / 1000UL) / 6UL))

#endif /* MCAL_TWI_TWI_CFG_H_ */
