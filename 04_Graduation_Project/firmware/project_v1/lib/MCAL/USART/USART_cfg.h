/*
 * USART_cfg.h
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */

#ifndef MCAL_USART_USART_CFG_H_
#define MCAL_USART_USART_CFG_H_

/* F_CPU comes from platformio.ini (board_build.f_cpu) */
#define USART_BAUD_RATE		9600UL
/* Normal speed (U2X = 0) : UBRR = F_CPU / (16 * baud) - 1
 * 16 MHz , 9600 baud -> 103 , real baud 9615 , error +0.16 % (no U2X needed) */
#define USART_UBRR_VALUE	((F_CPU / (16UL * USART_BAUD_RATE)) - 1)

#endif /* MCAL_USART_USART_CFG_H_ */
