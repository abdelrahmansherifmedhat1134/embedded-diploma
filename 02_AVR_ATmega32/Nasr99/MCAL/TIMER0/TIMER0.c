#include "../../Service/std_types.h"
#include "../../Service/Bit_math.h"
#include "../reg_def.h"
#include "TIMER0.h"


void (*TIMER0_ov_ptr)(void)= NULL;
void (*TIMER0_oc_ptr)(void)= NULL;

void TIMER0_voidInit(u8 Copy_u8PreScalare,u8 Copy_u8Mode){
	TCCR0 &= 0b11111000; // mask
	TCCR0 |= Copy_u8PreScalare ; // set with mask

	switch(Copy_u8Mode){
	case TIMER0_CTC_DISCONNECTED:
		CLR_BIT(TCCR0,TCCR0_WGM00);
		CLR_BIT(TCCR0,TCCR0_WGM01);
		break;
	case TIMER0_CTC:
		CLR_BIT(TCCR0,TCCR0_WGM00);
		SET_BIT(TCCR0,TCCR0_WGM01);
		break;
	case TIMER0_PWM:
		SET_BIT(TCCR0,TCCR0_WGM00);
		CLR_BIT(TCCR0,TCCR0_WGM01);
		break;
	case TIMER0_FAST_PWM:
		SET_BIT(TCCR0,TCCR0_WGM00);
		SET_BIT(TCCR0,TCCR0_WGM01);
		break;
	default:
		/*Error*/
		break ;

	}

}
void TIMER0_voidSetPreload(u8 Copy_u8Preload){
	TCNT0 = Copy_u8Preload;
}
void TIMER0_voidSetOCR(u8 Copy_u8OCR_Val){
	OCR0 = Copy_u8OCR_Val;
}

void TIMER0_voidEnableOVInterrupt(){
	SET_BIT(TIMSK,TIMSK_TOIE0);
}
void TIMER0_voidEnableOCInterrupt(){
	SET_BIT(TIMSK,TIMSK_OCIE0);

}
void TIMER0_voidDisableOVInterrupt(){
	CLR_BIT(TIMSK,TIMSK_TOIE0);

}
void TIMER0_voidDisableOCInterrupt(){
	CLR_BIT(TIMSK,TIMSK_OCIE0);

}
void TIMER0_SetCallBack_OV(void (*ptr)(void)){
	TIMER0_ov_ptr = ptr;
}
void TIMER0_SetCallBack_OC(void (*ptr)(void)){
	TIMER0_oc_ptr = ptr;
}

void TIMER0_GenerateWave_CTC(u8 Copy_u8mode){
	switch(Copy_u8mode){
		case TIMER0_CTC_DISCONNECTED:
			CLR_BIT(TCCR0,TCCR0_COM00);
			CLR_BIT(TCCR0,TCCR0_COM01);
			break;
		case TIMER0_CTC_TOG:
			SET_BIT(TCCR0,TCCR0_COM00);
			CLR_BIT(TCCR0,TCCR0_COM01);
			break;
		case TIMER0_CTC_SET:
			CLR_BIT(TCCR0,TCCR0_COM00);
			SET_BIT(TCCR0,TCCR0_COM01);
			break;
		case TIMER0_CTC_CLR:
			SET_BIT(TCCR0,TCCR0_COM00);
			SET_BIT(TCCR0,TCCR0_COM01);
			break;
		default:
			break;

	}
}

void TIMER0_GeneratePWM(u8 Copy_u8mode , u8 DutyCycle){
	switch(Copy_u8mode){
	case TIMER0_PWM_DISCONNECTED:
		CLR_BIT(TCCR0,TCCR0_COM00);
		CLR_BIT(TCCR0,TCCR0_COM01);
		break;
	case TIMER0_PWM_INVERTED:
		CLR_BIT(TCCR0,TCCR0_COM00);
		SET_BIT(TCCR0,TCCR0_COM01);
		OCR0 =256*DutyCycle/100;
		break;
	case TIMER0_PWM_NONINVERTED:
		SET_BIT(TCCR0,TCCR0_COM00);
		SET_BIT(TCCR0,TCCR0_COM01);
		OCR0 =256*DutyCycle/100;
		break;
	default :
		break;
	}
}

void __vector_11 () {
	if (TIMER0_ov_ptr!=NULL){
		TIMER0_ov_ptr();
	}
}
void __vector_10 () {
	if (TIMER0_oc_ptr!=NULL){
		TIMER0_oc_ptr();
	}
}
