/*
 * DIMMER.h
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */

#ifndef HAL_DIMMER_DIMMER_H_
#define HAL_DIMMER_DIMMER_H_

void DIMMER_voidInit();                                /* Timer0 fast PWM , level 0 */
void DIMMER_voidSetLevel(u8 Copy_u8Percent);           /* 0..100 , bigger values give 100 */

#endif /* HAL_DIMMER_DIMMER_H_ */
