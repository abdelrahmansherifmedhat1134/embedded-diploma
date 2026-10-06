/*
 * ALARM.h
 *
 *  Created on: Oct 3, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef APP_ALARM_ALARM_H_
#define APP_ALARM_ALARM_H_

void ALARM_voidInit();
void ALARM_voidTrigger();                              /* lockdown , latched until MCU reset */
u8   ALARM_u8IsActive();
void ALARM_voidTask500ms();                            /* buzzer pattern */

#endif /* APP_ALARM_ALARM_H_ */
