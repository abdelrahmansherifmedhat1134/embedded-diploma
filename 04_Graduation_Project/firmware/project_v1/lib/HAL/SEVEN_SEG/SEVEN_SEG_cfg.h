/*
 * SEVEN_SEG_cfg.h
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */

#ifndef HAL_SEVEN_SEG_SEVEN_SEG_CFG_H_
#define HAL_SEVEN_SEG_SEVEN_SEG_CFG_H_

#define SEVEN_SEG_BCD_PORT          DIO_PORTC
#define SEVEN_SEG_BCD_PIN_A         DIO_PIN_2
#define SEVEN_SEG_BCD_PIN_B         DIO_PIN_3
#define SEVEN_SEG_BCD_PIN_C         DIO_PIN_4
#define SEVEN_SEG_BCD_PIN_D         DIO_PIN_5
#define SEVEN_SEG_DIGIT_PORT        DIO_PORTC
#define SEVEN_SEG_TENS_PIN          DIO_PIN_6          /* digit 1 */
#define SEVEN_SEG_UNITS_PIN         DIO_PIN_7          /* digit 2 */
#define SEVEN_SEG_DIGIT_ON_LEVEL    DIO_PIN_LOW        /* PNP (2N3906) high-side drivers in the schematic */
#define SEVEN_SEG_TICKS_PER_DIGIT   5                  /* 5 ms per digit -> 100 Hz refresh */

#endif /* HAL_SEVEN_SEG_SEVEN_SEG_CFG_H_ */
