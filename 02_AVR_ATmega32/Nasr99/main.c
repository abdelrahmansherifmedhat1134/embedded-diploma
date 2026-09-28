#include <avr/delay.h>
//#include <avr/interrupt.h>
#include "Service/std_types.h"
#include "MCAL/DIO/DIO.h"
#include "MCAL/ADC/ADC.h"
#include <MCAL/GIE/GIE.h>
#include "MCAL/TIMER0/TIMER0.h"
#include "MCAL/USART/USART.h"
#include "MCAL/TWI/twi.h"
#include "HAL/CLCD/CLCD.h"
#include "Free_RTOS/FreeRTOS.h"
#include "Free_RTOS/FreeRTOSConfig.h"
#include "Free_RTOS/task.h"
xTaskHandle t1 ;
xTaskHandle t2;
void task(){

	while(1){
		DIO_voidTogPin(DIO_PORTD,DIO_PIN_1);
		vTaskDelay(500*portTICK_RATE_MS);
	}
	vTaskDelete(NULL);
}
void task2(){

	while(1){
		DIO_voidTogPin(DIO_PORTD,DIO_PIN_2);
		vTaskDelay(2);
	}
	vTaskDelete(NULL);

}
u8 arr[100];
int main(){
u8 test = 5;
	DIO_voidSetPinDirection(DIO_PORTD,DIO_PIN_1,DIO_PIN_OUTPUT);
	DIO_voidSetPinDirection(DIO_PORTD,DIO_PIN_2,DIO_PIN_OUTPUT);
	xTaskCreate(task,"task1",100,NULL,4,t1);
	xTaskCreate(task2,"task2",100,NULL,3,t2);
	vTaskStartScheduler(); //start OS
	while(1){

			}




	return 0 ;
}




