/*
 * EVQ.h
 *
 *  Created on: Oct 3, 2026
 *      Author: eslam
 */

#ifndef SERVICE_EVQ_EVQ_H_
#define SERVICE_EVQ_EVQ_H_

/* Events : a feature module posts , the remote terminal (UIREM) reads and prints (REQ-RUI-02) */
#define EVQ_LAMP              1     /* arg = lamp number (state is read from LIGHT) */
#define EVQ_DIMMER            2
#define EVQ_AC                3
#define EVQ_HEATER_POWER      4
#define EVQ_HEATER_ELEMENT    5
#define EVQ_HEATER_SET        6
#define EVQ_LOCAL_SESSION     7     /* arg = 1 login , 0 logout */
#define EVQ_STORAGE_FAULT     8
#define EVQ_LOCKDOWN          9

/* Main-loop context only : never call these from an ISR */
void EVQ_voidInit();
void EVQ_voidPost(u8 Copy_u8Event, u8 Copy_u8Arg);     /* dropped when full or muted */
u8   EVQ_u8Get(u8 * Copy_pu8Event, u8 * Copy_pu8Arg);  /* 1 = one event returned */
void EVQ_voidSetMute(u8 Copy_u8State);                 /* 1 = posts are dropped */

#endif /* SERVICE_EVQ_EVQ_H_ */
