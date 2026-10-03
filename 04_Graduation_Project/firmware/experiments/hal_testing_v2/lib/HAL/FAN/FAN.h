/*
 * FAN.h
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */

#ifndef HAL_FAN_FAN_H_
#define HAL_FAN_FAN_H_

void FAN_voidInit();                                   /* TIMER1_voidInit , stopped */
void FAN_voidSetSpeed(u8 Copy_u8Percent);              /* 0..100 , bigger values give 100 */

#endif /* HAL_FAN_FAN_H_ */
