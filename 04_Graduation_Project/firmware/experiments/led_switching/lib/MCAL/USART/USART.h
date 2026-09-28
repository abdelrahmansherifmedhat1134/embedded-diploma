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

#endif /* MCAL_USART_USART_H_ */
