/*
 * UILOC.c
 *
 *  Created on: Oct 4, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../../../lib/Service/std_types.h"
#include "../../../lib/Service/flash_str.h"
#include "../../../lib/Service/FMT/FMT.h"
#include "../../../lib/Service/ESTORE/ESTORE_cfg.h"
#include "../../../lib/HAL/KPAD/KPAD.h"
#include "../../../lib/HAL/LCD_BUF/LCD_BUF.h"
#include "../../../lib/HAL/LAMP/LAMP.h"
#include "../../../lib/HAL/LAMP/LAMP_cfg.h"
#include "../SEC/SEC.h"
#include "../ALARM/ALARM.h"
#include "../LIGHT/LIGHT.h"
#include "../LIGHT/LIGHT_cfg.h"
#include "../CLIMATE/CLIMATE.h"
#include "../CLIMATE/CLIMATE_cfg.h"
#include "../HEATER/HEATER.h"
#include "../HEATER/HEATER_cfg.h"
#include "UILOC.h"
#include "UILOC_cfg.h"

/* states (architecture.md 5.7). The logged-in screens are the last ones : state >= UILOC_STATE_MENU */
#define UILOC_STATE_STATUS     0
#define UILOC_STATE_ASK_ID     1
#define UILOC_STATE_ASK_PIN    2
#define UILOC_STATE_MESSAGE    3
#define UILOC_STATE_LOCKED     4
#define UILOC_STATE_MENU       5
#define UILOC_STATE_LAMPS      6
#define UILOC_STATE_DIMMER     7
#define UILOC_STATE_AC         8
#define UILOC_STATE_HEATER     9

#define UILOC_ROW_1            0
#define UILOC_ROW_2            1
#define UILOC_TEMP_MAX         99                      /* the screens have two columns for a temperature */

static u8 Global_u8State = UILOC_STATE_STATUS ;
static u8 Global_u8NextState = UILOC_STATE_STATUS ;    /* where a message goes when its time is over */
static u8 Global_u8MessageTimer = 0 ;                  /* x 10 ms */
static u8 Global_u8IdleSeconds = 0 ;
static u8 Global_u8Page = 0 ;                          /* status page 0 / 1 */
static u8 Global_u8PageSeconds = 0 ;
/* typed ID and PIN : digits + '\0' (the same sizes as an account record) */
static c8 Global_c8Id [USERDB_NAME_SIZE + 1] ;
static c8 Global_c8Pin[USERDB_PASS_SIZE + 1] ;
static u8 Global_u8IdLength = 0 ;
static u8 Global_u8PinLength = 0 ;

/*************************** screen helpers ***************************/
/* writes a number , returns the column after it */
static u8 UILOC_u8PutNumber(u8 Copy_u8Row, u8 Copy_u8Col, u8 Copy_u8Number){
	c8 Local_c8Text[FMT_NUMBER_TEXT_SIZE] ;
	u8 Local_u8Length = FMT_u8NumberToText(Copy_u8Number,Local_c8Text);
	LCD_BUF_voidWriteRam(Copy_u8Row,Copy_u8Col,Local_c8Text);
	return Copy_u8Col + Local_u8Length ;
}
/* "29C" : a temperature , limited to two digits */
static void UILOC_voidPutTemp(u8 Copy_u8Row, u8 Copy_u8Col, u8 Copy_u8Temp){
	if(Copy_u8Temp > UILOC_TEMP_MAX){
		Copy_u8Temp = UILOC_TEMP_MAX ;
	}
	Copy_u8Col = UILOC_u8PutNumber(Copy_u8Row,Copy_u8Col,Copy_u8Temp);
	LCD_BUF_voidWriteChar(Copy_u8Row,Copy_u8Col,'C');
}
static void UILOC_voidPutRoomTemp(u8 Copy_u8Row, u8 Copy_u8Col){
	if(CLIMATE_u8IsTempValid() == 1){
		UILOC_voidPutTemp(Copy_u8Row,Copy_u8Col,CLIMATE_u8GetRoomTemp());
	}else{
		LCD_BUF_voidWriteFlash(Copy_u8Row,Copy_u8Col,FLASH_STR("--C"));    /* first second after boot */
	}
}

/* REQ-LUI-03 : status page 1 = lamps , dimmer , AC , room temperature */
static void UILOC_voidDrawStatus1(){
	u8 Local_u8Lamp ;
	u8 Local_u8Dimmer = LIGHT_u8GetDimmer();
	u8 Local_u8Col ;
	/* "Lamps:1-3-- D70%" : the number of a lamp that is on , '-' for one that is off */
	LCD_BUF_voidWriteFlash(UILOC_ROW_1,0,FLASH_STR("Lamps:"));
	for(Local_u8Lamp = 1 ; Local_u8Lamp <= LAMP_COUNT ; Local_u8Lamp++){
		LCD_BUF_voidWriteChar(UILOC_ROW_1,5 + Local_u8Lamp,(LIGHT_u8GetLamp(Local_u8Lamp) == LAMP_ON) ? ('0' + Local_u8Lamp) : '-');
	}
	Local_u8Col = (Local_u8Dimmer >= 100) ? 11 : 12 ;  /* "D100%" needs one column more */
	LCD_BUF_voidWriteChar(UILOC_ROW_1,Local_u8Col,'D');
	Local_u8Col = UILOC_u8PutNumber(UILOC_ROW_1,Local_u8Col + 1,Local_u8Dimmer);
	LCD_BUF_voidWriteChar(UILOC_ROW_1,Local_u8Col,'%');
	/* "AC:ON   Room:29C" */
	LCD_BUF_voidWriteFlash(UILOC_ROW_2,0,(CLIMATE_u8IsAcOn() == 1) ? FLASH_STR("AC:ON") : FLASH_STR("AC:OFF"));
	LCD_BUF_voidWriteFlash(UILOC_ROW_2,8,FLASH_STR("Room:"));
	UILOC_voidPutRoomTemp(UILOC_ROW_2,13);
}
/* status page 2 = heater state , water and set temperature */
static void UILOC_voidDrawStatus2(){
	/* "Heater:HEATING" */
	LCD_BUF_voidWriteFlash(UILOC_ROW_1,0,FLASH_STR("Heater:"));
	if(HEATER_u8IsOn() == 0){
		LCD_BUF_voidWriteFlash(UILOC_ROW_1,7,FLASH_STR("OFF"));
	}else{
		switch(HEATER_u8GetElement()){
		case HEATER_ELEMENT_HEATING: LCD_BUF_voidWriteFlash(UILOC_ROW_1,7,FLASH_STR("HEATING")); break ;
		case HEATER_ELEMENT_COOLING: LCD_BUF_voidWriteFlash(UILOC_ROW_1,7,FLASH_STR("COOLING")); break ;
		default:                     LCD_BUF_voidWriteFlash(UILOC_ROW_1,7,FLASH_STR("IDLE"));    break ;
		}
	}
	/* "Water:55C Set:60" */
	LCD_BUF_voidWriteFlash(UILOC_ROW_2,0,FLASH_STR("Water:"));
	UILOC_voidPutTemp(UILOC_ROW_2,6,HEATER_u8GetWaterTemp());
	LCD_BUF_voidWriteFlash(UILOC_ROW_2,10,FLASH_STR("Set:"));
	UILOC_u8PutNumber(UILOC_ROW_2,14,HEATER_u8GetSetTemp());
}
/* "User ID:1111" , and in the PIN step "PIN:****" (REQ-LUI-04 : a PIN digit is never shown) */
static void UILOC_voidDrawLogin(){
	u8 i ;
	Global_c8Id[Global_u8IdLength] = '\0' ;
	LCD_BUF_voidWriteFlash(UILOC_ROW_1,0,FLASH_STR("User ID:"));
	LCD_BUF_voidWriteRam(UILOC_ROW_1,8,Global_c8Id);
	if(Global_u8State == UILOC_STATE_ASK_PIN){
		LCD_BUF_voidWriteFlash(UILOC_ROW_2,0,FLASH_STR("PIN:"));
		for(i = 0 ; i < Global_u8PinLength ; i++){
			LCD_BUF_voidWriteChar(UILOC_ROW_2,4 + i,'*');
		}
	}
}
static void UILOC_voidDrawLamps(){
	u8 Local_u8Lamp ;
	/* the mark of a lamp is under its number : columns 5 , 7 , 9 , 11 , 13 */
	LCD_BUF_voidWriteFlash(UILOC_ROW_1,0,FLASH_STR("Lamp 1 2 3 4 5"));
	for(Local_u8Lamp = 1 ; Local_u8Lamp <= LAMP_COUNT ; Local_u8Lamp++){
		LCD_BUF_voidWriteChar(UILOC_ROW_2,3 + (2 * Local_u8Lamp),(LIGHT_u8GetLamp(Local_u8Lamp) == LAMP_ON) ? '*' : '-');
	}
}
static void UILOC_voidDrawDimmer(){
	u8 Local_u8Dimmer = LIGHT_u8GetDimmer();
	u8 Local_u8Col = 8 ;                               /* "Dimmer:  70%" : the number ends in column 10 */
	if(Local_u8Dimmer < 100){
		Local_u8Col++;
	}
	if(Local_u8Dimmer < 10){
		Local_u8Col++;
	}
	LCD_BUF_voidWriteFlash(UILOC_ROW_1,0,FLASH_STR("Dimmer:"));
	UILOC_u8PutNumber(UILOC_ROW_1,Local_u8Col,Local_u8Dimmer);
	LCD_BUF_voidWriteChar(UILOC_ROW_1,11,'%');
	LCD_BUF_voidWriteFlash(UILOC_ROW_2,0,FLASH_STR("+ / -     C=Back"));
}
static void UILOC_voidDrawAc(){
	u8 Local_u8Col ;
	/* "AC:ON  Room:29C" */
	LCD_BUF_voidWriteFlash(UILOC_ROW_1,0,(CLIMATE_u8IsAcOn() == 1) ? FLASH_STR("AC:ON") : FLASH_STR("AC:OFF"));
	LCD_BUF_voidWriteFlash(UILOC_ROW_1,7,FLASH_STR("Room:"));
	UILOC_voidPutRoomTemp(UILOC_ROW_1,12);
	/* "on>28 off<21" (REQ-AC-02 limits) */
	LCD_BUF_voidWriteFlash(UILOC_ROW_2,0,FLASH_STR("on>"));
	Local_u8Col = UILOC_u8PutNumber(UILOC_ROW_2,3,CLIMATE_AC_ON_ABOVE_C);
	LCD_BUF_voidWriteFlash(UILOC_ROW_2,Local_u8Col + 1,FLASH_STR("off<"));
	UILOC_u8PutNumber(UILOC_ROW_2,Local_u8Col + 5,CLIMATE_AC_OFF_BELOW_C);
}
static void UILOC_voidDrawHeater(){
	u8 Local_u8Col ;
	/* "Water:55C HEAT" */
	LCD_BUF_voidWriteFlash(UILOC_ROW_1,0,FLASH_STR("Water:"));
	UILOC_voidPutTemp(UILOC_ROW_1,6,HEATER_u8GetWaterTemp());
	if(HEATER_u8IsOn() == 0){
		LCD_BUF_voidWriteFlash(UILOC_ROW_1,10,FLASH_STR("OFF"));
	}else{
		switch(HEATER_u8GetElement()){
		case HEATER_ELEMENT_HEATING: LCD_BUF_voidWriteFlash(UILOC_ROW_1,10,FLASH_STR("HEAT")); break ;
		case HEATER_ELEMENT_COOLING: LCD_BUF_voidWriteFlash(UILOC_ROW_1,10,FLASH_STR("COOL")); break ;
		default:                     LCD_BUF_voidWriteFlash(UILOC_ROW_1,10,FLASH_STR("IDLE")); break ;
		}
	}
	/* "Set:60C  + / -" */
	LCD_BUF_voidWriteFlash(UILOC_ROW_2,0,FLASH_STR("Set:"));
	Local_u8Col = UILOC_u8PutNumber(UILOC_ROW_2,4,HEATER_u8GetSetTemp());
	LCD_BUF_voidWriteChar(UILOC_ROW_2,Local_u8Col,'C');
	LCD_BUF_voidWriteFlash(UILOC_ROW_2,9,FLASH_STR("+ / -"));
}

/* Writes the whole screen of the current state again. RAM only : LCD_BUF sends just the
 * characters that changed , one per 5 ms , so a redraw never blocks and never flickers. */
static void UILOC_voidDraw(){
	LCD_BUF_voidClear();
	switch(Global_u8State){
	case UILOC_STATE_STATUS:
		if(Global_u8Page == 0){
			UILOC_voidDrawStatus1();
		}else{
			UILOC_voidDrawStatus2();
		}
		break ;
	case UILOC_STATE_ASK_ID:
	case UILOC_STATE_ASK_PIN:
		UILOC_voidDrawLogin();
		break ;
	case UILOC_STATE_MENU:
		/* REQ-LUI-02 ; no door entry : REQ-SEC-06 */
		LCD_BUF_voidWriteFlash(UILOC_ROW_1,0,FLASH_STR("1Lamps  2Dimmer"));
		LCD_BUF_voidWriteFlash(UILOC_ROW_2,0,FLASH_STR("3AC 4Heat C=Exit"));
		break ;
	case UILOC_STATE_LAMPS:  UILOC_voidDrawLamps();  break ;
	case UILOC_STATE_DIMMER: UILOC_voidDrawDimmer(); break ;
	case UILOC_STATE_AC:     UILOC_voidDrawAc();     break ;
	case UILOC_STATE_HEATER: UILOC_voidDrawHeater(); break ;
	case UILOC_STATE_LOCKED:
		/* REQ-SEC-05 (Section 12 #2) */
		LCD_BUF_voidWriteFlash(UILOC_ROW_1,0,FLASH_STR("SYSTEM LOCKED"));
		LCD_BUF_voidWriteFlash(UILOC_ROW_2,0,FLASH_STR("Reset required"));
		break ;
	default:
		break ;
	}
}
static void UILOC_voidEnter(u8 Copy_u8State){
	Global_u8State = Copy_u8State ;
	if(Copy_u8State == UILOC_STATE_STATUS){
		Global_u8Page = 0 ;
		Global_u8PageSeconds = 0 ;
	}
	UILOC_voidDraw();
}
/* two lines for UILOC_MESSAGE_TIME , then the screen of Copy_u8NextState */
static void UILOC_voidShowMessage(const __flash c8 * Copy_pc8Line1, const __flash c8 * Copy_pc8Line2, u8 Copy_u8NextState){
	LCD_BUF_voidClear();
	LCD_BUF_voidWriteFlash(UILOC_ROW_1,0,Copy_pc8Line1);
	LCD_BUF_voidWriteFlash(UILOC_ROW_2,0,Copy_pc8Line2);
	Global_u8State = UILOC_STATE_MESSAGE ;
	Global_u8NextState = Copy_u8NextState ;
	Global_u8MessageTimer = UILOC_MESSAGE_TIME ;
}
static void UILOC_voidShowBlocked(){
	/* REQ-SEC-08 */
	UILOC_voidShowMessage(FLASH_STR("Keypad blocked"),FLASH_STR("by admin"),UILOC_STATE_STATUS);
}

/**************************** key handlers ****************************/
static u8 UILOC_u8IsDigit(u8 Copy_u8Key){
	return ((Copy_u8Key >= '0') && (Copy_u8Key <= '9')) ? 1 : 0 ;
}
static u8 UILOC_u8IsBack(u8 Copy_u8Key){
	return ((Copy_u8Key == UILOC_KEY_BACK1) || (Copy_u8Key == UILOC_KEY_BACK2)) ? 1 : 0 ;
}
static void UILOC_voidKeyStatus(u8 Copy_u8Key){
	if(SEC_u8IsLocalAllowed() == 1){
		Global_u8IdLength = 0 ;
		Global_u8PinLength = 0 ;
		if(UILOC_u8IsDigit(Copy_u8Key) == 1){
			Global_c8Id[Global_u8IdLength++] = (c8)Copy_u8Key ;     /* the key that woke the screen is the first digit */
		}
		UILOC_voidEnter(UILOC_STATE_ASK_ID);
	}else{
		UILOC_voidShowBlocked();
	}
}
static void UILOC_voidKeyAskId(u8 Copy_u8Key){
	if(UILOC_u8IsDigit(Copy_u8Key) == 1){
		if(Global_u8IdLength < USERDB_NAME_SIZE){
			Global_c8Id[Global_u8IdLength++] = (c8)Copy_u8Key ;
		}
	}else if(UILOC_u8IsBack(Copy_u8Key) == 1){
		if(Global_u8IdLength > 0){
			Global_u8IdLength--;
		}else{
			Global_u8State = UILOC_STATE_STATUS ;      /* nothing typed : leave the screen */
			Global_u8Page = 0 ;
			Global_u8PageSeconds = 0 ;
		}
	}else if((Copy_u8Key == UILOC_KEY_ENTER) && (Global_u8IdLength > 0)){
		Global_u8PinLength = 0 ;
		Global_u8State = UILOC_STATE_ASK_PIN ;
	}else{
	}
	UILOC_voidDraw();
}
static void UILOC_voidKeyAskPin(u8 Copy_u8Key){
	u8 Local_u8Result ;
	if(UILOC_u8IsDigit(Copy_u8Key) == 1){
		if(Global_u8PinLength < USERDB_PASS_SIZE){
			Global_c8Pin[Global_u8PinLength++] = (c8)Copy_u8Key ;
		}
	}else if(UILOC_u8IsBack(Copy_u8Key) == 1){
		if(Global_u8PinLength > 0){
			Global_u8PinLength--;
		}else{
			Global_u8State = UILOC_STATE_ASK_ID ;      /* back to the ID , which can be edited */
		}
	}else if((Copy_u8Key == UILOC_KEY_ENTER) && (Global_u8PinLength > 0)){
		/* ASSUMPTION: '=' with no PIN digit is ignored , it is not a login attempt */
		Global_c8Id [Global_u8IdLength]  = '\0' ;
		Global_c8Pin[Global_u8PinLength] = '\0' ;
		/* REQ-SEC-01 , REQ-SEC-02 : SEC checks only the keypad list for this source */
		Local_u8Result = SEC_u8Login(SEC_SOURCE_LOCAL,Global_c8Id,Global_c8Pin);
		Global_u8IdLength = 0 ;
		Global_u8PinLength = 0 ;
		if(Local_u8Result == SEC_LOGIN_USER){
			Global_u8State = UILOC_STATE_MENU ;
		}else if(Local_u8Result == SEC_LOGIN_FAILED){
			/* "2 tries left" : the number is put in front of the text */
			UILOC_voidShowMessage(FLASH_STR("Wrong ID or PIN"),FLASH_STR("  tries left"),UILOC_STATE_ASK_ID);
			LCD_BUF_voidWriteChar(UILOC_ROW_2,0,'0' + SEC_u8GetAttemptsLeft(SEC_SOURCE_LOCAL));
			return ;
		}else{
			return ;                                   /* locked (REQ-SEC-05) : the next task call shows it */
		}
	}else{
	}
	UILOC_voidDraw();
}
static void UILOC_voidKeyMenu(u8 Copy_u8Key){
	switch(Copy_u8Key){
	case '1': Global_u8State = UILOC_STATE_LAMPS ;  break ;
	case '2': Global_u8State = UILOC_STATE_DIMMER ; break ;
	case '3': Global_u8State = UILOC_STATE_AC ;     break ;
	case '4': Global_u8State = UILOC_STATE_HEATER ; break ;
	default:
		if(UILOC_u8IsBack(Copy_u8Key) == 1){
			SEC_voidLogout(SEC_SOURCE_LOCAL);
			Global_u8State = UILOC_STATE_STATUS ;
			Global_u8Page = 0 ;
			Global_u8PageSeconds = 0 ;
		}
		break ;
	}
	UILOC_voidDraw();
}
/* the four feature screens ; "back" returns to the menu from each of them */
static void UILOC_voidKeyFeature(u8 Copy_u8Key){
	u8 Local_u8Value ;
	if(UILOC_u8IsBack(Copy_u8Key) == 1){
		Global_u8State = UILOC_STATE_MENU ;
	}else if(Global_u8State == UILOC_STATE_LAMPS){
		/* REQ-LGT-01 */
		if((Copy_u8Key >= '1') && (Copy_u8Key < ('1' + LAMP_COUNT))){
			LIGHT_u8ToggleLamp(Copy_u8Key - '0');
		}
	}else if(Global_u8State == UILOC_STATE_DIMMER){
		/* REQ-LGT-02 */
		Local_u8Value = LIGHT_u8GetDimmer();
		if((Copy_u8Key == UILOC_KEY_UP) && (Local_u8Value < LIGHT_DIMMER_MAX)){
			LIGHT_voidSetDimmer(Local_u8Value + LIGHT_DIMMER_STEP);
		}else if((Copy_u8Key == UILOC_KEY_DOWN) && (Local_u8Value >= LIGHT_DIMMER_STEP)){
			LIGHT_voidSetDimmer(Local_u8Value - LIGHT_DIMMER_STEP);
		}else{
		}
	}else if(Global_u8State == UILOC_STATE_HEATER){
		/* REQ-HTR-14 (D-9) : HEATER rejects a value outside 35..75 , so the ends need no check here */
		Local_u8Value = HEATER_u8GetSetTemp();
		if(Copy_u8Key == UILOC_KEY_UP){
			HEATER_u8SetSetTemp(Local_u8Value + HEATER_SET_STEP);
		}else if(Copy_u8Key == UILOC_KEY_DOWN){
			HEATER_u8SetSetTemp(Local_u8Value - HEATER_SET_STEP);
		}else{
		}
	}else{
		/* AC screen : view only */
	}
	UILOC_voidDraw();
}

/****************************** public ******************************/
void UILOC_voidInit(){
	Global_u8IdLength = 0 ;
	Global_u8PinLength = 0 ;
	Global_u8IdleSeconds = 0 ;
	Global_u8MessageTimer = 0 ;
	UILOC_voidEnter(UILOC_STATE_STATUS);
}

void UILOC_voidTask10ms(){
	/* the key is always taken , so a key pressed while it means nothing is not kept for later */
	u8 Local_u8Key = KPAD_u8GetKey();

	/*1. lockdown : keys ignored until reset (REQ-SEC-05) */
	if(Global_u8State == UILOC_STATE_LOCKED){
		return ;
	}
	if(ALARM_u8IsActive() == 1){
		UILOC_voidEnter(UILOC_STATE_LOCKED);
		return ;
	}
	/*2. a message stays for its time */
	if(Global_u8State == UILOC_STATE_MESSAGE){
		Global_u8MessageTimer--;
		if(Global_u8MessageTimer == 0){
			UILOC_voidEnter(Global_u8NextState);
		}
		return ;
	}
	/*3. REQ-SEC-08 : the admin logged in or blocked the keypad , SEC already ended this session */
	if((Global_u8State >= UILOC_STATE_MENU) && (SEC_u8GetRole(SEC_SOURCE_LOCAL) == SEC_ROLE_NONE)){
		UILOC_voidEnter(UILOC_STATE_STATUS);
		return ;
	}
	/* the same while an ID or PIN is being typed : without this the login would be refused
	 * by SEC and counted as a wrong attempt */
	if(((Global_u8State == UILOC_STATE_ASK_ID) || (Global_u8State == UILOC_STATE_ASK_PIN)) && (SEC_u8IsLocalAllowed() == 0)){
		UILOC_voidShowBlocked();
		return ;
	}
	/*4. keys */
	if(Local_u8Key == KPAD_NO_KEY){
		return ;
	}
	Global_u8IdleSeconds = 0 ;
	switch(Global_u8State){
	case UILOC_STATE_STATUS:  UILOC_voidKeyStatus(Local_u8Key);  break ;
	case UILOC_STATE_ASK_ID:  UILOC_voidKeyAskId(Local_u8Key);   break ;
	case UILOC_STATE_ASK_PIN: UILOC_voidKeyAskPin(Local_u8Key);  break ;
	case UILOC_STATE_MENU:    UILOC_voidKeyMenu(Local_u8Key);    break ;
	default:                  UILOC_voidKeyFeature(Local_u8Key); break ;
	}
}

void UILOC_voidTask1s(){
	if((Global_u8State == UILOC_STATE_LOCKED) || (Global_u8State == UILOC_STATE_MESSAGE)){
		return ;
	}
	if(Global_u8State == UILOC_STATE_STATUS){
		/* REQ-LUI-03 : fresh values every second , the other page every UILOC_STATUS_PAGE_S */
		Global_u8PageSeconds++;
		if(Global_u8PageSeconds >= UILOC_STATUS_PAGE_S){
			Global_u8PageSeconds = 0 ;
			Global_u8Page ^= 1 ;
		}
		UILOC_voidDraw();
		return ;
	}
	/* ASSUMPTION: the idle time also ends a login that was started and left (ID / PIN screens) ,
	 * not only a session (Section 12 #8) */
	Global_u8IdleSeconds++;
	if(Global_u8IdleSeconds >= UILOC_IDLE_TIMEOUT_S){
		Global_u8IdleSeconds = 0 ;
		if(SEC_u8GetRole(SEC_SOURCE_LOCAL) != SEC_ROLE_NONE){
			SEC_voidLogout(SEC_SOURCE_LOCAL);
		}
		UILOC_voidEnter(UILOC_STATE_STATUS);
	}else{
		UILOC_voidDraw();                              /* the values on the screen may have changed (AC , heater , remote commands) */
	}
}
