/*
 * TIMER2.c
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */
#include "../../Service/std_types.h"
#include "../../Service/Bit_math.h"
#include "../reg_def.h"
#include "TIMER2.h"

static void (*TIMER2_oc_ptr)(void)= NULL;

void TIMER2_voidInit(u8 Copy_u8PreScalare,u8 Copy_u8Mode){
	/*1. Prescaler */
	TCCR2 &= 0b11111000; // mask
	TCCR2 |= Copy_u8PreScalare ; // set with mask

	/*2. Mode */
	switch(Copy_u8Mode){
	case TIMER2_NORMAL:
		CLR_BIT(TCCR2,TCCR2_WGM20);
		CLR_BIT(TCCR2,TCCR2_WGM21);
		break;
	case TIMER2_CTC:
		CLR_BIT(TCCR2,TCCR2_WGM20);
		SET_BIT(TCCR2,TCCR2_WGM21);
		break;
	default:
		/*Error*/
		break ;
	}
}
void TIMER2_voidSetOCR(u8 Copy_u8OCR_Val){
	OCR2 = Copy_u8OCR_Val;
}
void TIMER2_voidEnableOCInterrupt(){
	SET_BIT(TIMSK,TIMSK_OCIE2);
}
void TIMER2_voidDisableOCInterrupt(){
	CLR_BIT(TIMSK,TIMSK_OCIE2);
}
void TIMER2_voidSetCallBack_OC(void (*ptr)(void)){
	TIMER2_oc_ptr = ptr;
}

void __vector_4 () {
	if (TIMER2_oc_ptr!=NULL){
		TIMER2_oc_ptr();
	}
}
