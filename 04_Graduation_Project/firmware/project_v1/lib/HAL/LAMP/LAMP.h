/*
 * LAMP.h
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef HAL_LAMP_LAMP_H_
#define HAL_LAMP_LAMP_H_

#define LAMP_OFF        0
#define LAMP_ON         1
/* returned by LAMP_u8SetState for a lamp number outside 1..LAMP_COUNT (TWI error codes are 1..6) */
#define LAMP_ERR_NUMBER 0xFF

void LAMP_voidInit();                                          /* all lamps OFF */
u8   LAMP_u8SetState(u8 Copy_u8LampNumber, u8 Copy_u8State);   /* lamp 1..5 ; returns TWI_OK or an error code */
u8   LAMP_u8GetState(u8 Copy_u8LampNumber);                    /* last state written successfully */

#endif /* HAL_LAMP_LAMP_H_ */
