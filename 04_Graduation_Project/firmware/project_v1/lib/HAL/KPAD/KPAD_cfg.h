/*
 * KPAD_cfg.h
 *
 *  Created on: Jan 5, 2026
 *      Author: Eslam
 */

#ifndef HAL_KPAD_KPAD_CFG_H_
#define HAL_KPAD_KPAD_CFG_H_

/* PC0/PC1 = I2C (SCL/SDA) and PD0/PD1 = UART are reserved.
 * Keypad uses the pins freed by moving the LCD to I2C (PA4-PA7 , PB0-PB2) + PB4 */
#define KPAD_COL_PORT    DIO_PORTB
#define KPAD_COL_PIN0    DIO_PIN_0
#define KPAD_COL_PIN1    DIO_PIN_1
#define KPAD_COL_PIN2    DIO_PIN_2
#define KPAD_COL_PIN3    DIO_PIN_4

#define KPAD_ROW_PORT    DIO_PORTA
#define KPAD_ROW_PIN0    DIO_PIN_4
#define KPAD_ROW_PIN1    DIO_PIN_5
#define KPAD_ROW_PIN2    DIO_PIN_6
#define KPAD_ROW_PIN3    DIO_PIN_7

/* a key is accepted after this many equal scans in a row (2 x 10 ms = 20 ms) */
#define KPAD_DEBOUNCE_SCANS    2

#endif /* HAL_KPAD_KPAD_CFG_H_ */
