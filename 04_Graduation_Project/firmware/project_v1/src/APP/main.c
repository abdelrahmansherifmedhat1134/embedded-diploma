/*
 * main.c
 *
 *  Created on: Oct 4, 2026
 *      Author: eslam
 *
 *  ATmega32 smart home + water heater (CLAUDE.md Section 5, docs/architecture.md 4.2 and 4.6).
 *  main() only initialises the modules , enables interrupts and runs the super-loop.
 */
#include "../lib/Service/std_types.h"
#include "../lib/MCAL/GIE/GIE.h"
#include "../lib/HAL/RELAY/RELAY.h"
#include "../lib/HAL/LED/LED.h"
#include "../lib/HAL/BUZZER/BUZZER.h"
#include "../lib/HAL/FAN/FAN.h"
#include "../lib/HAL/SERVO/SERVO.h"
#include "../lib/HAL/DIMMER/DIMMER.h"
#include "../lib/HAL/LAMP/LAMP.h"
#include "../lib/HAL/SEVEN_SEG/SEVEN_SEG.h"
#include "../lib/HAL/BUTTON/BUTTON.h"
#include "../lib/HAL/LM35/LM35.h"
#include "../lib/HAL/KPAD/KPAD.h"
#include "../lib/HAL/LCD_BUF/LCD_BUF.h"
#include "../lib/Service/SCHED/SCHED.h"
#include "../lib/Service/TERM/TERM.h"
#include "../lib/Service/EVQ/EVQ.h"
#include "../lib/Service/ESTORE/ESTORE.h"
#include "../lib/Service/USERDB/USERDB.h"
#include "LIGHT/LIGHT.h"
#include "DOOR/DOOR.h"
#include "CLIMATE/CLIMATE.h"
#include "HEATER/HEATER.h"
#include "ALARM/ALARM.h"
#include "SEC/SEC.h"
#include "UILOC/UILOC.h"
#include "UIREM/UIREM.h"

int main() {

	/*1. outputs to their safe state first : both heater elements off , buzzer off , fan off (REQ-HTR-06 , REQ-ALM-01) */
	RELAY_voidInit();
	LED_voidInit();
	BUZZER_voidInit();
	FAN_voidInit();
	SERVO_voidInit();
	DIMMER_voidInit();

	/*2. terminal (REQ-RUI-01) , I2C parts (LCD , lamps , 7-segment) , inputs (keypad , buttons , LM35) */
	TERM_voidInit();
	LCD_BUF_voidInit();
	LAMP_voidInit();
	SEVEN_SEG_voidInit();
	KPAD_voidInit();
	BUTTON_voidInit();
	LM35_voidInit();

	/*3. storage : EEPROM image or first-boot defaults (REQ-EEP-01 , REQ-SEC-09) */
	ESTORE_voidInit();
	USERDB_voidInit();
	EVQ_voidInit();

	/*4. APP modules . No APP init posts an EVQ event and none calls SCHED */
	ALARM_voidInit();
	SEC_voidInit();
	LIGHT_voidInit();
	DOOR_voidInit();
	CLIMATE_voidInit();
	HEATER_voidInit();
	UILOC_voidInit();
	UIREM_voidInit();

	/*5. 1 ms tick , then interrupts on (UART and the tick need them) */
	SCHED_voidInit();
	GIE_voidEnableGlobalInterrupt();
	SCHED_voidStart();

	/*6. super-loop (architecture 4.2) : every task runs from its SCHED flag , nothing blocks */
	while(1){
		/* 5 ms : LCD one byte at a time , terminal line editor and output (REQ-LUI-02 , REQ-RUI-01) */
		if(SCHED_u8IsTaskDue(SCHED_TASK_5MS)){
			LCD_BUF_voidUpdate();
			UIREM_voidTask5ms();
		}
		/* 10 ms : keypad and button debounce , heater buttons , keys , EEPROM write-behind (REQ-HTR-01 , REQ-EEP-03) */
		if(SCHED_u8IsTaskDue(SCHED_TASK_10MS)){
			KPAD_voidUpdate();
			BUTTON_voidUpdate();
			HEATER_voidTask10ms();
			UILOC_voidTask10ms();
			ESTORE_voidUpdate();
		}
		/* 100 ms : water and room temperature , heater control law , AC hysteresis (REQ-HTR-07 , REQ-AC-02) */
		if(SCHED_u8IsTaskDue(SCHED_TASK_100MS)){
			HEATER_voidTask100ms();
			CLIMATE_voidTask100ms();
		}
		/* 500 ms : heater blink (REQ-HTR-11 , REQ-HTR-13) , alarm buzzer beep (REQ-ALM-01) */
		if(SCHED_u8IsTaskDue(SCHED_TASK_500MS)){
			HEATER_voidTask500ms();
			ALARM_voidTask500ms();
		}
		/* 1 s : idle timeouts and status screen (REQ-LUI-03 , decision 8) */
		if(SCHED_u8IsTaskDue(SCHED_TASK_1S)){
			UILOC_voidTask1s();
			UIREM_voidTask1s();
		}
	}

	return 0;
}
