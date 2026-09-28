#include "../lib/service/Std_Types.h"
#include "../lib/MCAL/DIO/DIO.h"
#include "../lib/MCAL/USART/USART.h"
#include<util/delay.h>

int main() {

  DIO_voidSetPinDirection(DIO_PORTA, DIO_PIN_0, DIO_PIN_OUTPUT);
  USART_voidInit();
  USART_voidSendString("hello this is verification that the code is working");
    
  while(1){
    DIO_voidSetPinValue(DIO_PORTA, DIO_PIN_0, DIO_PIN_LOW);
    USART_voidSendString("led off");
    _delay_ms(500);  
    DIO_voidSetPinValue(DIO_PORTA, DIO_PIN_0, DIO_PIN_HIGH);
    USART_voidSendString("led on");
    _delay_ms(500);
  }
  
  return 0;  
}