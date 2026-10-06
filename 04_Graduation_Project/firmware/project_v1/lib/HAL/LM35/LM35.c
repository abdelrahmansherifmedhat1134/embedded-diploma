/*
 * LM35.c
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../../Service/std_types.h"
#include "../../MCAL/ADC/ADC.h"
#include "LM35.h"
#include "LM35_cfg.h"

void LM35_voidInit(){
	ADC_voidInit();
}
u16 LM35_u16ReadTempX4(u8 Copy_u8SensorID){
	u16 Local_u16Value = 0 ;
	switch(Copy_u8SensorID){
	case LM35_AMBIENT: Local_u16Value = ADC_u16StartConversion(LM35_AMBIENT_CHANNEL); break ;
	case LM35_WATER  : Local_u16Value = ADC_u16StartConversion(LM35_WATER_CHANNEL);   break ;
	default: /* error */ break ;
	}
	/* 2.56 V / 1024 = 2.5 mV per step , LM35 = 10 mV per C --> value = temperature x LM35_STEPS_PER_C (= 4) */
	return Local_u16Value ;
}
