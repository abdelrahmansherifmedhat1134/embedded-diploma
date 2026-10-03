/*
 * SCHED.h
 *
 *  Created on: Oct 3, 2026
 *      Author: eslam
 */

#ifndef SERVICE_SCHED_SCHED_H_
#define SERVICE_SCHED_SCHED_H_

/* Task IDs = bit numbers in the flags byte */
#define SCHED_TASK_5MS      0
#define SCHED_TASK_10MS     1
#define SCHED_TASK_100MS    2
#define SCHED_TASK_500MS    3
#define SCHED_TASK_1S       4
#define SCHED_TASK_COUNT    5

void SCHED_voidInit();                                 /* Timer2 CTC , callback ; interrupt still off */
void SCHED_voidStart();                                /* enable the tick interrupt */
u8   SCHED_u8IsTaskDue(u8 Copy_u8TaskID);              /* 1 once per period , clears the flag atomically */
u32  SCHED_u32GetTickMs();                             /* read with interrupts off */
u16  SCHED_u16GetOverruns();                           /* flags that were set again before being served */

#endif /* SERVICE_SCHED_SCHED_H_ */
