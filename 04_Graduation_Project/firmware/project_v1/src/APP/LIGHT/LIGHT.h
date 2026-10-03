/*
 * LIGHT.h
 *
 *  Created on: Oct 3, 2026
 *      Author: eslam
 */

#ifndef APP_LIGHT_LIGHT_H_
#define APP_LIGHT_LIGHT_H_

void LIGHT_voidInit();                                 /* all lamps off , dimmer 0 % */
u8   LIGHT_u8ToggleLamp(u8 Copy_u8LampNumber);         /* lamp 1..5 ; returns the new state (LAMP_ON / LAMP_OFF) */
u8   LIGHT_u8GetLamp(u8 Copy_u8LampNumber);
void LIGHT_voidSetDimmer(u8 Copy_u8Percent);           /* rounded down to a multiple of LIGHT_DIMMER_STEP , max 100 */
u8   LIGHT_u8GetDimmer();

#endif /* APP_LIGHT_LIGHT_H_ */
