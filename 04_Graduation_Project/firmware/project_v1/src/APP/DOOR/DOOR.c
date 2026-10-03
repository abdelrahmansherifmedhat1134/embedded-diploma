/*
 * DOOR.c
 *
 *  Created on: Oct 3, 2026
 *      Author: eslam
 */
#include "../../../lib/Service/std_types.h"
#include "../../../lib/HAL/SERVO/SERVO.h"
#include "DOOR.h"
#include "DOOR_cfg.h"

static u8 Global_u8State = DOOR_CLOSED ;

void DOOR_voidInit(){
	/* ASSUMPTION: the door state is not stored (D-14) : boot = closed */
	SERVO_voidSetAngle(DOOR_CLOSED_ANGLE);
	Global_u8State = DOOR_CLOSED ;
}

/* The door is only moved from the terminal , so it posts no event (architecture.md 5.4) */
u8 DOOR_u8SetState(u8 Copy_u8State){
	u8 Local_u8Moved = 0 ;
	if((Copy_u8State == DOOR_OPEN) && (Global_u8State == DOOR_CLOSED)){
		/* REQ-DOR-01 , REQ-DOR-02 */
		SERVO_voidSetAngle(DOOR_OPEN_ANGLE);
		Global_u8State = DOOR_OPEN ;
		Local_u8Moved = 1 ;
	}else if((Copy_u8State == DOOR_CLOSED) && (Global_u8State == DOOR_OPEN)){
		/* REQ-DOR-01 , REQ-DOR-02 */
		SERVO_voidSetAngle(DOOR_CLOSED_ANGLE);
		Global_u8State = DOOR_CLOSED ;
		Local_u8Moved = 1 ;
	}else{
		/* already there (or a wrong argument) : nothing moves */
	}
	return Local_u8Moved ;
}

u8 DOOR_u8GetState(){
	return Global_u8State ;
}
