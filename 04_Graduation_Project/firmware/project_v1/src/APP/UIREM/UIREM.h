/*
 * UIREM.h
 *
 *  Created on: Oct 4, 2026
 *      Author: eslam
 */

#ifndef APP_UIREM_UIREM_H_
#define APP_UIREM_UIREM_H_

/* Remote UI : UART terminal , admin and remote users (REQ-RUI-01..04 , architecture.md 5.8).
 * The texts are the ones of docs/uart_protocol.md. */
void UIREM_voidInit();                                 /* banner + username prompt are queued */
void UIREM_voidTask5ms();                              /* one print step , or one typed line , or one event */
void UIREM_voidTask1s();                               /* idle logout (D-11) */

#endif /* APP_UIREM_UIREM_H_ */
