/*
 * TIMER1_cfg.h
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef MCAL_TIMER1_TIMER1_CFG_H_
#define MCAL_TIMER1_TIMER1_CFG_H_

/* F_CPU comes from platformio.ini (board_build.f_cpu) */
#define TIMER1_PRESCALER          8UL                 /* CS11 */
#define TIMER1_PWM_FREQ_HZ        50UL                /* 20 ms period for the servo */
/* 16 MHz : 16000000 / (8 * 50) - 1 = 39999 */
#define TIMER1_TOP_VALUE          ((u16)((F_CPU / (TIMER1_PRESCALER * TIMER1_PWM_FREQ_HZ)) - 1))
/* 16 MHz : 2000 timer ticks per ms (1 tick = 0.5 us) */
#define TIMER1_TICKS_PER_MS       ((u16)(F_CPU / (TIMER1_PRESCALER * 1000UL)))

#endif /* MCAL_TIMER1_TIMER1_CFG_H_ */
