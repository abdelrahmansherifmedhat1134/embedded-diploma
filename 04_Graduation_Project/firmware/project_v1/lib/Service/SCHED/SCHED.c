/*
 * SCHED.c
 *
 *  Created on: Oct 3, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../std_types.h"
#include "../Bit_math.h"
#include "../../MCAL/GIE/GIE.h"
#include "../../MCAL/TIMER2/TIMER2.h"
#include "SCHED.h"
#include "SCHED_cfg.h"

/* period of every task in ticks (1 tick = 1 ms) , index = task ID */
static const __flash u16 SCHED_PERIOD_ARR[SCHED_TASK_COUNT] = {5,10,100,500,1000};
/* first tick every task is due on , counted from its first full period */
static const __flash u8  SCHED_OFFSET_ARR[SCHED_TASK_COUNT] = {SCHED_OFFSET_5MS,SCHED_OFFSET_10MS,SCHED_OFFSET_100MS,SCHED_OFFSET_500MS,SCHED_OFFSET_1S};

/* written by the tick ISR , read by the main loop with interrupts off */
static volatile u32 Global_u32Tick = 0 ;
static volatile u8  Global_u8Flags = 0 ;               /* bit n = task n is due */
static volatile u16 Global_u16Overruns = 0 ;
/* used by the ISR only (loaded in SCHED_voidInit , before the interrupt is on) */
static u16 Global_u16DownCount[SCHED_TASK_COUNT] ;

/* Timer2 compare ISR callback , every 1 ms : count and set flags , no real work.
 * ASSUMPTION: D-20 , no tick hook (the 7-segment display needs no refresh). */
static void SCHED_voidTick(void){
	u8 i ;
	u8 Local_u8Mask = 1 ;                              /* 1 << i , kept as a mask : no variable shift in the ISR */
	/*1. time base */
	Global_u32Tick++;
	/*2. one down-counter per task */
	for(i = 0 ; i < SCHED_TASK_COUNT ; i++){
		Global_u16DownCount[i]--;
		if(Global_u16DownCount[i] == 0){
			Global_u16DownCount[i] = SCHED_PERIOD_ARR[i] ;
			if((Global_u8Flags & Local_u8Mask) != 0){
				/* the main loop did not serve the last one in time */
				Global_u16Overruns++;
			}
			Global_u8Flags |= Local_u8Mask ;
		}
		Local_u8Mask <<= 1 ;
	}
}

void SCHED_voidInit(){
	u8 i ;
	Global_u32Tick = 0 ;
	Global_u8Flags = 0 ;
	Global_u16Overruns = 0 ;
	for(i = 0 ; i < SCHED_TASK_COUNT ; i++){
		/* first time due on tick period + offset : 5 , 12 , 101 , 503 , 1004 */
		Global_u16DownCount[i] = SCHED_PERIOD_ARR[i] + SCHED_OFFSET_ARR[i] ;
	}
	/* Timer2 belongs to the scheduler : CTC , 1 ms compare match */
	TIMER2_voidInit(SCHED_TIMER_CLOCK,TIMER2_CTC);
	TIMER2_voidSetOCR(SCHED_OCR_VALUE);
	TIMER2_voidSetCallBack_OC(SCHED_voidTick);
}

void SCHED_voidStart(){
	TIMER2_voidEnableOCInterrupt();
}

/* Main-loop context only (interrupts are enabled there , so disable / enable is correct) */
u8 SCHED_u8IsTaskDue(u8 Copy_u8TaskID){
	u8 Local_u8Due = 0 ;
	if(Copy_u8TaskID < SCHED_TASK_COUNT){
		/* test-and-clear must not be cut by the tick ISR (it sets bits in the same byte) */
		GIE_voidDisableGlobalInterrupt();
		if(GET_BIT(Global_u8Flags,Copy_u8TaskID)){
			CLR_BIT(Global_u8Flags,Copy_u8TaskID);
			Local_u8Due = 1 ;
		}
		GIE_voidEnableGlobalInterrupt();
	}else{
		//error
	}
	return Local_u8Due ;
}

u32 SCHED_u32GetTickMs(){
	u32 Local_u32Tick ;
	/* 4 bytes : the ISR must not change them half way through the copy */
	GIE_voidDisableGlobalInterrupt();
	Local_u32Tick = Global_u32Tick ;
	GIE_voidEnableGlobalInterrupt();
	return Local_u32Tick ;
}

u16 SCHED_u16GetOverruns(){
	u16 Local_u16Overruns ;
	GIE_voidDisableGlobalInterrupt();
	Local_u16Overruns = Global_u16Overruns ;
	GIE_voidEnableGlobalInterrupt();
	return Local_u16Overruns ;
}
