/*
 * SERVO.h
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */

#ifndef HAL_SERVO_SERVO_H_
#define HAL_SERVO_SERVO_H_

void SERVO_voidInit();                                 /* TIMER1_voidInit , closed position (0 degrees) */
void SERVO_voidSetAngle(u8 Copy_u8Angle);              /* 0..180 degrees , bigger values give 180 */

#endif /* HAL_SERVO_SERVO_H_ */
