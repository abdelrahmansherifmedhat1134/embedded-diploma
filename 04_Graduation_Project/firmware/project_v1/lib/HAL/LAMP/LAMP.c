/*
 * LAMP.c
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../../Service/std_types.h"
#include "../../Service/Bit_math.h"
#include "../../MCAL/TWI/TWI.h"
#include "../PCF8574/PCF8574.h"
#include "LAMP.h"
#include "LAMP_cfg.h"

/* Copy of the PCF8574 port : the lamp bits plus the unused pins (kept HIGH = input).
 * 0xFF = all lamps OFF for an active-low wiring. */
static u8 Global_u8Port = 0xFF ;

void LAMP_voidInit(){
	PCF8574_voidInit();
	/* ASSUMPTION: every lamp bit is set to its OFF level , whatever LAMP_ON_LEVEL is */
	Global_u8Port = 0xFF ;
	for(u8 i = 0 ; i < LAMP_COUNT ; i++){
		u8 Local_u8Bit = LAMP_FIRST_BIT + i ;
		if(LAMP_ON_LEVEL == 0){
			SET_BIT(Global_u8Port,Local_u8Bit);
		}else{
			CLR_BIT(Global_u8Port,Local_u8Bit);
		}
	}
	/* if the chip does not answer , the next LAMP_u8SetState reports it */
	PCF8574_u8WritePort(LAMP_I2C_ADDRESS,Global_u8Port);
}
u8 LAMP_u8SetState(u8 Copy_u8LampNumber, u8 Copy_u8State){
	u8 Local_u8Error = LAMP_ERR_NUMBER ;
	if((Copy_u8LampNumber >= 1) && (Copy_u8LampNumber <= LAMP_COUNT)){
		u8 Local_u8Bit = LAMP_FIRST_BIT + (Copy_u8LampNumber - 1) ;
		u8 Local_u8NewPort = Global_u8Port ;
		/* pin level for "ON" is LAMP_ON_LEVEL , for "OFF" the opposite */
		if((Copy_u8State == LAMP_ON) == (LAMP_ON_LEVEL == 1)){
			SET_BIT(Local_u8NewPort,Local_u8Bit);
		}else{
			CLR_BIT(Local_u8NewPort,Local_u8Bit);
		}
		Local_u8Error = PCF8574_u8WritePort(LAMP_I2C_ADDRESS,Local_u8NewPort);
		if(Local_u8Error == TWI_OK){
			/* the copy changes only when the chip really got the value */
			Global_u8Port = Local_u8NewPort ;
		}
	}else{
		//error
	}
	return Local_u8Error ;
}
u8 LAMP_u8GetState(u8 Copy_u8LampNumber){
	u8 Local_u8State = LAMP_OFF ;
	if((Copy_u8LampNumber >= 1) && (Copy_u8LampNumber <= LAMP_COUNT)){
		u8 Local_u8Bit = LAMP_FIRST_BIT + (Copy_u8LampNumber - 1) ;
		u8 Local_u8Level = GET_BIT(Global_u8Port,Local_u8Bit) ;
		if(Local_u8Level == LAMP_ON_LEVEL){
			Local_u8State = LAMP_ON ;
		}
	}else{
		//error
	}
	return Local_u8State ;
}
