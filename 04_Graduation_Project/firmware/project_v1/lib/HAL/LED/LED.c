/*
 * LED.c
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */
#include "../../Service/std_types.h"
#include "../../MCAL/DIO/DIO.h"
#include "LED.h"
#include "LED_cfg.h"

#define LED_OFF_LEVEL   (!LED_ON_LEVEL)

static void LED_voidSet(u8 Copy_u8LedID, u8 Copy_u8Level){
	switch(Copy_u8LedID){
	case LED_HEATER: DIO_voidSetPinValue(LED_HEATER_PORT,LED_HEATER_PIN,Copy_u8Level); break ;
	default: /* error */ break ;
	}
}
void LED_voidInit(){
	LED_voidSet(LED_HEATER,LED_OFF_LEVEL);
	DIO_voidSetPinDirection(LED_HEATER_PORT,LED_HEATER_PIN,DIO_PIN_OUTPUT);
}
void LED_voidOn(u8 Copy_u8LedID){
	LED_voidSet(Copy_u8LedID,LED_ON_LEVEL);
}
void LED_voidOff(u8 Copy_u8LedID){
	LED_voidSet(Copy_u8LedID,LED_OFF_LEVEL);
}
