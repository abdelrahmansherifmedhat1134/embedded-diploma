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

void USART_voidInit(){
	/* Set Baud rate select 9600*/
	UBRRL = 103 ;
	UBRRH = 0 ;
//	/* Disable select parity  */
	SET_BIT(UCSRC,UCSRC_URSEL);/* select to register */
	CLR_BIT(UCSRC,UCSRC_UPM0);
	CLR_BIT(UCSRC,UCSRC_UPM1);
	/*enable Receiver and Transmitter */
	SET_BIT(UCSRB,UCSRB_TXEN);
	SET_BIT(UCSRB,UCSRB_RXEN);

	/*set format 8bit ,, 1 stop bit */
	SET_BIT(UCSRC,UCSRC_URSEL);/* select to register */
	SET_BIT(UCSRC,UCSRC_UCSZ0);
	SET_BIT(UCSRC,UCSRC_UCSZ1);
	CLR_BIT(UCSRC,UCSRC_USBS);
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
void USART_voidSendString(const u8*str){
	while(*str != '\0'){
		USART_voidSend(*str++);
	}
}
