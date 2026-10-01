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
