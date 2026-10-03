/*
 * TIMER1.c
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */
#include "../../Service/std_types.h"
#include "../../Service/Bit_math.h"
#include "../reg_def.h"
#include "TIMER1.h"
#include "TIMER1_cfg.h"

/* No interrupt is used. TIMER1_voidInit may be called twice (SERVO and FAN both call it). */
void TIMER1_voidInit(){
	/*1. Mode 14 : fast PWM , TOP = ICR1  (WGM13:10 = 1110) */
	CLR_BIT(TCCR1A,TCCR1A_WGM10);
	SET_BIT(TCCR1A,TCCR1A_WGM11);
	SET_BIT(TCCR1B,TCCR1B_WGM12);
	SET_BIT(TCCR1B,TCCR1B_WGM13);
	/*2. TOP : 16 bit registers are written high byte first */
	ICR1H = (u8)(TIMER1_TOP_VALUE >> 8);
	ICR1L = (u8)TIMER1_TOP_VALUE;
	/*3. Prescaler 8 (CS12:CS10 = 010) */
	CLR_BIT(TCCR1B,TCCR1B_CS10);
	SET_BIT(TCCR1B,TCCR1B_CS11);
	CLR_BIT(TCCR1B,TCCR1B_CS12);
}
void TIMER1_voidSetOCRA(u16 Copy_u16Value){
	OCR1AH = (u8)(Copy_u16Value >> 8);
	OCR1AL = (u8)Copy_u16Value;
}
void TIMER1_voidSetOCRB(u16 Copy_u16Value){
	OCR1BH = (u8)(Copy_u16Value >> 8);
	OCR1BL = (u8)Copy_u16Value;
}
void TIMER1_voidGeneratePWM_A(u8 Copy_u8mode){
	switch(Copy_u8mode){
	case TIMER1_PWM_DISCONNECTED:
		CLR_BIT(TCCR1A,TCCR1A_COM1A0);
		CLR_BIT(TCCR1A,TCCR1A_COM1A1);
		break;
	case TIMER1_PWM_INVERTED:
		/* COM1A1:COM1A0 = 11 */
		SET_BIT(TCCR1A,TCCR1A_COM1A0);
		SET_BIT(TCCR1A,TCCR1A_COM1A1);
		break;
	case TIMER1_PWM_NONINVERTED:
		/* COM1A1:COM1A0 = 10 */
		CLR_BIT(TCCR1A,TCCR1A_COM1A0);
		SET_BIT(TCCR1A,TCCR1A_COM1A1);
		break;
	default :
		break;
	}
}
void TIMER1_voidGeneratePWM_B(u8 Copy_u8mode){
	switch(Copy_u8mode){
	case TIMER1_PWM_DISCONNECTED:
		CLR_BIT(TCCR1A,TCCR1A_COM1B0);
		CLR_BIT(TCCR1A,TCCR1A_COM1B1);
		break;
	case TIMER1_PWM_INVERTED:
		/* COM1B1:COM1B0 = 11 */
		SET_BIT(TCCR1A,TCCR1A_COM1B0);
		SET_BIT(TCCR1A,TCCR1A_COM1B1);
		break;
	case TIMER1_PWM_NONINVERTED:
		/* COM1B1:COM1B0 = 10 */
		CLR_BIT(TCCR1A,TCCR1A_COM1B0);
		SET_BIT(TCCR1A,TCCR1A_COM1B1);
		break;
	default :
		break;
	}
}
