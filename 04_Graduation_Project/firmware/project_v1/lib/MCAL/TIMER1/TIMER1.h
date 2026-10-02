/*
 * TIMER1.h
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */

#ifndef MCAL_TIMER1_TIMER1_H_
#define MCAL_TIMER1_TIMER1_H_

/*Fast PWM Modes */
#define TIMER1_PWM_DISCONNECTED   0
#define TIMER1_PWM_INVERTED       1
#define TIMER1_PWM_NONINVERTED    2

void TIMER1_voidInit();                         /* mode 14 : fast PWM , TOP = ICR1 , from TIMER1_cfg.h */
void TIMER1_voidSetOCRA(u16 Copy_u16Value);
void TIMER1_voidSetOCRB(u16 Copy_u16Value);
void TIMER1_voidGeneratePWM_A(u8 Copy_u8mode);  /* OC1A = PD5 */
void TIMER1_voidGeneratePWM_B(u8 Copy_u8mode);  /* OC1B = PD4 */

#endif /* MCAL_TIMER1_TIMER1_H_ */
