/*
 * main.c  (hal_testing : 7-segment + 7447 test bench)
 *
 *  Created on: Oct 3, 2026
 *
 * =====================================================================
 *  Tests ONLY the 2-digit multiplexed 7-segment display behind a 7447
 * =====================================================================
 *  Build : pio run
 *  Hex   : .pio/build/seven_seg_test/firmware.hex
 *
 *  The test runs in two parts and repeats forever. The UART text tells you
 *  what you should see on the display at every step.
 *
 *  PART 1 - wiring test , the SEVEN_SEG driver is NOT used
 *    Pins are driven directly with DIO (no interrupt , no multiplexing).
 *    A1  tens digit only  , BCD 0 .. 9  (1.5 s each)
 *    A2  units digit only , BCD 0 .. 9  (1.5 s each)
 *    A3  both digits ON together with BCD 8 --> "88"
 *    After every write the BCD pins are read back : "OK" = the pin follows
 *    the port , "MISMATCH" = the pin does NOT follow (see the hints below).
 *  PART 2 - SEVEN_SEG driver (Timer2 1 ms tick calls SEVEN_SEG_voidRefresh)
 *    B0  blank right after init
 *    B1  number 37 for 3 s
 *    B2  number 88 for 2 s
 *    B3  counts 00 .. 99 (200 ms each)
 *    B4  pin sampling for 20 ms : both digits on together ? BCD correct ?
 *    B5  Disable : display blank for 2 s
 *
 *  What a failure tells you :
 *    A1/A2 show wrong digits but the readback says OK  --> 7447 wiring
 *          (A,B,C,D order , LT / BI / RBI not tied to +5 V , wrong display type :
 *           the 7447 needs a COMMON-ANODE display)
 *    A1/A2 readback says MISMATCH on PC2..PC5          --> the pins are still JTAG :
 *          in Proteus open the ATmega32 properties and UNCHECK the JTAGEN fuse
 *          (Program JTAG Interface)
 *    A3 shows "88" but B2 does not                      --> multiplexing / timing problem
 *    one digit never lights                             --> its PNP transistor / base resistor
 *
 * ---------------------- Proteus parts and wiring ----------------------
 *  ATMEGA32          Clock Frequency = 16 MHz (must equal F_CPU) ; JTAGEN fuse OFF
 *  VIRTUAL TERMINAL  9600 baud , 8N1 : terminal RXD <- PD1 (TXD)
 *  7447              A B C D <- PC2 PC3 PC4 PC5 ; LT , BI/RBO , RBI -> +5 V ;
 *                    outputs a..g -> 7 x 330R -> segments a..g of BOTH digits
 *  2 x 7SEG-COM-ANODE  digit 1 (tens) common anode <- collector of PNP 1
 *                      digit 2 (units) common anode <- collector of PNP 2
 *  2 x PNP (2N3906)  emitter -> +5 V , base <- 1k <- PC6 (tens) / PC7 (units)
 *                    active LOW : pin LOW = digit ON
 * =====================================================================
 */
#include <util/delay.h>
#include "../lib/Service/std_types.h"
#include "../lib/Service/Bit_math.h"
#include "../lib/MCAL/reg_def.h"
#include "../lib/MCAL/DIO/DIO.h"
#include "../lib/MCAL/GIE/GIE.h"
#include "../lib/MCAL/TIMER2/TIMER2.h"
#include "../lib/MCAL/USART/USART.h"
#include "../lib/HAL/SEVEN_SEG/SEVEN_SEG.h"
#include "../lib/HAL/SEVEN_SEG/SEVEN_SEG_cfg.h"

/* Keep text in flash , not RAM (2 KB RAM only). */
#define FLASH_STR(str)    (__extension__({ static const __flash c8 Local_c8Text[] = (str); &Local_c8Text[0]; }))

/* Timer2 : prescaler 64 , CTC , OCR2 = F_CPU / 64 / 1000 - 1  (16 MHz -> 249 = 1 ms) */
#define TEST_T2_OCR               ((u8)(F_CPU / 64UL / 1000UL - 1UL))
#define SEVEN_SEG_DIGIT_OFF       (!SEVEN_SEG_DIGIT_ON_LEVEL)

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
static void TEST_voidWaitMs(u16 Copy_u16Ms){
	while(Copy_u16Ms > 0){
		_delay_ms(1);
		Copy_u16Ms--;
	}
}

/*************************** timer callback ***************************/
/* The driver counts 5 refresh calls per digit. Calling it only every Global_u8SlowFactor-th
 * millisecond makes the multiplexing slower by that factor (1 = normal : 5 ms per digit,
 * 100 = 500 ms per digit , so every digit can be watched on its own). */
static volatile u8 Global_u8SlowFactor = 1 ;
static u8 Global_u8SlowCount = 0 ;
static void TEST_voidTickCallback(void){
	Global_u8SlowCount++;
	if(Global_u8SlowCount >= Global_u8SlowFactor){
		Global_u8SlowCount = 0 ;
		SEVEN_SEG_voidRefresh();
	}
}

/*************************** PART 1 : direct pin drive ***************************/
static u8 TEST_u8ReadBcdPins(){
	u8 Local_u8Bcd = 0 ;
	Local_u8Bcd |= (DIO_u8GetPinValue(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_A) << 0) ;
	Local_u8Bcd |= (DIO_u8GetPinValue(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_B) << 1) ;
	Local_u8Bcd |= (DIO_u8GetPinValue(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_C) << 2) ;
	Local_u8Bcd |= (DIO_u8GetPinValue(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_D) << 3) ;
	return Local_u8Bcd ;
}
static void TEST_voidDirectInit(){
	/* the same pin set-up as SEVEN_SEG_voidInit , but written here so the driver is not involved */
	DIO_voidDisableJTAG();
	DIO_voidSetPinValue(SEVEN_SEG_DIGIT_PORT,SEVEN_SEG_TENS_PIN ,SEVEN_SEG_DIGIT_OFF);
	DIO_voidSetPinValue(SEVEN_SEG_DIGIT_PORT,SEVEN_SEG_UNITS_PIN,SEVEN_SEG_DIGIT_OFF);
	DIO_voidSetPinDirection(SEVEN_SEG_DIGIT_PORT,SEVEN_SEG_TENS_PIN ,DIO_PIN_OUTPUT);
	DIO_voidSetPinDirection(SEVEN_SEG_DIGIT_PORT,SEVEN_SEG_UNITS_PIN,DIO_PIN_OUTPUT);
	DIO_voidSetPinDirection(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_A,DIO_PIN_OUTPUT);
	DIO_voidSetPinDirection(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_B,DIO_PIN_OUTPUT);
	DIO_voidSetPinDirection(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_C,DIO_PIN_OUTPUT);
	DIO_voidSetPinDirection(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_D,DIO_PIN_OUTPUT);
}
/* digits off , put BCD on the 7447 , read it back , then switch the wanted digits on */
static void TEST_voidDirectShow(u8 Copy_u8TensOn, u8 Copy_u8UnitsOn, u8 Copy_u8Bcd){
	u8 Local_u8Read ;
	DIO_voidSetPinValue(SEVEN_SEG_DIGIT_PORT,SEVEN_SEG_TENS_PIN ,SEVEN_SEG_DIGIT_OFF);
	DIO_voidSetPinValue(SEVEN_SEG_DIGIT_PORT,SEVEN_SEG_UNITS_PIN,SEVEN_SEG_DIGIT_OFF);
	DIO_voidSetPinValue(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_A,GET_BIT(Copy_u8Bcd,0));
	DIO_voidSetPinValue(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_B,GET_BIT(Copy_u8Bcd,1));
	DIO_voidSetPinValue(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_C,GET_BIT(Copy_u8Bcd,2));
	DIO_voidSetPinValue(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_D,GET_BIT(Copy_u8Bcd,3));
	_delay_us(10);
	Local_u8Read = TEST_u8ReadBcdPins();
	TEST_voidPrint(FLASH_STR("       BCD written = "));
	TEST_voidPrintNum(Copy_u8Bcd);
	TEST_voidPrint(FLASH_STR(" , pins read = "));
	TEST_voidPrintNum(Local_u8Read);
	if(Local_u8Read == Copy_u8Bcd){
		TEST_voidPrintLine(FLASH_STR("  OK"));
	}else{
		TEST_voidPrintLine(FLASH_STR("  MISMATCH (pins do not follow the port : JTAG fuse ?)"));
	}
	if(Copy_u8TensOn){
		DIO_voidSetPinValue(SEVEN_SEG_DIGIT_PORT,SEVEN_SEG_TENS_PIN,SEVEN_SEG_DIGIT_ON_LEVEL);
	}
	if(Copy_u8UnitsOn){
		DIO_voidSetPinValue(SEVEN_SEG_DIGIT_PORT,SEVEN_SEG_UNITS_PIN,SEVEN_SEG_DIGIT_ON_LEVEL);
	}
}
static void TEST_voidPart1(){
	TEST_voidPrintLine(FLASH_STR("--- PART 1 : direct pin drive (driver not used) ---"));
	TEST_voidDirectInit();
	TEST_voidPrint(FLASH_STR("MCUCSR = "));
	TEST_voidPrintNum(MCUCSR);
	TEST_voidPrintLine(FLASH_STR(" (bit 7 = JTD = value 128 or more after DIO_voidDisableJTAG)"));
	TEST_voidPrintLine(FLASH_STR("A1 : TENS digit only , BCD 0 .. 9 (1.5 s each)"));
	for(u8 i = 0 ; i <= 9 ; i++){
		TEST_voidPrint(FLASH_STR("A1 expect tens digit = "));
		TEST_voidPrintNum(i);
		TEST_voidPrintLine(FLASH_STR(" , units dark"));
		TEST_voidDirectShow(1,0,i);
		TEST_voidWaitMs(1500);
	}
	TEST_voidPrintLine(FLASH_STR("A2 : UNITS digit only , BCD 0 .. 9 (1.5 s each)"));
	for(u8 i = 0 ; i <= 9 ; i++){
		TEST_voidPrint(FLASH_STR("A2 expect units digit = "));
		TEST_voidPrintNum(i);
		TEST_voidPrintLine(FLASH_STR(" , tens dark"));
		TEST_voidDirectShow(0,1,i);
		TEST_voidWaitMs(1500);
	}
	TEST_voidPrintLine(FLASH_STR("A3 : both digits ON together , BCD 8 --> expect 88 (3 s)"));
	TEST_voidDirectShow(1,1,8);
	TEST_voidWaitMs(3000);
	/* digits off */
	DIO_voidSetPinValue(SEVEN_SEG_DIGIT_PORT,SEVEN_SEG_TENS_PIN ,SEVEN_SEG_DIGIT_OFF);
	DIO_voidSetPinValue(SEVEN_SEG_DIGIT_PORT,SEVEN_SEG_UNITS_PIN,SEVEN_SEG_DIGIT_OFF);
}

/*************************** PART 2 : SEVEN_SEG driver ***************************/
/* The sampling reads the port ONCE (one atomic snapshot). Reading the digit pins and the BCD
 * pins one by one would let the tick ISR switch digits in between and give false errors. */
#if SEVEN_SEG_BCD_PORT != SEVEN_SEG_DIGIT_PORT
#error "TEST_voidSample needs the BCD pins and the digit pins on the same port"
#endif
static void TEST_voidSample(u8 Copy_u8ExpectedTens, u8 Copy_u8ExpectedUnits){
	u8 Local_u8TensSeen = 0 ;
	u8 Local_u8UnitsSeen = 0 ;
	u8 Local_u8BothOn = 0 ;
	u8 Local_u8Wrong = 0 ;
	for(u16 i = 0 ; i < 400 ; i++){
		u8 Local_u8Port = DIO_u8GetPortValue(SEVEN_SEG_BCD_PORT) ;
		u8 Local_u8Tens = (GET_BIT(Local_u8Port,SEVEN_SEG_TENS_PIN) == SEVEN_SEG_DIGIT_ON_LEVEL) ;
		u8 Local_u8Units = (GET_BIT(Local_u8Port,SEVEN_SEG_UNITS_PIN) == SEVEN_SEG_DIGIT_ON_LEVEL) ;
		u8 Local_u8Bcd = (GET_BIT(Local_u8Port,SEVEN_SEG_BCD_PIN_A) << 0)
				| (GET_BIT(Local_u8Port,SEVEN_SEG_BCD_PIN_B) << 1)
				| (GET_BIT(Local_u8Port,SEVEN_SEG_BCD_PIN_C) << 2)
				| (GET_BIT(Local_u8Port,SEVEN_SEG_BCD_PIN_D) << 3) ;
		if(Local_u8Tens && Local_u8Units){ Local_u8BothOn = 1 ; }
		if(Local_u8Tens){
			Local_u8TensSeen = 1 ;
			if(Local_u8Bcd != Copy_u8ExpectedTens){ Local_u8Wrong = 1 ; }
		}
		if(Local_u8Units){
			Local_u8UnitsSeen = 1 ;
			if(Local_u8Bcd != Copy_u8ExpectedUnits){ Local_u8Wrong = 1 ; }
		}
		_delay_us(50);
	}
	TEST_voidPrint(FLASH_STR("B4 tens digit switched on : "));
	TEST_voidPrintLine(Local_u8TensSeen ? FLASH_STR("yes") : FLASH_STR("NO"));
	TEST_voidPrint(FLASH_STR("B4 units digit switched on : "));
	TEST_voidPrintLine(Local_u8UnitsSeen ? FLASH_STR("yes") : FLASH_STR("NO"));
	TEST_voidPrint(FLASH_STR("B4 both digits on together : "));
	TEST_voidPrintLine(Local_u8BothOn ? FLASH_STR("YES (ghosting !)") : FLASH_STR("no"));
	TEST_voidPrint(FLASH_STR("B4 BCD matches the digit on : "));
	TEST_voidPrintLine(Local_u8Wrong ? FLASH_STR("NO") : FLASH_STR("yes"));
}
static void TEST_voidPart2(){
	TEST_voidPrintLine(FLASH_STR("--- PART 2 : SEVEN_SEG driver , 1 ms tick ---"));
	SEVEN_SEG_voidInit();
	TEST_voidPrintLine(FLASH_STR("B0 : display must be blank right after init (1 s)"));
	TEST_voidWaitMs(1000);
	/* from here only the Timer2 ISR writes the display pins */
	TIMER2_voidEnableOCInterrupt();
	SEVEN_SEG_voidEnable();
	/* slow multiplexing : each digit can be watched on its own */
	TEST_voidPrintLine(FLASH_STR("B0a : SLOW mux 500 ms per digit , number 37 (8 s) : 3 then 7 , alternating"));
	SEVEN_SEG_voidSetNumber(37);
	Global_u8SlowFactor = 100 ;
	TEST_voidWaitMs(8000);
	TEST_voidPrintLine(FLASH_STR("B0b : SLOW mux 100 ms per digit , number 37 (5 s)"));
	Global_u8SlowFactor = 20 ;
	TEST_voidWaitMs(5000);
	TEST_voidPrintLine(FLASH_STR("B0c : mux 20 ms per digit , number 37 (5 s)"));
	Global_u8SlowFactor = 4 ;
	TEST_voidWaitMs(5000);
	TEST_voidPrintLine(FLASH_STR("B0d : NORMAL mux 5 ms per digit from here on"));
	Global_u8SlowFactor = 1 ;
	TEST_voidPrintLine(FLASH_STR("B1 : number 37 (3 s)"));
	SEVEN_SEG_voidSetNumber(37);
	TEST_voidWaitMs(3000);
	TEST_voidPrintLine(FLASH_STR("B2 : number 88 (2 s)"));
	SEVEN_SEG_voidSetNumber(88);
	TEST_voidWaitMs(2000);
	TEST_voidPrintLine(FLASH_STR("B3 : counts 00 .. 99 (200 ms each)"));
	for(u8 i = 0 ; i < 100 ; i++){
		SEVEN_SEG_voidSetNumber(i);
		TEST_voidWaitMs(200);
	}
	TEST_voidPrintLine(FLASH_STR("B4 : pin sampling with the number 37 on the display"));
	SEVEN_SEG_voidSetNumber(37);
	TEST_voidWaitMs(20);
	TEST_voidSample(3,7);
	TEST_voidPrintLine(FLASH_STR("B5 : Disable --> display blank (2 s)"));
	SEVEN_SEG_voidDisable();
	TEST_voidWaitMs(2000);
	/* stop the ISR , so PART 1 can drive the pins again on the next round */
	TIMER2_voidDisableOCInterrupt();
}

int main(void){
	USART_voidInit();
	TIMER2_voidInit(TIMER2_DIV_64,TIMER2_CTC);
	TIMER2_voidSetOCR(TEST_T2_OCR);
	TIMER2_voidSetCallBack_OC(TEST_voidTickCallback);
	GIE_voidEnableGlobalInterrupt();
	TEST_voidPrintLine(FLASH_STR("===== hal_testing : 7-segment + 7447 test ====="));
	while(1){
		TEST_voidPart1();
		TEST_voidPart2();
		TEST_voidPrintLine(FLASH_STR("===== round finished , starting again ====="));
	}
	return 0 ;
}
