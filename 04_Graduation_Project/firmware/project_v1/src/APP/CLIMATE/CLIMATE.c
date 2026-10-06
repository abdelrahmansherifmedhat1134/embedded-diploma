/*
 * CLIMATE.c
 *
 *  Created on: Oct 3, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../../../lib/Service/std_types.h"
#include "../../../lib/Service/MAVG/MAVG.h"
#include "../../../lib/Service/EVQ/EVQ.h"
#include "../../../lib/HAL/LM35/LM35.h"
#include "../../../lib/HAL/LM35/LM35_cfg.h"
#include "../../../lib/HAL/FAN/FAN.h"
#include "CLIMATE.h"
#include "CLIMATE_cfg.h"

#define CLIMATE_STATE_WAIT          0                  /* boot : the average is not full yet */
#define CLIMATE_STATE_AC_OFF        1
#define CLIMATE_STATE_AC_ON         2
#define CLIMATE_STATE_FORCED_OFF    3                  /* lockdown : stays until reset */

/* thresholds in LM35 steps (0.25 C) : integer math only */
#define CLIMATE_ON_ABOVE_X4         (CLIMATE_AC_ON_ABOVE_C  * LM35_STEPS_PER_C)
#define CLIMATE_OFF_BELOW_X4        (CLIMATE_AC_OFF_BELOW_C * LM35_STEPS_PER_C)
#define CLIMATE_TEMP_MAX            255                /* biggest value a u8 getter can return */

static MAVG_t Global_RoomAvg ;
static u8 Global_u8State = CLIMATE_STATE_WAIT ;

void CLIMATE_voidInit(){
	MAVG_voidReset(&Global_RoomAvg);
	FAN_voidSetSpeed(0);
	Global_u8State = CLIMATE_STATE_WAIT ;
}

void CLIMATE_voidTask100ms(){
	/* REQ-AC-01 */
	CLIMATE_voidFeedSample(LM35_u16ReadTempX4(LM35_AMBIENT));
}

void CLIMATE_voidFeedSample(u16 Copy_u16TempX4){
	u16 Local_u16AvgX4 ;
	/* the room temperature is averaged in every state , so the UIs can always show it */
	MAVG_voidAddSample(&Global_RoomAvg,Copy_u16TempX4);
	if((Global_u8State == CLIMATE_STATE_WAIT) && (MAVG_u8IsFull(&Global_RoomAvg) == 1)){
		Global_u8State = CLIMATE_STATE_AC_OFF ;
	}
	Local_u16AvgX4 = MAVG_u16GetAverage(&Global_RoomAvg);
	switch(Global_u8State){
	case CLIMATE_STATE_AC_OFF:
		/* REQ-AC-02 : higher than 28 C -> on */
		if(Local_u16AvgX4 > CLIMATE_ON_ABOVE_X4){
			FAN_voidSetSpeed(CLIMATE_FAN_SPEED);       /* REQ-AC-03 : fixed speed (ramp not built , D-16) */
			Global_u8State = CLIMATE_STATE_AC_ON ;
			EVQ_voidPost(EVQ_AC,1);
		}
		break ;
	case CLIMATE_STATE_AC_ON:
		/* REQ-AC-02 : lower than 21 C -> off ; 21..28 keeps the state (hysteresis) */
		if(Local_u16AvgX4 < CLIMATE_OFF_BELOW_X4){
			FAN_voidSetSpeed(0);
			Global_u8State = CLIMATE_STATE_AC_OFF ;
			EVQ_voidPost(EVQ_AC,0);
		}
		break ;
	default:
		/* WAIT , FORCED_OFF : no decision */
		break ;
	}
}

u8 CLIMATE_u8IsAcOn(){
	return (Global_u8State == CLIMATE_STATE_AC_ON) ? 1 : 0 ;
}

u8 CLIMATE_u8IsTempValid(){
	return MAVG_u8IsFull(&Global_RoomAvg);
}

u8 CLIMATE_u8GetRoomTemp(){
	/* quarter degrees -> whole degrees , rounded */
	u16 Local_u16Temp = (MAVG_u16GetAverage(&Global_RoomAvg) + (LM35_STEPS_PER_C / 2)) / LM35_STEPS_PER_C ;
	if(Local_u16Temp > CLIMATE_TEMP_MAX){
		Local_u16Temp = CLIMATE_TEMP_MAX ;
	}
	return (u8)Local_u16Temp ;
}

void CLIMATE_voidForceOff(){
	/* REQ-SEC-05 : safe state , fan off until reset */
	FAN_voidSetSpeed(0);
	Global_u8State = CLIMATE_STATE_FORCED_OFF ;
}
