/*
 * UASRT.c
 *
 *  Created on: Feb 18, 2026
 *      Author: eslam
 */
#include "../../Service/std_types.h"
#include "../../Service/Bit_math.h"
#include "../reg_def.h"
#include "USART.h"
#include "USART_cfg.h"

static void (*USART_rx_ptr)(u8) = NULL;
static void (*USART_tx_ptr)(void) = NULL;

void USART_voidInit(){
	/* Set Baud rate (USART_cfg.h , computed from F_CPU) */
	UBRRH = (u8)(USART_UBRR_VALUE >> 8) ;
	UBRRL = (u8)USART_UBRR_VALUE ;
	/*enable Receiver and Transmitter */
	SET_BIT(UCSRB,UCSRB_TXEN);
	SET_BIT(UCSRB,UCSRB_RXEN);

	/*set format : 8 bit , no parity , 1 stop bit
	 * UCSRC shares its address with UBRRH : one read returns UBRRH ,
	 * so SET_BIT/CLR_BIT (read-modify-write) would corrupt UBRRH.
	 * Write the whole register at once with URSEL = 1 (select UCSRC) */
	UCSRC = (1<<UCSRC_URSEL) | (1<<UCSRC_UCSZ1) | (1<<UCSRC_UCSZ0) ;
}
void USART_voidSend(u8 data){
	/*wait on Empty transmit buffer */
	while(GET_BIT(UCSRA,UCSRA_UDRE) == 0){
		/*wait ....*/
	}
	/*Put Data into data Register */
	UDR = data ;
}
u8   USART_u8Recieve(){
	/* wait on RXC */
	while(GET_BIT(UCSRA,UCSRA_RXC) == 0){
			/*wait ....*/
	}
	/*return UDR */
	return UDR ;
}
void USART_voidSendString(const c8*str){
	while(*str != '\0'){
		USART_voidSend(*str++);
	}
	USART_voidSend('\r');
	USART_voidSend('\n');
}

void USART_voidEnableRxInterrupt(){
	SET_BIT(UCSRB,UCSRB_RXCIE);
}
void USART_voidEnableTxInterrupt(){
	SET_BIT(UCSRB,UCSRB_UDRIE);
}
void USART_voidDisableTxInterrupt(){
	CLR_BIT(UCSRB,UCSRB_UDRIE);
}
void USART_voidSetCallBack_RX(void (*ptr)(u8)){
	USART_rx_ptr = ptr;
}
void USART_voidSetCallBack_TX(void (*ptr)(void)){
	USART_tx_ptr = ptr;
}
void USART_voidWriteData(u8 data){
	/* caller makes sure UDRE = 1 (the UDRE interrupt does) */
	UDR = data ;
}

void __vector_13 (){
	/* always read UDR : it clears RXC , even when no callback is set */
	u8 Local_u8data = UDR ;
	if (USART_rx_ptr!=NULL){
		USART_rx_ptr(Local_u8data);
	}
}
void __vector_14 (){
	if (USART_tx_ptr!=NULL){
		USART_tx_ptr();
	}else{
		/* nobody to feed UDR : switch UDRIE off , or the interrupt fires forever */
		CLR_BIT(UCSRB,UCSRB_UDRIE);
	}
}
