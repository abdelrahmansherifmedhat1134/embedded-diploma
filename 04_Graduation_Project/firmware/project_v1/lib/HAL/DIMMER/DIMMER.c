/*
 * DIMMER.c
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../../Service/std_types.h"
#include "../../MCAL/DIO/DIO.h"
#include "../../MCAL/TIMER0/TIMER0.h"
#include "DIMMER.h"
#include "DIMMER_cfg.h"

void DIMMER_voidInit(){
	/*1. Pin low and output */
	DIO_voidSetPinValue(DIMMER_PORT,DIMMER_PIN,DIO_PIN_LOW);
	DIO_voidSetPinDirection(DIMMER_PORT,DIMMER_PIN,DIO_PIN_OUTPUT);
	/*2. Timer0 fast PWM , output disconnected = level 0 */
	TIMER0_voidInit(DIMMER_TIMER_PRESCALER,TIMER0_FAST_PWM);
	TIMER0_GeneratePWM(TIMER0_PWM_DISCONNECTED,0);
}
void DIMMER_voidSetLevel(u8 Copy_u8Percent){
	if(Copy_u8Percent > 100){
		Copy_u8Percent = 100 ;
	}
	if(Copy_u8Percent == 0){
		/* compare value 0 would still give a one-tick spike : disconnect the pin and drive it low */
		TIMER0_GeneratePWM(TIMER0_PWM_DISCONNECTED,0);
		DIO_voidSetPinValue(DIMMER_PORT,DIMMER_PIN,DIO_PIN_LOW);
	}else{
		/* OCR0 = percent x 255 / 100 (100 % = 255 = constantly high) */
		TIMER0_GeneratePWM(TIMER0_PWM_NONINVERTED,Copy_u8Percent);
	}
}
