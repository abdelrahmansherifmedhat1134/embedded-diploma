/*
 * test_app.c
 *
 *  Created on: Oct 3, 2026
 *
 * =====================================================================
 *  Phase 2 test of the APP layer , PART A
 * =====================================================================
 *  Tested : LIGHT , DOOR , CLIMATE , HEATER , ALARM , SEC
 *           (UILOC and UIREM come in part B)
 *
 *  Build : pio run -e test_app
 *  Hex   : .pio/build/test_app/firmware.hex
 *
 *  All output goes through TERM (TX ring + interrupt).
 *  There is no status LED in this test : PA3 is the heater LED and belongs
 *  to HEATER. The result is the SUMMARY line on the terminal.
 *
 * ---------------------- Proteus parts and wiring ----------------------
 *  (docs/pin_map.md ; the LCD and the keypad may stay , they are not used)
 *  ATMEGA32          Clock Frequency = 16 MHz (must equal F_CPU)
 *  VIRTUAL TERMINAL  9600 baud , 8N1 : terminal RXD <- PD1 , terminal TXD -> PD0
 *  I2C bus           PC0 = SCL , PC1 = SDA , 4.7k pull-up to VCC on both lines
 *  24C08 EEPROM      address 0x50 , WP = GND
 *  PCF8574 0x20      lamps 1..5 on P0..P4 : +5 V -> 220R -> LED -> pin (active low)
 *  PCF8574 0x21/0x22 7-segment tens / units digit , common anode , P0..P6 = a..g
 *  LM35 ambient      PA0            LM35 water       PA1
 *  Heater LED        PA3 -> 330R -> LED -> GND
 *  Buttons to GND    ON/OFF = PD6 , Up = PD7 , Down = PB5
 *  Heating element   PB6 (red LED)   Cooling element  PB7 (blue LED)
 *  Dimmer            PB3 (OC0) , scope channel A
 *  AC fan            PD4 (OC1B) -> NPN driver -> DC motor
 *  Door servo        PD5 (OC1A) , Min/Max Angle = 0 / 180
 *  Buzzer            PD3 (Operating Voltage 3 V , Load Resistance 150 ohm)
 *
 * --------------------------- Expected result ---------------------------
 *  1. Automatic part (about 2 s , do not hold a button) : one line per check
 *     "[PASS] <module>: <what>" or "[FAIL] ..." , then "SUMMARY".
 *     The test presses the three heater buttons itself (it pulls their pins
 *     low for a moment) and feeds temperatures straight into CLIMATE and
 *     HEATER , so lamps , relays , servo , fan and buzzer move for a moment.
 *     Two test accounts are added and removed again ; the stored set
 *     temperature is put back.
 *  2. Live part : the super-loop of architecture.md 4.2 with the part A modules.
 *     Every event is printed as "[INFO] <event> <arg>" , and every 5 s a line
 *     "[STAT] room .. ac .. water .. set .. on .. el .." (el : 1 = heating ,
 *     2 = cooling). Follow docs/test_plan.md 3.6 and 3.7 :
 *       heater panel : ON/OFF , Up , Down buttons , 7-segment , heater LED , relays
 *       AC           : change the ambient LM35 , watch the fan and "[INFO] AC"
 *     Typed lines (Enter at the end) stand in for the UIs of part B :
 *       1..5 = toggle a lamp     + / - = dimmer 10 % up / down
 *       o / c = open / close the door      35..75 = heater set temperature
 *       a = lockdown (ALARM , test_plan 3.8) : only RESET ends it
 * =====================================================================
 */
#include "../lib/Service/std_types.h"
#include "../lib/Service/flash_str.h"
#include "../lib/MCAL/DIO/DIO.h"
#include "../lib/MCAL/GIE/GIE.h"
#include "../lib/HAL/RELAY/RELAY.h"
#include "../lib/HAL/RELAY/RELAY_cfg.h"
#include "../lib/HAL/LED/LED.h"
#include "../lib/HAL/BUZZER/BUZZER.h"
#include "../lib/HAL/BUZZER/BUZZER_cfg.h"
#include "../lib/HAL/FAN/FAN.h"
#include "../lib/HAL/SERVO/SERVO.h"
#include "../lib/HAL/DIMMER/DIMMER.h"
#include "../lib/HAL/LAMP/LAMP.h"
#include "../lib/HAL/LAMP/LAMP_cfg.h"
#include "../lib/HAL/SEVEN_SEG/SEVEN_SEG.h"
#include "../lib/HAL/BUTTON/BUTTON.h"
#include "../lib/HAL/BUTTON/BUTTON_cfg.h"
#include "../lib/HAL/LM35/LM35.h"
#include "../lib/Service/FMT/FMT.h"
#include "../lib/Service/SCHED/SCHED.h"
#include "../lib/Service/TERM/TERM.h"
#include "../lib/Service/TERM/TERM_cfg.h"
#include "../lib/Service/EVQ/EVQ.h"
#include "../lib/Service/ESTORE/ESTORE.h"
#include "../lib/Service/ESTORE/ESTORE_cfg.h"
#include "../lib/Service/USERDB/USERDB.h"
#include "../src/APP/LIGHT/LIGHT.h"
#include "../src/APP/LIGHT/LIGHT_cfg.h"
#include "../src/APP/DOOR/DOOR.h"
#include "../src/APP/CLIMATE/CLIMATE.h"
#include "../src/APP/HEATER/HEATER.h"
#include "../src/APP/HEATER/HEATER_cfg.h"
#include "../src/APP/ALARM/ALARM.h"
#include "../src/APP/SEC/SEC.h"

/******************************* settings *******************************/
#define TEST_DEBOUNCE_CALLS       (BUTTON_DEBOUNCE_SAMPLES + 2)    /* BUTTON_voidUpdate calls per simulated edge */
#define TEST_WINDOW               10             /* samples that fill the average (MAVG_WINDOW) */
#define TEST_STATUS_PERIOD_S      5
#define TEST_EVENT_ROOM           32             /* free TX bytes needed for one event line */
#define TEST_STATUS_ROOM          64             /* free TX bytes needed for the status line */

static u8 Global_u8PassCount = 0 ;
static u8 Global_u8FailCount = 0 ;
static u8 Global_u8StatusCount = 0 ;
/* RAM strings for SEC / USERDB : the real admin record is read from ESTORE */
static c8 Global_c8AdminName[USERDB_NAME_SIZE + 1] ;
static c8 Global_c8AdminPass[USERDB_PASS_SIZE + 1] ;
static c8 Global_c8UserName[] = "zt9" ;          /* test remote user */
static c8 Global_c8KeyName[]  = "97" ;           /* test keypad user */
static c8 Global_c8UserPass[] = "9731" ;
static c8 Global_c8Wrong[]    = "x" ;            /* never a valid name or password */

/*************************** print helpers ***************************/
/* TERM drops what does not fit : in the automatic part wait for room first */
static void TEST_voidPrint(const __flash c8 * Copy_pc8Text){
	while(*Copy_pc8Text != '\0'){
		while(TERM_u8TxFree() == 0){
		}
		TERM_voidPutChar(*Copy_pc8Text++);
	}
}
static void TEST_voidPrintLine(const __flash c8 * Copy_pc8Text){
	TEST_voidPrint(Copy_pc8Text);
	TEST_voidPrint(FLASH_STR("\r\n"));
}
static void TEST_voidPrintNum(u16 Copy_u16Num){
	while(TERM_u8TxFree() < FMT_NUMBER_TEXT_SIZE){
	}
	TERM_voidPutNumber(Copy_u16Num);
}
/* one "[PASS] module: what" or "[FAIL] module: what" line */
static void TEST_voidCheck(u8 Copy_u8Ok, const __flash c8 * Copy_pc8Module, const __flash c8 * Copy_pc8What){
	if(Copy_u8Ok){
		TEST_voidPrint(FLASH_STR("[PASS] "));
		Global_u8PassCount++;
	}else{
		TEST_voidPrint(FLASH_STR("[FAIL] "));
		Global_u8FailCount++;
	}
	TEST_voidPrint(Copy_pc8Module);
	TEST_voidPrint(FLASH_STR(": "));
	TEST_voidPrintLine(Copy_pc8What);
}

/*************************** test helpers ***************************/
/* APP inits in boot order (architecture.md 4.6 step 4) = what an MCU reset does to the APP layer */
static void TEST_voidDropEvents(){
	u8 Local_u8Event ;
	u8 Local_u8Arg ;
	while(EVQ_u8Get(&Local_u8Event,&Local_u8Arg) == 1){
	}
}
static void TEST_voidAppInit(){
	ALARM_voidInit();
	SEC_voidInit();
	LIGHT_voidInit();
	DOOR_voidInit();
	CLIMATE_voidInit();
	HEATER_voidInit();
	TEST_voidDropEvents();
}
/* 1 = the oldest event in the queue is exactly this one (it is taken out) */
static u8 TEST_u8NextEvent(u8 Copy_u8Event, u8 Copy_u8Arg){
	u8 Local_u8Event = 0 ;
	u8 Local_u8Arg = 0 ;
	if(EVQ_u8Get(&Local_u8Event,&Local_u8Arg) == 0){
		return 0 ;
	}
	return ((Local_u8Event == Copy_u8Event) && (Local_u8Arg == Copy_u8Arg)) ? 1 : 0 ;
}
/* A heater button without a finger : the pin is pulled low as an output (the real button also
 * connects it to GND , so nothing can be damaged) , then given back as an input with pull-up.
 * BUTTON debounces it and HEATER reads the event , exactly as in the super-loop. */
static void TEST_voidButton(u8 Copy_u8Port, u8 Copy_u8Pin, u8 Copy_u8Pressed){
	u8 i ;
	if(Copy_u8Pressed == 1){
		DIO_voidSetPinValue(Copy_u8Port,Copy_u8Pin,DIO_PIN_LOW);
		DIO_voidSetPinDirection(Copy_u8Port,Copy_u8Pin,DIO_PIN_OUTPUT);
	}else{
		DIO_voidSetPinDirection(Copy_u8Port,Copy_u8Pin,DIO_INPUT);
		DIO_voidSetPinValue(Copy_u8Port,Copy_u8Pin,DIO_PIN_HIGH);
	}
	for(i = 0 ; i < TEST_DEBOUNCE_CALLS ; i++){
		BUTTON_voidUpdate();
	}
	HEATER_voidTask10ms();
}
static void TEST_voidClick(u8 Copy_u8Port, u8 Copy_u8Pin, u8 Copy_u8Times){
	while(Copy_u8Times > 0){
		TEST_voidButton(Copy_u8Port,Copy_u8Pin,1);
		TEST_voidButton(Copy_u8Port,Copy_u8Pin,0);
		Copy_u8Times--;
	}
}
/* a full average window of one temperature (whole degrees -> LM35 steps of 0.25 C) */
static void TEST_voidFeedRoom(u8 Copy_u8TempC, u8 Copy_u8Samples){
	while(Copy_u8Samples > 0){
		CLIMATE_voidFeedSample((u16)Copy_u8TempC * 4);
		Copy_u8Samples--;
	}
}
static void TEST_voidFeedWater(u8 Copy_u8TempC, u8 Copy_u8Samples){
	while(Copy_u8Samples > 0){
		HEATER_voidFeedSample((u16)Copy_u8TempC * 4);
		Copy_u8Samples--;
	}
}
/* 1 = the element state and both relay pins agree with Copy_u8Element */
static u8 TEST_u8ElementIs(u8 Copy_u8Element){
	u8 Local_u8Heating = (DIO_u8GetPinValue(RELAY_HEATING_PORT,RELAY_HEATING_PIN) == RELAY_ON_LEVEL) ? 1 : 0 ;
	u8 Local_u8Cooling = (DIO_u8GetPinValue(RELAY_COOLING_PORT,RELAY_COOLING_PIN) == RELAY_ON_LEVEL) ? 1 : 0 ;
	if(HEATER_u8GetElement() != Copy_u8Element){
		return 0 ;
	}
	if(Local_u8Heating != ((Copy_u8Element == HEATER_ELEMENT_HEATING) ? 1 : 0)){
		return 0 ;
	}
	if(Local_u8Cooling != ((Copy_u8Element == HEATER_ELEMENT_COOLING) ? 1 : 0)){
		return 0 ;
	}
	return 1 ;
}
static u8 TEST_u8BuzzerIsOn(){
	return (DIO_u8GetPinValue(BUZZER_PORT,BUZZER_PIN) == BUZZER_ON_LEVEL) ? 1 : 0 ;
}

/******************************* LIGHT *******************************/
static void TEST_voidLight(){
	u8 Local_u8Ok ;
	TEST_voidDropEvents();
	/* REQ-LGT-02 */
	LIGHT_voidSetDimmer(57);
	TEST_voidCheck((LIGHT_u8GetDimmer() == 50) && TEST_u8NextEvent(EVQ_DIMMER,50),FLASH_STR("LIGHT"),FLASH_STR("dimmer 57 -> 50 + event"));
	LIGHT_voidSetDimmer(200);
	TEST_voidCheck(LIGHT_u8GetDimmer() == 100,FLASH_STR("LIGHT"),FLASH_STR("dimmer 200 -> 100"));
	LIGHT_voidSetDimmer(9);
	TEST_voidCheck(LIGHT_u8GetDimmer() == 0,FLASH_STR("LIGHT"),FLASH_STR("dimmer 9 -> 0"));
	TEST_voidDropEvents();
	/* REQ-LGT-01 */
	Local_u8Ok = (LIGHT_u8ToggleLamp(3) == LAMP_ON) && (LIGHT_u8GetLamp(3) == LAMP_ON) && (LIGHT_u8GetLamp(2) == LAMP_OFF) && TEST_u8NextEvent(EVQ_LAMP,3);
	Local_u8Ok = Local_u8Ok && (LIGHT_u8ToggleLamp(3) == LAMP_OFF) && (LIGHT_u8GetLamp(3) == LAMP_OFF);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("LIGHT"),FLASH_STR("lamp 3 on , off + event"));
	TEST_voidDropEvents();
	TEST_voidCheck((LIGHT_u8ToggleLamp(6) == LAMP_OFF) && (TEST_u8NextEvent(EVQ_LAMP,6) == 0),FLASH_STR("LIGHT"),FLASH_STR("lamp 6 rejected"));
}

/******************************* DOOR *******************************/
static void TEST_voidDoor(){
	u8 Local_u8Ok ;
	/* REQ-DOR-01 */
	TEST_voidCheck((DOOR_u8SetState(DOOR_CLOSED) == 0) && (DOOR_u8GetState() == DOOR_CLOSED),FLASH_STR("DOOR"),FLASH_STR("close when closed = 0"));
	Local_u8Ok = (DOOR_u8SetState(DOOR_OPEN) == 1) && (DOOR_u8GetState() == DOOR_OPEN) && (DOOR_u8SetState(DOOR_OPEN) == 0);
	Local_u8Ok = Local_u8Ok && (DOOR_u8SetState(DOOR_CLOSED) == 1) && (DOOR_u8GetState() == DOOR_CLOSED);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("DOOR"),FLASH_STR("open 1 , open 0 , close 1"));
}

/****************************** CLIMATE ******************************/
static void TEST_voidClimate(){
	u8 Local_u8Ok ;
	CLIMATE_voidInit();
	TEST_voidDropEvents();
	/* REQ-AC-02 */
	TEST_voidFeedRoom(30,TEST_WINDOW - 1);
	Local_u8Ok = (CLIMATE_u8IsTempValid() == 0) && (CLIMATE_u8IsAcOn() == 0);
	TEST_voidFeedRoom(30,1);
	Local_u8Ok = Local_u8Ok && (CLIMATE_u8IsTempValid() == 1) && (CLIMATE_u8IsAcOn() == 1) && TEST_u8NextEvent(EVQ_AC,1);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("CLIMATE"),FLASH_STR("30 C : on with the 10th sample + event"));
	TEST_voidFeedRoom(28,TEST_WINDOW);
	Local_u8Ok = (CLIMATE_u8IsAcOn() == 1);
	TEST_voidFeedRoom(21,TEST_WINDOW);
	Local_u8Ok = Local_u8Ok && (CLIMATE_u8IsAcOn() == 1) && (CLIMATE_u8GetRoomTemp() == 21);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("CLIMATE"),FLASH_STR("stays on at 28 and 21"));
	TEST_voidFeedRoom(20,TEST_WINDOW);
	TEST_voidCheck((CLIMATE_u8IsAcOn() == 0) && TEST_u8NextEvent(EVQ_AC,0),FLASH_STR("CLIMATE"),FLASH_STR("off below 21 + event"));
	TEST_voidFeedRoom(25,TEST_WINDOW);
	Local_u8Ok = (CLIMATE_u8IsAcOn() == 0);
	TEST_voidFeedRoom(28,TEST_WINDOW);
	Local_u8Ok = Local_u8Ok && (CLIMATE_u8IsAcOn() == 0);
	TEST_voidFeedRoom(29,TEST_WINDOW);
	Local_u8Ok = Local_u8Ok && (CLIMATE_u8IsAcOn() == 1);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("CLIMATE"),FLASH_STR("stays off at 25 and 28 , on at 29"));
	CLIMATE_voidInit();
}

/******************************* HEATER *******************************/
static void TEST_voidHeater(){
	u8 Local_u8Ok ;
	u8 i ;
	u8 Local_u8Stored = ESTORE_u8ReadByte(ESTORE_ADDR_HEATER_SET);     /* put back at the end */

	/* REQ-HTR-03 , REQ-HTR-14 : validation */
	Local_u8Ok = (HEATER_u8SetSetTemp(60) == 1) && (HEATER_u8SetSetTemp(62) == 0) && (HEATER_u8SetSetTemp(80) == 0) && (HEATER_u8SetSetTemp(30) == 0);
	Local_u8Ok = Local_u8Ok && (HEATER_u8GetSetTemp() == 60) && (ESTORE_u8ReadByte(ESTORE_ADDR_HEATER_SET) == 60);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("HEATER"),FLASH_STR("set 60 ok ; 62 , 80 , 30 rejected"));
	/* REQ-HTR-03 , REQ-HTR-04 : value from ESTORE at init */
	ESTORE_voidWriteByte(ESTORE_ADDR_HEATER_SET,62);
	HEATER_voidInit();
	Local_u8Ok = (HEATER_u8GetSetTemp() == HEATER_SET_DEFAULT);
	ESTORE_voidWriteByte(ESTORE_ADDR_HEATER_SET,50);
	HEATER_voidInit();
	Local_u8Ok = Local_u8Ok && (HEATER_u8GetSetTemp() == 50);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("HEATER"),FLASH_STR("stored 62 -> 60 , stored 50 -> 50"));
	HEATER_u8SetSetTemp(60);

	/* REQ-HTR-05 , D-10 */
	TEST_voidClick(BUTTON_UP_PORT,BUTTON_UP_PIN,2);
	TEST_voidClick(BUTTON_DOWN_PORT,BUTTON_DOWN_PIN,1);
	Local_u8Ok = (HEATER_u8IsOn() == 0) && (HEATER_u8GetSetTemp() == 60);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("HEATER"),FLASH_STR("Up / Down ignored while off"));
	TEST_voidButton(BUTTON_ONOFF_PORT,BUTTON_ONOFF_PIN,1);
	Local_u8Ok = (HEATER_u8IsOn() == 0);
	TEST_voidDropEvents();
	TEST_voidButton(BUTTON_ONOFF_PORT,BUTTON_ONOFF_PIN,0);
	Local_u8Ok = Local_u8Ok && (HEATER_u8IsOn() == 1) && TEST_u8NextEvent(EVQ_HEATER_POWER,1);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("HEATER"),FLASH_STR("on at ON/OFF release , not at press"));

	/* REQ-HTR-08 , REQ-HTR-09 */
	TEST_voidFeedWater(40,TEST_WINDOW - 1);
	Local_u8Ok = TEST_u8ElementIs(HEATER_ELEMENT_NONE);
	TEST_voidFeedWater(40,1);
	Local_u8Ok = Local_u8Ok && TEST_u8ElementIs(HEATER_ELEMENT_HEATING) && TEST_u8NextEvent(EVQ_HEATER_ELEMENT,HEATER_ELEMENT_HEATING);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("HEATER"),FLASH_STR("no decision before 10 samples , then heating"));
	TEST_voidFeedWater(54,TEST_WINDOW);
	Local_u8Ok = TEST_u8ElementIs(HEATER_ELEMENT_HEATING);
	TEST_voidFeedWater(56,TEST_WINDOW);
	Local_u8Ok = Local_u8Ok && TEST_u8ElementIs(HEATER_ELEMENT_HEATING);
	TEST_voidFeedWater(65,TEST_WINDOW);
	Local_u8Ok = Local_u8Ok && TEST_u8ElementIs(HEATER_ELEMENT_HEATING);
	TEST_voidFeedWater(66,TEST_WINDOW);
	Local_u8Ok = Local_u8Ok && TEST_u8ElementIs(HEATER_ELEMENT_COOLING);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("HEATER"),FLASH_STR("set 60 : heating up to 65 , cooling at 66"));
	TEST_voidFeedWater(62,TEST_WINDOW);
	Local_u8Ok = TEST_u8ElementIs(HEATER_ELEMENT_COOLING);
	TEST_voidFeedWater(55,TEST_WINDOW);
	Local_u8Ok = Local_u8Ok && TEST_u8ElementIs(HEATER_ELEMENT_COOLING);
	TEST_voidFeedWater(54,TEST_WINDOW);
	Local_u8Ok = Local_u8Ok && TEST_u8ElementIs(HEATER_ELEMENT_HEATING);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("HEATER"),FLASH_STR("cooling down to 55 , heating at 54"));

	/* REQ-HTR-01 , REQ-HTR-02 , REQ-HTR-03 */
	TEST_voidClick(BUTTON_UP_PORT,BUTTON_UP_PIN,1);
	Local_u8Ok = (HEATER_u8GetSetTemp() == 60);
	TEST_voidClick(BUTTON_UP_PORT,BUTTON_UP_PIN,1);
	Local_u8Ok = Local_u8Ok && (HEATER_u8GetSetTemp() == 65);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("HEATER"),FLASH_STR("1st Up no change , 2nd = 65"));
	TEST_voidClick(BUTTON_UP_PORT,BUTTON_UP_PIN,5);
	Local_u8Ok = (HEATER_u8GetSetTemp() == HEATER_SET_MAX);
	TEST_voidClick(BUTTON_DOWN_PORT,BUTTON_DOWN_PIN,12);
	Local_u8Ok = Local_u8Ok && (HEATER_u8GetSetTemp() == HEATER_SET_MIN);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("HEATER"),FLASH_STR("clamp at 75 and 35"));

	/* REQ-HTR-04 , REQ-HTR-12 : saved when setting mode ends (these calls read the real water sensor) */
	TEST_voidDropEvents();
	for(i = 0 ; i < (HEATER_SETTING_TIMEOUT - 1) ; i++){
		HEATER_voidTask100ms();
	}
	Local_u8Ok = (ESTORE_u8ReadByte(ESTORE_ADDR_HEATER_SET) == 60);
	HEATER_voidTask100ms();
	Local_u8Ok = Local_u8Ok && (ESTORE_u8ReadByte(ESTORE_ADDR_HEATER_SET) == HEATER_SET_MIN);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("HEATER"),FLASH_STR("35 saved after 50 x 100 ms , not before"));

	/* REQ-HTR-06 */
	TEST_voidClick(BUTTON_ONOFF_PORT,BUTTON_ONOFF_PIN,1);
	TEST_voidCheck((HEATER_u8IsOn() == 0) && TEST_u8ElementIs(HEATER_ELEMENT_NONE),FLASH_STR("HEATER"),FLASH_STR("off : both elements off"));

	ESTORE_voidWriteByte(ESTORE_ADDR_HEATER_SET,Local_u8Stored);
	HEATER_voidInit();
}

/******************************* ALARM *******************************/
static void TEST_voidAlarm(){
	u8 Local_u8Ok ;
	/* everything running : heater heating , AC on , lamp 1 on , dimmer 50 % , door open */
	TEST_voidAppInit();
	TEST_voidClick(BUTTON_ONOFF_PORT,BUTTON_ONOFF_PIN,1);
	TEST_voidFeedWater(20,TEST_WINDOW);                /* 20 C is below set - 5 for every legal set temperature */
	TEST_voidFeedRoom(30,TEST_WINDOW);
	LIGHT_u8ToggleLamp(1);
	LIGHT_voidSetDimmer(50);
	DOOR_u8SetState(DOOR_OPEN);
	Local_u8Ok = TEST_u8ElementIs(HEATER_ELEMENT_HEATING) && (CLIMATE_u8IsAcOn() == 1) && (ALARM_u8IsActive() == 0) && (TEST_u8BuzzerIsOn() == 0);
	TEST_voidDropEvents();

	/* REQ-SEC-05 , REQ-ALM-01 */
	ALARM_voidTrigger();
	Local_u8Ok = Local_u8Ok && (ALARM_u8IsActive() == 1) && (HEATER_u8IsOn() == 0) && TEST_u8ElementIs(HEATER_ELEMENT_NONE) && (CLIMATE_u8IsAcOn() == 0);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("ALARM"),FLASH_STR("heater , cooler , fan forced off"));
	TEST_voidCheck(TEST_u8NextEvent(EVQ_HEATER_POWER,0) && TEST_u8NextEvent(EVQ_LOCKDOWN,0),FLASH_STR("ALARM"),FLASH_STR("events : heater off , lockdown"));
	Local_u8Ok = (TEST_u8BuzzerIsOn() == 1);
	ALARM_voidTask500ms();
	Local_u8Ok = Local_u8Ok && (TEST_u8BuzzerIsOn() == 0);
	ALARM_voidTask500ms();
	Local_u8Ok = Local_u8Ok && (TEST_u8BuzzerIsOn() == 1);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("ALARM"),FLASH_STR("buzzer on , off , on every 500 ms"));
	TEST_voidCheck((LIGHT_u8GetLamp(1) == LAMP_ON) && (LIGHT_u8GetDimmer() == 50) && (DOOR_u8GetState() == DOOR_OPEN),FLASH_STR("ALARM"),FLASH_STR("lamp , dimmer , door unchanged"));
	/* latched : the heater button and a hot room change nothing , a second trigger posts nothing */
	TEST_voidClick(BUTTON_ONOFF_PORT,BUTTON_ONOFF_PIN,1);
	TEST_voidFeedRoom(35,TEST_WINDOW);
	ALARM_voidTrigger();
	Local_u8Ok = (ALARM_u8IsActive() == 1) && (HEATER_u8IsOn() == 0) && (CLIMATE_u8IsAcOn() == 0) && (TEST_u8NextEvent(EVQ_LOCKDOWN,0) == 0);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("ALARM"),FLASH_STR("latched"));
	TEST_voidAppInit();
}

/******************************** SEC ********************************/
static void TEST_voidSec(){
	u8 Local_u8Ok ;
	u8 Local_u8Status ;
	/* the real admin name and password (a record field has no '\0' when it is 8 characters long) */
	ESTORE_voidReadBlock(ESTORE_ADDR_ADMIN + USERDB_NAME_OFFSET,(u8 *)Global_c8AdminName,USERDB_NAME_SIZE);
	ESTORE_voidReadBlock(ESTORE_ADDR_ADMIN + USERDB_PASS_OFFSET,(u8 *)Global_c8AdminPass,USERDB_PASS_SIZE);
	Global_c8AdminName[USERDB_NAME_SIZE] = '\0' ;
	Global_c8AdminPass[USERDB_PASS_SIZE] = '\0' ;

	/* two test accounts (removed again below) */
	TEST_voidAppInit();
	USERDB_voidSetWriteAccess(1);
	Local_u8Status = USERDB_u8AddUser(USERDB_LIST_REMOTE,Global_c8UserName,Global_c8UserPass);
	Local_u8Ok = (Local_u8Status == USERDB_OK) || (Local_u8Status == USERDB_ERR_EXISTS);
	Local_u8Status = USERDB_u8AddUser(USERDB_LIST_KEYPAD,Global_c8KeyName,Global_c8UserPass);
	Local_u8Ok = Local_u8Ok && ((Local_u8Status == USERDB_OK) || (Local_u8Status == USERDB_ERR_EXISTS));
	USERDB_voidSetWriteAccess(0);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("SEC"),FLASH_STR("test accounts added"));

	/* REQ-SEC-05 : remote counter */
	Local_u8Ok = (SEC_u8Login(SEC_SOURCE_REMOTE,Global_c8AdminName,Global_c8Wrong) == SEC_LOGIN_FAILED) && (SEC_u8GetAttemptsLeft(SEC_SOURCE_REMOTE) == 2);
	Local_u8Ok = Local_u8Ok && (SEC_u8Login(SEC_SOURCE_REMOTE,Global_c8Wrong,Global_c8AdminPass) == SEC_LOGIN_FAILED) && (SEC_u8GetAttemptsLeft(SEC_SOURCE_REMOTE) == 1);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("SEC"),FLASH_STR("wrong name = wrong password , both counted"));
	Local_u8Ok = (SEC_u8GetAttemptsLeft(SEC_SOURCE_LOCAL) == 3) && (SEC_u8Login(SEC_SOURCE_REMOTE,Global_c8AdminName,Global_c8AdminPass) == SEC_LOGIN_ADMIN);
	Local_u8Ok = Local_u8Ok && (SEC_u8GetRole(SEC_SOURCE_REMOTE) == SEC_ROLE_ADMIN) && (SEC_u8GetAttemptsLeft(SEC_SOURCE_REMOTE) == 3);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("SEC"),FLASH_STR("admin login resets the remote counter"));
	SEC_voidLogout(SEC_SOURCE_REMOTE);
	Local_u8Ok = (SEC_u8Login(SEC_SOURCE_REMOTE,Global_c8UserName,Global_c8Wrong) == SEC_LOGIN_FAILED) && (SEC_u8Login(SEC_SOURCE_REMOTE,Global_c8UserName,Global_c8Wrong) == SEC_LOGIN_FAILED);
	Local_u8Ok = Local_u8Ok && (ALARM_u8IsActive() == 0) && (SEC_u8Login(SEC_SOURCE_REMOTE,Global_c8UserName,Global_c8Wrong) == SEC_LOGIN_LOCKED) && (ALARM_u8IsActive() == 1);
	Local_u8Ok = Local_u8Ok && (SEC_u8Login(SEC_SOURCE_REMOTE,Global_c8AdminName,Global_c8AdminPass) == SEC_LOGIN_LOCKED) && (SEC_u8IsLocalAllowed() == 0);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("SEC"),FLASH_STR("remote : 3rd failure locks , then nothing logs in"));

	/* REQ-SEC-01 , REQ-SEC-05 : keypad counter ("reset" = APP inits again) */
	TEST_voidAppInit();
	Local_u8Ok = (SEC_u8Login(SEC_SOURCE_LOCAL,Global_c8AdminName,Global_c8AdminPass) == SEC_LOGIN_FAILED) && (SEC_u8GetAttemptsLeft(SEC_SOURCE_LOCAL) == 2);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("SEC"),FLASH_STR("admin cannot log in on the keypad"));
	Local_u8Ok = (SEC_u8Login(SEC_SOURCE_LOCAL,Global_c8KeyName,Global_c8UserPass) == SEC_LOGIN_USER) && (SEC_u8GetAttemptsLeft(SEC_SOURCE_LOCAL) == 3);
	Local_u8Ok = Local_u8Ok && (SEC_u8GetRole(SEC_SOURCE_LOCAL) == SEC_ROLE_USER) && TEST_u8NextEvent(EVQ_LOCAL_SESSION,1);
	SEC_voidLogout(SEC_SOURCE_LOCAL);
	Local_u8Ok = Local_u8Ok && (SEC_u8GetRole(SEC_SOURCE_LOCAL) == SEC_ROLE_NONE) && TEST_u8NextEvent(EVQ_LOCAL_SESSION,0);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("SEC"),FLASH_STR("keypad login resets its counter ; login / logout events"));
	/* the remote user name is not in the keypad list (REQ-SEC-02) */
	Local_u8Ok = (SEC_u8Login(SEC_SOURCE_LOCAL,Global_c8UserName,Global_c8UserPass) == SEC_LOGIN_FAILED) && (SEC_u8Login(SEC_SOURCE_LOCAL,Global_c8KeyName,Global_c8Wrong) == SEC_LOGIN_FAILED);
	Local_u8Ok = Local_u8Ok && (SEC_u8GetAttemptsLeft(SEC_SOURCE_REMOTE) == 3) && (SEC_u8Login(SEC_SOURCE_LOCAL,Global_c8KeyName,Global_c8Wrong) == SEC_LOGIN_LOCKED) && (ALARM_u8IsActive() == 1);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("SEC"),FLASH_STR("keypad : own list , own counter , 3rd failure locks"));

	/* REQ-SEC-06 , REQ-SEC-07 , REQ-SEC-08 : roles */
	TEST_voidAppInit();
	Local_u8Ok = (SEC_u8Login(SEC_SOURCE_REMOTE,Global_c8UserName,Global_c8UserPass) == SEC_LOGIN_USER) && (SEC_u8GetRole(SEC_SOURCE_REMOTE) == SEC_ROLE_USER);
	Local_u8Ok = Local_u8Ok && (USERDB_u8AddUser(USERDB_LIST_REMOTE,Global_c8KeyName,Global_c8UserPass) == USERDB_ERR_READ_ONLY);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("SEC"),FLASH_STR("remote user : role USER , accounts read-only"));
	Local_u8Ok = (SEC_u8IsLocalAllowed() == 1) && (SEC_u8Login(SEC_SOURCE_LOCAL,Global_c8KeyName,Global_c8UserPass) == SEC_LOGIN_USER);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("SEC"),FLASH_STR("keypad user beside a remote user"));
	SEC_voidLogout(SEC_SOURCE_REMOTE);
	Local_u8Ok = (SEC_u8Login(SEC_SOURCE_REMOTE,Global_c8AdminName,Global_c8AdminPass) == SEC_LOGIN_ADMIN) && (SEC_u8GetRole(SEC_SOURCE_LOCAL) == SEC_ROLE_NONE);
	Local_u8Ok = Local_u8Ok && (SEC_u8IsLocalAllowed() == 0) && (SEC_u8Login(SEC_SOURCE_LOCAL,Global_c8KeyName,Global_c8UserPass) == SEC_LOGIN_FAILED);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("SEC"),FLASH_STR("admin login ends and blocks the keypad session"));
	SEC_voidSetLocalAllowed(1);
	Local_u8Ok = (SEC_u8IsLocalAllowed() == 1) && (SEC_u8Login(SEC_SOURCE_LOCAL,Global_c8KeyName,Global_c8UserPass) == SEC_LOGIN_USER);
	SEC_voidSetLocalAllowed(0);
	Local_u8Ok = Local_u8Ok && (SEC_u8IsLocalAllowed() == 0) && (SEC_u8GetRole(SEC_SOURCE_LOCAL) == SEC_ROLE_NONE);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("SEC"),FLASH_STR("admin allows , then blocks the keypad"));
	Local_u8Ok = (USERDB_u8RemoveUser(USERDB_LIST_REMOTE,Global_c8UserName) == USERDB_OK) && (USERDB_u8RemoveUser(USERDB_LIST_KEYPAD,Global_c8KeyName) == USERDB_OK);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("SEC"),FLASH_STR("admin may write accounts (test accounts removed)"));
	SEC_voidLogout(SEC_SOURCE_REMOTE);
	SEC_voidSetLocalAllowed(0);
	Local_u8Ok = (SEC_u8GetRole(SEC_SOURCE_REMOTE) == SEC_ROLE_NONE) && (SEC_u8IsLocalAllowed() == 1);
	Local_u8Ok = Local_u8Ok && (USERDB_u8AddUser(USERDB_LIST_REMOTE,Global_c8UserName,Global_c8UserPass) == USERDB_ERR_READ_ONLY);
	TEST_voidCheck(Local_u8Ok,FLASH_STR("SEC"),FLASH_STR("admin logout : gate closed , keypad free"));
	TEST_voidAppInit();
}

/***************************** live part *****************************/
/* " name value" : only called when there is room in the TX ring */
static void TEST_voidField(const __flash c8 * Copy_pc8Name, u8 Copy_u8Value){
	TERM_voidPutFlash(Copy_pc8Name);
	TERM_voidPutNumber(Copy_u8Value);
}
/* one event per call , as "[INFO] <event> <arg>" */
static void TEST_voidLiveEvents(){
	u8 Local_u8Event ;
	u8 Local_u8Arg ;
	if((TERM_u8TxFree() >= TEST_EVENT_ROOM) && (EVQ_u8Get(&Local_u8Event,&Local_u8Arg) == 1)){
		TERM_voidPutFlash(FLASH_STR("[INFO] "));
		switch(Local_u8Event){
		case EVQ_LAMP:           TERM_voidPutFlash(FLASH_STR("LAMP"));           break ;
		case EVQ_DIMMER:         TERM_voidPutFlash(FLASH_STR("DIMMER"));         break ;
		case EVQ_AC:             TERM_voidPutFlash(FLASH_STR("AC"));             break ;
		case EVQ_HEATER_POWER:   TERM_voidPutFlash(FLASH_STR("HEATER_POWER"));   break ;
		case EVQ_HEATER_ELEMENT: TERM_voidPutFlash(FLASH_STR("HEATER_ELEMENT")); break ;
		case EVQ_HEATER_SET:     TERM_voidPutFlash(FLASH_STR("HEATER_SET"));     break ;
		case EVQ_STORAGE_FAULT:  TERM_voidPutFlash(FLASH_STR("STORAGE_FAULT"));  break ;
		case EVQ_LOCKDOWN:       TERM_voidPutFlash(FLASH_STR("LOCKDOWN"));       break ;
		default:                 TERM_voidPutFlash(FLASH_STR("EVENT"));          break ;
		}
		TEST_voidField(FLASH_STR(" "),Local_u8Arg);
		TERM_voidNewLine();
	}
}
/* typed lines stand in for the UIs of part B (see the header) */
static void TEST_voidLiveInput(){
	c8 Local_c8Line[TERM_LINE_MAX + 1] ;
	u16 Local_u16Number = 0 ;
	if(TERM_u8GetLine(Local_c8Line) != TERM_LINE_READY){
		return ;
	}
	if(ALARM_u8IsActive() == 1){
		return ;                                       /* lockdown : input ignored until reset */
	}
	if(FMT_u8TextToNumber(Local_c8Line,&Local_u16Number) == 1){
		if(Local_u16Number <= LAMP_COUNT){
			LIGHT_u8ToggleLamp((u8)Local_u16Number);
		}else if((Local_u16Number > 255) || (HEATER_u8SetSetTemp((u8)Local_u16Number) == 0)){
			TERM_voidPutFlash(FLASH_STR("[INFO] rejected\r\n"));
		}else{
		}
		return ;
	}
	switch(Local_c8Line[0]){
	case '+': LIGHT_voidSetDimmer(LIGHT_u8GetDimmer() + LIGHT_DIMMER_STEP); break ;
	case '-': if(LIGHT_u8GetDimmer() >= LIGHT_DIMMER_STEP){ LIGHT_voidSetDimmer(LIGHT_u8GetDimmer() - LIGHT_DIMMER_STEP); } break ;
	case 'o': TEST_voidField(FLASH_STR("[INFO] DOOR moved "),DOOR_u8SetState(DOOR_OPEN));   TERM_voidNewLine(); break ;
	case 'c': TEST_voidField(FLASH_STR("[INFO] DOOR moved "),DOOR_u8SetState(DOOR_CLOSED)); TERM_voidNewLine(); break ;
	case 'a': ALARM_voidTrigger(); break ;
	default:  break ;
	}
}
/* every TEST_STATUS_PERIOD_S seconds : what the UIs will show in part B */
static void TEST_voidLiveStatus(){
	Global_u8StatusCount++;
	if((Global_u8StatusCount >= TEST_STATUS_PERIOD_S) && (TERM_u8TxFree() >= TEST_STATUS_ROOM)){
		Global_u8StatusCount = 0 ;
		TEST_voidField(FLASH_STR("[STAT] room "),CLIMATE_u8GetRoomTemp());
		TEST_voidField(FLASH_STR(" ac "),CLIMATE_u8IsAcOn());
		TEST_voidField(FLASH_STR(" water "),HEATER_u8GetWaterTemp());
		TEST_voidField(FLASH_STR(" set "),HEATER_u8GetSetTemp());
		TEST_voidField(FLASH_STR(" on "),HEATER_u8IsOn());
		TEST_voidField(FLASH_STR(" el "),HEATER_u8GetElement());
		TERM_voidNewLine();
	}
}

int main(void){
	/*1. boot order of architecture.md 4.6 : outputs to their safe state first */
	RELAY_voidInit();
	LED_voidInit();
	BUZZER_voidInit();
	FAN_voidInit();
	SERVO_voidInit();
	DIMMER_voidInit();
	/*2. terminal , I2C parts , inputs (no LCD and no keypad in part A) */
	TERM_voidInit();
	LAMP_voidInit();
	SEVEN_SEG_voidInit();
	BUTTON_voidInit();
	LM35_voidInit();
	/*3. storage */
	ESTORE_voidInit();
	USERDB_voidInit();
	EVQ_voidInit();
	/*4. APP (ALARM , SEC , LIGHT , DOOR , CLIMATE , HEATER) : no SCHED call in any of them */
	TEST_voidAppInit();
	/*5. tick */
	SCHED_voidInit();
	GIE_voidEnableGlobalInterrupt();
	SCHED_voidStart();

	/*6. automatic part */
	TEST_voidPrintLine(FLASH_STR("\r\n===== test_app : APP layer , part A ====="));
	/* ESTORE_FIRST_BOOT is not an error : the defaults were loaded */
	TEST_voidCheck(ESTORE_u8GetStatus() != ESTORE_FAULT,FLASH_STR("ESTORE"),FLASH_STR("EEPROM answers"));
	TEST_voidLight();
	TEST_voidDoor();
	TEST_voidClimate();
	TEST_voidHeater();
	TEST_voidAlarm();
	TEST_voidSec();
	TEST_voidPrint(FLASH_STR("SUMMARY: "));
	TEST_voidPrintNum(Global_u8PassCount);
	TEST_voidPrint(FLASH_STR(" passed , "));
	TEST_voidPrintNum(Global_u8FailCount);
	TEST_voidPrintLine(FLASH_STR(" failed"));
	TEST_voidPrintLine(FLASH_STR("LIVE: heater panel + AC (test_plan 3.6 , 3.7). Lines: 1..5 + - o c a 35..75"));

	/*7. live part : the super-loop of architecture.md 4.2 with the part A modules */
	while(1){
		if(SCHED_u8IsTaskDue(SCHED_TASK_5MS)){
			TEST_voidLiveInput();
			TEST_voidLiveEvents();
		}
		if(SCHED_u8IsTaskDue(SCHED_TASK_10MS)){
			BUTTON_voidUpdate();
			HEATER_voidTask10ms();
			ESTORE_voidUpdate();
		}
		if(SCHED_u8IsTaskDue(SCHED_TASK_100MS)){
			HEATER_voidTask100ms();
			CLIMATE_voidTask100ms();
		}
		if(SCHED_u8IsTaskDue(SCHED_TASK_500MS)){
			HEATER_voidTask500ms();
			ALARM_voidTask500ms();
		}
		if(SCHED_u8IsTaskDue(SCHED_TASK_1S)){
			TEST_voidLiveStatus();
		}
	}
	return 0 ;
}
