/*
 * UILOC.h
 *
 *  Created on: Oct 4, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef APP_UILOC_UILOC_H_
#define APP_UILOC_UILOC_H_

/* Local UI : LCD + keypad , user mode only (REQ-LUI-01..04 , architecture.md 5.7) */
void UILOC_voidInit();                                 /* status screen */
void UILOC_voidTask10ms();                             /* keys , message timer , lockdown / kick checks */
void UILOC_voidTask1s();                               /* status pages , screen refresh , idle logout */

#endif /* APP_UILOC_UILOC_H_ */
