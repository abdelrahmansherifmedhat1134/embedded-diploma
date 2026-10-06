/*
 * BUTTON.h
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef HAL_BUTTON_BUTTON_H_
#define HAL_BUTTON_BUTTON_H_

#define BUTTON_ONOFF    0
#define BUTTON_UP       1
#define BUTTON_DOWN     2

void BUTTON_voidInit();                                /* inputs with pull-up */
void BUTTON_voidUpdate();                              /* call every 10 ms */
u8   BUTTON_u8IsPressed(u8 Copy_u8ButtonID);           /* debounced level */
u8   BUTTON_u8GetPressEvent  (u8 Copy_u8ButtonID);     /* 1 once per press   , cleared by reading */
u8   BUTTON_u8GetReleaseEvent(u8 Copy_u8ButtonID);     /* 1 once per release , cleared by reading */

#endif /* HAL_BUTTON_BUTTON_H_ */
