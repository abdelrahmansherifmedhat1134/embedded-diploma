/*
 * RELAY.c
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../../Service/std_types.h"
#include "../../MCAL/DIO/DIO.h"
#include "RELAY.h"
#include "RELAY_cfg.h"

#define RELAY_OFF_LEVEL   (!RELAY_ON_LEVEL)

static void RELAY_voidSet(u8 Copy_u8RelayID, u8 Copy_u8Level){
	switch(Copy_u8RelayID){
	case RELAY_HEATING: DIO_voidSetPinValue(RELAY_HEATING_PORT,RELAY_HEATING_PIN,Copy_u8Level); break ;
	case RELAY_COOLING: DIO_voidSetPinValue(RELAY_COOLING_PORT,RELAY_COOLING_PIN,Copy_u8Level); break ;
	default: /* error */ break ;
	}
}
void RELAY_voidInit(){
	/* level first , then output : the element never switches on by accident */
	RELAY_voidSet(RELAY_HEATING,RELAY_OFF_LEVEL);
	RELAY_voidSet(RELAY_COOLING,RELAY_OFF_LEVEL);
	DIO_voidSetPinDirection(RELAY_HEATING_PORT,RELAY_HEATING_PIN,DIO_PIN_OUTPUT);
	DIO_voidSetPinDirection(RELAY_COOLING_PORT,RELAY_COOLING_PIN,DIO_PIN_OUTPUT);
}
void RELAY_voidOn(u8 Copy_u8RelayID){
	RELAY_voidSet(Copy_u8RelayID,RELAY_ON_LEVEL);
}
void RELAY_voidOff(u8 Copy_u8RelayID){
	RELAY_voidSet(Copy_u8RelayID,RELAY_OFF_LEVEL);
}
