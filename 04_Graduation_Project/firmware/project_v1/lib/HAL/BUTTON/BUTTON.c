/*
 * BUTTON.c
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../../Service/std_types.h"
#include "../../Service/Bit_math.h"
#include "../../MCAL/DIO/DIO.h"
#include "BUTTON.h"
#include "BUTTON_cfg.h"

static const __flash u8 BUTTON_PORT_ARR[BUTTON_COUNT] = {BUTTON_ONOFF_PORT,BUTTON_UP_PORT,BUTTON_DOWN_PORT};
static const __flash u8 BUTTON_PIN_ARR [BUTTON_COUNT] = {BUTTON_ONOFF_PIN ,BUTTON_UP_PIN ,BUTTON_DOWN_PIN };

static u8 Global_u8Pressed[BUTTON_COUNT] ;         /* debounced level : 1 = pressed */
static u8 Global_u8Samples[BUTTON_COUNT] ;         /* samples in a row that differ from the debounced level */
static u8 Global_u8PressEvent[BUTTON_COUNT] ;
static u8 Global_u8ReleaseEvent[BUTTON_COUNT] ;

void BUTTON_voidInit(){
	for(u8 i = 0 ; i < BUTTON_COUNT ; i++){
		/*1. input with pull-up */
		DIO_voidSetPinDirection(BUTTON_PORT_ARR[i],BUTTON_PIN_ARR[i],DIO_INPUT);
		DIO_voidEnablePullUp(BUTTON_PORT_ARR[i],BUTTON_PIN_ARR[i]);
		/*2. start as "released" with no events */
		Global_u8Pressed[i] = 0 ;
		Global_u8Samples[i] = 0 ;
		Global_u8PressEvent[i] = 0 ;
		Global_u8ReleaseEvent[i] = 0 ;
	}
}
void BUTTON_voidUpdate(){
	for(u8 i = 0 ; i < BUTTON_COUNT ; i++){
		u8 Local_u8Raw = (DIO_u8GetPinValue(BUTTON_PORT_ARR[i],BUTTON_PIN_ARR[i]) == BUTTON_PRESSED_LEVEL) ;
		if(Local_u8Raw != Global_u8Pressed[i]){
			/* level differs : accept it after BUTTON_DEBOUNCE_SAMPLES samples in a row */
			Global_u8Samples[i]++;
			if(Global_u8Samples[i] >= BUTTON_DEBOUNCE_SAMPLES){
				Global_u8Pressed[i] = Local_u8Raw ;
				Global_u8Samples[i] = 0 ;
				if(Local_u8Raw == 1){
					Global_u8PressEvent[i] = 1 ;
				}else{
					Global_u8ReleaseEvent[i] = 1 ;
				}
			}
		}else{
			/* a bounce back to the old level restarts the count */
			Global_u8Samples[i] = 0 ;
		}
	}
}
u8 BUTTON_u8IsPressed(u8 Copy_u8ButtonID){
	u8 Local_u8Result = 0 ;
	if(Copy_u8ButtonID < BUTTON_COUNT){
		Local_u8Result = Global_u8Pressed[Copy_u8ButtonID] ;
	}else{
		//error
	}
	return Local_u8Result ;
}
u8 BUTTON_u8GetPressEvent(u8 Copy_u8ButtonID){
	u8 Local_u8Result = 0 ;
	if(Copy_u8ButtonID < BUTTON_COUNT){
		Local_u8Result = Global_u8PressEvent[Copy_u8ButtonID] ;
		Global_u8PressEvent[Copy_u8ButtonID] = 0 ;
	}else{
		//error
	}
	return Local_u8Result ;
}
u8 BUTTON_u8GetReleaseEvent(u8 Copy_u8ButtonID){
	u8 Local_u8Result = 0 ;
	if(Copy_u8ButtonID < BUTTON_COUNT){
		Local_u8Result = Global_u8ReleaseEvent[Copy_u8ButtonID] ;
		Global_u8ReleaseEvent[Copy_u8ButtonID] = 0 ;
	}else{
		//error
	}
	return Local_u8Result ;
}
