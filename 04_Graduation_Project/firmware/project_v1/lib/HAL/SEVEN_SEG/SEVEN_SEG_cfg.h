/*
 * SEVEN_SEG_cfg.h
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */

#ifndef HAL_SEVEN_SEG_SEVEN_SEG_CFG_H_
#define HAL_SEVEN_SEG_SEVEN_SEG_CFG_H_

/* One PCF8574 per common-anode digit : P0..P6 = segments a..g , P7 = dp.
 * No multiplexing and no port pins : each digit is lit all the time by its own chip. */
#define SEVEN_SEG_TENS_ADDRESS     0x21               /* PCF8574 , A2 A1 A0 = 0 0 1 */
#define SEVEN_SEG_UNITS_ADDRESS    0x22               /* PCF8574 , A2 A1 A0 = 0 1 0 */
/* Common anode (+5 V) : a segment lights when its pin is LOW. The tables in SEVEN_SEG.c are written
 * for this level ; 0xFF = every segment and the dp off = blank digit. */
#define SEVEN_SEG_ON_LEVEL         0

#endif /* HAL_SEVEN_SEG_SEVEN_SEG_CFG_H_ */
