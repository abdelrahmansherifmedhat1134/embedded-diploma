/*
 * USART.h
 *
 *  Created on: Feb 18, 2026
 *      Author: eslam
 */

#ifndef MCAL_USART_USART_H_
#define MCAL_USART_USART_H_

void USART_voidInit();
void USART_voidSend(u8 data);
u8 USART_u8Recieve();
void USART_voidSendString(const c8 *str);

/* Non-blocking API : the ring buffers live in the upper layer (TERM) */
void USART_voidEnableRxInterrupt();
void USART_voidEnableTxInterrupt();
void USART_voidDisableTxInterrupt();
void USART_voidSetCallBack_RX(void (*ptr)(u8));
void USART_voidSetCallBack_TX(void (*ptr)(void));
void USART_voidWriteData(u8 data);
void __vector_13 () __attribute__ ((signal, used, externally_visible)) ;	/* RX complete */
void __vector_14 () __attribute__ ((signal, used, externally_visible)) ;	/* data register empty */

#endif /* MCAL_USART_USART_H_ */
