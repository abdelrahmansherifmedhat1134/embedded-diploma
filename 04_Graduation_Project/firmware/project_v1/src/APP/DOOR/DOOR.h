/*
 * DOOR.h
 *
 *  Created on: Oct 3, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef APP_DOOR_DOOR_H_
#define APP_DOOR_DOOR_H_

#define DOOR_CLOSED    0
#define DOOR_OPEN      1

/* Admin only (REQ-DOR-01 , REQ-SEC-06) : the caller (UIREM) checks the role before calling */
void DOOR_voidInit();                                  /* closed */
u8   DOOR_u8SetState(u8 Copy_u8State);                 /* 1 = it moved , 0 = it was already there */
u8   DOOR_u8GetState();

#endif /* APP_DOOR_DOOR_H_ */
