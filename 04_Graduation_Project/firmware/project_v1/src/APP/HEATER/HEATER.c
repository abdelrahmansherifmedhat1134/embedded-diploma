/*
 * HEATER.c
 *
 *  Created on: Oct 3, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../../../lib/Service/std_types.h"
#include "../../../lib/Service/MAVG/MAVG.h"
#include "../../../lib/Service/EVQ/EVQ.h"
#include "../../../lib/Service/ESTORE/ESTORE.h"
#include "../../../lib/Service/ESTORE/ESTORE_cfg.h"
#include "../../../lib/HAL/BUTTON/BUTTON.h"
#include "../../../lib/HAL/LM35/LM35.h"
#include "../../../lib/HAL/LM35/LM35_cfg.h"
#include "../../../lib/HAL/RELAY/RELAY.h"
#include "../../../lib/HAL/LED/LED.h"
#include "../../../lib/HAL/SEVEN_SEG/SEVEN_SEG.h"
#include "HEATER.h"
#include "HEATER_cfg.h"

/* panel states (architecture.md 5.6) */
#define HEATER_STATE_OFF          0
#define HEATER_STATE_RUN          1                    /* on , display = water temperature */
#define HEATER_STATE_SET          2                    /* on , setting mode : display = set temperature , blinking */
#define HEATER_STATE_LOCKED       3                    /* lockdown : off , buttons ignored until reset */

/* what the 7-segment display shows now : it is written only when this changes */
#define HEATER_SHOWN_BLANK        0
#define HEATER_SHOWN_NUMBER       1
#define HEATER_SHOWN_UNKNOWN      2                    /* the last I2C write failed : write again next time */

#define HEATER_DISPLAY_MAX        99                   /* 2 digits */
#define HEATER_TEMP_MAX           255                  /* biggest value a u8 getter can return */

static MAVG_t Global_WaterAvg ;
static u8 Global_u8State = HEATER_STATE_OFF ;
static u8 Global_u8Element = HEATER_ELEMENT_NONE ;
static u8 Global_u8SetTemp = HEATER_SET_DEFAULT ;      /* working copy ; the stored copy is in ESTORE */
static u8 Global_u8SetTimer = 0 ;                      /* 100 ms tasks left in setting mode */
static u8 Global_u8BlinkPhase = 0 ;                    /* toggles every 500 ms : one blink period = 1 s */
static u8 Global_u8Shown = HEATER_SHOWN_BLANK ;
static u8 Global_u8ShownNumber = 0 ;

/*************************** helpers ***************************/
/* REQ-HTR-03 : 35..75 in steps of 5 */
static u8 HEATER_u8IsValidSetTemp(u8 Copy_u8Temp){
	if((Copy_u8Temp >= HEATER_SET_MIN) && (Copy_u8Temp <= HEATER_SET_MAX) && ((Copy_u8Temp % HEATER_SET_STEP) == 0)){
		return 1 ;
	}
	return 0 ;
}

/* REQ-HTR-04 : the stored value , or 60 when the stored byte is not a legal set temperature */
static u8 HEATER_u8ReadStoredSetTemp(){
	u8 Local_u8Temp = ESTORE_u8ReadByte(ESTORE_ADDR_HEATER_SET);
	if(HEATER_u8IsValidSetTemp(Local_u8Temp) == 0){
		Local_u8Temp = HEATER_SET_DEFAULT ;
	}
	return Local_u8Temp ;
}

/* REQ-HTR-04 : save only when the value really changed (no write , no event otherwise) */
static void HEATER_voidSaveIfChanged(){
	if(Global_u8SetTemp != ESTORE_u8ReadByte(ESTORE_ADDR_HEATER_SET)){
		ESTORE_voidWriteByte(ESTORE_ADDR_HEATER_SET,Global_u8SetTemp);
		EVQ_voidPost(EVQ_HEATER_SET,Global_u8SetTemp);
	}
}

/* the two elements are never on together : always "one off , then the other on" */
static void HEATER_voidSetElement(u8 Copy_u8Element){
	switch(Copy_u8Element){
	case HEATER_ELEMENT_HEATING:
		RELAY_voidOff(RELAY_COOLING);
		RELAY_voidOn(RELAY_HEATING);
		break ;
	case HEATER_ELEMENT_COOLING:
		RELAY_voidOff(RELAY_HEATING);
		RELAY_voidOn(RELAY_COOLING);
		break ;
	default:
		RELAY_voidOff(RELAY_HEATING);
		RELAY_voidOff(RELAY_COOLING);
		Copy_u8Element = HEATER_ELEMENT_NONE ;
		break ;
	}
	Global_u8Element = Copy_u8Element ;
}

/* 7-segment : the chips are written only when the number or the blank state changes */
static void HEATER_voidShow(u8 Copy_u8On, u8 Copy_u8Number){
	u8 Local_u8Written = 0 ;
	if(Copy_u8On == 0){
		if(Global_u8Shown != HEATER_SHOWN_BLANK){
			SEVEN_SEG_voidDisable();
			Global_u8Shown = HEATER_SHOWN_BLANK ;
			Local_u8Written = 1 ;
		}
	}else if((Global_u8Shown != HEATER_SHOWN_NUMBER) || (Global_u8ShownNumber != Copy_u8Number)){
		SEVEN_SEG_voidSetNumber(Copy_u8Number);
		SEVEN_SEG_voidEnable();
		Global_u8Shown = HEATER_SHOWN_NUMBER ;
		Global_u8ShownNumber = Copy_u8Number ;
		Local_u8Written = 1 ;
	}else{
		/* already shown : no I2C traffic */
	}
	if((Local_u8Written == 1) && (SEVEN_SEG_u8GetStatus() != 0)){
		Global_u8Shown = HEATER_SHOWN_UNKNOWN ;
	}
}

/* display and heater LED , from the panel state , the element and the blink phase */
static void HEATER_voidUpdateOutputs(){
	u8 Local_u8Temp ;
	switch(Global_u8State){
	case HEATER_STATE_RUN:
		/* REQ-HTR-10 : water temperature (nothing new is shown before the first sample after turn-on) */
		if(Global_WaterAvg.Count != 0){
			Local_u8Temp = HEATER_u8GetWaterTemp();
			if(Local_u8Temp > HEATER_DISPLAY_MAX){
				Local_u8Temp = HEATER_DISPLAY_MAX ;
			}
			HEATER_voidShow(1,Local_u8Temp);
		}
		break ;
	case HEATER_STATE_SET:
		/* REQ-HTR-11 : set temperature , blinking with a 1 s period */
		HEATER_voidShow(Global_u8BlinkPhase,Global_u8SetTemp);
		break ;
	default:
		/* REQ-HTR-06 : OFF and LOCKED = blank */
		HEATER_voidShow(0,0);
		break ;
	}
	/* REQ-HTR-13 : heating = blink , cooling = steady on , neither = off */
	if((Global_u8Element == HEATER_ELEMENT_COOLING) || ((Global_u8Element == HEATER_ELEMENT_HEATING) && (Global_u8BlinkPhase == 1))){
		LED_voidOn(LED_HEATER);
	}else{
		LED_voidOff(LED_HEATER);
	}
}

static void HEATER_voidTurnOn(){
	/* REQ-HTR-05 ; ASSUMPTION: the average restarts at turn-on (Section 12 #5) */
	Global_u8SetTemp = HEATER_u8ReadStoredSetTemp();
	MAVG_voidReset(&Global_WaterAvg);
	Global_u8State = HEATER_STATE_RUN ;
	EVQ_voidPost(EVQ_HEATER_POWER,1);
}

static void HEATER_voidTurnOff(u8 Copy_u8NextState){
	if((Global_u8State == HEATER_STATE_RUN) || (Global_u8State == HEATER_STATE_SET)){
		/* REQ-HTR-04 : a value changed in setting mode is not lost (Section 12 #6) */
		HEATER_voidSaveIfChanged();
		EVQ_voidPost(EVQ_HEATER_POWER,0);
	}
	/* REQ-HTR-06 : both elements off at once ; the display and the LED follow in the next 100 ms task */
	HEATER_voidSetElement(HEATER_ELEMENT_NONE);
	Global_u8State = Copy_u8NextState ;
}

static void HEATER_voidRestartSetting(){
	Global_u8SetTimer = HEATER_SETTING_TIMEOUT ;       /* REQ-HTR-12 */
	Global_u8BlinkPhase = 1 ;                          /* REQ-HTR-11 : the value is visible at once */
}

/*************************** public ***************************/
void HEATER_voidInit(){
	Global_u8SetTemp = HEATER_u8ReadStoredSetTemp();   /* REQ-HTR-03 , REQ-HTR-04 */
	MAVG_voidReset(&Global_WaterAvg);
	/* REQ-HTR-05 , REQ-HTR-06 : OFF at power-up , everything off */
	HEATER_voidSetElement(HEATER_ELEMENT_NONE);
	LED_voidOff(LED_HEATER);
	SEVEN_SEG_voidDisable();
	Global_u8Shown = HEATER_SHOWN_BLANK ;
	Global_u8State = HEATER_STATE_OFF ;
	Global_u8SetTimer = 0 ;
	Global_u8BlinkPhase = 0 ;
}

void HEATER_voidTask10ms(){
	/* the events are always read , so a press made while OFF or LOCKED is not kept for later */
	u8 Local_u8OnOff = BUTTON_u8GetReleaseEvent(BUTTON_ONOFF);     /* REQ-HTR-05 : act on release */
	u8 Local_u8Up    = BUTTON_u8GetPressEvent(BUTTON_UP);
	u8 Local_u8Down  = BUTTON_u8GetPressEvent(BUTTON_DOWN);
	switch(Global_u8State){
	case HEATER_STATE_OFF:
		/* ASSUMPTION: Up / Down are ignored while OFF (D-10) */
		if(Local_u8OnOff == 1){
			HEATER_voidTurnOn();
		}
		break ;
	case HEATER_STATE_RUN:
		if(Local_u8OnOff == 1){
			HEATER_voidTurnOff(HEATER_STATE_OFF);
		}else if((Local_u8Up == 1) || (Local_u8Down == 1)){
			/* REQ-HTR-01 : the first press only enters setting mode */
			HEATER_voidRestartSetting();
			Global_u8State = HEATER_STATE_SET ;
		}else{
		}
		break ;
	case HEATER_STATE_SET:
		if(Local_u8OnOff == 1){
			HEATER_voidTurnOff(HEATER_STATE_OFF);
		}else{
			if(Local_u8Up == 1){
				/* REQ-HTR-02 , REQ-HTR-03 : +5 , not above 75 */
				if(Global_u8SetTemp <= (HEATER_SET_MAX - HEATER_SET_STEP)){
					Global_u8SetTemp += HEATER_SET_STEP ;
				}
				HEATER_voidRestartSetting();
			}
			if(Local_u8Down == 1){
				/* REQ-HTR-02 , REQ-HTR-03 : -5 , not below 35 */
				if(Global_u8SetTemp >= (HEATER_SET_MIN + HEATER_SET_STEP)){
					Global_u8SetTemp -= HEATER_SET_STEP ;
				}
				HEATER_voidRestartSetting();
			}
		}
		break ;
	default:
		/* LOCKED : buttons ignored until reset (REQ-SEC-05) */
		break ;
	}
}

void HEATER_voidTask100ms(){
	/* REQ-HTR-07 : sampled every 100 ms in every panel state */
	HEATER_voidFeedSample(LM35_u16ReadTempX4(LM35_WATER));
	if(Global_u8State == HEATER_STATE_SET){
		/* REQ-HTR-12 : 5 s without Up / Down ends setting mode */
		Global_u8SetTimer--;
		if(Global_u8SetTimer == 0){
			HEATER_voidSaveIfChanged();                /* REQ-HTR-04 */
			Global_u8State = HEATER_STATE_RUN ;
		}
	}
	HEATER_voidUpdateOutputs();
}

void HEATER_voidTask500ms(){
	/* REQ-HTR-11 , REQ-HTR-13 : 500 ms on , 500 ms off */
	Global_u8BlinkPhase ^= 1 ;
	HEATER_voidUpdateOutputs();
}

void HEATER_voidFeedSample(u16 Copy_u16TempX4){
	u16 Local_u16AvgX4 ;
	u8 Local_u8Element = Global_u8Element ;
	MAVG_voidAddSample(&Global_WaterAvg,Copy_u16TempX4);
	/* REQ-HTR-08 : decisions only while on , and only with 10 samples */
	if(((Global_u8State == HEATER_STATE_RUN) || (Global_u8State == HEATER_STATE_SET)) && (MAVG_u8IsFull(&Global_WaterAvg) == 1)){
		Local_u16AvgX4 = MAVG_u16GetAverage(&Global_WaterAvg);
		/* REQ-HTR-09 : compared in LM35 steps (0.25 C) ; between the two limits nothing changes */
		if(Local_u16AvgX4 < ((u16)(Global_u8SetTemp - HEATER_BAND) * LM35_STEPS_PER_C)){
			Local_u8Element = HEATER_ELEMENT_HEATING ;
		}else if(Local_u16AvgX4 > ((u16)(Global_u8SetTemp + HEATER_BAND) * LM35_STEPS_PER_C)){
			Local_u8Element = HEATER_ELEMENT_COOLING ;
		}else{
		}
		if(Local_u8Element != Global_u8Element){
			HEATER_voidSetElement(Local_u8Element);
			EVQ_voidPost(EVQ_HEATER_ELEMENT,Local_u8Element);      /* uart_protocol.md 9 */
		}
	}
}

u8 HEATER_u8IsOn(){
	return ((Global_u8State == HEATER_STATE_RUN) || (Global_u8State == HEATER_STATE_SET)) ? 1 : 0 ;
}

u8 HEATER_u8GetElement(){
	return Global_u8Element ;
}

u8 HEATER_u8GetWaterTemp(){
	/* quarter degrees -> whole degrees , rounded */
	u16 Local_u16Temp = (MAVG_u16GetAverage(&Global_WaterAvg) + (LM35_STEPS_PER_C / 2)) / LM35_STEPS_PER_C ;
	if(Local_u16Temp > HEATER_TEMP_MAX){
		Local_u16Temp = HEATER_TEMP_MAX ;
	}
	return (u8)Local_u16Temp ;
}

u8 HEATER_u8GetSetTemp(){
	return Global_u8SetTemp ;
}

u8 HEATER_u8SetSetTemp(u8 Copy_u8Temp){
	u8 Local_u8Accepted = 0 ;
	/* REQ-HTR-14 : same rules as the panel (35..75 , steps of 5) , saved at once */
	if(HEATER_u8IsValidSetTemp(Copy_u8Temp) == 1){
		Global_u8SetTemp = Copy_u8Temp ;
		HEATER_voidSaveIfChanged();
		if(Global_u8State == HEATER_STATE_SET){
			HEATER_voidRestartSetting();
		}
		Local_u8Accepted = 1 ;
	}else{
		//error
	}
	return Local_u8Accepted ;
}

void HEATER_voidForceOff(){
	/* REQ-SEC-05 : as "ON/OFF released" , then locked until reset */
	HEATER_voidTurnOff(HEATER_STATE_LOCKED);
}
