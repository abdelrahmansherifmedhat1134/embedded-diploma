/*
 * test_service.c
 *
 *  Created on: Oct 3, 2026
 *
 * =====================================================================
 *  Phase 2 test of the SERVICE layer
 * =====================================================================
 *  Tested : RINGBUF , MAVG , FMT , SCHED , TERM , EVQ , ESTORE , USERDB
 *           (flash_str.h is used by every line of text)
 *
 *  Build : pio run -e test_service
 *  Hex   : .pio/build/test_service/firmware.hex
 *
 *  Time base = SCHED (Timer2 , 1 ms tick). Timer1 only runs as a stop watch.
 *  Output : the blocking USART functions until TERM_voidInit runs , from then
 *  on everything goes through TERM (TX ring + interrupt) , never both.
 *
 * ---------------------- Proteus parts and wiring ----------------------
 *  (the test_hal schematic can stay as it is ; only these parts are used)
 *  ATMEGA32          Clock Frequency = 16 MHz (must equal F_CPU)
 *  Status LED        PA3 -> 330R -> LED-RED -> GND
 *  VIRTUAL TERMINAL  9600 baud , 8N1 : terminal RXD <- PD1 , terminal TXD -> PD0
 *                    (you type in it : click the terminal window first)
 *  I2C bus           PC0 = SCL , PC1 = SDA , 4.7k pull-up to VCC on both lines
 *  24C08 EEPROM      address 0x50 (A2 = GND , A1 A0 = GND) , WP = GND
 *  SWITCH (SPST)     PC1 (SDA) -> switch -> GND , the one from test_mcal
 *                    (open = normal , closed = EEPROM "removed")
 *
 * --------------------------- Expected result ---------------------------
 *  UART : one line per check "[PASS] <module>: <what>" or "[FAIL] ...",
 *         "[SKIP]" = a manual step was not done , "[INFO]" = a number or a
 *         simulation difference , "MANUAL:" lines tell you what to do ,
 *         last line = SUMMARY.
 *  Status LED : slow blink        = test running
 *               solid ON          = all checks passed
 *               fast blink (5 Hz) = at least one check failed
 *
 *  Things you must do by hand (the terminal tells you when) :
 *   1. SCHED   : nothing , just wait 10 s while the flags are counted
 *   2. TERM    : look at the line of 128 dots (no '#' in it) , then type what
 *                each MANUAL line asks for (20 s per step) :
 *                abc Enter / 1234 Enter (shown as ****) / abx Backspace c Enter /
 *                20 characters Enter / Enter alone / a Ctrl+J / b Enter Ctrl+J
 *   3. ESTORE  : CLOSE the SDA switch when asked (15 s) , OPEN it again when asked
 *   4. RESET   : after the SUMMARY press the RESET button of the ATmega32 :
 *                the next run is short and checks that the data survived
 *                (press RESET once more to get the full test again)
 *
 *  The test makes the EEPROM blank itself (page 0 = 0xFF) , so every full
 *  run starts with a first boot. It leaves on the chip : admin / 1234 ,
 *  remote user ali / pass1 , keypad user 1234 / 5678 , set temperature 65.
 * =====================================================================
 */
#include "../lib/Service/std_types.h"
#include "../lib/Service/Bit_math.h"
#include "../lib/Service/flash_str.h"
#include "../lib/MCAL/reg_def.h"
#include "../lib/MCAL/DIO/DIO.h"
#include "../lib/MCAL/GIE/GIE.h"
#include "../lib/MCAL/TIMER1/TIMER1.h"
#include "../lib/MCAL/TIMER1/TIMER1_cfg.h"
#include "../lib/MCAL/USART/USART.h"
#include "../lib/HAL/EXT_EEPROM/EXT_EEPROM.h"
#include "../lib/HAL/EXT_EEPROM/EXT_EEPROM_cfg.h"
#include "../lib/Service/RINGBUF/RINGBUF.h"
#include "../lib/Service/MAVG/MAVG.h"
#include "../lib/Service/FMT/FMT.h"
#include "../lib/Service/SCHED/SCHED.h"
#include "../lib/Service/TERM/TERM.h"
#include "../lib/Service/TERM/TERM_cfg.h"
#include "../lib/Service/EVQ/EVQ.h"
#include "../lib/Service/EVQ/EVQ_cfg.h"
#include "../lib/Service/ESTORE/ESTORE.h"
#include "../lib/Service/ESTORE/ESTORE_cfg.h"
#include "../lib/Service/USERDB/USERDB.h"

/* Test configuration */
#define TEST_LED_PORT             DIO_PORTA      /* status LED = heater LED pin */
#define TEST_LED_PIN              DIO_PIN_3
#define TEST_BLINK_10MS           25             /* 25 x 10 ms --> 2 Hz blink */
#define TEST_TYPE_TIMEOUT_MS      20000
#define TEST_FAULT_TIMEOUT_MS     15000
#define TEST_QUIET_MS             300            /* no second line may arrive in this time */
#define TEST_EE_WAIT_MS           50
#define TEST_TEXT_SIZE            24             /* RAM copies of test texts */

/* Timer1 counts 0 .. TOP : used as a stop watch (1 tick = 0.5 us at 16 MHz) */
#define TEST_T1_PERIOD            ((u16)(TIMER1_TOP_VALUE + 1))
#define TEST_UPDATE_LIMIT_US      2500           /* longest ESTORE_voidUpdate call allowed */

/* SCHED : the counting window is ticks 28 .. 10028. No task is due on ticks 26..29
 * (or 10026..10029) , so a flag can never fall on the edge of the window. */
#define TEST_SCHED_START_TICK     28UL
#define TEST_SCHED_WINDOW_MS      10000UL
#define TEST_SCHED_T1_PERIODS     500            /* 10 s / 20 ms */

#define TEST_RING_SIZE            5              /* capacity 4 */

/* reserved bytes of page 1 : free for write tests , put back to 0xFF */
#define TEST_EE_SPARE_A           0x1E
#define TEST_EE_SPARE_B           0x1F
#define TEST_HEATER_SET_VALUE     65
/* a page outside the ESTORE map : tells the next run "RESET was pressed , check the data" */
#define TEST_EE_MARK_ADDR         0x03E0
#define TEST_EE_MARK_0            0x5A
#define TEST_EE_MARK_1            0xC3

/* what the chip must hold in page 2 after a first boot : admin + 1234 , padded with 0x00 */
static const __flash u8 TEST_ADMIN_RECORD_ARR[USERDB_RECORD_SIZE] = {'a','d','m','i','n',0,0,0,'1','2','3','4',0,0,0,0};

static u8 Global_u8PassCount = 0 ;
static u8 Global_u8FailCount = 0 ;
static u8 Global_u8SkipCount = 0 ;
static u8 Global_u8UseTerm = 0 ;               /* 0 = blocking USART , 1 = TERM */
static u8 Global_u8StoreRunning = 0 ;          /* 1 = TEST_voidService calls ESTORE_voidUpdate every 10 ms */
static u8 Global_u8BlinkCount = 0 ;
static u8 Global_u8SawTyping = 0 ;             /* TERM_u8IsLineEmpty was 0 while waiting for a line */
static u16 Global_u16MaxUpdateTicks = 0 ;      /* longest ESTORE_voidUpdate call , in Timer1 ticks */
static c8 Global_c8TextA[TEST_TEXT_SIZE] ;
static c8 Global_c8TextB[TEST_TEXT_SIZE] ;

/*************************** time base ***************************/
/* 16-bit timer register : low byte first (it latches the high byte) */
static u16 TEST_u16ReadTCNT1(){
	u16 Local_u16Value = TCNT1L ;
	Local_u16Value |= ((u16)TCNT1H << 8) ;
	return Local_u16Value ;
}
/* Timer1 ticks between two TCNT1 readings (Timer1 wraps at TOP + 1) */
static u16 TEST_u16TicksSince(u16 Copy_u16Start){
	u16 Local_u16Now = TEST_u16ReadTCNT1();
	if(Local_u16Now >= Copy_u16Start){
		return Local_u16Now - Copy_u16Start ;
	}
	return (u16)(TEST_T1_PERIOD - Copy_u16Start + Local_u16Now) ;
}
/* one ESTORE step , timed with the stop watch */
static void TEST_voidStoreStep(){
	u16 Local_u16Start = TEST_u16ReadTCNT1();
	u16 Local_u16Ticks ;
	ESTORE_voidUpdate();
	Local_u16Ticks = TEST_u16TicksSince(Local_u16Start);
	if(Local_u16Ticks > Global_u16MaxUpdateTicks){
		Global_u16MaxUpdateTicks = Local_u16Ticks ;
	}
}
/* the "super-loop" of this test : every 10 ms the EEPROM write-behind and the status LED */
static void TEST_voidService(){
	if(SCHED_u8IsTaskDue(SCHED_TASK_10MS)){
		if(Global_u8StoreRunning){
			TEST_voidStoreStep();
		}
		Global_u8BlinkCount++;
		if(Global_u8BlinkCount >= TEST_BLINK_10MS){
			Global_u8BlinkCount = 0 ;
			DIO_voidTogPin(TEST_LED_PORT,TEST_LED_PIN);
		}
	}
}
static void TEST_voidWaitMs(u16 Copy_u16Ms){
	u32 Local_u32Start = SCHED_u32GetTickMs();
	while((SCHED_u32GetTickMs() - Local_u32Start) < Copy_u16Ms){
		TEST_voidService();
	}
}

/*************************** print helpers ***************************/
static void TEST_voidPutChar(c8 Copy_c8Char){
	TEST_voidService();
	if(Global_u8UseTerm == 0){
		USART_voidSend((u8)Copy_c8Char);
	}else{
		/* TERM drops what does not fit : wait for room first , so no test line is cut */
		while(TERM_u8TxFree() == 0){
			TEST_voidService();
		}
		TERM_voidPutChar(Copy_c8Char);
	}
}
static void TEST_voidPrint(const __flash c8 * Copy_pc8Text){
	while(*Copy_pc8Text != '\0'){
		TEST_voidPutChar(*Copy_pc8Text++);
	}
}
static void TEST_voidNewLine(){
	TEST_voidPutChar('\r');
	TEST_voidPutChar('\n');
}
static void TEST_voidPrintLine(const __flash c8 * Copy_pc8Text){
	TEST_voidPrint(Copy_pc8Text);
	TEST_voidNewLine();
}
static void TEST_voidPrintNum(u16 Copy_u16Num){
	c8 Local_c8Text[FMT_NUMBER_TEXT_SIZE] ;
	u8 Local_u8Length = FMT_u8NumberToText(Copy_u16Num,Local_c8Text);
	for(u8 i = 0 ; i < Local_u8Length ; i++){
		TEST_voidPutChar(Local_c8Text[i]);
	}
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
static void TEST_voidSkip(const __flash c8 * Copy_pc8Module, const __flash c8 * Copy_pc8Why){
	TEST_voidPrint(FLASH_STR("[SKIP] "));
	TEST_voidPrint(Copy_pc8Module);
	TEST_voidPrint(FLASH_STR(": "));
	TEST_voidPrintLine(Copy_pc8Why);
	Global_u8SkipCount++;
}
/* not a pass or a fail : a measured number , or something the simulation does differently */
static void TEST_voidInfo(const __flash c8 * Copy_pc8Module, const __flash c8 * Copy_pc8Text){
	TEST_voidPrint(FLASH_STR("[INFO] "));
	TEST_voidPrint(Copy_pc8Module);
	TEST_voidPrint(FLASH_STR(": "));
	TEST_voidPrintLine(Copy_pc8Text);
}
/* "[INFO] module: text <number> unit" */
static void TEST_voidInfoNum(const __flash c8 * Copy_pc8Module, const __flash c8 * Copy_pc8Text, u16 Copy_u16Num, const __flash c8 * Copy_pc8Unit){
	TEST_voidPrint(FLASH_STR("[INFO] "));
	TEST_voidPrint(Copy_pc8Module);
	TEST_voidPrint(FLASH_STR(": "));
	TEST_voidPrint(Copy_pc8Text);
	TEST_voidPrintNum(Copy_u16Num);
	TEST_voidPrintLine(Copy_pc8Unit);
}
static void TEST_voidManual(const __flash c8 * Copy_pc8Text){
	TEST_voidPrint(FLASH_STR("MANUAL: "));
	TEST_voidPrintLine(Copy_pc8Text);
}

/*************************** text helpers ***************************/
/* The SERVICE functions take RAM strings. The test texts stay in flash and are
 * copied into one of two small RAM buffers just before the call. */
static void TEST_voidCopyText(c8 * Copy_pc8Ram, const __flash c8 * Copy_pc8Text){
	u8 i = 0 ;
	while((Copy_pc8Text[i] != '\0') && (i < (TEST_TEXT_SIZE - 1))){
		Copy_pc8Ram[i] = Copy_pc8Text[i] ;
		i++;
	}
	Copy_pc8Ram[i] = '\0' ;
}
static c8 * TEST_pc8TextA(const __flash c8 * Copy_pc8Text){
	TEST_voidCopyText(Global_c8TextA,Copy_pc8Text);
	return Global_c8TextA ;
}
static c8 * TEST_pc8TextB(const __flash c8 * Copy_pc8Text){
	TEST_voidCopyText(Global_c8TextB,Copy_pc8Text);
	return Global_c8TextB ;
}
static u8 TEST_u8TextEqual(const c8 * Copy_pc8Ram, const __flash c8 * Copy_pc8Text){
	while((*Copy_pc8Ram != '\0') && (*Copy_pc8Ram == *Copy_pc8Text)){
		Copy_pc8Ram++;
		Copy_pc8Text++;
	}
	return (*Copy_pc8Ram == *Copy_pc8Text) ? 1 : 0 ;
}

/*************************** RINGBUF ***************************/
static void TEST_voidRingbuf(){
	volatile u8 Local_u8Storage[TEST_RING_SIZE] ;
	RINGBUF_t Local_Ring ;
	u8 Local_u8Data = 0 ;
	u8 Local_u8Ok = 1 ;
	u8 Local_u8In = 0 ;
	u8 Local_u8Out = 0 ;
	u8 i ;
	RINGBUF_voidInit(&Local_Ring,Local_u8Storage,TEST_RING_SIZE);
	/*1. empty */
	TEST_voidCheck((RINGBUF_u8GetCount(&Local_Ring) == 0) && (RINGBUF_u8GetFree(&Local_Ring) == (TEST_RING_SIZE - 1)), FLASH_STR("RINGBUF"), FLASH_STR("new ring : count 0 , free = Size - 1"));
	TEST_voidCheck(RINGBUF_u8Get(&Local_Ring,&Local_u8Data) == RINGBUF_EMPTY, FLASH_STR("RINGBUF"), FLASH_STR("Get on an empty ring returns RINGBUF_EMPTY"));
	/*2. full */
	for(i = 0 ; i < (TEST_RING_SIZE - 1) ; i++){
		if(RINGBUF_u8Put(&Local_Ring,10 + i) != RINGBUF_OK){
			Local_u8Ok = 0 ;
		}
	}
	TEST_voidCheck(Local_u8Ok, FLASH_STR("RINGBUF"), FLASH_STR("Size - 1 bytes fit"));
	TEST_voidCheck(RINGBUF_u8Put(&Local_Ring,99) == RINGBUF_FULL, FLASH_STR("RINGBUF"), FLASH_STR("one more returns RINGBUF_FULL"));
	TEST_voidCheck((RINGBUF_u8GetCount(&Local_Ring) == (TEST_RING_SIZE - 1)) && (RINGBUF_u8GetFree(&Local_Ring) == 0), FLASH_STR("RINGBUF"), FLASH_STR("full ring : count = Size - 1 , free 0"));
	/*3. order */
	Local_u8Ok = 1 ;
	for(i = 0 ; i < (TEST_RING_SIZE - 1) ; i++){
		if((RINGBUF_u8Get(&Local_Ring,&Local_u8Data) != RINGBUF_OK) || (Local_u8Data != (10 + i))){
			Local_u8Ok = 0 ;
		}
	}
	TEST_voidCheck(Local_u8Ok && (RINGBUF_u8GetCount(&Local_Ring) == 0), FLASH_STR("RINGBUF"), FLASH_STR("bytes come out in the order they went in (the rejected one is not there)"));
	/*4. wrap : 3 in , 3 out , ten times = the indexes go round the 5-byte storage 6 times */
	Local_u8Ok = 1 ;
	for(i = 0 ; i < 10 ; i++){
		RINGBUF_u8Put(&Local_Ring,Local_u8In++);
		RINGBUF_u8Put(&Local_Ring,Local_u8In++);
		RINGBUF_u8Put(&Local_Ring,Local_u8In++);
		if(RINGBUF_u8GetCount(&Local_Ring) != 3){
			Local_u8Ok = 0 ;
		}
		for(u8 j = 0 ; j < 3 ; j++){
			if((RINGBUF_u8Get(&Local_Ring,&Local_u8Data) != RINGBUF_OK) || (Local_u8Data != Local_u8Out++)){
				Local_u8Ok = 0 ;
			}
		}
	}
	TEST_voidCheck(Local_u8Ok, FLASH_STR("RINGBUF"), FLASH_STR("wrap : 30 bytes through a 5-byte storage , order and count kept"));
	TEST_voidCheck(RINGBUF_u8Get(&Local_Ring,&Local_u8Data) == RINGBUF_EMPTY, FLASH_STR("RINGBUF"), FLASH_STR("empty again after the wrap"));
}

/*************************** MAVG ***************************/
static void TEST_voidMavg(){
	MAVG_t Local_Avg ;
	u8 Local_u8NotFull = 1 ;
	u8 i ;
	MAVG_voidReset(&Local_Avg);
	TEST_voidCheck((MAVG_u16GetAverage(&Local_Avg) == 0) && (MAVG_u8IsFull(&Local_Avg) == 0), FLASH_STR("MAVG"), FLASH_STR("after reset : average 0 , not full"));
	/*1. 1 , 1 , 2 : 4 / 3 = 1.33 --> 1 (rounds down) */
	MAVG_voidAddSample(&Local_Avg,1);
	MAVG_voidAddSample(&Local_Avg,1);
	MAVG_voidAddSample(&Local_Avg,2);
	TEST_voidCheck(MAVG_u16GetAverage(&Local_Avg) == 1, FLASH_STR("MAVG"), FLASH_STR("3 samples 1 1 2 : average 1 (1.33 rounded)"));
	/*2. 1 .. 10 : 55 / 10 = 5.5 --> 6 (rounds up) */
	MAVG_voidReset(&Local_Avg);
	for(i = 1 ; i <= 10 ; i++){
		if(MAVG_u8IsFull(&Local_Avg)){
			Local_u8NotFull = 0 ;
		}
		MAVG_voidAddSample(&Local_Avg,i);
	}
	TEST_voidCheck(Local_u8NotFull, FLASH_STR("MAVG"), FLASH_STR("not full with 0 .. 9 samples"));
	TEST_voidCheck(MAVG_u8IsFull(&Local_Avg) == 1, FLASH_STR("MAVG"), FLASH_STR("full with 10 samples (REQ-HTR-08)"));
	TEST_voidCheck(MAVG_u16GetAverage(&Local_Avg) == 6, FLASH_STR("MAVG"), FLASH_STR("samples 1 .. 10 : average 6 (5.5 rounded)"));
	/*3. 11th sample pushes the oldest (1) out : 2 .. 10 + 1001 = 1055 --> 106 */
	MAVG_voidAddSample(&Local_Avg,1001);
	TEST_voidCheck(MAVG_u16GetAverage(&Local_Avg) == 106, FLASH_STR("MAVG"), FLASH_STR("11th sample replaces the oldest : average 106"));
	/*4. biggest ADC value : 10 x 1023 = 10230 fits in the u16 sum */
	for(i = 0 ; i < 10 ; i++){
		MAVG_voidAddSample(&Local_Avg,1023);
	}
	TEST_voidCheck(MAVG_u16GetAverage(&Local_Avg) == 1023, FLASH_STR("MAVG"), FLASH_STR("10 x 1023 : average 1023 (sum does not overflow)"));
	MAVG_voidReset(&Local_Avg);
	TEST_voidCheck((MAVG_u16GetAverage(&Local_Avg) == 0) && (MAVG_u8IsFull(&Local_Avg) == 0), FLASH_STR("MAVG"), FLASH_STR("reset empties a full window"));
}

/*************************** FMT ***************************/
static u8 TEST_u8FmtNumber(u16 Copy_u16Number, const __flash c8 * Copy_pc8Expected, u8 Copy_u8Length){
	c8 Local_c8Text[FMT_NUMBER_TEXT_SIZE] ;
	if(FMT_u8NumberToText(Copy_u16Number,Local_c8Text) != Copy_u8Length){
		return 0 ;
	}
	return TEST_u8TextEqual(Local_c8Text,Copy_pc8Expected) ;
}
/* 1 = the text was accepted AND gave the expected number */
static u8 TEST_u8FmtText(const __flash c8 * Copy_pc8Text, u16 Copy_u16Expected){
	u16 Local_u16Number = 0 ;
	if(FMT_u8TextToNumber(TEST_pc8TextA(Copy_pc8Text),&Local_u16Number) == 0){
		return 0 ;
	}
	return (Local_u16Number == Copy_u16Expected) ? 1 : 0 ;
}
static u8 TEST_u8FmtRejects(const __flash c8 * Copy_pc8Text){
	u16 Local_u16Number = 0 ;
	return (FMT_u8TextToNumber(TEST_pc8TextA(Copy_pc8Text),&Local_u16Number) == 0) ? 1 : 0 ;
}
static void TEST_voidFmt(){
	TEST_voidCheck(TEST_u8FmtNumber(0,FLASH_STR("0"),1), FLASH_STR("FMT"), FLASH_STR("0 -> \"0\""));
	TEST_voidCheck(TEST_u8FmtNumber(9,FLASH_STR("9"),1), FLASH_STR("FMT"), FLASH_STR("9 -> \"9\""));
	TEST_voidCheck(TEST_u8FmtNumber(10,FLASH_STR("10"),2), FLASH_STR("FMT"), FLASH_STR("10 -> \"10\""));
	TEST_voidCheck(TEST_u8FmtNumber(255,FLASH_STR("255"),3), FLASH_STR("FMT"), FLASH_STR("255 -> \"255\""));
	TEST_voidCheck(TEST_u8FmtNumber(65535,FLASH_STR("65535"),5), FLASH_STR("FMT"), FLASH_STR("65535 -> \"65535\""));
	TEST_voidCheck(TEST_u8FmtText(FLASH_STR("0"),0) && TEST_u8FmtText(FLASH_STR("75"),75) && TEST_u8FmtText(FLASH_STR("00042"),42), FLASH_STR("FMT"), FLASH_STR("text -> number : \"0\" , \"75\" , \"00042\""));
	TEST_voidCheck(TEST_u8FmtText(FLASH_STR("65535"),65535), FLASH_STR("FMT"), FLASH_STR("text -> number : \"65535\" accepted"));
	TEST_voidCheck(TEST_u8FmtRejects(FLASH_STR("65536")) && TEST_u8FmtRejects(FLASH_STR("70000")) && TEST_u8FmtRejects(FLASH_STR("655350")), FLASH_STR("FMT"), FLASH_STR("text -> number rejects 65536 , 70000 , 655350 (no overflow)"));
	TEST_voidCheck(TEST_u8FmtRejects(FLASH_STR("12a")) && TEST_u8FmtRejects(FLASH_STR("abc")), FLASH_STR("FMT"), FLASH_STR("text -> number rejects letters"));
	TEST_voidCheck(TEST_u8FmtRejects(FLASH_STR("")), FLASH_STR("FMT"), FLASH_STR("text -> number rejects empty text"));
	TEST_voidCheck(TEST_u8FmtRejects(FLASH_STR(" 5")) && TEST_u8FmtRejects(FLASH_STR("-1")) && TEST_u8FmtRejects(FLASH_STR("5 ")), FLASH_STR("FMT"), FLASH_STR("text -> number rejects space and sign"));
}

/*************************** EVQ ***************************/
static void TEST_voidEvq(){
	u8 Local_u8Event = 0 ;
	u8 Local_u8Arg = 0 ;
	u8 Local_u8Ok = 1 ;
	u8 i ;
	EVQ_voidInit();
	TEST_voidCheck(EVQ_u8Get(&Local_u8Event,&Local_u8Arg) == 0, FLASH_STR("EVQ"), FLASH_STR("empty queue returns 0"));
	/*1. order and argument */
	EVQ_voidPost(EVQ_LAMP,3);
	EVQ_voidPost(EVQ_AC,1);
	EVQ_voidPost(EVQ_HEATER_SET,65);
	if((EVQ_u8Get(&Local_u8Event,&Local_u8Arg) != 1) || (Local_u8Event != EVQ_LAMP) || (Local_u8Arg != 3)){
		Local_u8Ok = 0 ;
	}
	if((EVQ_u8Get(&Local_u8Event,&Local_u8Arg) != 1) || (Local_u8Event != EVQ_AC) || (Local_u8Arg != 1)){
		Local_u8Ok = 0 ;
	}
	if((EVQ_u8Get(&Local_u8Event,&Local_u8Arg) != 1) || (Local_u8Event != EVQ_HEATER_SET) || (Local_u8Arg != 65)){
		Local_u8Ok = 0 ;
	}
	TEST_voidCheck(Local_u8Ok && (EVQ_u8Get(&Local_u8Event,&Local_u8Arg) == 0), FLASH_STR("EVQ"), FLASH_STR("3 events come out in order with their arguments"));
	/*2. full : EVQ_MAX_EVENTS fit , two more are dropped */
	for(i = 0 ; i < (EVQ_MAX_EVENTS + 2) ; i++){
		EVQ_voidPost(EVQ_DIMMER,i);
	}
	Local_u8Ok = 1 ;
	for(i = 0 ; i < EVQ_MAX_EVENTS ; i++){
		if((EVQ_u8Get(&Local_u8Event,&Local_u8Arg) != 1) || (Local_u8Event != EVQ_DIMMER) || (Local_u8Arg != i)){
			Local_u8Ok = 0 ;
		}
	}
	TEST_voidCheck(Local_u8Ok, FLASH_STR("EVQ"), FLASH_STR("16 events fit (EVQ_MAX_EVENTS is real)"));
	TEST_voidCheck(EVQ_u8Get(&Local_u8Event,&Local_u8Arg) == 0, FLASH_STR("EVQ"), FLASH_STR("events 17 and 18 were dropped (queue full)"));
	/*3. mute */
	EVQ_voidSetMute(1);
	EVQ_voidPost(EVQ_LAMP,1);
	TEST_voidCheck(EVQ_u8Get(&Local_u8Event,&Local_u8Arg) == 0, FLASH_STR("EVQ"), FLASH_STR("muted : a post is dropped"));
	EVQ_voidSetMute(0);
	EVQ_voidPost(EVQ_LAMP,2);
	TEST_voidCheck((EVQ_u8Get(&Local_u8Event,&Local_u8Arg) == 1) && (Local_u8Event == EVQ_LAMP) && (Local_u8Arg == 2), FLASH_STR("EVQ"), FLASH_STR("unmuted : posts are stored again"));
}

/*************************** SCHED ***************************/
static void TEST_voidSched(){
	u16 Local_u16Count[SCHED_TASK_COUNT] = {0,0,0,0,0} ;
	u16 Local_u16Periods = 0 ;                 /* Timer1 periods (20 ms) seen during the window */
	u16 Local_u16Last ;
	u16 Local_u16Timer ;
	u16 Local_u16Overruns ;
	u32 Local_u32Now ;
	u8 Local_u8Counting = 0 ;
	u8 i ;
	TEST_voidPrintLine(FLASH_STR("       SCHED: counting the five task flags for 10 s , please wait ..."));
	/*1. restart the scheduler : tick 0 , no flags , no overruns (the tick interrupt stays on) */
	GIE_voidDisableGlobalInterrupt();
	SCHED_voidInit();
	GIE_voidEnableGlobalInterrupt();
	Local_u16Last = TEST_u16ReadTCNT1();
	/*2. serve every flag as a super-loop would , count inside the window */
	while(1){
		Local_u32Now = SCHED_u32GetTickMs();
		if((Local_u8Counting == 0) && (Local_u32Now >= TEST_SCHED_START_TICK)){
			Local_u8Counting = 1 ;
			Local_u16Periods = 0 ;
			for(i = 0 ; i < SCHED_TASK_COUNT ; i++){
				Local_u16Count[i] = 0 ;
			}
		}
		for(i = 0 ; i < SCHED_TASK_COUNT ; i++){
			if(SCHED_u8IsTaskDue(i)){
				Local_u16Count[i]++;
				if(i == SCHED_TASK_500MS){
					DIO_voidTogPin(TEST_LED_PORT,TEST_LED_PIN);
				}
			}
		}
		Local_u16Timer = TEST_u16ReadTCNT1();
		if(Local_u16Timer < Local_u16Last){
			Local_u16Periods++;
		}
		Local_u16Last = Local_u16Timer ;
		if(Local_u32Now >= (TEST_SCHED_START_TICK + TEST_SCHED_WINDOW_MS)){
			break ;
		}
	}
	/*3. report (the overruns are read first : printing lets flags pile up) */
	Local_u16Overruns = SCHED_u16GetOverruns();
	TEST_voidPrint(FLASH_STR("       flags served in 10 s : 5 ms "));
	TEST_voidPrintNum(Local_u16Count[SCHED_TASK_5MS]);
	TEST_voidPrint(FLASH_STR(" , 10 ms "));
	TEST_voidPrintNum(Local_u16Count[SCHED_TASK_10MS]);
	TEST_voidPrint(FLASH_STR(" , 100 ms "));
	TEST_voidPrintNum(Local_u16Count[SCHED_TASK_100MS]);
	TEST_voidPrint(FLASH_STR(" , 500 ms "));
	TEST_voidPrintNum(Local_u16Count[SCHED_TASK_500MS]);
	TEST_voidPrint(FLASH_STR(" , 1 s "));
	TEST_voidPrintNum(Local_u16Count[SCHED_TASK_1S]);
	TEST_voidNewLine();
	TEST_voidCheck(Local_u16Count[SCHED_TASK_5MS] == 2000, FLASH_STR("SCHED"), FLASH_STR("5 ms flag served 2000 times in 10 s"));
	TEST_voidCheck(Local_u16Count[SCHED_TASK_10MS] == 1000, FLASH_STR("SCHED"), FLASH_STR("10 ms flag served 1000 times"));
	TEST_voidCheck(Local_u16Count[SCHED_TASK_100MS] == 100, FLASH_STR("SCHED"), FLASH_STR("100 ms flag served 100 times"));
	TEST_voidCheck(Local_u16Count[SCHED_TASK_500MS] == 20, FLASH_STR("SCHED"), FLASH_STR("500 ms flag served 20 times"));
	TEST_voidCheck(Local_u16Count[SCHED_TASK_1S] == 10, FLASH_STR("SCHED"), FLASH_STR("1 s flag served 10 times"));
	TEST_voidCheck(Local_u16Overruns == 0, FLASH_STR("SCHED"), FLASH_STR("SCHED_u16GetOverruns() = 0 : no period was skipped"));
	TEST_voidInfoNum(FLASH_STR("SCHED"), FLASH_STR("Timer1 periods (20 ms) in the same window = "), Local_u16Periods, FLASH_STR(" (expected 500)"));
	TEST_voidCheck((Local_u16Periods >= (TEST_SCHED_T1_PERIODS - 1)) && (Local_u16Periods <= (TEST_SCHED_T1_PERIODS + 1)), FLASH_STR("SCHED"), FLASH_STR("10000 ticks = 10.00 s against the Timer1 stop watch"));
	TEST_voidCheck(SCHED_u8IsTaskDue(SCHED_TASK_COUNT) == 0, FLASH_STR("SCHED"), FLASH_STR("unknown task ID is never due"));
}

/*************************** TERM ***************************/
/* waits for one line ; TERM_LINE_NONE = nothing within the time */
static u8 TEST_u8WaitLine(c8 * Copy_pc8Line, u16 Copy_u16TimeoutMs){
	u32 Local_u32Start = SCHED_u32GetTickMs();
	u8 Local_u8Result ;
	Global_u8SawTyping = 0 ;
	while((SCHED_u32GetTickMs() - Local_u32Start) < Copy_u16TimeoutMs){
		TEST_voidService();
		if(SCHED_u8IsTaskDue(SCHED_TASK_5MS)){
			Local_u8Result = TERM_u8GetLine(Copy_pc8Line);
			if(Local_u8Result != TERM_LINE_NONE){
				return Local_u8Result ;
			}
			if(TERM_u8IsLineEmpty() == 0){
				Global_u8SawTyping = 1 ;
			}
		}
	}
	return TERM_LINE_NONE ;
}
static void TEST_voidTermOutput(){
	u16 Local_u16Ms = 0 ;
	u8 Local_u8Ok = 1 ;
	u8 i ;
	/*1. empty ring : the whole cfg size is free */
	while((TERM_u8TxFree() != TERM_TX_BUFFER_SIZE) && (Local_u16Ms < 1000)){
		TEST_voidWaitMs(1);
		Local_u16Ms++;
	}
	TEST_voidCheck(TERM_u8TxFree() == TERM_TX_BUFFER_SIZE, FLASH_STR("TERM"), FLASH_STR("empty TX ring : TxFree = 128 (TERM_TX_BUFFER_SIZE is real)"));
	/* the [PASS] line above went into the ring itself : let it leave , or only part of the dots would fit */
	Local_u16Ms = 0 ;
	while((TERM_u8TxFree() != TERM_TX_BUFFER_SIZE) && (Local_u16Ms < 1000)){
		TEST_voidWaitMs(1);
		Local_u16Ms++;
	}
	/*2. fill it with the interrupts off (nothing can leave) , then 10 characters too many */
	GIE_voidDisableGlobalInterrupt();
	for(i = 0 ; i < TERM_TX_BUFFER_SIZE ; i++){
		if(TERM_u8TxFree() == 0){
			Local_u8Ok = 0 ;
		}
		TERM_voidPutChar('.');
	}
	if(TERM_u8TxFree() != 0){
		Local_u8Ok = 0 ;
	}
	for(i = 0 ; i < 10 ; i++){
		TERM_voidPutChar('#');
	}
	GIE_voidEnableGlobalInterrupt();
	TEST_voidNewLine();
	TEST_voidCheck(Local_u8Ok, FLASH_STR("TERM"), FLASH_STR("128 characters fit , then TxFree = 0"));
	TEST_voidManual(FLASH_STR("TERM - the line of dots above has 128 dots and NO '#' (the overflow was dropped , nothing waited)."));
	/*3. the four output functions */
	while(TERM_u8TxFree() < 64){
		TEST_voidService();
	}
	TERM_voidPutFlash(FLASH_STR("        flash "));
	TERM_voidPutRam(TEST_pc8TextA(FLASH_STR("RAM ")));
	TERM_voidPutNumber(65535);
	TERM_voidPutChar('!');
	TERM_voidNewLine();
	TEST_voidManual(FLASH_STR("TERM - the line above reads : flash RAM 65535!"));
}
static void TEST_voidTermTyping(){
	c8 Local_c8Line[TERM_LINE_MAX + 1] ;
	u8 Local_u8Result ;
	u32 Local_u32Start ;
	u8 Local_u8Seen = 0 ;

	/*1. normal echo */
	TEST_voidManual(FLASH_STR("TERM - click the terminal , type  abc  and press Enter (20 s). You must SEE abc while typing."));
	TERM_u8IsRxActive();
	Local_u8Result = TEST_u8WaitLine(Local_c8Line,TEST_TYPE_TIMEOUT_MS);
	if(Local_u8Result == TERM_LINE_NONE){
		TEST_voidSkip(FLASH_STR("TERM"), FLASH_STR("nothing typed : echo not tested"));
	}else{
		TEST_voidCheck((Local_u8Result == TERM_LINE_READY) && TEST_u8TextEqual(Local_c8Line,FLASH_STR("abc")), FLASH_STR("TERM"), FLASH_STR("line \"abc\" received (echo normal)"));
		TEST_voidCheck(Global_u8SawTyping == 1, FLASH_STR("TERM"), FLASH_STR("IsLineEmpty = 0 while the line was being typed"));
		TEST_voidCheck(TERM_u8IsLineEmpty() == 1, FLASH_STR("TERM"), FLASH_STR("IsLineEmpty = 1 after the line"));
		TEST_voidCheck(TERM_u8IsRxActive() == 1, FLASH_STR("TERM"), FLASH_STR("IsRxActive = 1 after typing"));
		TEST_voidCheck(TERM_u8IsRxActive() == 0, FLASH_STR("TERM"), FLASH_STR("... and 0 on the next call"));
		/* a terminal that sends CR LF for Enter must still give ONE line */
		TEST_voidCheck(TEST_u8WaitLine(Local_c8Line,TEST_QUIET_MS) == TERM_LINE_NONE, FLASH_STR("TERM"), FLASH_STR("one Enter = one line (no empty second line)"));
	}

	/*2. masked echo */
	TERM_voidSetEcho(TERM_ECHO_MASKED);
	TEST_voidManual(FLASH_STR("TERM - type  1234  and press Enter (20 s). You must see **** , not the digits."));
	Local_u8Result = TEST_u8WaitLine(Local_c8Line,TEST_TYPE_TIMEOUT_MS);
	TERM_voidSetEcho(TERM_ECHO_NORMAL);
	if(Local_u8Result == TERM_LINE_NONE){
		TEST_voidSkip(FLASH_STR("TERM"), FLASH_STR("nothing typed : masked echo not tested"));
	}else{
		TEST_voidCheck((Local_u8Result == TERM_LINE_READY) && TEST_u8TextEqual(Local_c8Line,FLASH_STR("1234")), FLASH_STR("TERM"), FLASH_STR("line \"1234\" received while the screen showed ****"));
	}

	/*3. backspace */
	TEST_voidManual(FLASH_STR("TERM - type  abx  , press Backspace , type  c  , press Enter (20 s). The x must vanish."));
	Local_u8Result = TEST_u8WaitLine(Local_c8Line,TEST_TYPE_TIMEOUT_MS);
	if(Local_u8Result == TERM_LINE_NONE){
		TEST_voidSkip(FLASH_STR("TERM"), FLASH_STR("nothing typed : backspace not tested"));
	}else{
		TEST_voidCheck((Local_u8Result == TERM_LINE_READY) && TEST_u8TextEqual(Local_c8Line,FLASH_STR("abc")), FLASH_STR("TERM"), FLASH_STR("backspace removed the x : line is \"abc\""));
	}

	/*4. too long line */
	TEST_voidManual(FLASH_STR("TERM - type  12345678901234567890  (20 characters) and press Enter (20 s). Only 16 are echoed."));
	Local_u8Result = TEST_u8WaitLine(Local_c8Line,TEST_TYPE_TIMEOUT_MS);
	if(Local_u8Result == TERM_LINE_NONE){
		TEST_voidSkip(FLASH_STR("TERM"), FLASH_STR("nothing typed : too-long line not tested"));
	}else{
		TEST_voidCheck(Local_u8Result == TERM_LINE_TOO_LONG, FLASH_STR("TERM"), FLASH_STR("more than 16 characters gives TERM_LINE_TOO_LONG"));
		TEST_voidCheck((TEST_u8WaitLine(Local_c8Line,TEST_QUIET_MS) == TERM_LINE_NONE) && (TERM_u8IsLineEmpty() == 1), FLASH_STR("TERM"), FLASH_STR("the rest of that line was ignored (no second line)"));
	}

	/*5. empty line */
	TEST_voidManual(FLASH_STR("TERM - press Enter alone (20 s)."));
	Local_u8Result = TEST_u8WaitLine(Local_c8Line,TEST_TYPE_TIMEOUT_MS);
	if(Local_u8Result == TERM_LINE_NONE){
		TEST_voidSkip(FLASH_STR("TERM"), FLASH_STR("nothing typed : empty line not tested"));
	}else{
		TEST_voidCheck((Local_u8Result == TERM_LINE_READY) && (Local_c8Line[0] == '\0'), FLASH_STR("TERM"), FLASH_STR("Enter alone gives an empty line"));
	}

	/*6. LF alone ends a line */
	TEST_voidManual(FLASH_STR("TERM - type  a  then press Ctrl+J (= LF , NOT Enter) (20 s)."));
	Local_u8Result = TEST_u8WaitLine(Local_c8Line,TEST_TYPE_TIMEOUT_MS);
	if(Local_u8Result == TERM_LINE_NONE){
		TEST_voidSkip(FLASH_STR("TERM"), FLASH_STR("no line : LF not tested (the terminal may not send LF for Ctrl+J)"));
		/* drop the half line so the next step starts clean */
		TEST_voidManual(FLASH_STR("TERM - press Enter once to go on (10 s)."));
		TEST_u8WaitLine(Local_c8Line,10000);
	}else{
		TEST_voidCheck((Local_u8Result == TERM_LINE_READY) && TEST_u8TextEqual(Local_c8Line,FLASH_STR("a")), FLASH_STR("TERM"), FLASH_STR("LF ends a line : \"a\" received"));
	}

	/*7. CR then LF = one line */
	TEST_voidManual(FLASH_STR("TERM - type  b  , press Enter , then press Ctrl+J (10 s after the Enter)."));
	Local_u8Result = TEST_u8WaitLine(Local_c8Line,TEST_TYPE_TIMEOUT_MS);
	if(Local_u8Result == TERM_LINE_NONE){
		TEST_voidSkip(FLASH_STR("TERM"), FLASH_STR("nothing typed : CR LF not tested"));
	}else{
		TEST_voidCheck((Local_u8Result == TERM_LINE_READY) && TEST_u8TextEqual(Local_c8Line,FLASH_STR("b")), FLASH_STR("TERM"), FLASH_STR("CR ends a line : \"b\" received"));
		/* wait for the LF byte to arrive , then it must NOT be a line */
		TERM_u8IsRxActive();
		Local_u32Start = SCHED_u32GetTickMs();
		while(((SCHED_u32GetTickMs() - Local_u32Start) < 10000) && (Local_u8Seen == 0)){
			TEST_voidService();
			Local_u8Seen = TERM_u8IsRxActive();
		}
		if(Local_u8Seen){
			TEST_voidCheck((TEST_u8WaitLine(Local_c8Line,TEST_QUIET_MS) == TERM_LINE_NONE) && (TERM_u8IsLineEmpty() == 1), FLASH_STR("TERM"), FLASH_STR("the LF after a CR is not a second line (CR LF = one line)"));
		}else{
			TEST_voidSkip(FLASH_STR("TERM"), FLASH_STR("no Ctrl+J after the Enter : CR LF not tested"));
		}
	}
}

/*************************** ESTORE ***************************/
static u8 TEST_u8ChipByte(u16 Copy_u16Address){
	u8 Local_u8Data = 0 ;
	EXT_EEPROM_u8ReadBlock(Copy_u16Address,&Local_u8Data,1);
	return Local_u8Data ;
}
/* ms until the chip answers again after a write , 0xFFFF = not within 50 ms */
static u16 TEST_u16WaitChipReady(){
	u16 Local_u16Ms = 0 ;
	while(Local_u16Ms < TEST_EE_WAIT_MS){
		if(EXT_EEPROM_u8IsReady()){
			return Local_u16Ms ;
		}
		TEST_voidWaitMs(1);
		Local_u16Ms++;
	}
	return 0xFFFF ;
}
/* 1 = ESTORE finished all its writes within the time */
static u8 TEST_u8WaitStoreIdle(u16 Copy_u16TimeoutMs){
	u32 Local_u32Start = SCHED_u32GetTickMs();
	while(ESTORE_u8IsBusy()){
		TEST_voidService();
		if((SCHED_u32GetTickMs() - Local_u32Start) >= Copy_u16TimeoutMs){
			return 0 ;
		}
	}
	return 1 ;
}
/* writes 2 bytes to the marker page (outside the ESTORE map) and waits for the chip */
static void TEST_voidWriteMark(u8 Copy_u8Byte0, u8 Copy_u8Byte1){
	u8 Local_u8Mark[2] ;
	Local_u8Mark[0] = Copy_u8Byte0 ;
	Local_u8Mark[1] = Copy_u8Byte1 ;
	EXT_EEPROM_u8WritePage(TEST_EE_MARK_ADDR,Local_u8Mark,2);
	TEST_u16WaitChipReady();
}
static u8 TEST_u8IsListEmpty(u8 Copy_u8List){
	c8 Local_c8Name[USERDB_NAME_TEXT_SIZE] ;
	u8 i ;
	for(i = 0 ; i < USERDB_REMOTE_SLOTS ; i++){
		if(USERDB_u8GetUserName(Copy_u8List,i,Local_c8Name) == 1){
			return 0 ;
		}
	}
	return 1 ;
}

static void TEST_voidEstoreFirstBoot(){
	u8 Local_u8Page[ESTORE_PAGE_SIZE] ;
	u32 Local_u32Start ;
	u16 Local_u16Ms ;
	u8 Local_u8Guard = 0 ;
	u8 Local_u8EarlyChecked = 0 ;
	u8 Local_u8EarlyOk = 0 ;
	u8 Local_u8Ok = 1 ;
	u8 i ;
	Global_u8StoreRunning = 0 ;

	/*1. make the chip blank : page 0 (magic , version) = 0xFF */
	for(i = 0 ; i < ESTORE_PAGE_SIZE ; i++){
		Local_u8Page[i] = 0xFF ;
	}
	TEST_voidCheck(EXT_EEPROM_u8WritePage(0,Local_u8Page,ESTORE_PAGE_SIZE) == EXT_EEPROM_OK, FLASH_STR("ESTORE"), FLASH_STR("test made the chip blank (page 0 = 0xFF)"));
	Local_u16Ms = TEST_u16WaitChipReady();
	if(Local_u16Ms == 0){
		TEST_voidInfo(FLASH_STR("ESTORE"), FLASH_STR("chip ready at once after a write (simulation model has no write-cycle delay)"));
	}else if(Local_u16Ms == 0xFFFF){
		TEST_voidCheck(0, FLASH_STR("ESTORE"), FLASH_STR("chip ready again within 50 ms"));
	}else{
		TEST_voidInfoNum(FLASH_STR("ESTORE"), FLASH_STR("chip write cycle = "), Local_u16Ms, FLASH_STR(" ms"));
	}

	/*2. boot on the blank chip (REQ-EEP-02 , REQ-SEC-09 , REQ-HTR-03) */
	ESTORE_voidInit();
	USERDB_voidInit();
	TEST_voidCheck(ESTORE_u8GetStatus() == ESTORE_FIRST_BOOT, FLASH_STR("ESTORE"), FLASH_STR("blank chip : status ESTORE_FIRST_BOOT"));
	TEST_voidCheck((ESTORE_u8ReadByte(ESTORE_ADDR_MAGIC) == ESTORE_MAGIC_VALUE) && (ESTORE_u8ReadByte(ESTORE_ADDR_VERSION) == ESTORE_VERSION_VALUE), FLASH_STR("ESTORE"), FLASH_STR("defaults : magic 0xA5 , version 1"));
	TEST_voidCheck(ESTORE_u8ReadByte(ESTORE_ADDR_HEATER_SET) == ESTORE_DEFAULT_HEATER_SET, FLASH_STR("ESTORE"), FLASH_STR("defaults : heater set temperature 60"));
	TEST_voidCheck((ESTORE_u8ReadByte(0x02) == 0xFF) && (ESTORE_u8ReadByte(0x0F) == 0xFF) && (ESTORE_u8ReadByte(0x11) == 0xFF), FLASH_STR("ESTORE"), FLASH_STR("defaults : reserved bytes 0xFF"));
	TEST_voidCheck(USERDB_u8CheckAdmin(TEST_pc8TextA(FLASH_STR("admin")),TEST_pc8TextB(FLASH_STR("1234"))) == 1, FLASH_STR("ESTORE"), FLASH_STR("defaults : admin / 1234"));
	TEST_voidCheck(TEST_u8IsListEmpty(USERDB_LIST_REMOTE) && TEST_u8IsListEmpty(USERDB_LIST_KEYPAD), FLASH_STR("ESTORE"), FLASH_STR("defaults : both user lists empty"));
	TEST_voidCheck((ESTORE_u8IsBusy() == 1) && (ESTORE_u16GetPageWrites() == 0), FLASH_STR("ESTORE"), FLASH_STR("usable from RAM at once : 13 pages wait , none written yet"));

	/*3. write-behind , one ESTORE_voidUpdate every 10 ms , stepped by the test itself */
	Local_u32Start = SCHED_u32GetTickMs();
	while(ESTORE_u8IsBusy() && (Local_u8Guard < 200)){
		while(SCHED_u8IsTaskDue(SCHED_TASK_10MS) == 0){
		}
		if((ESTORE_u16GetPageWrites() == (ESTORE_PAGE_COUNT - 1)) && (Local_u8EarlyChecked == 0)){
			/* 12 pages are on the chip (the 12th was started 10 ms ago) : page 0 must still be blank */
			Local_u8EarlyChecked = 1 ;
			if(TEST_u8ChipByte(ESTORE_ADDR_MAGIC) != ESTORE_MAGIC_VALUE){
				Local_u8EarlyOk = 1 ;
			}
		}
		TEST_voidStoreStep();
		Local_u8Guard++;
	}
	Local_u16Ms = (u16)(SCHED_u32GetTickMs() - Local_u32Start);
	TEST_voidCheck((ESTORE_u8IsBusy() == 0) && (ESTORE_u16GetPageWrites() == ESTORE_PAGE_COUNT), FLASH_STR("ESTORE"), FLASH_STR("first boot wrote exactly 13 pages"));
	TEST_voidCheck(Local_u8EarlyOk, FLASH_STR("ESTORE"), FLASH_STR("after 12 pages the chip had no magic yet (page 0 is written last)"));
	TEST_voidInfoNum(FLASH_STR("ESTORE"), FLASH_STR("first boot write-behind took "), Local_u16Ms, FLASH_STR(" ms in the background"));
	TEST_u16WaitChipReady();
	TEST_voidCheck((TEST_u8ChipByte(ESTORE_ADDR_MAGIC) == ESTORE_MAGIC_VALUE) && (TEST_u8ChipByte(ESTORE_ADDR_VERSION) == ESTORE_VERSION_VALUE) && (TEST_u8ChipByte(ESTORE_ADDR_HEATER_SET) == ESTORE_DEFAULT_HEATER_SET), FLASH_STR("ESTORE"), FLASH_STR("chip now holds magic , version and set temperature 60"));
	EXT_EEPROM_u8ReadBlock(ESTORE_ADDR_ADMIN,Local_u8Page,USERDB_RECORD_SIZE);
	for(i = 0 ; i < USERDB_RECORD_SIZE ; i++){
		if(Local_u8Page[i] != TEST_ADMIN_RECORD_ARR[i]){
			Local_u8Ok = 0 ;
		}
	}
	TEST_voidCheck(Local_u8Ok, FLASH_STR("ESTORE"), FLASH_STR("chip page 2 = admin + 1234 , padded with 0x00"));
	TEST_voidCheck(TEST_u8ChipByte(ESTORE_ADDR_KEYPAD_USERS + (4 * USERDB_RECORD_SIZE)) == 0x00, FLASH_STR("ESTORE"), FLASH_STR("chip page 12 (last keypad slot) is empty"));
	Global_u8StoreRunning = 1 ;
}

static void TEST_voidEstoreWrites(){
	u16 Local_u16Before = ESTORE_u16GetPageWrites();
	/*1. unchanged byte : no write , no wear */
	ESTORE_voidWriteByte(ESTORE_ADDR_HEATER_SET,ESTORE_DEFAULT_HEATER_SET);
	TEST_voidCheck(ESTORE_u8IsBusy() == 0, FLASH_STR("ESTORE"), FLASH_STR("unchanged byte : no dirty page"));
	TEST_voidWaitMs(50);
	TEST_voidCheck(ESTORE_u16GetPageWrites() == Local_u16Before, FLASH_STR("ESTORE"), FLASH_STR("... and no page write"));

	/*2. changed byte : exactly one page (REQ-HTR-04) */
	ESTORE_voidWriteByte(ESTORE_ADDR_HEATER_SET,70);
	TEST_voidCheck((ESTORE_u8ReadByte(ESTORE_ADDR_HEATER_SET) == 70) && (ESTORE_u8IsBusy() == 1), FLASH_STR("ESTORE"), FLASH_STR("changed byte : read back from RAM at once , page waits"));
	TEST_voidCheck(TEST_u8WaitStoreIdle(500), FLASH_STR("ESTORE"), FLASH_STR("write-behind finished within 500 ms"));
	TEST_voidCheck(ESTORE_u16GetPageWrites() == (Local_u16Before + 1), FLASH_STR("ESTORE"), FLASH_STR("byte change -> exactly one page write"));
	TEST_u16WaitChipReady();
	TEST_voidCheck(TEST_u8ChipByte(ESTORE_ADDR_HEATER_SET) == 70, FLASH_STR("ESTORE"), FLASH_STR("the chip holds the new value (70)"));

	/*3. two bytes of one page : still one page write */
	Local_u16Before = ESTORE_u16GetPageWrites();
	ESTORE_voidWriteByte(TEST_EE_SPARE_A,0x11);
	ESTORE_voidWriteByte(TEST_EE_SPARE_B,0x22);
	TEST_u8WaitStoreIdle(500);
	TEST_voidCheck(ESTORE_u16GetPageWrites() == (Local_u16Before + 1), FLASH_STR("ESTORE"), FLASH_STR("two bytes in one page -> one page write"));

	/*4. a byte changed while its page is being written : the page is written once more */
	Local_u16Before = ESTORE_u16GetPageWrites();
	Global_u8StoreRunning = 0 ;
	ESTORE_voidWriteByte(TEST_EE_SPARE_A,0x33);
	ESTORE_voidUpdate();                       /* IDLE -> the page write starts */
	ESTORE_voidWriteByte(TEST_EE_SPARE_B,0x44);    /* same page , during the write cycle */
	TEST_voidCheck(ESTORE_u8IsBusy() == 1, FLASH_STR("ESTORE"), FLASH_STR("write during WRITING : the page is marked again"));
	Global_u8StoreRunning = 1 ;
	TEST_u8WaitStoreIdle(500);
	TEST_voidCheck(ESTORE_u16GetPageWrites() == (Local_u16Before + 2), FLASH_STR("ESTORE"), FLASH_STR("... and written once more (2 page writes)"));
	TEST_u16WaitChipReady();
	TEST_voidCheck((TEST_u8ChipByte(TEST_EE_SPARE_A) == 0x33) && (TEST_u8ChipByte(TEST_EE_SPARE_B) == 0x44), FLASH_STR("ESTORE"), FLASH_STR("the chip holds both bytes"));

	/*5. outside the map */
	ESTORE_voidWriteByte(ESTORE_IMAGE_SIZE,0x55);
	TEST_voidCheck((ESTORE_u8IsBusy() == 0) && (ESTORE_u8ReadByte(ESTORE_IMAGE_SIZE) == 0xFF), FLASH_STR("ESTORE"), FLASH_STR("address 0xD0 (outside the map) : write ignored , read 0xFF"));

	/*6. factory reset : defaults again , all 13 pages written */
	Local_u16Before = ESTORE_u16GetPageWrites();
	ESTORE_voidLoadDefaults();
	TEST_voidCheck((ESTORE_u8ReadByte(ESTORE_ADDR_HEATER_SET) == ESTORE_DEFAULT_HEATER_SET) && (ESTORE_u8ReadByte(TEST_EE_SPARE_A) == 0xFF), FLASH_STR("ESTORE"), FLASH_STR("LoadDefaults : image back to the defaults"));
	TEST_voidCheck(TEST_u8WaitStoreIdle(1000) && (ESTORE_u16GetPageWrites() == (Local_u16Before + ESTORE_PAGE_COUNT)), FLASH_STR("ESTORE"), FLASH_STR("LoadDefaults : 13 page writes"));

	/*7. blocking budget */
	TEST_voidInfoNum(FLASH_STR("ESTORE"), FLASH_STR("longest ESTORE_voidUpdate call = "), (u16)(((u32)Global_u16MaxUpdateTicks * 1000UL) / TIMER1_TICKS_PER_MS), FLASH_STR(" us"));
	TEST_voidCheck(Global_u16MaxUpdateTicks < (u16)(((u32)TEST_UPDATE_LIMIT_US * TIMER1_TICKS_PER_MS) / 1000UL), FLASH_STR("ESTORE"), FLASH_STR("no ESTORE_voidUpdate call longer than 2.5 ms (REQ-EEP-03)"));
}

/* MANUAL : the SDA switch "removes" the chip. Run last , ESTORE stays in FAULT until the next init. */
static void TEST_voidEstoreFault(){
	u32 Local_u32Start = SCHED_u32GetTickMs();
	u8 Local_u8Event = 0 ;
	u8 Local_u8Arg = 0 ;
	u8 Local_u8Seen = 0 ;
	u8 Local_u8Ready = 0 ;
	/* empty the event queue : only the fault event must be in it afterwards */
	while(EVQ_u8Get(&Local_u8Event,&Local_u8Arg)){
	}
	TEST_voidManual(FLASH_STR("ESTORE - within 15 s CLOSE the SDA-to-GND switch (the EEPROM is \"removed\")."));
	while(((SCHED_u32GetTickMs() - Local_u32Start) < TEST_FAULT_TIMEOUT_MS) && (Local_u8Seen == 0)){
		TEST_voidWaitMs(100);
		if(EXT_EEPROM_u8IsReady() == 0){
			Local_u8Seen = 1 ;
		}
	}
	if(Local_u8Seen == 0){
		TEST_voidSkip(FLASH_STR("ESTORE"), FLASH_STR("SDA was not held low , fault not tested"));
		return ;
	}
	/*1. fault while running : one byte changes , its page write fails */
	ESTORE_voidWriteByte(TEST_EE_SPARE_B,0x00);
	TEST_voidWaitMs(200);
	TEST_voidCheck(ESTORE_u8GetStatus() == ESTORE_FAULT, FLASH_STR("ESTORE"), FLASH_STR("chip gone : status ESTORE_FAULT"));
	TEST_voidCheck((EVQ_u8Get(&Local_u8Event,&Local_u8Arg) == 1) && (Local_u8Event == EVQ_STORAGE_FAULT), FLASH_STR("ESTORE"), FLASH_STR("EVQ_STORAGE_FAULT posted"));
	TEST_voidCheck(EVQ_u8Get(&Local_u8Event,&Local_u8Arg) == 0, FLASH_STR("ESTORE"), FLASH_STR("... exactly once"));
	TEST_voidCheck((ESTORE_u8ReadByte(ESTORE_ADDR_HEATER_SET) == TEST_HEATER_SET_VALUE) && (ESTORE_u8ReadByte(TEST_EE_SPARE_B) == 0x00), FLASH_STR("ESTORE"), FLASH_STR("reads and writes still work on the RAM image"));
	TEST_voidCheck(ESTORE_u8IsBusy() == 0, FLASH_STR("ESTORE"), FLASH_STR("IsBusy = 0 in FAULT (nobody waits for ever)"));
	/*2. boot without a chip : defaults in RAM only */
	ESTORE_voidInit();
	TEST_voidCheck(ESTORE_u8GetStatus() == ESTORE_FAULT, FLASH_STR("ESTORE"), FLASH_STR("init without a chip : status ESTORE_FAULT"));
	TEST_voidCheck((ESTORE_u8ReadByte(ESTORE_ADDR_HEATER_SET) == ESTORE_DEFAULT_HEATER_SET) && (ESTORE_u8IsBusy() == 0), FLASH_STR("ESTORE"), FLASH_STR("... defaults in RAM , nothing waits to be written"));
	/*3. chip back */
	TEST_voidManual(FLASH_STR("ESTORE - now OPEN the switch again (15 s)."));
	Local_u32Start = SCHED_u32GetTickMs();
	while(((SCHED_u32GetTickMs() - Local_u32Start) < TEST_FAULT_TIMEOUT_MS) && (Local_u8Ready == 0)){
		TEST_voidWaitMs(100);
		Local_u8Ready = EXT_EEPROM_u8IsReady();
	}
	TEST_voidCheck(Local_u8Ready == 1, FLASH_STR("ESTORE"), FLASH_STR("the chip answers again after SDA is released"));
	ESTORE_voidInit();
	TEST_voidCheck((ESTORE_u8GetStatus() == ESTORE_OK) && (ESTORE_u8ReadByte(ESTORE_ADDR_HEATER_SET) == TEST_HEATER_SET_VALUE), FLASH_STR("ESTORE"), FLASH_STR("next init : status ESTORE_OK , set temperature 65 read from the chip"));
}

/*************************** USERDB ***************************/
static u8 TEST_u8Add(u8 Copy_u8List, const __flash c8 * Copy_pc8Name, const __flash c8 * Copy_pc8Pass){
	return USERDB_u8AddUser(Copy_u8List,TEST_pc8TextA(Copy_pc8Name),TEST_pc8TextB(Copy_pc8Pass)) ;
}
static u8 TEST_u8Login(u8 Copy_u8List, const __flash c8 * Copy_pc8Name, const __flash c8 * Copy_pc8Pass){
	return USERDB_u8CheckUser(Copy_u8List,TEST_pc8TextA(Copy_pc8Name),TEST_pc8TextB(Copy_pc8Pass)) ;
}
static u8 TEST_u8Admin(const __flash c8 * Copy_pc8Name, const __flash c8 * Copy_pc8Pass){
	return USERDB_u8CheckAdmin(TEST_pc8TextA(Copy_pc8Name),TEST_pc8TextB(Copy_pc8Pass)) ;
}
static u8 TEST_u8Remove(u8 Copy_u8List, const __flash c8 * Copy_pc8Name){
	return USERDB_u8RemoveUser(Copy_u8List,TEST_pc8TextA(Copy_pc8Name)) ;
}
static u8 TEST_u8SetPass(const __flash c8 * Copy_pc8Pass){
	return USERDB_u8SetAdminPassword(TEST_pc8TextA(Copy_pc8Pass)) ;
}
static void TEST_voidUserdb(){
	c8 Local_c8Name[USERDB_NAME_TEXT_SIZE] ;
	u16 Local_u16Before = ESTORE_u16GetPageWrites();
	USERDB_voidInit();

	/*1. write gate closed by default (REQ-SEC-07) */
	TEST_voidCheck(TEST_u8Add(USERDB_LIST_REMOTE,FLASH_STR("ali"),FLASH_STR("pass1")) == USERDB_ERR_READ_ONLY, FLASH_STR("USERDB"), FLASH_STR("write gate closed : AddUser gives USERDB_ERR_READ_ONLY"));
	TEST_voidCheck((TEST_u8Remove(USERDB_LIST_REMOTE,FLASH_STR("ali")) == USERDB_ERR_READ_ONLY) && (TEST_u8SetPass(FLASH_STR("abcd1234")) == USERDB_ERR_READ_ONLY), FLASH_STR("USERDB"), FLASH_STR("write gate closed : RemoveUser and SetAdminPassword too"));
	TEST_voidCheck(ESTORE_u8IsBusy() == 0, FLASH_STR("USERDB"), FLASH_STR("... and nothing was changed"));
	/* D-12 : the heater set temperature is not behind the gate (REQ-HTR-14) */
	ESTORE_voidWriteByte(ESTORE_ADDR_HEATER_SET,TEST_HEATER_SET_VALUE);
	TEST_voidCheck(ESTORE_u8ReadByte(ESTORE_ADDR_HEATER_SET) == TEST_HEATER_SET_VALUE, FLASH_STR("USERDB"), FLASH_STR("gate closed : heater set temperature can still be saved (65)"));

	/*2. admin login */
	TEST_voidCheck(TEST_u8Admin(FLASH_STR("admin"),FLASH_STR("1234")) == 1, FLASH_STR("USERDB"), FLASH_STR("admin / 1234 accepted"));
	TEST_voidCheck((TEST_u8Admin(FLASH_STR("admin"),FLASH_STR("1235")) == 0) && (TEST_u8Admin(FLASH_STR("admin"),FLASH_STR("12345")) == 0) && (TEST_u8Admin(FLASH_STR("admin"),FLASH_STR("123")) == 0), FLASH_STR("USERDB"), FLASH_STR("admin with a wrong , longer or shorter password rejected"));
	TEST_voidCheck((TEST_u8Admin(FLASH_STR("admi"),FLASH_STR("1234")) == 0) && (TEST_u8Admin(FLASH_STR("admin1"),FLASH_STR("1234")) == 0) && (TEST_u8Admin(FLASH_STR("Admin"),FLASH_STR("1234")) == 0), FLASH_STR("USERDB"), FLASH_STR("wrong admin name rejected (case sensitive)"));

	/*3. add (REQ-SEC-03) */
	USERDB_voidSetWriteAccess(1);
	TEST_voidCheck(TEST_u8Add(USERDB_LIST_REMOTE,FLASH_STR("ali"),FLASH_STR("pass1")) == USERDB_OK, FLASH_STR("USERDB"), FLASH_STR("gate open : remote user ali / pass1 added"));
	TEST_voidCheck(TEST_u8Login(USERDB_LIST_REMOTE,FLASH_STR("ali"),FLASH_STR("pass1")) == 1, FLASH_STR("USERDB"), FLASH_STR("ali / pass1 accepted"));
	TEST_voidCheck((TEST_u8Login(USERDB_LIST_REMOTE,FLASH_STR("ali"),FLASH_STR("pass2")) == 0) && (TEST_u8Login(USERDB_LIST_REMOTE,FLASH_STR("al"),FLASH_STR("pass1")) == 0), FLASH_STR("USERDB"), FLASH_STR("wrong password or name rejected"));
	TEST_voidCheck(TEST_u8Add(USERDB_LIST_REMOTE,FLASH_STR("ali"),FLASH_STR("other")) == USERDB_ERR_EXISTS, FLASH_STR("USERDB"), FLASH_STR("duplicate name : USERDB_ERR_EXISTS"));
	TEST_voidCheck(TEST_u8Add(USERDB_LIST_REMOTE,FLASH_STR("admin"),FLASH_STR("abcd")) == USERDB_ERR_EXISTS, FLASH_STR("USERDB"), FLASH_STR("remote user may not take the admin's name"));

	/*4. bad name , bad password */
	TEST_voidCheck((TEST_u8Add(USERDB_LIST_REMOTE,FLASH_STR(""),FLASH_STR("pass1")) == USERDB_ERR_BAD_NAME) && (TEST_u8Add(USERDB_LIST_REMOTE,FLASH_STR("ninechars"),FLASH_STR("pass1")) == USERDB_ERR_BAD_NAME), FLASH_STR("USERDB"), FLASH_STR("bad name : empty or 9 characters"));
	TEST_voidCheck(TEST_u8Add(USERDB_LIST_REMOTE,FLASH_STR("a b"),FLASH_STR("pass1")) == USERDB_ERR_BAD_NAME, FLASH_STR("USERDB"), FLASH_STR("bad name : a space inside"));
	TEST_voidCheck((TEST_u8Add(USERDB_LIST_REMOTE,FLASH_STR("omar"),FLASH_STR("abc")) == USERDB_ERR_BAD_PASS) && (TEST_u8Add(USERDB_LIST_REMOTE,FLASH_STR("omar"),FLASH_STR("123456789")) == USERDB_ERR_BAD_PASS), FLASH_STR("USERDB"), FLASH_STR("bad password : 3 or 9 characters"));

	/*5. 8 + 8 characters : stored without a terminator */
	TEST_voidCheck(TEST_u8Add(USERDB_LIST_REMOTE,FLASH_STR("abcdefgh"),FLASH_STR("12345678")) == USERDB_OK, FLASH_STR("USERDB"), FLASH_STR("8-character name and password added"));
	TEST_voidCheck(TEST_u8Login(USERDB_LIST_REMOTE,FLASH_STR("abcdefgh"),FLASH_STR("12345678")) == 1, FLASH_STR("USERDB"), FLASH_STR("... and accepted"));
	TEST_voidCheck((TEST_u8Login(USERDB_LIST_REMOTE,FLASH_STR("abcdefg"),FLASH_STR("12345678")) == 0) && (TEST_u8Login(USERDB_LIST_REMOTE,FLASH_STR("abcdefgh"),FLASH_STR("1234567")) == 0) && (TEST_u8Login(USERDB_LIST_REMOTE,FLASH_STR("abcdefghi"),FLASH_STR("12345678")) == 0), FLASH_STR("USERDB"), FLASH_STR("7 or 9 characters of it rejected"));
	TEST_voidCheck((USERDB_u8GetUserName(USERDB_LIST_REMOTE,1,Local_c8Name) == 1) && TEST_u8TextEqual(Local_c8Name,FLASH_STR("abcdefgh")), FLASH_STR("USERDB"), FLASH_STR("GetUserName(slot 1) = \"abcdefgh\" with a terminator"));

	/*6. keypad list : digits only , separate from the remote list (REQ-SEC-02) */
	TEST_voidCheck(TEST_u8Add(USERDB_LIST_KEYPAD,FLASH_STR("12a4"),FLASH_STR("5678")) == USERDB_ERR_BAD_NAME, FLASH_STR("USERDB"), FLASH_STR("keypad user : a letter in the ID gives USERDB_ERR_BAD_NAME"));
	TEST_voidCheck(TEST_u8Add(USERDB_LIST_KEYPAD,FLASH_STR("1234"),FLASH_STR("56a8")) == USERDB_ERR_BAD_PASS, FLASH_STR("USERDB"), FLASH_STR("keypad user : a letter in the PIN gives USERDB_ERR_BAD_PASS"));
	TEST_voidCheck(TEST_u8Add(USERDB_LIST_KEYPAD,FLASH_STR("1234"),FLASH_STR("5678")) == USERDB_OK, FLASH_STR("USERDB"), FLASH_STR("keypad user 1234 / 5678 added"));
	TEST_voidCheck(TEST_u8Login(USERDB_LIST_KEYPAD,FLASH_STR("1234"),FLASH_STR("5678")) == 1, FLASH_STR("USERDB"), FLASH_STR("1234 / 5678 accepted on the keypad list"));
	TEST_voidCheck((TEST_u8Login(USERDB_LIST_REMOTE,FLASH_STR("1234"),FLASH_STR("5678")) == 0) && (TEST_u8Login(USERDB_LIST_KEYPAD,FLASH_STR("ali"),FLASH_STR("pass1")) == 0), FLASH_STR("USERDB"), FLASH_STR("the two lists are separate"));
	TEST_voidCheck((TEST_u8Login(USERDB_LIST_KEYPAD,FLASH_STR(""),FLASH_STR("")) == 0) && (TEST_u8Login(USERDB_LIST_REMOTE,FLASH_STR(""),FLASH_STR("")) == 0), FLASH_STR("USERDB"), FLASH_STR("empty name + empty password never matches an empty slot"));

	/*7. full */
	TEST_voidCheck((TEST_u8Add(USERDB_LIST_REMOTE,FLASH_STR("u3"),FLASH_STR("pass3")) == USERDB_OK) && (TEST_u8Add(USERDB_LIST_REMOTE,FLASH_STR("u4"),FLASH_STR("pass4")) == USERDB_OK) && (TEST_u8Add(USERDB_LIST_REMOTE,FLASH_STR("u5"),FLASH_STR("pass5")) == USERDB_OK), FLASH_STR("USERDB"), FLASH_STR("remote users 3 , 4 and 5 added"));
	TEST_voidCheck(TEST_u8Add(USERDB_LIST_REMOTE,FLASH_STR("u6"),FLASH_STR("pass6")) == USERDB_ERR_FULL, FLASH_STR("USERDB"), FLASH_STR("6th remote user : USERDB_ERR_FULL"));

	/*8. remove */
	TEST_voidCheck(TEST_u8Remove(USERDB_LIST_REMOTE,FLASH_STR("u3")) == USERDB_OK, FLASH_STR("USERDB"), FLASH_STR("user u3 removed"));
	TEST_voidCheck(TEST_u8Login(USERDB_LIST_REMOTE,FLASH_STR("u3"),FLASH_STR("pass3")) == 0, FLASH_STR("USERDB"), FLASH_STR("u3 can not log in any more"));
	TEST_voidCheck(TEST_u8Remove(USERDB_LIST_REMOTE,FLASH_STR("u3")) == USERDB_ERR_NOT_FOUND, FLASH_STR("USERDB"), FLASH_STR("removing u3 again : USERDB_ERR_NOT_FOUND"));
	TEST_voidCheck((TEST_u8Add(USERDB_LIST_REMOTE,FLASH_STR("u6"),FLASH_STR("pass6")) == USERDB_OK) && (USERDB_u8GetUserName(USERDB_LIST_REMOTE,2,Local_c8Name) == 1) && TEST_u8TextEqual(Local_c8Name,FLASH_STR("u6")), FLASH_STR("USERDB"), FLASH_STR("the freed slot 2 is used again (u6)"));

	/*9. admin password */
	TEST_voidCheck(TEST_u8SetPass(FLASH_STR("abc")) == USERDB_ERR_BAD_PASS, FLASH_STR("USERDB"), FLASH_STR("admin password of 3 characters : USERDB_ERR_BAD_PASS"));
	TEST_voidCheck((TEST_u8SetPass(FLASH_STR("abcd1234")) == USERDB_OK) && (TEST_u8Admin(FLASH_STR("admin"),FLASH_STR("abcd1234")) == 1) && (TEST_u8Admin(FLASH_STR("admin"),FLASH_STR("1234")) == 0), FLASH_STR("USERDB"), FLASH_STR("admin password changed : new one accepted , old one rejected"));

	/*10. repair : an admin record without a name gets the default back (REQ-SEC-09) */
	ESTORE_voidWriteByte(ESTORE_ADDR_ADMIN,0xFF);
	USERDB_voidInit();
	TEST_voidCheck(TEST_u8Admin(FLASH_STR("admin"),FLASH_STR("1234")) == 1, FLASH_STR("USERDB"), FLASH_STR("invalid admin record repaired at init : admin / 1234"));
	TEST_voidCheck(TEST_u8Add(USERDB_LIST_REMOTE,FLASH_STR("omar"),FLASH_STR("pass7")) == USERDB_ERR_READ_ONLY, FLASH_STR("USERDB"), FLASH_STR("init closes the write gate again"));

	/*11. tidy up : keep ali and keypad user 1234 for the RESET check */
	USERDB_voidSetWriteAccess(1);
	TEST_u8Remove(USERDB_LIST_REMOTE,FLASH_STR("abcdefgh"));
	TEST_u8Remove(USERDB_LIST_REMOTE,FLASH_STR("u4"));
	TEST_u8Remove(USERDB_LIST_REMOTE,FLASH_STR("u5"));
	TEST_u8Remove(USERDB_LIST_REMOTE,FLASH_STR("u6"));
	USERDB_voidSetWriteAccess(0);
	TEST_voidCheck(TEST_u8WaitStoreIdle(2000), FLASH_STR("USERDB"), FLASH_STR("all account changes written to the chip within 2 s (REQ-SEC-04)"));
	TEST_voidInfoNum(FLASH_STR("USERDB"), FLASH_STR("page writes used by this test = "), ESTORE_u16GetPageWrites() - Local_u16Before, FLASH_STR(""));
	TEST_u16WaitChipReady();
	TEST_voidCheck((TEST_u8ChipByte(ESTORE_ADDR_REMOTE_USERS) == 'a') && (TEST_u8ChipByte(ESTORE_ADDR_KEYPAD_USERS) == '1') && (TEST_u8ChipByte(ESTORE_ADDR_REMOTE_USERS + USERDB_RECORD_SIZE) == 0x00), FLASH_STR("USERDB"), FLASH_STR("chip : remote slot 0 = ali , keypad slot 0 = 1234 , remote slot 1 empty"));
	TEST_voidCheck(ESTORE_u8GetStatus() != ESTORE_FAULT, FLASH_STR("USERDB"), FLASH_STR("no storage fault during the test"));
}

/*************************** second run (after RESET) ***************************/
static void TEST_voidAfterReset(){
	TEST_voidPrintLine(FLASH_STR("===== test_service : run 2 , after RESET (data must have survived) ====="));
	ESTORE_voidInit();
	USERDB_voidInit();
	Global_u8StoreRunning = 1 ;
	TEST_voidCheck(ESTORE_u8GetStatus() == ESTORE_OK, FLASH_STR("ESTORE"), FLASH_STR("after RESET : status ESTORE_OK (no second first boot)"));
	TEST_voidCheck(ESTORE_u8ReadByte(ESTORE_ADDR_HEATER_SET) == TEST_HEATER_SET_VALUE, FLASH_STR("ESTORE"), FLASH_STR("after RESET : heater set temperature 65 kept (REQ-HTR-04)"));
	TEST_voidCheck(TEST_u8Admin(FLASH_STR("admin"),FLASH_STR("1234")) == 1, FLASH_STR("USERDB"), FLASH_STR("after RESET : admin / 1234 kept (REQ-SEC-04)"));
	TEST_voidCheck(TEST_u8Login(USERDB_LIST_REMOTE,FLASH_STR("ali"),FLASH_STR("pass1")) == 1, FLASH_STR("USERDB"), FLASH_STR("after RESET : remote user ali / pass1 kept"));
	TEST_voidCheck(TEST_u8Login(USERDB_LIST_KEYPAD,FLASH_STR("1234"),FLASH_STR("5678")) == 1, FLASH_STR("USERDB"), FLASH_STR("after RESET : keypad user 1234 / 5678 kept"));
	TEST_voidCheck(TEST_u8Login(USERDB_LIST_REMOTE,FLASH_STR("u4"),FLASH_STR("pass4")) == 0, FLASH_STR("USERDB"), FLASH_STR("after RESET : removed user u4 is still gone"));
	TEST_voidWaitMs(500);
	TEST_voidCheck((ESTORE_u16GetPageWrites() == 0) && (ESTORE_u8IsBusy() == 0), FLASH_STR("ESTORE"), FLASH_STR("a normal boot writes nothing to the chip"));
	/* the marker goes : the next RESET starts the full test again */
	TEST_voidWriteMark(0xFF,0xFF);
}

static void TEST_voidSummary(const __flash c8 * Copy_pc8Run){
	TEST_voidPrint(FLASH_STR("SUMMARY "));
	TEST_voidPrint(Copy_pc8Run);
	TEST_voidPrint(FLASH_STR(": "));
	TEST_voidPrintNum(Global_u8PassCount);
	TEST_voidPrint(FLASH_STR(" passed , "));
	TEST_voidPrintNum(Global_u8FailCount);
	TEST_voidPrint(FLASH_STR(" failed , "));
	TEST_voidPrintNum(Global_u8SkipCount);
	TEST_voidPrintLine(FLASH_STR(" skipped"));
}

int main(void){
	u8 Local_u8Mark[2] = {0,0} ;
	/*1. Timer1 = stop watch , SCHED = time base , blocking UART for the first part */
	DIO_voidSetPinDirection(TEST_LED_PORT,TEST_LED_PIN,DIO_PIN_OUTPUT);
	USART_voidInit();
	TIMER1_voidInit();
	EVQ_voidInit();
	SCHED_voidInit();
	GIE_voidEnableGlobalInterrupt();
	SCHED_voidStart();
	EXT_EEPROM_voidInit();

	/*2. did the last run end with "press RESET" ? then this is the short second run */
	if((EXT_EEPROM_u8ReadBlock(TEST_EE_MARK_ADDR,Local_u8Mark,2) == EXT_EEPROM_OK) && (Local_u8Mark[0] == TEST_EE_MARK_0) && (Local_u8Mark[1] == TEST_EE_MARK_1)){
		TERM_voidInit();
		Global_u8UseTerm = 1 ;
		TEST_voidNewLine();
		TEST_voidAfterReset();
		TEST_voidSummary(FLASH_STR("run 2 (after RESET)"));
		TEST_voidPrintLine(FLASH_STR("MANUAL: press RESET once more for the full test."));
	}else{
		/*3. part 1 : no terminal needed , printed with the blocking USART functions */
		TEST_voidNewLine();
		TEST_voidPrintLine(FLASH_STR("===== test_service : Phase 2 test of the SERVICE layer ====="));
		TEST_voidPrintLine(FLASH_STR("Status LED: slow blink = running , solid = all passed , fast blink = a check failed"));
		TEST_voidRingbuf();
		TEST_voidMavg();
		TEST_voidFmt();
		TEST_voidEvq();
		TEST_voidSched();

		/*4. part 2 : everything through TERM (the last blocking byte leaves first) */
		TEST_voidWaitMs(5);
		TERM_voidInit();
		Global_u8UseTerm = 1 ;
		TEST_voidPrintLine(FLASH_STR("       --- from here on all output goes through TERM (TX ring + interrupt) ---"));
		TEST_voidTermOutput();
		TEST_voidTermTyping();
		TEST_voidEstoreFirstBoot();
		TEST_voidEstoreWrites();
		TEST_voidUserdb();
		/* marker first (the bus still works) , the fault step is the last one */
		TEST_voidWriteMark(TEST_EE_MARK_0,TEST_EE_MARK_1);
		TEST_voidEstoreFault();
		TEST_voidSummary(FLASH_STR("run 1 (full test)"));
		TEST_voidPrintLine(FLASH_STR("MANUAL: now press the RESET button of the ATmega32 : the next run checks that the EEPROM data survived."));
	}

	/*5. final state on the status LED */
	if(Global_u8FailCount == 0){
		DIO_voidSetPinValue(TEST_LED_PORT,TEST_LED_PIN,DIO_PIN_HIGH);
	}
	while(1){
		if((Global_u8FailCount != 0) && SCHED_u8IsTaskDue(SCHED_TASK_100MS)){
			DIO_voidTogPin(TEST_LED_PORT,TEST_LED_PIN);
		}
	}
	return 0 ;
}
