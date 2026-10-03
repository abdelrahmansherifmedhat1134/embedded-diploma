/*
 * TERM.h
 *
 *  Created on: Oct 3, 2026
 *      Author: eslam
 */

#ifndef SERVICE_TERM_TERM_H_
#define SERVICE_TERM_TERM_H_

#define TERM_ECHO_OFF       0
#define TERM_ECHO_NORMAL    1
#define TERM_ECHO_MASKED    2                          /* echo '*' : passwords */

#define TERM_LINE_NONE      0
#define TERM_LINE_READY     1
#define TERM_LINE_TOO_LONG  2                          /* reported once , when the too long line ends */

void TERM_voidInit();                                  /* USART init , rings , callbacks , RX interrupt */
u8   TERM_u8TxFree();                                  /* free bytes in the TX ring */
/* output never waits : what does not fit in the TX ring is dropped (check TERM_u8TxFree first) */
void TERM_voidPutChar  (c8 Copy_c8Char);
void TERM_voidPutFlash (const __flash c8 * Copy_pc8Text);
void TERM_voidPutRam   (const c8 * Copy_pc8Text);
void TERM_voidPutNumber(u16 Copy_u16Number);
void TERM_voidNewLine();                               /* "\r\n" */
void TERM_voidSetEcho(u8 Copy_u8Mode);
/* call every 5 ms ; copies the line (text + '\0' , buffer of TERM_LINE_MAX + 1) when TERM_LINE_READY */
u8   TERM_u8GetLine(c8 * Copy_pc8Line);
u8   TERM_u8IsLineEmpty();                             /* 1 = nothing typed yet (safe moment to print an event) */
u8   TERM_u8IsRxActive();                              /* 1 = a byte arrived since the last call (idle timer) */

#endif /* SERVICE_TERM_TERM_H_ */
