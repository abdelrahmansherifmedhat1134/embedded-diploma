/*
 * SEVEN_SEG.h
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef HAL_SEVEN_SEG_SEVEN_SEG_H_
#define HAL_SEVEN_SEG_SEVEN_SEG_H_

/* 2 common-anode digits , each behind its own PCF8574 (I2C). Every call that changes the
 * display writes the chips at once (one I2C write per digit , only when its pattern changed).
 * Call them from the main loop only (I2C is polled and shares the bus with the LCD and lamps). */
void SEVEN_SEG_voidInit();                             /* display blank */
void SEVEN_SEG_voidSetNumber(u8 Copy_u8Number);        /* 0..99 , bigger values show 99 (shown with a leading zero) */
void SEVEN_SEG_voidEnable();                           /* show the number */
void SEVEN_SEG_voidDisable();                          /* blank : both digits off */
/* status of the last write : 0 (TWI_OK) or a TWI error code. After an error the next call
 * writes the digit again, because its pattern is still counted as "not shown". */
u8   SEVEN_SEG_u8GetStatus();

#endif /* HAL_SEVEN_SEG_SEVEN_SEG_H_ */
