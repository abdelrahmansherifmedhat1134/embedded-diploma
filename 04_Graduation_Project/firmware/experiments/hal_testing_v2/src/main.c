/*
 * main.c  (hal_testing_v2 : 7-segment on two PCF8574 chips)
 *
 *  Created on: Oct 3, 2026
 *
 * =====================================================================
 *  Finds the exact problem of the I2C 7-segment display (D-20)
 * =====================================================================
 *  Build : pio run
 *  Hex   : .pio/build/seven_seg_i2c_test/firmware.hex
 *
 *  The I2C bus is the same as in project_v1 : LCD (0x27) and the 5 lamps
 *  (0x20) are on it too , so bus sharing is tested as well.
 *  The test runs ONCE , then shows the summary. Every failing check prints
 *  the values it saw (wrote = 0x.. read = 0x..) so the exact bit is known.
 *
 *   T0  bus scan             : which addresses 0x20..0x27 and 0x50 answer
 *   T1  raw chip port        : 22 patterns per chip (0xFF , 0x00 , 0xAA , 0x55 ,
 *                              walking 0 , walking 1) written and read back.
 *                              A bit that fails here = wiring / Proteus model problem
 *   T2  raw digit patterns   : the 10 digit patterns written straight to each chip
 *   T3  driver SetNumber     : all 100 numbers 00..99, both chips read back
 *   T4  driver behaviour     : init blank , Enable , Disable , clipping , same number twice
 *   T5  bus sharing          : lamps and LCD written between 7-segment writes ,
 *                              nothing may change on the other chips
 *   T6  stress               : 300 mixed writes (7-segment + lamps + LCD)
 *       timing               : time of SEVEN_SEG_voidSetNumber
 *   T7  looks (manual)       : all segments , one segment at a time on each digit ,
 *                              digits 0..9 , 37 , counting 00..99 , blank
 *
 *  How to read a failure :
 *   T0 fails            --> chip not on the bus / wrong address pins
 *   T1 fails on some bits only (e.g. read 0x7F after writing 0xFF)
 *                        --> that pin is pulled low by the circuit (segment wired to GND ,
 *                            short , or a display part that loads the pin)
 *   T1 passes , T2 fails --> table / test mismatch (should not happen)
 *   T1/T2 pass , T3 fails--> driver bug : the printed numbers show which
 *   T1..T3 pass but the display looks wrong (T7) --> segment-to-pin wiring order
 *
 * ---------------------- Proteus parts and wiring ----------------------
 *  ATMEGA32          Clock Frequency = 16 MHz (must equal F_CPU)
 *  Status LED        PA3 -> 330R -> LED -> GND  (blink = running , solid = all passed , fast = failure)
 *  VIRTUAL TERMINAL  9600 baud , 8N1 : terminal RXD <- PD1 (TXD)
 *  I2C bus           PC0 = SCL , PC1 = SDA , 4.7k pull-up to +5 V on both lines
 *  PCF8574 0x27      LCD (A2 A1 A0 = 1 1 1) , P0 = RS , P1 = RW , P2 = E , P4..P7 = D4..D7 , LM016L
 *  PCF8574 0x20      lamps (A2 A1 A0 = 0 0 0) , P0..P4 -> LEDs (+5 V -> 220R -> LED -> pin)
 *  PCF8574 0x21      TENS digit  (A2 A1 A0 = 0 0 1) , P0..P6 = segments a..g , P7 = dp
 *  PCF8574 0x22      UNITS digit (A2 A1 A0 = 0 1 0) , same wiring
 *  2 x 7SEG-COM-ANODE  common pin on +5 V , each segment pin -> 220R -> its PCF8574 pin
 *                      (a segment lights when its pin is LOW)
 * =====================================================================
 */
#include <util/delay.h>
#include "../lib/Service/std_types.h"
#include "../lib/Service/Bit_math.h"
#include "../lib/MCAL/reg_def.h"
#include "../lib/MCAL/DIO/DIO.h"
#include "../lib/MCAL/TIMER1/TIMER1.h"
#include "../lib/MCAL/TIMER1/TIMER1_cfg.h"
#include "../lib/MCAL/USART/USART.h"
#include "../lib/MCAL/TWI/TWI.h"
#include "../lib/HAL/PCF8574/PCF8574.h"
#include "../lib/HAL/CLCD/CLCD.h"
#include "../lib/HAL/LAMP/LAMP.h"
#include "../lib/HAL/LAMP/LAMP_cfg.h"
#include "../lib/HAL/SEVEN_SEG/SEVEN_SEG.h"
#include "../lib/HAL/SEVEN_SEG/SEVEN_SEG_cfg.h"

/* Keep text in flash , not RAM (2 KB RAM only). */
#define FLASH_STR(str)    (__extension__({ static const __flash c8 Local_c8Text[] = (str); &Local_c8Text[0]; }))

#define TEST_LED_PORT             DIO_PORTA
#define TEST_LED_PIN              DIO_PIN_3
#define TEST_FAIL_BLINK_MS        100
#define TEST_MAX_DETAILS          8            /* printed mismatch lines per check */
#define TEST_LCD_ADDRESS          0x27

static u8 Global_u8PassCount = 0 ;
static u8 Global_u8FailCount = 0 ;
static u8 Global_u8BlinkMs = 0 ;
static u8 Global_u8Details = 0 ;               /* mismatch lines already printed for the current check */

/* independent digit table : common anode , bit0 = a .. bit6 = g , bit7 = dp , bit = 0 lights the segment */
static const __flash u8 TEST_EXPECTED[10] = {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90};

/*************************** print helpers ***************************/
static void TEST_voidPrint(const __flash c8 * Copy_pc8Text){
	while(*Copy_pc8Text != '\0'){
		USART_voidSend(*Copy_pc8Text++);
	}
}
static void TEST_voidPrintLine(const __flash c8 * Copy_pc8Text){
	TEST_voidPrint(Copy_pc8Text);
	USART_voidSend('\r');
	USART_voidSend('\n');
}
static void TEST_voidNewLine(){
	USART_voidSend('\r');
	USART_voidSend('\n');
}
static void TEST_voidPrintNum(u16 Copy_u16Num){
	c8 Local_c8Digits[5];
	u8 Local_u8Count = 0 ;
	do{
		Local_c8Digits[Local_u8Count++] = '0' + (Copy_u16Num % 10);
		Copy_u16Num /= 10 ;
	}while(Copy_u16Num != 0);
	while(Local_u8Count > 0){
		USART_voidSend(Local_c8Digits[--Local_u8Count]);
	}
}
static void TEST_voidPrintHex(u8 Copy_u8Value){
	u8 Local_u8High = Copy_u8Value >> 4 ;
	u8 Local_u8Low  = Copy_u8Value & 0x0F ;
	USART_voidSend('0');
	USART_voidSend('x');
	USART_voidSend(Local_u8High < 10 ? ('0' + Local_u8High) : ('A' + Local_u8High - 10));
	USART_voidSend(Local_u8Low  < 10 ? ('0' + Local_u8Low)  : ('A' + Local_u8Low  - 10));
}
static void TEST_voidCheck(u8 Copy_u8Ok, const __flash c8 * Copy_pc8What){
	if(Copy_u8Ok){
		TEST_voidPrint(FLASH_STR("[PASS] "));
		Global_u8PassCount++;
	}else{
		TEST_voidPrint(FLASH_STR("[FAIL] "));
		Global_u8FailCount++;
	}
	TEST_voidPrintLine(Copy_pc8What);
	Global_u8Details = 0 ;
}
static void TEST_voidInfo(const __flash c8 * Copy_pc8Text){
	TEST_voidPrint(FLASH_STR("[INFO] "));
	TEST_voidPrintLine(Copy_pc8Text);
}
static void TEST_voidManual(const __flash c8 * Copy_pc8Text){
	TEST_voidPrint(FLASH_STR("MANUAL: "));
	TEST_voidPrintLine(Copy_pc8Text);
}
/* "       chip 0x21 : wrote 0x.. , read 0x.." (only the first TEST_MAX_DETAILS per check) */
static void TEST_voidDetail(u8 Copy_u8Address, u8 Copy_u8Wrote, u8 Copy_u8Read){
	if(Global_u8Details < TEST_MAX_DETAILS){
		TEST_voidPrint(FLASH_STR("       chip "));
		TEST_voidPrintHex(Copy_u8Address);
		TEST_voidPrint(FLASH_STR(" : expected "));
		TEST_voidPrintHex(Copy_u8Wrote);
		TEST_voidPrint(FLASH_STR(" , read "));
		TEST_voidPrintHex(Copy_u8Read);
		TEST_voidPrint(FLASH_STR(" , wrong bits "));
		TEST_voidPrintHex(Copy_u8Wrote ^ Copy_u8Read);
		TEST_voidNewLine();
	}
	Global_u8Details++;
}

/*************************** time / LCD helpers ***************************/
static void TEST_voidWaitMs(u16 Copy_u16Ms){
	while(Copy_u16Ms > 0){
		_delay_ms(1);
		Copy_u16Ms--;
		Global_u8BlinkMs++;
		if(Global_u8BlinkMs >= 250){
			Global_u8BlinkMs = 0 ;
			DIO_voidTogPin(TEST_LED_PORT,TEST_LED_PIN);
		}
	}
}
/* 16 bit stop watch : Timer1 counts 0 .. TOP (1 tick = 0.5 us at 16 MHz) */
static u16 TEST_u16ReadTCNT1(){
	u16 Local_u16Value = TCNT1L ;
	Local_u16Value |= ((u16)TCNT1H << 8) ;
	return Local_u16Value ;
}
static u16 TEST_u16TicksSince(u16 Copy_u16Start){
	u16 Local_u16Now = TEST_u16ReadTCNT1();
	if(Local_u16Now >= Copy_u16Start){
		return Local_u16Now - Copy_u16Start ;
	}
	return (u16)((u16)(TIMER1_TOP_VALUE + 1) - Copy_u16Start + Local_u16Now) ;
}
/* second LCD line = what the test is doing now (blocking is fine here) */
static void TEST_voidLcdStep(const __flash c8 * Copy_pc8Text){
	CLCD_voidSetCursorPosition(0,1);
	CLCD_voidSendFlashString(FLASH_STR("                "));
	CLCD_voidSetCursorPosition(0,1);
	CLCD_voidSendFlashString(Copy_pc8Text);
}

/*************************** raw chip helpers ***************************/
/* write a value to a chip and read the port back : 1 = equal */
static u8 TEST_u8Raw(u8 Copy_u8Address, u8 Copy_u8Value){
	u8 Local_u8Read = 0 ;
	u8 Local_u8Ok = 0 ;
	if(PCF8574_u8WritePort(Copy_u8Address,Copy_u8Value) == TWI_OK){
		if(PCF8574_u8ReadPort(Copy_u8Address,&Local_u8Read) == TWI_OK){
			Local_u8Ok = (Local_u8Read == Copy_u8Value) ;
		}
	}
	if(Local_u8Ok == 0){
		TEST_voidDetail(Copy_u8Address,Copy_u8Value,Local_u8Read);
	}
	return Local_u8Ok ;
}
/* read both digit chips and compare with the wanted patterns : 1 = equal */
static u8 TEST_u8Ports(u8 Copy_u8Tens, u8 Copy_u8Units){
	u8 Local_u8Tens = 0 ;
	u8 Local_u8Units = 0 ;
	u8 Local_u8Ok = 1 ;
	if(PCF8574_u8ReadPort(SEVEN_SEG_TENS_ADDRESS,&Local_u8Tens) != TWI_OK){ Local_u8Tens = 0x00 ; Local_u8Ok = 0 ; }
	if(PCF8574_u8ReadPort(SEVEN_SEG_UNITS_ADDRESS,&Local_u8Units) != TWI_OK){ Local_u8Units = 0x00 ; Local_u8Ok = 0 ; }
	if(Local_u8Tens != Copy_u8Tens){
		TEST_voidDetail(SEVEN_SEG_TENS_ADDRESS,Copy_u8Tens,Local_u8Tens);
		Local_u8Ok = 0 ;
	}
	if(Local_u8Units != Copy_u8Units){
		TEST_voidDetail(SEVEN_SEG_UNITS_ADDRESS,Copy_u8Units,Local_u8Units);
		Local_u8Ok = 0 ;
	}
	return Local_u8Ok ;
}
/* expected pattern of the number 0..99 */
static u8 TEST_u8TensOf(u8 Copy_u8Number){
	return TEST_EXPECTED[Copy_u8Number / 10] ;
}
static u8 TEST_u8UnitsOf(u8 Copy_u8Number){
	return TEST_EXPECTED[Copy_u8Number % 10] ;
}

/*************************** tests ***************************/
/* T0 : who answers on the bus */
static void TEST_voidScan(){
	u8 Local_u8Address ;
	TEST_voidPrintLine(FLASH_STR("--- T0 : I2C bus scan ---"));
	for(Local_u8Address = 0x20 ; Local_u8Address <= 0x27 ; Local_u8Address++){
		TEST_voidPrint(FLASH_STR("       "));
		TEST_voidPrintHex(Local_u8Address);
		if(TWI_u8ProbeAddress(Local_u8Address) == TWI_OK){
			TEST_voidPrintLine(FLASH_STR(" : ACK"));
		}else{
			TEST_voidPrintLine(FLASH_STR(" : --"));
		}
	}
	TEST_voidCheck((SEVEN_SEG_TENS_ADDRESS == 0x21) && (SEVEN_SEG_UNITS_ADDRESS == 0x22), FLASH_STR("config: tens = 0x21 , units = 0x22"));
	TEST_voidCheck(TWI_u8ProbeAddress(SEVEN_SEG_TENS_ADDRESS) == TWI_OK, FLASH_STR("tens chip 0x21 answers"));
	TEST_voidCheck(TWI_u8ProbeAddress(SEVEN_SEG_UNITS_ADDRESS) == TWI_OK, FLASH_STR("units chip 0x22 answers"));
	TEST_voidCheck(TWI_u8ProbeAddress(LAMP_I2C_ADDRESS) == TWI_OK, FLASH_STR("lamp chip 0x20 answers"));
	TEST_voidCheck(TWI_u8ProbeAddress(TEST_LCD_ADDRESS) == TWI_OK, FLASH_STR("LCD chip 0x27 answers"));
	TEST_voidCheck(TWI_u8ProbeAddress(0x23) != TWI_OK, FLASH_STR("nothing answers on 0x23 (no address clash)"));
}

/* T1 : the port of one chip as plain memory */
static void TEST_voidRawPort(u8 Copy_u8Address, const __flash c8 * Copy_pc8What){
	u8 Local_u8Ok = 1 ;
	if(TEST_u8Raw(Copy_u8Address,0xFF) == 0){ Local_u8Ok = 0 ; }
	if(TEST_u8Raw(Copy_u8Address,0x00) == 0){ Local_u8Ok = 0 ; }
	if(TEST_u8Raw(Copy_u8Address,0xAA) == 0){ Local_u8Ok = 0 ; }
	if(TEST_u8Raw(Copy_u8Address,0x55) == 0){ Local_u8Ok = 0 ; }
	for(u8 i = 0 ; i < 8 ; i++){
		if(TEST_u8Raw(Copy_u8Address,(u8)~(1 << i)) == 0){ Local_u8Ok = 0 ; }    /* walking 0 */
		if(TEST_u8Raw(Copy_u8Address,(u8)(1 << i)) == 0){ Local_u8Ok = 0 ; }     /* walking 1 */
	}
	PCF8574_u8WritePort(Copy_u8Address,0xFF);
	TEST_voidCheck(Local_u8Ok, Copy_pc8What);
}
static void TEST_voidT1(){
	TEST_voidPrintLine(FLASH_STR("--- T1 : raw chip port , 22 patterns per chip (driver not used) ---"));
	TEST_voidRawPort(SEVEN_SEG_TENS_ADDRESS,FLASH_STR("tens chip 0x21 : every pin follows what is written"));
	TEST_voidRawPort(SEVEN_SEG_UNITS_ADDRESS,FLASH_STR("units chip 0x22 : every pin follows what is written"));
}

/* T2 : the digit patterns straight to the chips */
static void TEST_voidT2(){
	u8 Local_u8OkTens = 1 ;
	u8 Local_u8OkUnits = 1 ;
	TEST_voidPrintLine(FLASH_STR("--- T2 : digit patterns 0..9 written straight to each chip ---"));
	for(u8 i = 0 ; i < 10 ; i++){
		if(TEST_u8Raw(SEVEN_SEG_TENS_ADDRESS,TEST_EXPECTED[i]) == 0){ Local_u8OkTens = 0 ; }
		if(TEST_u8Raw(SEVEN_SEG_UNITS_ADDRESS,TEST_EXPECTED[i]) == 0){ Local_u8OkUnits = 0 ; }
	}
	TEST_voidCheck(Local_u8OkTens, FLASH_STR("tens chip : patterns of 0..9 read back equal"));
	TEST_voidCheck(Local_u8OkUnits, FLASH_STR("units chip : patterns of 0..9 read back equal"));
	PCF8574_u8WritePort(SEVEN_SEG_TENS_ADDRESS,0xFF);
	PCF8574_u8WritePort(SEVEN_SEG_UNITS_ADDRESS,0xFF);
}

/* T3 : the driver for all 100 numbers */
static void TEST_voidT3(){
	u8 Local_u8Ok = 1 ;
	u8 Local_u8Count = 0 ;
	TEST_voidPrintLine(FLASH_STR("--- T3 : SEVEN_SEG_voidSetNumber 0..99, both chips read back ---"));
	SEVEN_SEG_voidInit();
	SEVEN_SEG_voidEnable();
	for(u8 n = 0 ; n < 100 ; n++){
		SEVEN_SEG_voidSetNumber(n);
		if((TEST_u8Ports(TEST_u8TensOf(n),TEST_u8UnitsOf(n)) == 0) || (SEVEN_SEG_u8GetStatus() != TWI_OK)){
			if(Global_u8Details <= TEST_MAX_DETAILS){
				TEST_voidPrint(FLASH_STR("       number "));
				TEST_voidPrintNum(n);
				TEST_voidNewLine();
			}
			Local_u8Ok = 0 ;
			Local_u8Count++;
		}
	}
	TEST_voidPrint(FLASH_STR("       wrong numbers : "));
	TEST_voidPrintNum(Local_u8Count);
	TEST_voidNewLine();
	TEST_voidCheck(Local_u8Ok, FLASH_STR("all 100 numbers show the right patterns on both chips"));
}

/* T4 : behaviour of the driver */
static void TEST_voidT4(){
	u8 Local_u8Port = 0 ;
	TEST_voidPrintLine(FLASH_STR("--- T4 : driver behaviour ---"));
	SEVEN_SEG_voidInit();
	TEST_voidCheck(TEST_u8Ports(0xFF,0xFF) && (SEVEN_SEG_u8GetStatus() == TWI_OK), FLASH_STR("Init : both chips blank (0xFF) , status OK"));
	SEVEN_SEG_voidSetNumber(37);
	TEST_voidCheck(TEST_u8Ports(0xFF,0xFF), FLASH_STR("SetNumber(37) while not enabled : still blank"));
	SEVEN_SEG_voidEnable();
	TEST_voidCheck(TEST_u8Ports(TEST_u8TensOf(37),TEST_u8UnitsOf(37)), FLASH_STR("Enable : 37 appears (0xB0 , 0xF8)"));
	SEVEN_SEG_voidDisable();
	TEST_voidCheck(TEST_u8Ports(0xFF,0xFF), FLASH_STR("Disable : blank again"));
	SEVEN_SEG_voidDisable();
	TEST_voidCheck(TEST_u8Ports(0xFF,0xFF) && (SEVEN_SEG_u8GetStatus() == TWI_OK), FLASH_STR("Disable twice : still blank , status OK"));
	SEVEN_SEG_voidEnable();
	TEST_voidCheck(TEST_u8Ports(TEST_u8TensOf(37),TEST_u8UnitsOf(37)), FLASH_STR("Enable : the number is kept (37)"));
	SEVEN_SEG_voidSetNumber(0);
	TEST_voidCheck(TEST_u8Ports(0xC0,0xC0), FLASH_STR("SetNumber(0) : 00 (leading zero shown)"));
	SEVEN_SEG_voidSetNumber(100);
	TEST_voidCheck(TEST_u8Ports(0x90,0x90), FLASH_STR("SetNumber(100) : limited to 99"));
	SEVEN_SEG_voidSetNumber(255);
	TEST_voidCheck(TEST_u8Ports(0x90,0x90), FLASH_STR("SetNumber(255) : limited to 99"));
	SEVEN_SEG_voidSetNumber(10);
	TEST_voidCheck(TEST_u8Ports(0xF9,0xC0), FLASH_STR("SetNumber(10) : tens 1 , units 0"));
	/* the driver writes only changed digits : change one chip behind its back and set the same number */
	SEVEN_SEG_voidSetNumber(55);
	PCF8574_u8WritePort(SEVEN_SEG_UNITS_ADDRESS,0x00);
	SEVEN_SEG_voidSetNumber(55);
	PCF8574_u8ReadPort(SEVEN_SEG_UNITS_ADDRESS,&Local_u8Port);
	if(Local_u8Port == 0x00){
		TEST_voidInfo(FLASH_STR("same number again is NOT rewritten (by design : only changed digits are written)"));
	}else{
		TEST_voidInfo(FLASH_STR("same number again was rewritten"));
	}
	SEVEN_SEG_voidSetNumber(56);
	TEST_voidCheck(TEST_u8Ports(TEST_u8TensOf(56),TEST_u8UnitsOf(56)), FLASH_STR("next different number repairs the display (56)"));
	SEVEN_SEG_voidDisable();
}

/* T5 : the other chips on the bus must not disturb the digits (and the other way round) */
static void TEST_voidT5(){
	u8 Local_u8Ok = 1 ;
	u8 Local_u8Lamps = 0 ;
	TEST_voidPrintLine(FLASH_STR("--- T5 : bus sharing with the lamps (0x20) and the LCD (0x27) ---"));
	SEVEN_SEG_voidInit();
	SEVEN_SEG_voidEnable();
	SEVEN_SEG_voidSetNumber(37);
	for(u8 lamp = 1 ; lamp <= LAMP_COUNT ; lamp++){
		LAMP_u8SetState(lamp,LAMP_ON);
		if(TEST_u8Ports(TEST_u8TensOf(37),TEST_u8UnitsOf(37)) == 0){ Local_u8Ok = 0 ; }
	}
	TEST_voidCheck(Local_u8Ok, FLASH_STR("5 lamp writes : the digits still show 37"));
	Local_u8Ok = 1 ;
	for(u8 lamp = 1 ; lamp <= LAMP_COUNT ; lamp++){
		LAMP_u8SetState(lamp,LAMP_OFF);
	}
	CLCD_voidSetCursorPosition(0,0);
	CLCD_voidSendFlashString(FLASH_STR("7SEG test v2    "));
	if(TEST_u8Ports(TEST_u8TensOf(37),TEST_u8UnitsOf(37)) == 0){ Local_u8Ok = 0 ; }
	TEST_voidCheck(Local_u8Ok, FLASH_STR("16 LCD characters written : the digits still show 37"));
	/* the other way round : the lamp port must not change when the digits change */
	LAMP_u8SetState(2,LAMP_ON);
	LAMP_u8SetState(4,LAMP_ON);
	SEVEN_SEG_voidSetNumber(58);
	SEVEN_SEG_voidSetNumber(21);
	PCF8574_u8ReadPort(LAMP_I2C_ADDRESS,&Local_u8Lamps);
	/* lamps 2 and 4 on = bits 1 and 3 low , the rest high */
	TEST_voidCheck(Local_u8Lamps == 0xF5, FLASH_STR("digit changes do not touch the lamp port (0xF5 expected)"));
	LAMP_u8SetState(2,LAMP_OFF);
	LAMP_u8SetState(4,LAMP_OFF);
	SEVEN_SEG_voidDisable();
}

/* T6 : many mixed writes */
static void TEST_voidT6(){
	u8 Local_u8Errors = 0 ;
	u8 Local_u8Lamp = 1 ;
	u16 Local_u16Start ;
	u16 Local_u16Ticks ;
	TEST_voidPrintLine(FLASH_STR("--- T6 : stress , 300 mixed writes ---"));
	SEVEN_SEG_voidInit();
	SEVEN_SEG_voidEnable();
	for(u16 i = 0 ; i < 300 ; i++){
		u8 Local_u8Number = (u8)((i * 7) % 100) ;
		SEVEN_SEG_voidSetNumber(Local_u8Number);
		if((i % 3) == 0){
			LAMP_u8SetState(Local_u8Lamp,(i % 2) ? LAMP_ON : LAMP_OFF);
			Local_u8Lamp = (Local_u8Lamp % LAMP_COUNT) + 1 ;
		}
		if((i % 4) == 0){
			CLCD_voidSetCursorPosition(15,0);
			CLCD_voidSendData('0' + (i % 10));
		}
		if((TEST_u8Ports(TEST_u8TensOf(Local_u8Number),TEST_u8UnitsOf(Local_u8Number)) == 0) || (SEVEN_SEG_u8GetStatus() != TWI_OK)){
			Local_u8Errors++;
		}
	}
	TEST_voidPrint(FLASH_STR("       errors : "));
	TEST_voidPrintNum(Local_u8Errors);
	TEST_voidNewLine();
	TEST_voidCheck(Local_u8Errors == 0, FLASH_STR("300 mixed writes : every read back is right"));
	for(u8 lamp = 1 ; lamp <= LAMP_COUNT ; lamp++){
		LAMP_u8SetState(lamp,LAMP_OFF);
	}
	/* timing : both digits change (11 <-> 22) */
	SEVEN_SEG_voidSetNumber(11);
	Local_u16Start = TEST_u16ReadTCNT1();
	SEVEN_SEG_voidSetNumber(22);
	Local_u16Ticks = TEST_u16TicksSince(Local_u16Start);
	TEST_voidPrint(FLASH_STR("       SetNumber with 2 changed digits takes "));
	TEST_voidPrintNum(Local_u16Ticks / 2);
	TEST_voidPrintLine(FLASH_STR(" us"));
	TEST_voidCheck(Local_u16Ticks < TIMER1_TICKS_PER_MS, FLASH_STR("SetNumber with 2 changed digits takes less than 1 ms"));
	SEVEN_SEG_voidSetNumber(22);
	Local_u16Start = TEST_u16ReadTCNT1();
	SEVEN_SEG_voidSetNumber(22);
	Local_u16Ticks = TEST_u16TicksSince(Local_u16Start);
	TEST_voidPrint(FLASH_STR("       SetNumber with no change takes "));
	TEST_voidPrintNum(Local_u16Ticks / 2);
	TEST_voidPrintLine(FLASH_STR(" us"));
	TEST_voidCheck(Local_u16Ticks < (TIMER1_TICKS_PER_MS / 10), FLASH_STR("SetNumber with no change takes less than 0.1 ms (no I2C)"));
	SEVEN_SEG_voidDisable();
}

/* T7 : what the display looks like */
static void TEST_voidT7(){
	TEST_voidPrintLine(FLASH_STR("--- T7 : looks (watch the display) ---"));
	/* all segments + dp (direct write , driver not used) */
	TEST_voidManual(FLASH_STR("ALL segments AND both decimal points lit for 3 s (8. 8.)"));
	TEST_voidLcdStep(FLASH_STR("all segments"));
	PCF8574_u8WritePort(SEVEN_SEG_TENS_ADDRESS,0x00);
	PCF8574_u8WritePort(SEVEN_SEG_UNITS_ADDRESS,0x00);
	TEST_voidWaitMs(3000);
	/* one segment at a time : a b c d e f g dp */
	TEST_voidManual(FLASH_STR("ONE segment at a time (a b c d e f g dp) , first on the TENS digit , then on the UNITS digit , 700 ms each"));
	for(u8 Local_u8Chip = 0 ; Local_u8Chip < 2 ; Local_u8Chip++){
		u8 Local_u8Address = Local_u8Chip ? SEVEN_SEG_UNITS_ADDRESS : SEVEN_SEG_TENS_ADDRESS ;
		TEST_voidLcdStep(Local_u8Chip ? FLASH_STR("units segments") : FLASH_STR("tens segments"));
		PCF8574_u8WritePort(SEVEN_SEG_TENS_ADDRESS,0xFF);
		PCF8574_u8WritePort(SEVEN_SEG_UNITS_ADDRESS,0xFF);
		for(u8 i = 0 ; i < 8 ; i++){
			PCF8574_u8WritePort(Local_u8Address,(u8)~(1 << i));
			TEST_voidPrint(FLASH_STR("       "));
			TEST_voidPrint(Local_u8Chip ? FLASH_STR("UNITS") : FLASH_STR("TENS "));
			TEST_voidPrint(FLASH_STR(" pin P"));
			TEST_voidPrintNum(i);
			TEST_voidPrint(FLASH_STR(" low --> segment "));
			if(i < 7){
				USART_voidSend('a' + i);
			}else{
				TEST_voidPrint(FLASH_STR("dp"));
			}
			TEST_voidNewLine();
			TEST_voidWaitMs(700);
		}
		PCF8574_u8WritePort(Local_u8Address,0xFF);
	}
	/* driver : same digit on both */
	SEVEN_SEG_voidInit();
	SEVEN_SEG_voidEnable();
	TEST_voidManual(FLASH_STR("00 11 22 33 44 55 66 77 88 99 (1 s each) : both digits must always be the SAME digit"));
	TEST_voidLcdStep(FLASH_STR("00 11 .. 99"));
	for(u8 i = 0 ; i < 10 ; i++){
		SEVEN_SEG_voidSetNumber(i * 11);
		TEST_voidWaitMs(1000);
	}
	TEST_voidManual(FLASH_STR("37 for 3 s : tens = 3 , units = 7 (this was shown as 33 with the 7447 version)"));
	TEST_voidLcdStep(FLASH_STR("37"));
	SEVEN_SEG_voidSetNumber(37);
	TEST_voidWaitMs(3000);
	TEST_voidManual(FLASH_STR("73 for 3 s"));
	TEST_voidLcdStep(FLASH_STR("73"));
	SEVEN_SEG_voidSetNumber(73);
	TEST_voidWaitMs(3000);
	TEST_voidManual(FLASH_STR("counting 00 .. 99 , 200 ms per step"));
	TEST_voidLcdStep(FLASH_STR("count 00..99"));
	for(u8 i = 0 ; i < 100 ; i++){
		SEVEN_SEG_voidSetNumber(i);
		TEST_voidWaitMs(200);
	}
	TEST_voidManual(FLASH_STR("blinking 88 : on 0.5 s , off 0.5 s , 6 times (like the heater setting mode)"));
	TEST_voidLcdStep(FLASH_STR("blink 88"));
	SEVEN_SEG_voidSetNumber(88);
	for(u8 i = 0 ; i < 6 ; i++){
		SEVEN_SEG_voidEnable();
		TEST_voidWaitMs(500);
		SEVEN_SEG_voidDisable();
		TEST_voidWaitMs(500);
	}
	TEST_voidManual(FLASH_STR("display is blank"));
	TEST_voidLcdStep(FLASH_STR("blank"));
	TEST_voidWaitMs(1500);
}

int main(void){
	DIO_voidSetPinDirection(TEST_LED_PORT,TEST_LED_PIN,DIO_PIN_OUTPUT);
	USART_voidInit();
	TIMER1_voidInit();                 /* free running : stop watch only */
	LAMP_voidInit();
	CLCD_voidInit();
	CLCD_voidSetCursorPosition(0,0);
	CLCD_voidSendFlashString(FLASH_STR("7SEG test v2    "));
	TEST_voidNewLine();
	TEST_voidPrintLine(FLASH_STR("===== hal_testing_v2 : 7-segment on 2 x PCF8574 ====="));
	TEST_voidPrintLine(FLASH_STR("Status LED: slow blink = running , solid = all passed , fast blink = a check failed"));

	TEST_voidScan();
	TEST_voidT1();
	TEST_voidT2();
	TEST_voidT3();
	TEST_voidT4();
	TEST_voidT5();
	TEST_voidT6();
	TEST_voidT7();

	TEST_voidPrint(FLASH_STR("SUMMARY: "));
	TEST_voidPrintNum(Global_u8PassCount);
	TEST_voidPrint(FLASH_STR(" passed , "));
	TEST_voidPrintNum(Global_u8FailCount);
	TEST_voidPrint(FLASH_STR(" failed --> "));
	if(Global_u8FailCount == 0){
		TEST_voidPrintLine(FLASH_STR("ALL PASSED"));
		DIO_voidSetPinValue(TEST_LED_PORT,TEST_LED_PIN,DIO_PIN_HIGH);
		while(1){
			/* solid ON = all passed */
		}
	}else{
		TEST_voidPrintLine(FLASH_STR("SOME CHECKS FAILED"));
		while(1){
			/* fast blink = at least one failure */
			DIO_voidTogPin(TEST_LED_PORT,TEST_LED_PIN);
			_delay_ms(TEST_FAIL_BLINK_MS);
		}
	}
	return 0 ;
}
