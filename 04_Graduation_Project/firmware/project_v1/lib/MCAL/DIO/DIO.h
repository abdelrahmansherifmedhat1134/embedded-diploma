
#ifndef DIO_H_
#define DIO_H_

#define DIO_PORTA   0
#define DIO_PORTB   1
#define DIO_PORTC   2
#define DIO_PORTD   3


#define DIO_PIN_0    0
#define DIO_PIN_1    1
#define DIO_PIN_2    2
#define DIO_PIN_3    3
#define DIO_PIN_4    4
#define DIO_PIN_5    5
#define DIO_PIN_6    6
#define DIO_PIN_7    7

#define DIO_PIN_OUTPUT    0x01
#define DIO_PORT_OUTPUT   0xFF
#define DIO_INPUT    0

#define DIO_PIN_HIGH 	1
#define DIO_PORT_HIGH 	0xff
#define DIO_PIN_LOW		0

void DIO_voidSetPinDirection(u8 Copy_u8PortID, u8 Copy_u8PinID, u8 Copy_u8Dierction) ;
void DIO_voidSetPinValue    (u8 Copy_u8PortID, u8 Copy_u8PinID, u8 Copy_u8Value) ;
u8   DIO_u8GetPinValue      (u8 Copy_u8PortID, u8 Copy_u8PinID);


void DIO_voidSetPortDirection(u8 Copy_u8PortID, u8 Copy_u8Dierction) ;
void DIO_voidSetPortValue    (u8 Copy_u8PortID, u8 Copy_u8Value) ;
u8   DIO_u8GetPortValue      (u8 Copy_u8PortID);
void DIO_voidEnablePullUp(u8 Copy_u8PortID, u8 Copy_u8PinID);
#endif
