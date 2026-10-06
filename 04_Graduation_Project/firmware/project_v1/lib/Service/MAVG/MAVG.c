/*
 * MAVG.c
 *
 *  Created on: Oct 3, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../std_types.h"
#include "MAVG.h"
#include "MAVG_cfg.h"

void MAVG_voidReset(MAVG_t * avg){
	/* the old samples need no clearing : Count says how many are valid */
	avg->Sum = 0 ;
	avg->Index = 0 ;
	avg->Count = 0 ;
}

/* REQ-HTR-08 : keeps the last MAVG_WINDOW samples and their running sum */
void MAVG_voidAddSample(MAVG_t * avg, u16 Copy_u16Sample){
	if(avg->Count == MAVG_WINDOW){
		/*1. window full : the oldest sample leaves the sum */
		avg->Sum -= avg->Samples[avg->Index] ;
	}else{
		avg->Count++;
	}
	/*2. the new sample takes its place */
	avg->Samples[avg->Index] = Copy_u16Sample ;
	avg->Sum += Copy_u16Sample ;
	avg->Index++;
	if(avg->Index >= MAVG_WINDOW){
		avg->Index = 0 ;
	}
}

u8 MAVG_u8IsFull(MAVG_t * avg){
	if(avg->Count == MAVG_WINDOW){
		return 1 ;
	}
	return 0 ;
}

u16 MAVG_u16GetAverage(MAVG_t * avg){
	if(avg->Count == 0){
		return 0 ;
	}
	/* integer math , rounded to nearest : add half of the divisor first */
	return (avg->Sum + (avg->Count / 2)) / avg->Count ;
}
