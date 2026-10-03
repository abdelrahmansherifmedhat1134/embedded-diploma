/*
 * test_hal.c
 *
 *  Created on: Oct 2, 2026
 *
 * =====================================================================
 *  Phase 2 test of the HAL layer
 * =====================================================================
 *  Tested : PCF8574 , CLCD , LCD_BUF , KPAD , BUTTON , SEVEN_SEG , LM35 ,
 *           RELAY , LED , BUZZER , LAMP , FAN , SERVO , DIMMER , EXT_EEPROM
 *  Not tested here : SSG (old driver , replaced by SEVEN_SEG)
 *
 *  Build : pio run -e test_hal
 *  Hex   : .pio/build/test_hal/firmware.hex
 *
 *  SCHED does not exist yet , so this test makes its own 1 ms tick with
 *  TIMER2 (the ISR counts milliseconds and sets a 5 ms and a 10 ms flag ;
 *  the main loop serves LCD_BUF every 5 ms and KPAD + BUTTON every 10 ms).
 *  Timer1 (servo / fan PWM) also serves as a stop watch.
 *
 * ---------------------- Proteus parts and wiring ----------------------
 *  (everything of the test_mcal schematic stays ; new parts are marked NEW)
 *  ATMEGA32          Clock Frequency = 16 MHz (must equal F_CPU)
 *  Status LED        PA3 -> 330R -> LED-RED -> GND
 *                    (PA3 is also the heater LED pin : the LED check uses it too)
 *  VIRTUAL TERMINAL  9600 baud , 8N1 : terminal RXD <- PD1 , terminal TXD -> PD0
 *  I2C bus           PC0 = SCL , PC1 = SDA , 4.7k pull-up to VCC on both lines
 *  PCF8574 (LCD)     address 0x27 (A2 A1 A0 = VCC)
 *                    P0 = RS , P1 = RW , P2 = E , P3 = back light , P4..P7 = D4..D7
 *  NEW  LM016L (16x2 LCD)  wired to that PCF8574 as above
 *  PCF8574 (lamps)   address 0x20 (A2 A1 A0 = GND)
 *  NEW  5 x LED      +5 V -> 220R -> LED (anode to the resistor) -> P0..P4 of the lamp PCF8574
 *                    (active low : lamp ON = pin LOW)
 *  24C08 EEPROM      address 0x50 (A2 = GND , A1 A0 = GND)
 *  LM35 (ambient)    VOUT -> PA0   (set the value by hand , 0..100 C)
 *  LM35 (water)      VOUT -> PA1
 *  NEW  KEYPAD       KEYPAD-SMALLCALC : rows A..D -> PA4..PA7 , columns 1..4 -> PB0 , PB1 , PB2 , PB4
 *  NEW  3 buttons    PD6 (ON/OFF) , PD7 (UP) , PB5 (DOWN) : each button between the pin and GND
 *                    (internal pull-ups are used)
 *  NEW  7-segment    two COMMON-ANODE digits (7SEG-COM-ANODE) , common pin of both on +5 V ,
 *                    each digit behind its own PCF8574 (no multiplexing , no transistors) :
 *                    PCF8574 0x21 (A2 A1 A0 = 0 0 1) = tens  digit , P0..P6 -> segments a..g , P7 -> dp
 *                    PCF8574 0x22 (A2 A1 A0 = 0 1 0) = units digit , same wiring
 *                    each segment pin -> 220R -> segment (a segment lights when its pin is LOW)
 *  NEW  Heating relay/LED   PB6 -> 330R -> LED -> GND   (heating element)
 *  NEW  Cooling relay/LED   PB7 -> 330R -> LED -> GND   (cooling element)
 *  NEW  Buzzer       PD3 -> BUZZER (or a transistor driving it) -> GND
 *  NEW  Servo        SERVO motor : signal -> PD5 (OC1A)
 *  NEW  DC motor fan MOTOR with NPN driver : base through 1k from PD4 (OC1B)
 *  NEW  Dimmer       PB3 (OC0) -> RC filter (1k + 10uF) -> voltmeter or LED , or just the oscilloscope
 *
 * --------------------------- Expected result ---------------------------
 *  UART : one line per check "[PASS] <module>: <what>" or "[FAIL] ...",
 *         "[SKIP]" = a manual step was not done , "MANUAL:" lines tell you
 *         what to do / what to look at , last line = SUMMARY.
 *  Status LED : slow blink (2 Hz) = test running
 *               solid ON         = all checks passed
 *               fast blink (5 Hz) = at least one check failed
 *
 *  Things you must do by hand (the terminal tells you when) :
 *   1. LCD     : two lines of text , NO cursor , NO blinking block
 *   2. LEDs / buzzer : heating LED 1 s , cooling LED 1 s , status LED 1 s , buzzer 1 s
 *   3. LAMPS   : lamps 1..5 light one after the other , then all five , then all off
 *   4. 7-SEG   : MANUAL (compare each "look: NN" line with the display) : blank (3 s) ,
 *                00 11 .. 99 (1 s each) , 37 73 10 99 (1 s each) , 150 shows 99 ,
 *                counting 00..99 (500 ms up to 55 , then 300 ms) , 88 (3 s) , blank (3 s) , 88 back
 *   5. LM35    : move the two sliders while the temperatures are printed (5 s)
 *   6. KEYPAD  : hold one key 2 s , press 3 keys , press 7 / C / + in order
 *   7. BUTTONS : hold each button 2 s : ON/OFF , UP , DOWN
 *   8. SERVO   : arm moves 0 / 90 / 180 degrees (2 s each)
 *   9. FAN     : speed ramps 0 .. 100 % in steps of 10 % (1 s each)
 *  10. DIMMER  : level ramps 0 .. 100 % in steps of 10 % (1 s each)
 *  11. EEPROM  : press RESET in Proteus after the SUMMARY and run again :
 *                the second run must report "data from before the reset is still there"
 * =====================================================================
 */
#include <util/delay.h>
#include "../lib/Service/std_types.h"
#include "../lib/Service/Bit_math.h"
#include "../lib/MCAL/reg_def.h"
#include "../lib/MCAL/DIO/DIO.h"
#include "../lib/MCAL/GIE/GIE.h"
#include "../lib/MCAL/TIMER1/TIMER1_cfg.h"
#include "../lib/MCAL/TIMER2/TIMER2.h"
#include "../lib/MCAL/USART/USART.h"
#include "../lib/MCAL/TWI/TWI.h"
#include "../lib/HAL/PCF8574/PCF8574.h"
#include "../lib/HAL/CLCD/CLCD.h"
#include "../lib/HAL/LCD_BUF/LCD_BUF.h"
#include "../lib/HAL/LCD_BUF/LCD_BUF_cfg.h"
#include "../lib/HAL/KPAD/KPAD.h"
#include "../lib/HAL/BUTTON/BUTTON.h"
#include "../lib/HAL/SEVEN_SEG/SEVEN_SEG.h"
#include "../lib/HAL/SEVEN_SEG/SEVEN_SEG_cfg.h"
#include "../lib/HAL/LM35/LM35.h"
#include "../lib/HAL/RELAY/RELAY.h"
#include "../lib/HAL/RELAY/RELAY_cfg.h"
#include "../lib/HAL/LED/LED.h"
#include "../lib/HAL/LED/LED_cfg.h"
#include "../lib/HAL/BUZZER/BUZZER.h"
#include "../lib/HAL/BUZZER/BUZZER_cfg.h"
#include "../lib/HAL/LAMP/LAMP.h"
#include "../lib/HAL/LAMP/LAMP_cfg.h"
#include "../lib/HAL/FAN/FAN.h"
#include "../lib/HAL/SERVO/SERVO.h"
#include "../lib/HAL/DIMMER/DIMMER.h"
#include "../lib/HAL/EXT_EEPROM/EXT_EEPROM.h"
#include "../lib/HAL/EXT_EEPROM/EXT_EEPROM_cfg.h"

/* Keep text in flash , not RAM (2 KB RAM only). Same idea as avr-libc PSTR() ,
 * but with the GCC "__flash" keyword , so no <avr/pgmspace.h> is needed
 * (that header pulls <avr/io.h> , which clashes with reg_def.h). */
#define FLASH_STR(str)    (__extension__({ static const __flash c8 Local_c8Text[] = (str); &Local_c8Text[0]; }))

/* Test configuration */
#define TEST_LED_PORT             DIO_PORTA      /* status LED = heater LED pin */
#define TEST_LED_PIN              DIO_PIN_3
#define TEST_BLINK_MS             250            /* 250 ms --> 2 Hz blink */
#define TEST_FAIL_BLINK_MS        100            /* 100 ms --> 5 Hz blink */
#define TEST_MANUAL_TIMEOUT_MS    15000
#define TEST_HOLD_MS              2000
#define TEST_EE_WAIT_MS           50

/* Timer2 : prescaler 64 , CTC , OCR2 = F_CPU / 64 / 1000 - 1  (16 MHz -> 249 = 1 ms) */
#define TEST_T2_OCR               ((u8)(F_CPU / 64UL / 1000UL - 1UL))
/* Timer1 counts 0 .. TOP : used as a stop watch (1 tick = 0.5 us at 16 MHz) */
#define TEST_T1_PERIOD            ((u16)(TIMER1_TOP_VALUE + 1))
#define TEST_T1_TICKS_PER_US_X2   ((u16)(TIMER1_TICKS_PER_MS / 500))   /* ticks per 2 us */
#define TEST_LM35_MAX_VALID       600            /* 150 C at 4 steps per C */

#define TEST_EE_PERSIST_ADDR      0x03F0         /* last page : must still be there after a RESET */
#define TEST_EE_BLOCK_A_ADDR      0x00F0         /* page before the 256-byte border */
#define TEST_EE_BLOCK_B_ADDR      0x0100         /* page after the border */

static u8 Global_u8PassCount = 0 ;
static u8 Global_u8FailCount = 0 ;
static u8 Global_u8SkipCount = 0 ;
static u8 Global_u8BlinkEnabled = 1 ;
static u16 Global_u16BlinkMs = 0 ;
static u16 Global_u16MaxUpdateTicks = 0 ;      /* longest LCD_BUF_voidUpdate call , in Timer1 ticks */

/* written by the Timer2 ISR , read by the main loop (single bytes , 16 bit read twice) */
static volatile u16 Global_u16Ms = 0 ;
static volatile u8  Global_u8Flag5 = 0 ;
static volatile u8  Global_u8Flag10 = 0 ;

/*************************** print helpers ***************************/
static void TEST_voidPrint(const __flash c8 * Copy_pc8Text){
	while(*Copy_pc8Text != '\0'){
		USART_voidSend(*Copy_pc8Text++);
	}
}
static void TEST_voidNewLine(){
	USART_voidSend('\r');
	USART_voidSend('\n');
}
static void TEST_voidPrintLine(const __flash c8 * Copy_pc8Text){
	TEST_voidPrint(Copy_pc8Text);
	TEST_voidNewLine();
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
/* quarter degrees --> "25.75" */
static void TEST_voidPrintTemp(u16 Copy_u16TempX4){
	TEST_voidPrintNum(Copy_u16TempX4 / 4);
	USART_voidSend('.');
	TEST_voidPrintNum((Copy_u16TempX4 % 4) * 25);
	if((Copy_u16TempX4 % 4) == 0){
		USART_voidSend('0');
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
/* not a pass or a fail : something the simulation model does differently from the real chip */
static void TEST_voidInfo(const __flash c8 * Copy_pc8Module, const __flash c8 * Copy_pc8Text){
	TEST_voidPrint(FLASH_STR("[INFO] "));
	TEST_voidPrint(Copy_pc8Module);
	TEST_voidPrint(FLASH_STR(": "));
	TEST_voidPrintLine(Copy_pc8Text);
}
static void TEST_voidManual(const __flash c8 * Copy_pc8Text){
	TEST_voidPrint(FLASH_STR("MANUAL: "));
	TEST_voidPrintLine(Copy_pc8Text);
}

/*************************** time base ***************************/
/* Timer2 ISR callback , every 1 ms */
static void TEST_voidTickCallback(void){
	Global_u16Ms++;
	if((Global_u16Ms % 5) == 0){
		Global_u8Flag5 = 1 ;
	}
	if((Global_u16Ms % 10) == 0){
		Global_u8Flag10 = 1 ;
	}
}
static u16 TEST_u16Now(){
	u16 Local_u16A ;
	u16 Local_u16B ;
	do{
		Local_u16A = Global_u16Ms ;
		Local_u16B = Global_u16Ms ;
	}while(Local_u16A != Local_u16B);
	return Local_u16A ;
}
/* 16-bit timer register : low byte first (it latches the high byte) */
static u16 TEST_u16ReadTCNT1(){
	u16 Local_u16Value = TCNT1L ;
	Local_u16Value |= ((u16)TCNT1H << 8) ;
	return Local_u16Value ;
}
static u16 TEST_u16ReadOCR1A(){
	u16 Local_u16Value = OCR1AL ;
	Local_u16Value |= ((u16)OCR1AH << 8) ;
	return Local_u16Value ;
}
static u16 TEST_u16ReadOCR1B(){
	u16 Local_u16Value = OCR1BL ;
	Local_u16Value |= ((u16)OCR1BH << 8) ;
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

/* what the "scheduler" does : LCD every 5 ms , keypad + buttons + status LED every 10 ms */
static void TEST_voidService(){
	if(Global_u8Flag5){
		u16 Local_u16Start = TEST_u16ReadTCNT1();
		u16 Local_u16Ticks ;
		Global_u8Flag5 = 0 ;
		LCD_BUF_voidUpdate();
		Local_u16Ticks = TEST_u16TicksSince(Local_u16Start);
		if(Local_u16Ticks > Global_u16MaxUpdateTicks){
			Global_u16MaxUpdateTicks = Local_u16Ticks ;
		}
	}
	if(Global_u8Flag10){
		Global_u8Flag10 = 0 ;
		KPAD_voidUpdate();
		BUTTON_voidUpdate();
		if(Global_u8BlinkEnabled){
			Global_u16BlinkMs += 10 ;
			if(Global_u16BlinkMs >= TEST_BLINK_MS){
				Global_u16BlinkMs = 0 ;
				DIO_voidTogPin(TEST_LED_PORT,TEST_LED_PIN);
			}
		}
	}
}
static void TEST_voidWaitMs(u16 Copy_u16Ms){
	u16 Local_u16Start = TEST_u16Now();
	while((u16)(TEST_u16Now() - Local_u16Start) < Copy_u16Ms){
		TEST_voidService();
	}
}

/* returns 1 when the 16 key characters contain this character */
static u8 TEST_u8IsKeyChar(u8 Copy_u8Key){
	const __flash c8 * Local_pc8Keys = FLASH_STR("789/456*123-C0=+");
	while(*Local_pc8Keys != '\0'){
		if((u8)*Local_pc8Keys++ == Copy_u8Key){
			return 1 ;
		}
	}
	return 0 ;
}

/*************************** tests ***************************/
static void TEST_voidPcf8574(){
	u8 Local_u8Value = 0 ;
	u8 Local_au8Two[2] = {0x5A,0xFF} ;
	PCF8574_voidInit();
	TEST_voidCheck(PCF8574_u8WritePort(LAMP_I2C_ADDRESS,0xA5) == TWI_OK, FLASH_STR("PCF8574"), FLASH_STR("write port 0x20 is acknowledged"));
	TEST_voidCheck((PCF8574_u8ReadPort(LAMP_I2C_ADDRESS,&Local_u8Value) == TWI_OK) && (Local_u8Value == 0xA5),
			FLASH_STR("PCF8574"), FLASH_STR("read back equals written (0xA5)"));
	TEST_voidCheck(PCF8574_u8WriteBytes(LAMP_I2C_ADDRESS,Local_au8Two,2) == TWI_OK, FLASH_STR("PCF8574"), FLASH_STR("WriteBytes sends 2 values in one transaction"));
	TEST_voidCheck((PCF8574_u8ReadPort(LAMP_I2C_ADDRESS,&Local_u8Value) == TWI_OK) && (Local_u8Value == 0xFF),
			FLASH_STR("PCF8574"), FLASH_STR("last of the 2 values (0xFF) is on the port"));
	TEST_voidCheck((PCF8574_u8WritePort(SEVEN_SEG_TENS_ADDRESS,0xFF) == TWI_OK) && (PCF8574_u8WritePort(SEVEN_SEG_UNITS_ADDRESS,0xFF) == TWI_OK),
			FLASH_STR("PCF8574"), FLASH_STR("7-segment chips 0x21 and 0x22 are acknowledged"));
	TEST_voidCheck(PCF8574_u8WritePort(0x60,0xFF) == TWI_ERR_SLA_NACK, FLASH_STR("PCF8574"), FLASH_STR("absent address 0x60 gives SLA NACK"));
}

static void TEST_voidLcd(){
	c8 Local_c8Digits[LCD_BUF_COLS + 1] ;
	u16 Local_u16Start ;
	u16 Local_u16Ticks ;
	/*1. CLCD alone (blocking driver) */
	CLCD_voidInit();
	TEST_voidCheck(CLCD_u8GetStatus() == TWI_OK, FLASH_STR("CLCD"), FLASH_STR("init : LCD PCF8574 (0x27) answers"));
	CLCD_voidSetCursorPosition(0,0);
	CLCD_voidSendFlashString(FLASH_STR("CLCD direct test"));
	TEST_voidManual(FLASH_STR("LCD shows \"CLCD direct test\" on line 1 , no cursor , no blinking block (1.5 s)"));
	TEST_voidWaitMs(1500);
	/* one byte = one I2C transaction of 4 port values : about 0.5 ms */
	Local_u16Start = TEST_u16ReadTCNT1();
	CLCD_voidSendData('!');
	Local_u16Ticks = TEST_u16TicksSince(Local_u16Start);
	TEST_voidPrint(FLASH_STR("       one LCD byte takes "));
	TEST_voidPrintNum(Local_u16Ticks / TEST_T1_TICKS_PER_US_X2 * 2);
	TEST_voidPrintLine(FLASH_STR(" us"));
	TEST_voidCheck(Local_u16Ticks < (TIMER1_TICKS_PER_MS * 4 / 5), FLASH_STR("CLCD"), FLASH_STR("one LCD byte takes less than 0.8 ms"));
	CLCD_voidClearDisp();
	TEST_voidCheck(CLCD_u8GetStatus() == TWI_OK, FLASH_STR("CLCD"), FLASH_STR("send data , clear : no I2C error"));

	/*2. LCD_BUF : writers are instant , the 5 ms update draws the screen */
	LCD_BUF_voidInit();
	for(u8 i = 0 ; i < LCD_BUF_COLS ; i++){
		Local_c8Digits[i] = (i < 10) ? ('0' + i) : ('A' + (i - 10)) ;
	}
	Local_c8Digits[LCD_BUF_COLS] = '\0' ;
	LCD_BUF_voidWriteFlash(0,0,FLASH_STR("LCD_BUF test"));
	LCD_BUF_voidWriteChar(0,15,'#');
	LCD_BUF_voidWriteRam(1,0,Local_c8Digits);
	LCD_BUF_voidWriteFlash(1,12,FLASH_STR("WXYZ12345"));     /* too long : must be cut at the row end */
	LCD_BUF_voidWriteChar(2,0,'!');                           /* row 2 does not exist : ignored */
	Global_u16MaxUpdateTicks = 0 ;
	TEST_voidManual(FLASH_STR("line 1 = \"LCD_BUF test   #\" , line 2 = \"0123456789ABWXYZ\" (cut at 16 characters)"));
	TEST_voidWaitMs(600);      /* a full redraw takes about 175 ms */
	TEST_voidCheck(CLCD_u8GetStatus() == TWI_OK, FLASH_STR("LCD_BUF"), FLASH_STR("full redraw : no I2C error"));
	TEST_voidPrint(FLASH_STR("       longest LCD_BUF_voidUpdate call = "));
	TEST_voidPrintNum(Global_u16MaxUpdateTicks / TEST_T1_TICKS_PER_US_X2 * 2);
	TEST_voidPrintLine(FLASH_STR(" us"));
	TEST_voidCheck((Global_u16MaxUpdateTicks > 0) && (Global_u16MaxUpdateTicks < TIMER1_TICKS_PER_MS), FLASH_STR("LCD_BUF"), FLASH_STR("longest update call is below 1 ms"));
	/* only the changed cell is sent */
	Global_u16MaxUpdateTicks = 0 ;
	LCD_BUF_voidWriteChar(0,15,'*');
	TEST_voidWaitMs(100);
	TEST_voidCheck(CLCD_u8GetStatus() == TWI_OK, FLASH_STR("LCD_BUF"), FLASH_STR("one changed character : no I2C error"));
	TEST_voidManual(FLASH_STR("the '#' at the end of line 1 changed to '*'"));
	LCD_BUF_voidClear();
	LCD_BUF_voidWriteFlash(0,0,FLASH_STR("test_hal running"));
}

static void TEST_voidOutputs(){
	/* the status LED shares PA3 with the heater LED : stop its blinking for this test */
	Global_u8BlinkEnabled = 0 ;
	RELAY_voidInit();
	TEST_voidCheck((DIO_u8GetPinValue(RELAY_HEATING_PORT,RELAY_HEATING_PIN) != RELAY_ON_LEVEL) && (DIO_u8GetPinValue(RELAY_COOLING_PORT,RELAY_COOLING_PIN) != RELAY_ON_LEVEL),
			FLASH_STR("RELAY"), FLASH_STR("init : both relays OFF"));
	TEST_voidManual(FLASH_STR("heating relay (PB6) ON 1 s , then cooling relay (PB7) ON 1 s"));
	RELAY_voidOn(RELAY_HEATING);
	TEST_voidCheck((DIO_u8GetPinValue(RELAY_HEATING_PORT,RELAY_HEATING_PIN) == RELAY_ON_LEVEL) && (DIO_u8GetPinValue(RELAY_COOLING_PORT,RELAY_COOLING_PIN) != RELAY_ON_LEVEL),
			FLASH_STR("RELAY"), FLASH_STR("heating ON , cooling stays OFF"));
	TEST_voidWaitMs(1000);
	RELAY_voidOff(RELAY_HEATING);
	RELAY_voidOn(RELAY_COOLING);
	TEST_voidCheck((DIO_u8GetPinValue(RELAY_HEATING_PORT,RELAY_HEATING_PIN) != RELAY_ON_LEVEL) && (DIO_u8GetPinValue(RELAY_COOLING_PORT,RELAY_COOLING_PIN) == RELAY_ON_LEVEL),
			FLASH_STR("RELAY"), FLASH_STR("heating OFF , cooling ON"));
	TEST_voidWaitMs(1000);
	RELAY_voidOff(RELAY_COOLING);
	TEST_voidCheck(DIO_u8GetPinValue(RELAY_COOLING_PORT,RELAY_COOLING_PIN) != RELAY_ON_LEVEL, FLASH_STR("RELAY"), FLASH_STR("cooling OFF again"));

	LED_voidInit();
	TEST_voidCheck(DIO_u8GetPinValue(LED_HEATER_PORT,LED_HEATER_PIN) != LED_ON_LEVEL, FLASH_STR("LED"), FLASH_STR("init : LED OFF"));
	TEST_voidManual(FLASH_STR("status / heater LED (PA3) ON for 1 s"));
	LED_voidOn(LED_HEATER);
	TEST_voidCheck(DIO_u8GetPinValue(LED_HEATER_PORT,LED_HEATER_PIN) == LED_ON_LEVEL, FLASH_STR("LED"), FLASH_STR("LED ON"));
	TEST_voidWaitMs(1000);
	LED_voidOff(LED_HEATER);
	TEST_voidCheck(DIO_u8GetPinValue(LED_HEATER_PORT,LED_HEATER_PIN) != LED_ON_LEVEL, FLASH_STR("LED"), FLASH_STR("LED OFF"));
	Global_u8BlinkEnabled = 1 ;

	BUZZER_voidInit();
	TEST_voidCheck(DIO_u8GetPinValue(BUZZER_PORT,BUZZER_PIN) != BUZZER_ON_LEVEL, FLASH_STR("BUZZER"), FLASH_STR("init : buzzer OFF"));
	TEST_voidManual(FLASH_STR("buzzer sounds for 1 s"));
	BUZZER_voidOn();
	TEST_voidCheck(DIO_u8GetPinValue(BUZZER_PORT,BUZZER_PIN) == BUZZER_ON_LEVEL, FLASH_STR("BUZZER"), FLASH_STR("buzzer ON"));
	TEST_voidWaitMs(1000);
	BUZZER_voidOff();
	TEST_voidCheck(DIO_u8GetPinValue(BUZZER_PORT,BUZZER_PIN) != BUZZER_ON_LEVEL, FLASH_STR("BUZZER"), FLASH_STR("buzzer OFF"));
}

static void TEST_voidLamps(){
	u8 Local_u8Ok = 1 ;
	u8 Local_u8Port = 0 ;
	u8 Local_u8Expected ;
	LAMP_voidInit();
	TEST_voidCheck((PCF8574_u8ReadPort(LAMP_I2C_ADDRESS,&Local_u8Port) == TWI_OK) && (Local_u8Port == 0xFF), FLASH_STR("LAMP"), FLASH_STR("init : all lamps OFF (port = 0xFF)"));
	TEST_voidManual(FLASH_STR("lamps 1..5 light one after the other (400 ms) , then all five , then all off"));
	for(u8 Local_u8Lamp = 1 ; Local_u8Lamp <= LAMP_COUNT ; Local_u8Lamp++){
		u8 Local_u8Bit = LAMP_FIRST_BIT + (Local_u8Lamp - 1) ;
		Local_u8Expected = (u8)~(1 << Local_u8Bit) ;     /* active low : only this lamp bit is 0 */
		if(LAMP_u8SetState(Local_u8Lamp,LAMP_ON) != TWI_OK){ Local_u8Ok = 0 ; }
		if(LAMP_u8GetState(Local_u8Lamp) != LAMP_ON){ Local_u8Ok = 0 ; }
		if((PCF8574_u8ReadPort(LAMP_I2C_ADDRESS,&Local_u8Port) != TWI_OK) || (Local_u8Port != Local_u8Expected)){ Local_u8Ok = 0 ; }
		TEST_voidWaitMs(400);
		if(LAMP_u8SetState(Local_u8Lamp,LAMP_OFF) != TWI_OK){ Local_u8Ok = 0 ; }
		if(LAMP_u8GetState(Local_u8Lamp) != LAMP_OFF){ Local_u8Ok = 0 ; }
	}
	TEST_voidCheck(Local_u8Ok, FLASH_STR("LAMP"), FLASH_STR("chase 1..5 : write , GetState and the chip port agree"));
	/* all on */
	Local_u8Ok = 1 ;
	for(u8 Local_u8Lamp = 1 ; Local_u8Lamp <= LAMP_COUNT ; Local_u8Lamp++){
		if(LAMP_u8SetState(Local_u8Lamp,LAMP_ON) != TWI_OK){ Local_u8Ok = 0 ; }
	}
	Local_u8Expected = 0xFF ;
	for(u8 i = 0 ; i < LAMP_COUNT ; i++){
		Local_u8Expected &= (u8)~(1 << (LAMP_FIRST_BIT + i)) ;
	}
	TEST_voidCheck(Local_u8Ok && (PCF8574_u8ReadPort(LAMP_I2C_ADDRESS,&Local_u8Port) == TWI_OK) && (Local_u8Port == Local_u8Expected),
			FLASH_STR("LAMP"), FLASH_STR("all 5 ON : lamp bits 0 , other pins stay 1 (0xE0)"));
	TEST_voidWaitMs(800);
	for(u8 Local_u8Lamp = 1 ; Local_u8Lamp <= LAMP_COUNT ; Local_u8Lamp++){
		LAMP_u8SetState(Local_u8Lamp,LAMP_OFF);
	}
	TEST_voidCheck((PCF8574_u8ReadPort(LAMP_I2C_ADDRESS,&Local_u8Port) == TWI_OK) && (Local_u8Port == 0xFF), FLASH_STR("LAMP"), FLASH_STR("all OFF again (port = 0xFF)"));
	TEST_voidCheck((LAMP_u8SetState(0,LAMP_ON) == LAMP_ERR_NUMBER) && (LAMP_u8SetState(LAMP_COUNT + 1,LAMP_ON) == LAMP_ERR_NUMBER) && (LAMP_u8GetState(LAMP_COUNT + 1) == LAMP_OFF),
			FLASH_STR("LAMP"), FLASH_STR("lamp number 0 and 6 are rejected"));
}

/* "NN" on the terminal */
static void TEST_voidPrintTwoDigits(u8 Copy_u8Number){
	USART_voidSend('0' + (Copy_u8Number / 10));
	USART_voidSend('0' + (Copy_u8Number % 10));
}
/* show one number and say it on the terminal : "  7SEG shows 37 (tens 0xB0 units 0xF8)" */
static void TEST_voidShowStep(u8 Copy_u8Number, u16 Copy_u16Ms){
	SEVEN_SEG_voidSetNumber(Copy_u8Number);
	TEST_voidPrint(FLASH_STR("  look: "));
	TEST_voidPrintTwoDigits(Copy_u8Number);
	TEST_voidNewLine();
	TEST_voidWaitMs(Copy_u16Ms);
}
static void TEST_voidSevenSeg(){
	/* The chip ports are NOT read back here : in Proteus a pin that drives a segment LED reads
	 * a different value than the one written, while the display itself shows the right digit.
	 * So the digits are checked by eye (see CLAUDE.md, Proteus notes). */
	SEVEN_SEG_voidInit();
	TEST_voidCheck(SEVEN_SEG_u8GetStatus() == TWI_OK, FLASH_STR("SEVEN_SEG"), FLASH_STR("init: chips 0x21 0x22 answer"));
	SEVEN_SEG_voidEnable();
	TEST_voidCheck(SEVEN_SEG_u8GetStatus() == TWI_OK, FLASH_STR("SEVEN_SEG"), FLASH_STR("Enable: no I2C error"));
	SEVEN_SEG_voidSetNumber(150);
	TEST_voidCheck(SEVEN_SEG_u8GetStatus() == TWI_OK, FLASH_STR("SEVEN_SEG"), FLASH_STR("150: no I2C error"));
	SEVEN_SEG_voidDisable();

	/* MANUAL : slow, one short line per step. Compare the line with the display. */
	TEST_voidManual(FLASH_STR("7-seg: blank now (3 s)"));
	TEST_voidWaitMs(3000);
	SEVEN_SEG_voidEnable();
	TEST_voidManual(FLASH_STR("same digit twice, 1 s each"));
	for(u8 i = 0 ; i < 10 ; i++){
		TEST_voidShowStep(i * 11,1000);
	}
	TEST_voidManual(FLASH_STR("different digits, 1 s each"));
	TEST_voidShowStep(37,1000);
	TEST_voidShowStep(73,1000);
	TEST_voidShowStep(10,1000);
	TEST_voidShowStep(99,1000);
	TEST_voidManual(FLASH_STR("150 must show 99 (1 s)"));
	SEVEN_SEG_voidSetNumber(150);
	TEST_voidWaitMs(1000);
	TEST_voidManual(FLASH_STR("counting 00-99, 500 ms per step up to 55, then 300 ms"));
	for(u8 i = 0 ; i < 100 ; i++){
		TEST_voidShowStep(i,(i < 55) ? 500 : 300);
	}
	TEST_voidManual(FLASH_STR("88: every segment (3 s)"));
	TEST_voidShowStep(88,3000);
	TEST_voidManual(FLASH_STR("Disable: blank (3 s)"));
	SEVEN_SEG_voidDisable();
	TEST_voidWaitMs(3000);
	TEST_voidManual(FLASH_STR("Enable: 88 comes back (1 s)"));
	SEVEN_SEG_voidEnable();
	TEST_voidWaitMs(1000);
	SEVEN_SEG_voidDisable();
}

static void TEST_voidLm35(){
	u16 Local_u16Ambient ;
	u16 Local_u16Water ;
	u16 Local_u16Again ;
	LM35_voidInit();
	Local_u16Ambient = LM35_u16ReadTempX4(LM35_AMBIENT);
	Local_u16Water   = LM35_u16ReadTempX4(LM35_WATER);
	TEST_voidCheck((Local_u16Ambient <= TEST_LM35_MAX_VALID) && (Local_u16Water <= TEST_LM35_MAX_VALID), FLASH_STR("LM35"), FLASH_STR("both sensors read 0..150 C"));
	Local_u16Again = LM35_u16ReadTempX4(LM35_AMBIENT);
	TEST_voidCheck((Local_u16Again + 2 >= Local_u16Ambient) && (Local_u16Again <= Local_u16Ambient + 2), FLASH_STR("LM35"), FLASH_STR("two reads of the same sensor are within 0.5 C"));
	TEST_voidCheck(LM35_u16ReadTempX4(7) == 0, FLASH_STR("LM35"), FLASH_STR("unknown sensor ID reads 0"));
	TEST_voidManual(FLASH_STR("move the two LM35 sliders : the printed value must follow (10 mV per C) , 5 s"));
	for(u8 i = 0 ; i < 5 ; i++){
		TEST_voidPrint(FLASH_STR("       ambient = "));
		TEST_voidPrintTemp(LM35_u16ReadTempX4(LM35_AMBIENT));
		TEST_voidPrint(FLASH_STR(" C , water = "));
		TEST_voidPrintTemp(LM35_u16ReadTempX4(LM35_WATER));
		TEST_voidPrintLine(FLASH_STR(" C"));
		TEST_voidWaitMs(1000);
	}
}

/* wait for a key , KPAD_NO_KEY on timeout */
static u8 TEST_u8WaitKey(u16 Copy_u16TimeoutMs){
	u16 Local_u16Start = TEST_u16Now();
	u8 Local_u8Key = KPAD_NO_KEY ;
	while(((u16)(TEST_u16Now() - Local_u16Start) < Copy_u16TimeoutMs) && (Local_u8Key == KPAD_NO_KEY)){
		TEST_voidService();
		Local_u8Key = KPAD_u8GetKey();
	}
	return Local_u8Key ;
}
/* count the keys reported during a time */
static u8 TEST_u8CountKeys(u16 Copy_u16Ms){
	u16 Local_u16Start = TEST_u16Now();
	u8 Local_u8Count = 0 ;
	while((u16)(TEST_u16Now() - Local_u16Start) < Copy_u16Ms){
		TEST_voidService();
		if(KPAD_u8GetKey() != KPAD_NO_KEY){
			Local_u8Count++;
		}
	}
	return Local_u8Count ;
}
static void TEST_voidKeypad(){
	u8 Local_u8Key ;
	u8 Local_u8Count = 0 ;
	u8 Local_u8Extra ;
	u8 Local_u8Ok = 1 ;
	const __flash c8 * Local_pc8Order = FLASH_STR("7C+");
	KPAD_voidInit();
	/*1. a held key gives ONE event */
	TEST_voidManual(FLASH_STR("KEYPAD : press ANY key and HOLD it for 2 s (15 s time out)"));
	KPAD_u8GetKey();
	Local_u8Key = TEST_u8WaitKey(TEST_MANUAL_TIMEOUT_MS);
	if(Local_u8Key == KPAD_NO_KEY){
		TEST_voidSkip(FLASH_STR("KPAD"), FLASH_STR("no key pressed (held key test)"));
	}else{
		TEST_voidPrint(FLASH_STR("       key = "));
		USART_voidSend(Local_u8Key);
		TEST_voidNewLine();
		TEST_voidCheck(TEST_u8IsKeyChar(Local_u8Key), FLASH_STR("KPAD"), FLASH_STR("the key is one of the 16 key characters"));
		Local_u8Extra = TEST_u8CountKeys(TEST_HOLD_MS);
		TEST_voidCheck(Local_u8Extra == 0, FLASH_STR("KPAD"), FLASH_STR("a held key is reported only once"));
	}
	/*2. three separate presses give three events */
	TEST_voidManual(FLASH_STR("KEYPAD : release , then press 3 keys one after the other , quick taps (15 s time out)"));
	TEST_voidWaitMs(500);
	KPAD_u8GetKey();
	while(Local_u8Count < 3){
		Local_u8Key = TEST_u8WaitKey(TEST_MANUAL_TIMEOUT_MS);
		if(Local_u8Key == KPAD_NO_KEY){
			break ;
		}
		Local_u8Count++;
		TEST_voidPrint(FLASH_STR("       key = "));
		USART_voidSend(Local_u8Key);
		TEST_voidNewLine();
		if(TEST_u8IsKeyChar(Local_u8Key) == 0){
			Local_u8Ok = 0 ;
		}
	}
	if(Local_u8Count < 3){
		TEST_voidSkip(FLASH_STR("KPAD"), FLASH_STR("fewer than 3 keys pressed"));
	}else{
		Local_u8Extra = TEST_u8CountKeys(1000);
		TEST_voidCheck(Local_u8Ok && (Local_u8Extra == 0), FLASH_STR("KPAD"), FLASH_STR("3 presses give exactly 3 events (no extra ones)"));
	}
	/*3. matrix mapping : corner keys */
	TEST_voidManual(FLASH_STR("KEYPAD : press 7 , then C , then + (10 s each)"));
	Local_u8Ok = 1 ;
	for(u8 i = 0 ; i < 3 ; i++){
		Local_u8Key = TEST_u8WaitKey(10000);
		if(Local_u8Key == KPAD_NO_KEY){
			TEST_voidSkip(FLASH_STR("KPAD"), FLASH_STR("corner key not pressed"));
			Local_u8Ok = 2 ;
			break ;
		}
		if(Local_u8Key != (u8)Local_pc8Order[i]){
			Local_u8Ok = 0 ;
		}
	}
	if(Local_u8Ok != 2){
		TEST_voidCheck(Local_u8Ok, FLASH_STR("KPAD"), FLASH_STR("keys 7 , C , + are reported as 7 , C , + (matrix mapping)"));
	}
}

static void TEST_voidOneButton(u8 Copy_u8ButtonID, const __flash c8 * Copy_pc8Name){
	u16 Local_u16Start ;
	u8 Local_u8Pressed = 0 ;
	u8 Local_u8Extra = 0 ;
	u8 Local_u8StillDown = 1 ;
	TEST_voidPrint(FLASH_STR("MANUAL: BUTTON "));
	TEST_voidPrint(Copy_pc8Name);
	TEST_voidPrintLine(FLASH_STR(" : press and HOLD for 2 s , then release (15 s time out)"));
	BUTTON_u8GetPressEvent(Copy_u8ButtonID);
	BUTTON_u8GetReleaseEvent(Copy_u8ButtonID);
	Local_u16Start = TEST_u16Now();
	while(((u16)(TEST_u16Now() - Local_u16Start) < TEST_MANUAL_TIMEOUT_MS) && (Local_u8Pressed == 0)){
		TEST_voidService();
		Local_u8Pressed = BUTTON_u8GetPressEvent(Copy_u8ButtonID);
	}
	if(Local_u8Pressed == 0){
		TEST_voidSkip(FLASH_STR("BUTTON"), FLASH_STR("button not pressed"));
		return ;
	}
	TEST_voidCheck(BUTTON_u8IsPressed(Copy_u8ButtonID) == 1, FLASH_STR("BUTTON"), FLASH_STR("pressed : debounced level is 1"));
	/* hold : no second press event , no release event */
	Local_u16Start = TEST_u16Now();
	while((u16)(TEST_u16Now() - Local_u16Start) < 1500){
		TEST_voidService();
		Local_u8Extra += BUTTON_u8GetPressEvent(Copy_u8ButtonID);
		if(BUTTON_u8IsPressed(Copy_u8ButtonID) == 0){
			Local_u8StillDown = 0 ;
		}
	}
	TEST_voidCheck((Local_u8Extra == 0) && Local_u8StillDown, FLASH_STR("BUTTON"), FLASH_STR("held : still pressed , only one press event"));
	/* release */
	Local_u16Start = TEST_u16Now();
	while(((u16)(TEST_u16Now() - Local_u16Start) < TEST_MANUAL_TIMEOUT_MS) && BUTTON_u8IsPressed(Copy_u8ButtonID)){
		TEST_voidService();
	}
	TEST_voidWaitMs(100);
	if(BUTTON_u8IsPressed(Copy_u8ButtonID)){
		TEST_voidSkip(FLASH_STR("BUTTON"), FLASH_STR("button not released"));
	}else{
		TEST_voidCheck((BUTTON_u8GetReleaseEvent(Copy_u8ButtonID) == 1) && (BUTTON_u8GetReleaseEvent(Copy_u8ButtonID) == 0),
				FLASH_STR("BUTTON"), FLASH_STR("released : one release event , cleared by reading"));
	}
}
static void TEST_voidButtons(){
	BUTTON_voidInit();
	TEST_voidCheck((BUTTON_u8IsPressed(BUTTON_ONOFF) == 0) || (BUTTON_u8IsPressed(BUTTON_UP) == 0) || (BUTTON_u8IsPressed(BUTTON_DOWN) == 0),
			FLASH_STR("BUTTON"), FLASH_STR("init : buttons start as released"));
	TEST_voidOneButton(BUTTON_ONOFF,FLASH_STR("ON/OFF (PD6)"));
	TEST_voidOneButton(BUTTON_UP,FLASH_STR("UP (PD7)"));
	TEST_voidOneButton(BUTTON_DOWN,FLASH_STR("DOWN (PB5)"));
}

static void TEST_voidServo(){
	u16 Local_u16Ms = TIMER1_TICKS_PER_MS ;
	SERVO_voidInit();
	TEST_voidManual(FLASH_STR("SERVO : arm moves 0 , 90 , 180 degrees (2.5 s each)"));
	TEST_voidCheck(TEST_u16ReadOCR1A() == Local_u16Ms, FLASH_STR("SERVO"), FLASH_STR("init : 0 degrees = 1.0 ms pulse (OCR1A = 2000)"));
	TEST_voidWaitMs(2500);
	SERVO_voidSetAngle(90);
	TEST_voidCheck(TEST_u16ReadOCR1A() == (Local_u16Ms + (Local_u16Ms / 2)), FLASH_STR("SERVO"), FLASH_STR("90 degrees = 1.5 ms pulse (OCR1A = 3000)"));
	TEST_voidWaitMs(2500);
	SERVO_voidSetAngle(180);
	TEST_voidCheck(TEST_u16ReadOCR1A() == (Local_u16Ms * 2), FLASH_STR("SERVO"), FLASH_STR("180 degrees = 2.0 ms pulse (OCR1A = 4000)"));
	TEST_voidWaitMs(2500);
	SERVO_voidSetAngle(255);
	TEST_voidCheck(TEST_u16ReadOCR1A() == (Local_u16Ms * 2), FLASH_STR("SERVO"), FLASH_STR("angle above 180 is limited to 180"));
	SERVO_voidSetAngle(0);
	TEST_voidWaitMs(1000);
}

static void TEST_voidFan(){
	u8 Local_u8Ok = 1 ;
	FAN_voidInit();
	TEST_voidCheck((TCCR1A & ((1<<TCCR1A_COM1B1) | (1<<TCCR1A_COM1B0))) == 0, FLASH_STR("FAN"), FLASH_STR("init : PWM output disconnected (fan stopped)"));
	TEST_voidManual(FLASH_STR("FAN : speed ramps 0 .. 100 % in steps of 10 % (1 s each) , the motor speeds up"));
	for(u8 i = 0 ; i <= 100 ; i += 10){
		FAN_voidSetSpeed(i);
		if(i == 0){
			if((TCCR1A & ((1<<TCCR1A_COM1B1) | (1<<TCCR1A_COM1B0))) != 0){ Local_u8Ok = 0 ; }
		}else if(i == 100){
			if(TEST_u16ReadOCR1B() != TIMER1_TOP_VALUE){ Local_u8Ok = 0 ; }
		}else{
			/* i % of (TOP + 1) : 10 % = 4000 */
			if(TEST_u16ReadOCR1B() != (u16)(((u32)i * TEST_T1_PERIOD) / 100)){ Local_u8Ok = 0 ; }
		}
		TEST_voidWaitMs(1000);
	}
	TEST_voidCheck(Local_u8Ok, FLASH_STR("FAN"), FLASH_STR("0 % disconnected , 10..90 % = percent x 40000 / 100 , 100 % = TOP"));
	FAN_voidSetSpeed(200);
	TEST_voidCheck(TEST_u16ReadOCR1B() == TIMER1_TOP_VALUE, FLASH_STR("FAN"), FLASH_STR("speed above 100 is limited to 100 %"));
	FAN_voidSetSpeed(0);
	TEST_voidCheck((TCCR1A & ((1<<TCCR1A_COM1B1) | (1<<TCCR1A_COM1B0))) == 0, FLASH_STR("FAN"), FLASH_STR("0 % : PWM disconnected again"));
}

static void TEST_voidDimmer(){
	u8 Local_u8Ok = 1 ;
	DIMMER_voidInit();
	TEST_voidCheck((TCCR0 & ((1<<TCCR0_COM01) | (1<<TCCR0_COM00))) == 0, FLASH_STR("DIMMER"), FLASH_STR("init : level 0 (PWM output disconnected)"));
	TEST_voidManual(FLASH_STR("DIMMER : level ramps 0 .. 100 % in steps of 10 % (1 s each) , the lamp / voltmeter follows"));
	for(u8 i = 0 ; i <= 100 ; i += 10){
		DIMMER_voidSetLevel(i);
		if(i == 0){
			if((TCCR0 & ((1<<TCCR0_COM01) | (1<<TCCR0_COM00))) != 0){ Local_u8Ok = 0 ; }
		}else if(OCR0 != (u8)(((u16)i * 255) / 100)){
			Local_u8Ok = 0 ;
		}
		TEST_voidWaitMs(1000);
	}
	TEST_voidCheck(Local_u8Ok, FLASH_STR("DIMMER"), FLASH_STR("0 % disconnected , 10..100 % = percent x 255 / 100"));
	TEST_voidCheck(OCR0 == 255, FLASH_STR("DIMMER"), FLASH_STR("100 % gives OCR0 = 255"));
	DIMMER_voidSetLevel(250);
	TEST_voidCheck(OCR0 == 255, FLASH_STR("DIMMER"), FLASH_STR("level above 100 is limited to 100 %"));
	DIMMER_voidSetLevel(0);
}

/* wait until the EEPROM answers its address again : returns the milliseconds , 0xFFFF = never */
static u16 TEST_u16WaitEepromReady(){
	u16 Local_u16Start = TEST_u16Now();
	while((u16)(TEST_u16Now() - Local_u16Start) <= TEST_EE_WAIT_MS){
		if(EXT_EEPROM_u8IsReady()){
			return (u16)(TEST_u16Now() - Local_u16Start) ;
		}
		TEST_voidService();
	}
	return 0xFFFF ;
}
static void TEST_voidFillPattern(u8 * Copy_pu8Data, u8 Copy_u8Seed){
	for(u8 i = 0 ; i < EXT_EEPROM_PAGE_SIZE ; i++){
		Copy_pu8Data[i] = (u8)(Copy_u8Seed + (i * 7)) ;
	}
}
static u8 TEST_u8Equal(const u8 * Copy_pu8A, const u8 * Copy_pu8B, u8 Copy_u8Length){
	for(u8 i = 0 ; i < Copy_u8Length ; i++){
		if(Copy_pu8A[i] != Copy_pu8B[i]){
			return 0 ;
		}
	}
	return 1 ;
}
static void TEST_voidEeprom(){
	u8 Local_au8Pattern[EXT_EEPROM_PAGE_SIZE] ;
	u8 Local_au8PatternB[EXT_EEPROM_PAGE_SIZE] ;
	u8 Local_au8Read[EXT_EEPROM_PAGE_SIZE] ;
	u16 Local_u16Ms ;
	EXT_EEPROM_voidInit();
	TEST_voidFillPattern(Local_au8Pattern,0x30);
	TEST_voidFillPattern(Local_au8PatternB,0x90);

	/*1. persistence : did the pattern of the previous run survive the RESET ? */
	if((EXT_EEPROM_u8ReadBlock(TEST_EE_PERSIST_ADDR,Local_au8Read,EXT_EEPROM_PAGE_SIZE) == EXT_EEPROM_OK) && TEST_u8Equal(Local_au8Read,Local_au8Pattern,EXT_EEPROM_PAGE_SIZE)){
		TEST_voidCheck(1, FLASH_STR("EXT_EEPROM"), FLASH_STR("data from before the reset is still there (page 0x3F0)"));
	}else{
		TEST_voidSkip(FLASH_STR("EXT_EEPROM"), FLASH_STR("first run : press RESET after the SUMMARY and run again to check persistence"));
	}

	/*2. page write , busy flag , read back */
	TEST_voidCheck(EXT_EEPROM_u8IsReady() == 1, FLASH_STR("EXT_EEPROM"), FLASH_STR("idle chip answers the ACK poll"));
	TEST_voidCheck(EXT_EEPROM_u8WritePage(TEST_EE_PERSIST_ADDR,Local_au8Pattern,EXT_EEPROM_PAGE_SIZE) == EXT_EEPROM_OK, FLASH_STR("EXT_EEPROM"), FLASH_STR("write 16 bytes (page 0x3F0 , 4th 256-byte block)"));
	/* the real 24C08 is busy for 5 ms after a write ; the Proteus model finishes at once , so "ready" is only an INFO */
	if(EXT_EEPROM_u8IsReady() == 0){
		TEST_voidCheck(1, FLASH_STR("EXT_EEPROM"), FLASH_STR("busy right after the write (IsReady = 0)"));
	}else{
		TEST_voidInfo(FLASH_STR("EXT_EEPROM"), FLASH_STR("ready at once after the write (simulation model has no write-cycle delay)"));
	}
	Local_u16Ms = TEST_u16WaitEepromReady();
	TEST_voidPrint(FLASH_STR("       write cycle took "));
	TEST_voidPrintNum(Local_u16Ms);
	TEST_voidPrintLine(FLASH_STR(" ms"));
	TEST_voidCheck(Local_u16Ms != 0xFFFF, FLASH_STR("EXT_EEPROM"), FLASH_STR("ready again within 50 ms"));
	TEST_voidCheck((EXT_EEPROM_u8ReadBlock(TEST_EE_PERSIST_ADDR,Local_au8Read,EXT_EEPROM_PAGE_SIZE) == EXT_EEPROM_OK) && TEST_u8Equal(Local_au8Read,Local_au8Pattern,EXT_EEPROM_PAGE_SIZE),
			FLASH_STR("EXT_EEPROM"), FLASH_STR("read back equals written"));

	/*3. a read across the border between two 256-byte blocks (two I2C addresses) */
	EXT_EEPROM_u8WritePage(TEST_EE_BLOCK_A_ADDR,Local_au8Pattern,EXT_EEPROM_PAGE_SIZE);
	TEST_u16WaitEepromReady();
	EXT_EEPROM_u8WritePage(TEST_EE_BLOCK_B_ADDR,Local_au8PatternB,EXT_EEPROM_PAGE_SIZE);
	TEST_u16WaitEepromReady();
	TEST_voidCheck((EXT_EEPROM_u8ReadBlock(TEST_EE_BLOCK_A_ADDR + 8,Local_au8Read,EXT_EEPROM_PAGE_SIZE) == EXT_EEPROM_OK)
			&& TEST_u8Equal(Local_au8Read,&Local_au8Pattern[8],8) && TEST_u8Equal(&Local_au8Read[8],Local_au8PatternB,8),
			FLASH_STR("EXT_EEPROM"), FLASH_STR("read across the 0x100 block border (8 + 8 bytes)"));

	/*4. bad arguments are rejected without touching the bus */
	TEST_voidCheck(EXT_EEPROM_u8WritePage(TEST_EE_PERSIST_ADDR + 8,Local_au8Pattern,EXT_EEPROM_PAGE_SIZE) == EXT_EEPROM_ERROR, FLASH_STR("EXT_EEPROM"), FLASH_STR("write that crosses a page border is rejected"));
	TEST_voidCheck(EXT_EEPROM_u8WritePage(TEST_EE_PERSIST_ADDR,Local_au8Pattern,0) == EXT_EEPROM_ERROR, FLASH_STR("EXT_EEPROM"), FLASH_STR("write of 0 bytes is rejected"));
	TEST_voidCheck(EXT_EEPROM_u8WritePage(EXT_EEPROM_SIZE,Local_au8Pattern,1) == EXT_EEPROM_ERROR, FLASH_STR("EXT_EEPROM"), FLASH_STR("write at address 0x400 is rejected"));
	TEST_voidCheck(EXT_EEPROM_u8ReadBlock(EXT_EEPROM_SIZE - 8,Local_au8Read,EXT_EEPROM_PAGE_SIZE) == EXT_EEPROM_ERROR, FLASH_STR("EXT_EEPROM"), FLASH_STR("read beyond the end is rejected"));
}

int main(void){
	/*1. Outputs and PWM first : Timer1 runs from now on (stop watch) */
	DIO_voidSetPinDirection(TEST_LED_PORT,TEST_LED_PIN,DIO_PIN_OUTPUT);
	USART_voidInit();
	SERVO_voidInit();
	FAN_voidInit();
	DIMMER_voidInit();
	/*2. 1 ms tick */
	TIMER2_voidInit(TIMER2_DIV_64,TIMER2_CTC);
	TIMER2_voidSetOCR(TEST_T2_OCR);
	TIMER2_voidSetCallBack_OC(TEST_voidTickCallback);
	TIMER2_voidEnableOCInterrupt();
	GIE_voidEnableGlobalInterrupt();

	TEST_voidNewLine();
	TEST_voidPrintLine(FLASH_STR("===== test_hal : Phase 2 test of the HAL layer ====="));
	TEST_voidPrintLine(FLASH_STR("Status LED: slow blink = running , solid = all passed , fast blink = a check failed"));

	TEST_voidPcf8574();
	TEST_voidLcd();
	TEST_voidOutputs();
	TEST_voidLamps();
	TEST_voidSevenSeg();
	TEST_voidLm35();
	TEST_voidKeypad();
	TEST_voidButtons();
	TEST_voidServo();
	TEST_voidFan();
	TEST_voidDimmer();
	TEST_voidEeprom();

	TEST_voidPrint(FLASH_STR("SUMMARY: "));
	TEST_voidPrintNum(Global_u8PassCount);
	TEST_voidPrint(FLASH_STR(" passed , "));
	TEST_voidPrintNum(Global_u8FailCount);
	TEST_voidPrint(FLASH_STR(" failed , "));
	TEST_voidPrintNum(Global_u8SkipCount);
	TEST_voidPrint(FLASH_STR(" skipped --> "));
	Global_u8BlinkEnabled = 0 ;
	if(Global_u8FailCount == 0){
		TEST_voidPrintLine(FLASH_STR("ALL PASSED"));
		DIO_voidSetPinValue(TEST_LED_PORT,TEST_LED_PIN,DIO_PIN_HIGH);
		while(1){
			/* solid ON = all passed */
			TEST_voidService();
		}
	}else{
		TEST_voidPrintLine(FLASH_STR("SOME CHECKS FAILED"));
		while(1){
			/* fast blink = at least one failure */
			DIO_voidTogPin(TEST_LED_PORT,TEST_LED_PIN);
			TEST_voidWaitMs(TEST_FAIL_BLINK_MS);
		}
	}
	return 0 ;
}
