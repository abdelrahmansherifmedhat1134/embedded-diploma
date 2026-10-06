/*
 * BUZZER.c
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../../Service/std_types.h"
#include "../../MCAL/DIO/DIO.h"
#include "BUZZER.h"
#include "BUZZER_cfg.h"

#define BUZZER_OFF_LEVEL   (!BUZZER_ON_LEVEL)

void BUZZER_voidInit(){
	DIO_voidSetPinValue(BUZZER_PORT,BUZZER_PIN,BUZZER_OFF_LEVEL);
	DIO_voidSetPinDirection(BUZZER_PORT,BUZZER_PIN,DIO_PIN_OUTPUT);
}
void BUZZER_voidOn(){
	DIO_voidSetPinValue(BUZZER_PORT,BUZZER_PIN,BUZZER_ON_LEVEL);
}
void BUZZER_voidOff(){
	DIO_voidSetPinValue(BUZZER_PORT,BUZZER_PIN,BUZZER_OFF_LEVEL);
}
