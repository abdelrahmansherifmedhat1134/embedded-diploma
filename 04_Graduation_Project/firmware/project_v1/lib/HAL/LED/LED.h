/*
 * LED.h
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef HAL_LED_LED_H_
#define HAL_LED_LED_H_

#define LED_HEATER      0

void LED_voidInit();                   /* all LEDs OFF */
void LED_voidOn (u8 Copy_u8LedID);
void LED_voidOff(u8 Copy_u8LedID);

#endif /* HAL_LED_LED_H_ */
