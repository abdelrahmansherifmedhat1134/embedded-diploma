/*
 * SCHED_cfg.h
 *
 *  Created on: Oct 3, 2026
 *      Author: eslam
 */

#ifndef SERVICE_SCHED_SCHED_CFG_H_
#define SERVICE_SCHED_SCHED_CFG_H_

/* F_CPU comes from platformio.ini (board_build.f_cpu)
 * 16 MHz / 64 = 250 kHz timer clock , 250 counts = 1.000 ms exactly */
#define SCHED_TIMER_PRESCALER     64UL                 /* TIMER2_DIV_64 */
#define SCHED_TIMER_CLOCK         TIMER2_DIV_64        /* must be the code of SCHED_TIMER_PRESCALER */
#define SCHED_TICK_HZ             1000UL
#define SCHED_OCR_VALUE           ((u8)((F_CPU / (SCHED_TIMER_PRESCALER * SCHED_TICK_HZ)) - 1))    /* 249 */
#if (F_CPU % (SCHED_TIMER_PRESCALER * SCHED_TICK_HZ)) != 0
#error "F_CPU does not give an exact 1 ms tick with prescaler 64"
#endif
/* first tick each task becomes due on : all different , so two tasks never start in the same ms */
#define SCHED_OFFSET_5MS          0
#define SCHED_OFFSET_10MS         2
#define SCHED_OFFSET_100MS        1
#define SCHED_OFFSET_500MS        3
#define SCHED_OFFSET_1S           4

#endif /* SERVICE_SCHED_SCHED_CFG_H_ */
