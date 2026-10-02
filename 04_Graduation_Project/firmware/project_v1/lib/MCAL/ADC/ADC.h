/*
 * ADC.h
 *
 *  Created on: Jan 26, 2026
 *      Author: Eslam
 */

#ifndef MCAL_ADC_ADC_H_
#define MCAL_ADC_ADC_H_

#define ADC_CHANNEL_0   0b00000
#define ADC_CHANNEL_1   1
#define ADC_CHANNEL_2   2
#define ADC_CHANNEL_3   3
#define ADC_CHANNEL_4   4
#define ADC_CHANNEL_5   5
#define ADC_CHANNEL_6   6
#define ADC_CHANNEL_7   7
void ADC_voidInit();
u16 ADC_u16StartConversion(u8 Copy_u8ChannelID);
void ADC_voidStartConvertionAsyn(u8 Copy_u8ChannelID );
void ADC_SetCallBack(void (*ptr)(void), u16* Copy_u16data);
void __vector_16 () __attribute__ ((signal, used, externally_visible)) ;
#endif /* MCAL_ADC_ADC_H_ */
