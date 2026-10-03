/*
 * SERVO.c
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */
#include "../../Service/std_types.h"
#include "../../MCAL/DIO/DIO.h"
#include "../../MCAL/TIMER1/TIMER1.h"
#include "../../MCAL/TIMER1/TIMER1_cfg.h"
#include "SERVO.h"
#include "SERVO_cfg.h"

void SERVO_voidInit(){
	DIO_voidSetPinDirection(SERVO_PORT,SERVO_PIN,DIO_PIN_OUTPUT);
	/* Timer1 is shared with the fan : a second init changes nothing */
	TIMER1_voidInit();
	SERVO_voidSetAngle(0);
}
void SERVO_voidSetAngle(u8 Copy_u8Angle){
	u32 Local_u32PulseUs ;
	if(Copy_u8Angle > SERVO_MAX_ANGLE){
		Copy_u8Angle = SERVO_MAX_ANGLE ;
	}
	/*1. angle --> pulse width : 1000 us + angle x 1000 us / 180 */
	Local_u32PulseUs = SERVO_MIN_PULSE_US + (((SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) * Copy_u8Angle) / SERVO_MAX_ANGLE) ;
	/*2. pulse width --> timer ticks : 1.0 ms = 2000 , 1.5 ms = 3000 , 2.0 ms = 4000 at 16 MHz */
	TIMER1_voidSetOCRA((u16)((Local_u32PulseUs * TIMER1_TICKS_PER_MS) / 1000UL));
	TIMER1_voidGeneratePWM_A(TIMER1_PWM_NONINVERTED);
}
