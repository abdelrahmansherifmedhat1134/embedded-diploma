/*
 * TIMER2.h
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */

#ifndef MCAL_TIMER2_TIMER2_H_
#define MCAL_TIMER2_TIMER2_H_

/* Timer2 prescaler codes differ from Timer0 */
#define TIMER2_OFF                0
#define TIMER2_NO_PRE             1
#define TIMER2_DIV_8              2
#define TIMER2_DIV_32             3
#define TIMER2_DIV_64             4
#define TIMER2_DIV_128            5
#define TIMER2_DIV_256            6
#define TIMER2_DIV_1024           7

/*Timer MOdes */
#define TIMER2_NORMAL             0
#define TIMER2_CTC                1

void TIMER2_voidInit(u8 Copy_u8PreScalare,u8 Copy_u8Mode);
void TIMER2_voidSetOCR(u8 Copy_u8OCR_Val);
void TIMER2_voidEnableOCInterrupt();
void TIMER2_voidDisableOCInterrupt();
void TIMER2_voidSetCallBack_OC(void (*ptr)(void));
void __vector_4 () __attribute__ ((signal, used, externally_visible)) ;

#endif /* MCAL_TIMER2_TIMER2_H_ */
