/*
 * SEVEN_SEG.h
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */

#ifndef HAL_SEVEN_SEG_SEVEN_SEG_H_
#define HAL_SEVEN_SEG_SEVEN_SEG_H_

/* 2 digits , one 7447 BCD decoder shared by both , digits multiplexed */
void SEVEN_SEG_voidInit();                             /* pins output , display blank */
void SEVEN_SEG_voidSetNumber(u8 Copy_u8Number);        /* 0..99 , bigger values show 99 (shown with a leading zero) */
void SEVEN_SEG_voidEnable();
void SEVEN_SEG_voidDisable();                          /* blank : both digits off (takes effect at the next refresh) */
void SEVEN_SEG_voidRefresh();                          /* called from the 1 ms tick ISR */

#endif /* HAL_SEVEN_SEG_SEVEN_SEG_H_ */
