/*
 * TIMER0.h
 *
 *  Created on: Feb 4, 2026
 *      Author: Eslam
 */

#ifndef MCAL_TIMER0_TIMER0_H_
#define MCAL_TIMER0_TIMER0_H_

#define TIMER0_OFF                0
#define TIMER0_NO_PRE             1
#define TIMER0_DIV_8              2
#define TIMER0_DIV_64       	  3
#define TIMER0_DIV_256            4
#define TIMER0_DIV_1024           5
#define TIMER0_FALLING_EXT        6
#define TIMER0_RISING_EXT  		  7


/*Timer MOdes */
#define TIMER0_NORMAL			0
#define TIMER0_CTC				1
#define TIMER0_PWM				2
#define TIMER0_FAST_PWM			3

/*CTC Wave Generation Modes */
#define TIMER0_CTC_DISCONNECTED     0
#define TIMER0_CTC_TOG 				1
#define TIMER0_CTC_SET				2
#define TIMER0_CTC_CLR				3
/*Fast PWM Modes */
#define TIMER0_PWM_DISCONNECTED   0
#define TIMER0_PWM_INVERTED       1
#define TIMER0_PWM_NONINVERTED    2

void TIMER0_voidInit(u8 Copy_u8PreScalare,u8 Copy_u8Mode);
void TIMER0_voidSetPreload(u8 Copy_u8Preload);
void TIMER0_voidSetOCR(u8 Copy_u8OCR_Val);
void TIMER0_voidEnableOVInterrupt();
void TIMER0_voidEnableOCInterrupt();
void TIMER0_voidDisableOVInterrupt();
void TIMER0_voidDisableOCInterrupt();
void TIMER0_GenerateWave_CTC(u8 Copy_u8mode);
void TIMER0_SetCallBack_OV(void (*ptr)(void));
void TIMER0_SetCallBack_OC(void (*ptr)(void));
void TIMER0_GeneratePWM(u8 Copy_u8mode , u8 DutyCycle);
void __vector_11 () __attribute__ ((signal, used, externally_visible)) ;
void __vector_10 () __attribute__ ((signal, used, externally_visible)) ;

#endif /* MCAL_TIMER0_TIMER0_H_ */
