/*
 * ADC.c
 *
 *  Created on: Jan 26, 2026
 *      Author: Eslam
 */
#include "../../Service/std_types.h"
#include "../../Service/Bit_math.h"
#include "../reg_def.h"
#include "ADC.h"

void (*ADC_Ptr)() = NULL;
u16 * data = NULL;
void ADC_voidInit(){
	/*1. Select ref */
	/*1. Select AVCC */
	SET_BIT(ADMUX,ADMUX_REFS0);
	SET_BIT(ADMUX,ADMUX_REFS1);
	/*2. Prescaler 128 */
	SET_BIT(ADCSRA,ADCSRA_ADPS0);
	SET_BIT(ADCSRA,ADCSRA_ADPS1);
	SET_BIT(ADCSRA,ADCSRA_ADPS2);
	/*3. Enable ADC */
	SET_BIT(ADCSRA,ADCSRA_ADEN);
}
u16 ADC_u16StartConversion(u8 Copy_u8ChannelID){
	/* select channel */
	ADMUX &= 0b11100000;   // mask channel ID
	ADMUX |= Copy_u8ChannelID;
	/* start conversion */
	SET_BIT(ADCSRA,ADCSRA_ADSC);
	/*polling (wait until conversion finish )*/
	while(GET_BIT(ADCSRA,ADCSRA_ADIF)==0){
		/*Busy wait*/
	}
	/*clear flag */
	SET_BIT(ADCSRA,ADCSRA_ADIF);
	/*return ADC val */
	return ADC;
}
void ADC_voidStartConvertionAsyn(u8 Copy_u8ChannelID ){
	/* select channel */
	ADMUX &= 0b11100000;   // mask channel ID
	ADMUX |= Copy_u8ChannelID;
	/*Enable Interrupt */
	SET_BIT(ADCSRA,ADCSRA_ADIE);
	/* start conversion */
	SET_BIT(ADCSRA,ADCSRA_ADSC);

}
void ADC_SetCallBack(void (*ptr)(void), u16* Copy_u16data){
	data = Copy_u16data;
	ADC_Ptr = ptr;
}
void __vector_16 (){
	if(data != NULL ){

		*data = ADC ;
	}
	if(ADC_Ptr != NULL ){
		ADC_Ptr();
	}
}
