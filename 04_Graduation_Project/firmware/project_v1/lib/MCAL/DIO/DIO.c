#include"../../Service/std_types.h"
#include"../../Service/Bit_math.h"
#include "DIO.h"
#include "../reg_def.h"
void DIO_voidSetPinDirection(u8 Copy_u8PortID, u8 Copy_u8PinID, u8 Copy_u8Dierction) {
	if((Copy_u8PinID<=DIO_PIN_7 )&& (Copy_u8PortID<=DIO_PORTD )&& (Copy_u8Dierction <= DIO_PIN_OUTPUT)){
		if(Copy_u8Dierction == DIO_PIN_OUTPUT){
			switch(Copy_u8PortID){
			case DIO_PORTA: SET_BIT(DDRA,Copy_u8PinID);break ;
			case DIO_PORTB: SET_BIT(DDRB,Copy_u8PinID);break ;
			case DIO_PORTC: SET_BIT(DDRC,Copy_u8PinID);break ;
			case DIO_PORTD: SET_BIT(DDRD,Copy_u8PinID);break ;
			default: break;
			}
		}else{
			switch(Copy_u8PortID){
			case DIO_PORTA: CLR_BIT(DDRA,Copy_u8PinID);break ;
			case DIO_PORTB: CLR_BIT(DDRB,Copy_u8PinID);break ;
			case DIO_PORTC: CLR_BIT(DDRC,Copy_u8PinID);break ;
			case DIO_PORTD: CLR_BIT(DDRD,Copy_u8PinID);break ;
			default: break;
			}
		}
	}else{
		//error
	}
}
void DIO_voidSetPinValue    (u8 Copy_u8PortID, u8 Copy_u8PinID, u8 Copy_u8Value) {
	if((Copy_u8PinID<=DIO_PIN_7 )&& (Copy_u8PortID<=DIO_PORTD )&& (Copy_u8Value <= DIO_PIN_HIGH)){
		if(Copy_u8Value == DIO_PIN_HIGH){
			switch(Copy_u8PortID){
			case DIO_PORTA: SET_BIT(PORTA,Copy_u8PinID);break ;
			case DIO_PORTB: SET_BIT(PORTB,Copy_u8PinID);break ;
			case DIO_PORTC: SET_BIT(PORTC,Copy_u8PinID);break ;
			case DIO_PORTD: SET_BIT(PORTD,Copy_u8PinID);break ;
			default: break;
			}
		}else{
			switch(Copy_u8PortID){
			case DIO_PORTA: CLR_BIT(PORTA,Copy_u8PinID);break ;
			case DIO_PORTB: CLR_BIT(PORTB,Copy_u8PinID);break ;
			case DIO_PORTC: CLR_BIT(PORTC,Copy_u8PinID);break ;
			case DIO_PORTD: CLR_BIT(PORTD,Copy_u8PinID);break ;
			default: break;
			}
		}
	}else{
		//error
	}
}

void DIO_voidSetPortDirection(u8 Copy_u8PortID, u8 Copy_u8Dierction) {
	if((Copy_u8PortID<=DIO_PORTD )){
		switch(Copy_u8PortID){
		case DIO_PORTA:DDRA = Copy_u8Dierction; break ;
		case DIO_PORTB:DDRB = Copy_u8Dierction; break ;
		case DIO_PORTC:DDRC = Copy_u8Dierction; break ;
		case DIO_PORTD:DDRD = Copy_u8Dierction; break ;
		default :  break ;
		}
	}else {
		//			error

	}
}

void DIO_voidSetPortValue    (u8 Copy_u8PortID, u8 Copy_u8Value) {

	if((Copy_u8PortID<=DIO_PORTD )){
		switch(Copy_u8PortID){
		case DIO_PORTA:PORTA = Copy_u8Value; break ;
		case DIO_PORTB:PORTB = Copy_u8Value; break ;
		case DIO_PORTC:PORTC = Copy_u8Value; break ;
		case DIO_PORTD:PORTD = Copy_u8Value; break ;
		default :  break ;
		}
	}else {
		//			error

	}
}
u8 	 DIO_u8GetPinValue(u8 Copy_u8PortID, u8 Copy_u8PinID){
	u8 Local_u8data = 0 ;
	switch(Copy_u8PortID){
	case DIO_PORTA : Local_u8data = GET_BIT(PINA,Copy_u8PinID); break ;
	case DIO_PORTB : Local_u8data = GET_BIT(PINB,Copy_u8PinID); break ;
	case DIO_PORTC : Local_u8data = GET_BIT(PINC,Copy_u8PinID); break ;
	case DIO_PORTD : Local_u8data = GET_BIT(PIND,Copy_u8PinID); break ;
	default : break ; // error
	}

	//	GET_BIT(,Copy_u8PinID)
	return Local_u8data;
}

u8 	 DIO_u8GetPortValue(u8 Copy_u8PortID){
	u8 Local_u8data = 0 ;
	switch(Copy_u8PortID){
	case DIO_PORTA : Local_u8data =PINA; break ;
	case DIO_PORTB : Local_u8data =PINB; break ;
	case DIO_PORTC : Local_u8data =PINC; break ;
	case DIO_PORTD : Local_u8data =PIND; break ;

	}
	return Local_u8data;
}

void DIO_voidEnablePullUp(u8 Copy_u8PortID, u8 Copy_u8PinID){
	switch(Copy_u8PortID){
	case DIO_PORTA : SET_BIT(PORTA,Copy_u8PinID); break ;
	case DIO_PORTB : SET_BIT(PORTB,Copy_u8PinID); break ;
	case DIO_PORTC : SET_BIT(PORTC,Copy_u8PinID); break ;
	case DIO_PORTD : SET_BIT(PORTD,Copy_u8PinID); break ;
	default : break ; // error

	}

}

void DIO_voidTogPin   (u8 Copy_u8PortID, u8 Copy_u8PinID) {


			switch(Copy_u8PortID){
			case DIO_PORTA: TOG_BIT(PORTA,Copy_u8PinID);break ;
			case DIO_PORTB: TOG_BIT(PORTB,Copy_u8PinID);break ;
			case DIO_PORTC: TOG_BIT(PORTC,Copy_u8PinID);break ;
			case DIO_PORTD: TOG_BIT(PORTD,Copy_u8PinID);break ;
			default: break;
			}
}
