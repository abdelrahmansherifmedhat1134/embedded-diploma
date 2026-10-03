/*
 * LIGHT.c
 *
 *  Created on: Oct 3, 2026
 *      Author: eslam
 */
#include "../../../lib/Service/std_types.h"
#include "../../../lib/Service/Bit_math.h"
#include "../../../lib/Service/EVQ/EVQ.h"
#include "../../../lib/HAL/LAMP/LAMP.h"
#include "../../../lib/HAL/LAMP/LAMP_cfg.h"
#include "../../../lib/HAL/DIMMER/DIMMER.h"
#include "LIGHT.h"
#include "LIGHT_cfg.h"

/* No modes , only data (architecture.md 5.3) */
static u8 Global_u8LampMask = 0 ;                      /* bit 0 = lamp 1 ... bit 4 = lamp 5 , 1 = ON */
static u8 Global_u8DimmerPercent = 0 ;

void LIGHT_voidInit(){
	u8 Local_u8Lamp ;
	/* ASSUMPTION: lamp and dimmer states are not stored (D-14) : boot = all off , 0 % */
	for(Local_u8Lamp = 1 ; Local_u8Lamp <= LAMP_COUNT ; Local_u8Lamp++){
		LAMP_u8SetState(Local_u8Lamp,LAMP_OFF);
	}
	DIMMER_voidSetLevel(0);
	Global_u8LampMask = 0 ;
	Global_u8DimmerPercent = 0 ;
}

u8 LIGHT_u8ToggleLamp(u8 Copy_u8LampNumber){
	u8 Local_u8State = LAMP_OFF ;
	if((Copy_u8LampNumber >= 1) && (Copy_u8LampNumber <= LAMP_COUNT)){
		/* REQ-LGT-01 */
		TOG_BIT(Global_u8LampMask,(Copy_u8LampNumber - 1));
		Local_u8State = GET_BIT(Global_u8LampMask,(Copy_u8LampNumber - 1));
		if(LAMP_u8SetState(Copy_u8LampNumber,Local_u8State) == 0){
			EVQ_voidPost(EVQ_LAMP,Copy_u8LampNumber);
		}else{
			/* I2C error : the lamp did not change , so the bit goes back and nothing is reported */
			TOG_BIT(Global_u8LampMask,(Copy_u8LampNumber - 1));
			Local_u8State = GET_BIT(Global_u8LampMask,(Copy_u8LampNumber - 1));
		}
	}else{
		//error
	}
	return Local_u8State ;
}

u8 LIGHT_u8GetLamp(u8 Copy_u8LampNumber){
	u8 Local_u8State = LAMP_OFF ;
	if((Copy_u8LampNumber >= 1) && (Copy_u8LampNumber <= LAMP_COUNT)){
		Local_u8State = GET_BIT(Global_u8LampMask,(Copy_u8LampNumber - 1));
	}else{
		//error
	}
	return Local_u8State ;
}

void LIGHT_voidSetDimmer(u8 Copy_u8Percent){
	/* REQ-LGT-02 : limited to 100 , then rounded down to a multiple of the step */
	if(Copy_u8Percent > LIGHT_DIMMER_MAX){
		Copy_u8Percent = LIGHT_DIMMER_MAX ;
	}
	Copy_u8Percent = Copy_u8Percent - (Copy_u8Percent % LIGHT_DIMMER_STEP) ;
	DIMMER_voidSetLevel(Copy_u8Percent);
	if(Copy_u8Percent != Global_u8DimmerPercent){
		Global_u8DimmerPercent = Copy_u8Percent ;
		EVQ_voidPost(EVQ_DIMMER,Copy_u8Percent);
	}
}

u8 LIGHT_u8GetDimmer(){
	return Global_u8DimmerPercent ;
}
