/*
 * ALARM.c
 *
 *  Created on: Oct 3, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../../../lib/Service/std_types.h"
#include "../../../lib/Service/EVQ/EVQ.h"
#include "../../../lib/HAL/BUZZER/BUZZER.h"
#include "../HEATER/HEATER.h"
#include "../CLIMATE/CLIMATE.h"
#include "ALARM.h"
#include "ALARM_cfg.h"

static u8 Global_u8Active = 0 ;                        /* 0 = IDLE , 1 = ACTIVE (never goes back) */
#if ALARM_BUZZER_BEEP == 1
static u8 Global_u8BuzzerOn = 0 ;
#endif

void ALARM_voidInit(){
	BUZZER_voidOff();
	Global_u8Active = 0 ;
#if ALARM_BUZZER_BEEP == 1
	Global_u8BuzzerOn = 0 ;
#endif
}

void ALARM_voidTrigger(){
	if(Global_u8Active == 0){
		/* REQ-SEC-05 : safe state. Lamps , dimmer and door keep their state (Section 12 #2) */
		HEATER_voidForceOff();
		CLIMATE_voidForceOff();
		/* REQ-ALM-01 */
		BUZZER_voidOn();
#if ALARM_BUZZER_BEEP == 1
		Global_u8BuzzerOn = 1 ;
#endif
		Global_u8Active = 1 ;
		EVQ_voidPost(EVQ_LOCKDOWN,0);
	}else{
		/* already active : latched , nothing to do */
	}
}

u8 ALARM_u8IsActive(){
	return Global_u8Active ;
}

void ALARM_voidTask500ms(){
#if ALARM_BUZZER_BEEP == 1
	/* REQ-ALM-01 , ASSUMPTION: the buzzer beeps 0.5 s on / 0.5 s off (D-15) */
	if(Global_u8Active == 1){
		if(Global_u8BuzzerOn == 1){
			BUZZER_voidOff();
			Global_u8BuzzerOn = 0 ;
		}else{
			BUZZER_voidOn();
			Global_u8BuzzerOn = 1 ;
		}
	}
#endif
}
