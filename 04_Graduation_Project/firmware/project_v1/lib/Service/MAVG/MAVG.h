/*
 * MAVG.h
 *
 *  Created on: Oct 3, 2026
 *      Author: eslam
 */

#ifndef SERVICE_MAVG_MAVG_H_
#define SERVICE_MAVG_MAVG_H_

#include "MAVG_cfg.h"                                  /* MAVG_WINDOW sizes the struct */

typedef struct{
	u16 Samples[MAVG_WINDOW] ;
	u16 Sum ;                                          /* sum of the samples stored so far */
	u8  Index ;                                        /* where the next sample goes (= the oldest one when full) */
	u8  Count ;                                        /* samples stored , 0 .. MAVG_WINDOW */
}MAVG_t;

void MAVG_voidReset(MAVG_t * avg);
void MAVG_voidAddSample(MAVG_t * avg, u16 Copy_u16Sample);
u8   MAVG_u8IsFull(MAVG_t * avg);                      /* 1 when MAVG_WINDOW samples are stored */
u16  MAVG_u16GetAverage(MAVG_t * avg);                 /* rounded average of the samples stored so far , 0 when empty */

#endif /* SERVICE_MAVG_MAVG_H_ */
