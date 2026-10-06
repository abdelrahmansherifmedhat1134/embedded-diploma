/*
 * FAN.c
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../../Service/std_types.h"
#include "../../MCAL/DIO/DIO.h"
#include "../../MCAL/TIMER1/TIMER1.h"
#include "../../MCAL/TIMER1/TIMER1_cfg.h"
#include "FAN.h"
#include "FAN_cfg.h"

void FAN_voidInit(){
	/*1. Pin low and output */
	DIO_voidSetPinValue(FAN_PORT,FAN_PIN,DIO_PIN_LOW);
	DIO_voidSetPinDirection(FAN_PORT,FAN_PIN,DIO_PIN_OUTPUT);
	/*2. Timer1 (shared with the servo , a second init changes nothing) , PWM output disconnected = fan stopped */
	TIMER1_voidInit();
	TIMER1_voidGeneratePWM_B(TIMER1_PWM_DISCONNECTED);
}
void FAN_voidSetSpeed(u8 Copy_u8Percent){
	if(Copy_u8Percent > 100){
		Copy_u8Percent = 100 ;
	}
	if(Copy_u8Percent == 0){
		/* compare value 0 would still give a one-tick spike : disconnect the pin and drive it low */
		TIMER1_voidGeneratePWM_B(TIMER1_PWM_DISCONNECTED);
		DIO_voidSetPinValue(FAN_PORT,FAN_PIN,DIO_PIN_LOW);
	}else if(Copy_u8Percent == 100){
		/* compare value = TOP : constantly high */
		TIMER1_voidSetOCRB(TIMER1_TOP_VALUE);
		TIMER1_voidGeneratePWM_B(TIMER1_PWM_NONINVERTED);
	}else{
		/* 10 % of (TOP + 1) = 4000 ticks = 2 ms at 16 MHz */
		TIMER1_voidSetOCRB((u16)(((u32)Copy_u8Percent * ((u32)TIMER1_TOP_VALUE + 1)) / 100));
		TIMER1_voidGeneratePWM_B(TIMER1_PWM_NONINVERTED);
	}
}
