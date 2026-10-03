/*
 * test_base.c
 *
 *  Created on: Sep 30, 2026
 *
 * =====================================================================
 *  Phase 0 smoke test of the EXISTING drivers (no driver is changed)
 * =====================================================================
 *  Tested : MCAL  DIO , USART , GIE , TIMER0 , ADC , TWI , EXTI
 *           HAL   PCF8574 , CLCD (I2C mode) , KPAD
 *  Skipped: SPI  (empty stub , nothing to test)
 *           SSG  (writes raw segments to a whole port , does not fit the
 *                 7447 pin map , will be replaced in Phase 2)
 *
 *  Build : pio run -e test_base
 *  Hex   : .pio/build/test_base/firmware.hex
 *
 * ---------------------- Proteus parts and wiring ----------------------
 *  ATMEGA32          Clock Frequency = 16 MHz (must equal F_CPU)
 *  Status LED        PA3 -> 330R -> LED-RED -> GND
 *  VIRTUAL TERMINAL  9600 baud , 8N1 : terminal RXD <- PD1 (TXD) ,
 *                    terminal TXD -> PD0 (RXD)
 *  LM35 (ambient)    VOUT -> PA0
 *  LM35 (water)      VOUT -> PA1
 *  PB5               leave OPEN or a button to GND , not pressed (pull-up check ;
 *                    PB5 is the heater Down button pin)
 *  I2C bus           PC0 = SCL , PC1 = SDA , 4.7k pull-up to VCC on BOTH lines
 *                    (optional: I2C DEBUGGER on SCL/SDA to watch the traffic)
 *  PCF8574 (LCD)     A0 = A1 = A2 = VCC  --> address 0x27
 *                    P0 -> LCD RS , P1 -> LCD RW , P2 -> LCD E , P3 = backlight (NC)
 *                    P4..P7 -> LCD D4..D7
 *  LM016L (16x2)     D0..D3 not connected , VSS/VEE = GND , VDD = VCC
 *  KEYPAD-SMALLCALC  keypad ROW pins    A , B , C , D -> PA4 , PA5 , PA6 , PA7
 *                    keypad COLUMN pins 1 , 2 , 3 , 4 -> PB0 , PB1 , PB2 , PB4
 *  BUTTON            PD2 (INT0) -> button -> GND (internal pull-up is used)
 *  OSCILLOSCOPE      channel A on PB3 (OC0 , Timer0 PWM)
 *
 * --------------------------- Expected result ---------------------------
 *  UART : one line per check "[PASS] <module>: <what>" or "[FAIL] ...",
 *         "MANUAL:" lines tell you what to do / what to look at ,
 *         last line = SUMMARY.
 *  Status LED : slow blink (2 Hz) = test running
 *               solid ON         = all checks passed
 *               fast blink (5 Hz) = at least one check failed
 *
 *  Things you must do by hand (the terminal tells you when):
 *   1. USART RX : type any key in the Virtual Terminal (10 s)
 *   2. EXTI     : press the button on PD2 (10 s)
 *   3. KPAD     : press 4 keys on the keypad (20 s) , check the printed keys
 *   4. ADC      : change both LM35 values during the 10 live readings
 *   5. CLCD     : look at the LCD (line 1 "test_base" + smiley , line 2 "Keys:")
 *   6. TIMER0   : look at the PWM on PB3 with the oscilloscope
 *
 *  Driver issues found in the Phase 0 review. These checks are EXPECTED to
 *  [FAIL] until the driver is fixed in Phase 2 (so the LED will fast blink):
 *   - TIMER0 : TIMER0_PWM_NONINVERTED sets COM01:0 = 11 (that is inverted)
 *              and 100 % duty gives OCR0 = 256 -> 0.
 *  Already fixed: USART UCSRC init (one write instead of read-modify-write) ,
 *  KPAD key table transposed + column left LOW after a key.
 *
 *  Proteus note: in Proteus a single read of the UBRRH/UCSRC address seems to
 *  return UCSRC (a real ATmega32 returns UBRRH) , so UBRRH cannot be checked
 *  in simulation. Only UCSRC is checked.
 * =====================================================================
 */
#include <util/delay.h>
#include "../lib/Service/std_types.h"
#include "../lib/Service/Bit_math.h"
#include "../lib/MCAL/reg_def.h"
#include "../lib/MCAL/DIO/DIO.h"
#include "../lib/MCAL/GIE/GIE.h"
#include "../lib/MCAL/TIMER0/TIMER0.h"
#include "../lib/MCAL/ADC/ADC.h"
#include "../lib/MCAL/EXTI/EXTI.h"
#include "../lib/MCAL/USART/USART.h"
#include "../lib/MCAL/TWI/TWI.h"
#include "../lib/MCAL/TWI/TWI_cfg.h"
#include "../lib/HAL/PCF8574/PCF8574.h"
#include "../lib/HAL/CLCD/CLCD.h"
#include "../lib/HAL/CLCD/CLCD_cfg.h"
#include "../lib/HAL/KPAD/KPAD.h"
#include "../lib/HAL/KPAD/KPAD_cfg.h"

/* EXTI.c has this function but EXTI.h does not declare it */
void EXTI_voidINT0_callBack(void (*p)());

/* FLASH_STR("...") : text kept in flash , shared by all layers */
#include "../lib/Service/flash_str.h"

/* Test configuration */
#define TEST_LED_PORT             DIO_PORTA
#define TEST_LED_PIN              DIO_PIN_3
#define TEST_PULLUP_PORT          DIO_PORTB
#define TEST_PULLUP_PIN           DIO_PIN_5
#define TEST_PWM_PORT             DIO_PORTB
#define TEST_PWM_PIN              DIO_PIN_3
#define TEST_INT0_PORT            DIO_PORTD
#define TEST_INT0_PIN             DIO_PIN_2
#define TEST_TWI_ABSENT_ADDRESS   0x60        /* no device uses this address */

#define TEST_TICK_MS              10          /* one wait step */
#define TEST_BLINK_TICKS          25          /* 25 x 10 ms = 250 ms --> 2 Hz blink */
#define TEST_FAIL_BLINK_MS        100         /* 100 ms --> 5 Hz blink */
#define TEST_MANUAL_TIMEOUT_TICKS 1000        /* 1000 x 10 ms = 10 s */
#define TEST_KPAD_TIMEOUT_TICKS   2000        /* 20 s */
#define TEST_KPAD_KEYS            4
#define TEST_ADC_LIVE_READINGS    10

/* Timer0 , prescaler 64 :
 * normal mode overflows per 100 ms = F_CPU / 64 / 256 / 10 (16 MHz -> 97)
 * CTC 1 ms : OCR0 = F_CPU / 64 / 1000 - 1                   (16 MHz -> 249) */
#define TEST_T0_OVF_PER_100MS     ((u16)(F_CPU / 64UL / 256UL / 10UL))
#define TEST_T0_CTC_1MS_OCR       ((u8)(F_CPU / 64UL / 1000UL - 1UL))
#define TEST_T0_CTC_PER_100MS     100

/* LM35 = 10 mV/C , ADC ref 2.56 V --> 2.5 mV/step --> 4 steps per C */
#define TEST_ADC_STEPS_PER_C      4
#define TEST_ADC_MAX_VALID        600         /* 150 C */

static u8 Global_u8PassCount = 0 ;
static u8 Global_u8FailCount = 0 ;
static u8 Global_u8SkipCount = 0 ;
static u8 Global_u8BlinkTicks = 0 ;

static volatile u16 Global_u16Timer0Count = 0 ;
static volatile u8  Global_u8Int0Count = 0 ;
static volatile u8  Global_u8AdcDone = 0 ;
static volatile u16 Global_u16AdcAsyncValue = 0 ;
static u16 Global_u16AdcIsrData = 0 ;         /* written by the ADC driver ISR */

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
static void TEST_voidLcdPrint(const __flash c8 * Copy_pc8Text){
	while(*Copy_pc8Text != '\0'){
		CLCD_voidSendData(*Copy_pc8Text++);
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

/* wait 10 ms and keep the status LED blinking (test running) */
static void TEST_voidTick(){
	_delay_ms(TEST_TICK_MS);
	Global_u8BlinkTicks++;
	if(Global_u8BlinkTicks >= TEST_BLINK_TICKS){
		Global_u8BlinkTicks = 0 ;
		DIO_voidTogPin(TEST_LED_PORT,TEST_LED_PIN);
	}
}
static void TEST_voidWaitTicks(u16 Copy_u16Ticks){
	for(u16 i = 0 ; i < Copy_u16Ticks ; i++){
		TEST_voidTick();
	}
}

/*************************** callbacks (ISR context) ***************************/
static void TEST_voidTimer0Callback(void){
	Global_u16Timer0Count++;
}
static void TEST_voidInt0Callback(void){
	Global_u8Int0Count++;
}
static void TEST_voidAdcCallback(void){
	/* the ADC driver already copied the result into Global_u16AdcIsrData */
	Global_u16AdcAsyncValue = Global_u16AdcIsrData ;
	Global_u8AdcDone = 1 ;
}

/*************************** tests ***************************/
static void TEST_voidDio(){
	u8 Local_u8Ok ;
	/* PA3 = status LED , output : PINx of an output pin reads the driven level */
	DIO_voidSetPinDirection(TEST_LED_PORT,TEST_LED_PIN,DIO_PIN_OUTPUT);
	DIO_voidSetPinValue(TEST_LED_PORT,TEST_LED_PIN,DIO_PIN_HIGH);
	TEST_voidCheck(DIO_u8GetPinValue(TEST_LED_PORT,TEST_LED_PIN) == DIO_PIN_HIGH, FLASH_STR("DIO"), FLASH_STR("output HIGH reads back 1"));
	DIO_voidSetPinValue(TEST_LED_PORT,TEST_LED_PIN,DIO_PIN_LOW);
	TEST_voidCheck(DIO_u8GetPinValue(TEST_LED_PORT,TEST_LED_PIN) == DIO_PIN_LOW, FLASH_STR("DIO"), FLASH_STR("output LOW reads back 0"));
	DIO_voidTogPin(TEST_LED_PORT,TEST_LED_PIN);
	TEST_voidCheck(DIO_u8GetPinValue(TEST_LED_PORT,TEST_LED_PIN) == DIO_PIN_HIGH, FLASH_STR("DIO"), FLASH_STR("TogPin toggles LOW -> HIGH"));
	/* GetPortValue must give the same bit as GetPinValue , for HIGH and for LOW */
	Local_u8Ok = GET_BIT(DIO_u8GetPortValue(TEST_LED_PORT),TEST_LED_PIN) == DIO_u8GetPinValue(TEST_LED_PORT,TEST_LED_PIN) ;
	DIO_voidSetPinValue(TEST_LED_PORT,TEST_LED_PIN,DIO_PIN_LOW);
	Local_u8Ok = Local_u8Ok && (GET_BIT(DIO_u8GetPortValue(TEST_LED_PORT),TEST_LED_PIN) == DIO_u8GetPinValue(TEST_LED_PORT,TEST_LED_PIN)) ;
	TEST_voidCheck(Local_u8Ok, FLASH_STR("DIO"), FLASH_STR("GetPortValue bit matches GetPinValue (HIGH and LOW)"));

	/* PB5 open , input with pull-up --> reads 1 */
	DIO_voidSetPinDirection(TEST_PULLUP_PORT,TEST_PULLUP_PIN,DIO_INPUT);
	DIO_voidEnablePullUp(TEST_PULLUP_PORT,TEST_PULLUP_PIN);
	_delay_ms(1);
	TEST_voidCheck(DIO_u8GetPinValue(TEST_PULLUP_PORT,TEST_PULLUP_PIN) == DIO_PIN_HIGH, FLASH_STR("DIO"), FLASH_STR("input with pull-up reads 1 (PB5 open)"));
	/* SetPortDirection / SetPortValue are not tested : they would disturb the other pins */
}

static void TEST_voidUsartRegisters(){
	u8 Local_u8Ucsrc ;
	TEST_voidPrintLine(FLASH_STR("MANUAL: if you can read this line , USART TX works."));
	/* UBRRH and UCSRC share one address : two reads in a row return UCSRC
	 * (datasheet , "Accessing UBRRH/UCSRC"). UBRRH (one read) is not checked :
	 * Proteus seems to return UCSRC for it too. */
	Local_u8Ucsrc = UCSRC ;
	Local_u8Ucsrc = UCSRC ;
	/* UPM1:0 = 00 (no parity) , USBS = 0 (1 stop) , UCSZ1:0 = 11 (8 bit) */
	TEST_voidCheck((Local_u8Ucsrc & 0b00111110) == 0b00000110, FLASH_STR("USART"), FLASH_STR("UCSRC = 8N1 after init"));
}

static void TEST_voidTimer0AndGie(){
	u16 Local_u16Count ;

	/*1. Normal mode , overflow interrupt */
	TIMER0_SetCallBack_OV(TEST_voidTimer0Callback);
	TIMER0_voidInit(TIMER0_DIV_64,TIMER0_NORMAL);
	TIMER0_voidSetPreload(0);
	Global_u16Timer0Count = 0 ;
	TIMER0_voidEnableOVInterrupt();
	GIE_voidEnableGlobalInterrupt();
	_delay_ms(100);
	TIMER0_voidDisableOVInterrupt();
	Local_u16Count = Global_u16Timer0Count ;   /* interrupt off --> safe 16-bit read */
	TEST_voidPrint(FLASH_STR("       overflows in 100 ms = "));
	TEST_voidPrintNum(Local_u16Count);
	TEST_voidPrint(FLASH_STR(" (expected "));
	TEST_voidPrintNum(TEST_T0_OVF_PER_100MS);
	TEST_voidPrintLine(FLASH_STR(")"));
	TEST_voidCheck((Local_u16Count >= (TEST_T0_OVF_PER_100MS * 8) / 10) && (Local_u16Count <= (TEST_T0_OVF_PER_100MS * 12) / 10),
			FLASH_STR("TIMER0"), FLASH_STR("overflow interrupt rate (+-20 %)"));

	/*2. GIE : disabled --> no interrupt , enabled --> interrupts again */
	Global_u16Timer0Count = 0 ;
	GIE_voidDisableGlobalInterrupt();
	TIMER0_voidEnableOVInterrupt();
	_delay_ms(20);
	TIMER0_voidDisableOVInterrupt();
	TEST_voidCheck(Global_u16Timer0Count == 0, FLASH_STR("GIE"), FLASH_STR("disable blocks interrupts"));
	TIMER0_voidEnableOVInterrupt();
	GIE_voidEnableGlobalInterrupt();
	_delay_ms(20);
	TIMER0_voidDisableOVInterrupt();
	Local_u16Count = Global_u16Timer0Count ;
	TEST_voidCheck(Local_u16Count > 0, FLASH_STR("GIE"), FLASH_STR("enable allows interrupts"));

	/*3. CTC mode , compare interrupt every 1 ms */
	TIMER0_SetCallBack_OC(TEST_voidTimer0Callback);
	TIMER0_voidInit(TIMER0_DIV_64,TIMER0_CTC);
	TIMER0_GenerateWave_CTC(TIMER0_CTC_DISCONNECTED);
	TIMER0_voidSetOCR(TEST_T0_CTC_1MS_OCR);
	TIMER0_voidSetPreload(0);
	Global_u16Timer0Count = 0 ;
	TIMER0_voidEnableOCInterrupt();
	_delay_ms(100);
	TIMER0_voidDisableOCInterrupt();
	Local_u16Count = Global_u16Timer0Count ;
	TEST_voidPrint(FLASH_STR("       compare matches in 100 ms = "));
	TEST_voidPrintNum(Local_u16Count);
	TEST_voidPrintLine(FLASH_STR(" (expected 100)"));
	TEST_voidCheck((Local_u16Count >= (TEST_T0_CTC_PER_100MS * 8) / 10) && (Local_u16Count <= (TEST_T0_CTC_PER_100MS * 12) / 10),
			FLASH_STR("TIMER0"), FLASH_STR("CTC compare interrupt every 1 ms (+-20 %)"));
}

static void TEST_voidTimer0Pwm(){
	/* Fast PWM on OC0 (PB3) , prescaler 64 --> 16 MHz / 64 / 256 = 976 Hz */
	DIO_voidSetPinDirection(TEST_PWM_PORT,TEST_PWM_PIN,DIO_PIN_OUTPUT);
	TIMER0_voidInit(TIMER0_DIV_64,TIMER0_FAST_PWM);

	TIMER0_GeneratePWM(TIMER0_PWM_NONINVERTED,100);
	TEST_voidCheck(OCR0 == 255, FLASH_STR("TIMER0"), FLASH_STR("100 % duty gives OCR0 = 255 (known bug: 0)"));

	TIMER0_GeneratePWM(TIMER0_PWM_NONINVERTED,25);
	TEST_voidCheck(OCR0 == 64, FLASH_STR("TIMER0"), FLASH_STR("25 % duty gives OCR0 = 64"));
	TEST_voidCheck((TCCR0 & ((1<<TCCR0_COM01) | (1<<TCCR0_COM00))) == (1<<TCCR0_COM01),
			FLASH_STR("TIMER0"), FLASH_STR("non-inverted PWM sets COM01:0 = 10 (known bug: 11)"));
	TEST_voidPrintLine(FLASH_STR("MANUAL: TIMER0 - scope on PB3: 976 Hz PWM , 25 % HIGH requested"));
	TEST_voidPrintLine(FLASH_STR("        (with the known COM bug you will see 75 % HIGH). PWM stays on."));
}

static void TEST_voidAdcPrintReading(u16 Copy_u16Value){
	TEST_voidPrintNum(Copy_u16Value);
	TEST_voidPrint(FLASH_STR(" ("));
	TEST_voidPrintNum(Copy_u16Value / TEST_ADC_STEPS_PER_C);
	TEST_voidPrint(FLASH_STR(" C)"));
}

static void TEST_voidAdcSync(){
	u16 Local_u16Ch0 ;
	u16 Local_u16Ch1 ;
	ADC_voidInit();
	TEST_voidCheck((ADMUX & 0b11000000) == 0b11000000, FLASH_STR("ADC"), FLASH_STR("reference = internal 2.56 V"));
	Local_u16Ch0 = ADC_u16StartConversion(ADC_CHANNEL_0);
	Local_u16Ch1 = ADC_u16StartConversion(ADC_CHANNEL_1);
	TEST_voidPrint(FLASH_STR("       ADC0 (ambient) = "));
	TEST_voidAdcPrintReading(Local_u16Ch0);
	TEST_voidPrint(FLASH_STR("   ADC1 (water) = "));
	TEST_voidAdcPrintReading(Local_u16Ch1);
	TEST_voidNewLine();
	TEST_voidCheck(Local_u16Ch0 <= TEST_ADC_MAX_VALID, FLASH_STR("ADC"), FLASH_STR("channel 0 LM35 value in range 0..150 C"));
	TEST_voidCheck(Local_u16Ch1 <= TEST_ADC_MAX_VALID, FLASH_STR("ADC"), FLASH_STR("channel 1 LM35 value in range 0..150 C"));
}

static void TEST_voidTwiPcf8574(){
	u8 Local_u8Error ;
	u8 Local_u8Value = 0 ;
	PCF8574_voidInit();
	TEST_voidCheck(TWBR == TWI_TWBR_VALUE, FLASH_STR("TWI"), FLASH_STR("bit rate register set for 100 kHz"));

	/* 0x08 = backlight bit only , LCD E/RS low (safe before LCD init) */
	Local_u8Error = PCF8574_u8WritePort(CLCD_I2C_ADDRESS,0x08);
	TEST_voidCheck(Local_u8Error == TWI_OK, FLASH_STR("PCF8574"), FLASH_STR("LCD expander at 0x27 ACKs a write"));
	Local_u8Error = PCF8574_u8ReadPort(CLCD_I2C_ADDRESS,&Local_u8Value);
	TEST_voidCheck(Local_u8Error == TWI_OK, FLASH_STR("PCF8574"), FLASH_STR("LCD expander at 0x27 ACKs a read"));
	TEST_voidPrint(FLASH_STR("       port value read = "));
	TEST_voidPrintNum(Local_u8Value);
	TEST_voidNewLine();

	Local_u8Error = PCF8574_u8WritePort(TEST_TWI_ABSENT_ADDRESS,0);
	TEST_voidCheck(Local_u8Error == TWI_ERR_SLA_NACK, FLASH_STR("TWI"), FLASH_STR("no device at 0x60 --> NACK error returned"));
}

static void TEST_voidClcd(){
	/* smiley for CGRAM slot 0 */
	u8 Local_u8Smiley[8] = {0b00000, 0b01010, 0b01010, 0b00000, 0b10001, 0b01110, 0b00000, 0b00000};
	CLCD_voidInit();
	CLCD_voidCreatSpecialChar(0,Local_u8Smiley);
	CLCD_voidSetCursorPosition(0,0);
	TEST_voidLcdPrint(FLASH_STR("test_base "));
	CLCD_voidSendData(0);
	CLCD_voidSetCursorPosition(0,1);
	TEST_voidLcdPrint(FLASH_STR("Keys:"));
	TEST_voidPrintLine(FLASH_STR("MANUAL: CLCD - LCD line 1 = 'test_base' + smiley , line 2 = 'Keys:'"));
}

static void TEST_voidUsartReceive(){
	u16 Local_u16Ticks = 0 ;
	u8 Local_u8Received = 0 ;
	u8 Local_u8Char = 0 ;
	TEST_voidPrintLine(FLASH_STR("MANUAL: USART RX - type any key in the Virtual Terminal (10 s)"));
	while((Local_u16Ticks < TEST_MANUAL_TIMEOUT_TICKS) && (Local_u8Received == 0)){
		/* poll the RX flag , USART_u8Recieve() would wait forever */
		if(GET_BIT(UCSRA,UCSRA_RXC) == 1){
			Local_u8Char = USART_u8Recieve();
			Local_u8Received = 1 ;
		}else{
			TEST_voidTick();
			Local_u16Ticks++;
		}
	}
	if(Local_u8Received){
		TEST_voidPrint(FLASH_STR("       received '"));
		USART_voidSend(Local_u8Char);
		TEST_voidPrintLine(FLASH_STR("'"));
	}
	TEST_voidCheck(Local_u8Received, FLASH_STR("USART"), FLASH_STR("RX receives a key"));
}

static void TEST_voidExti(){
	u16 Local_u16Ticks = 0 ;
	/* callback first : the EXTI ISR calls it without a NULL check */
	EXTI_voidINT0_callBack(TEST_voidInt0Callback);
	DIO_voidSetPinDirection(TEST_INT0_PORT,TEST_INT0_PIN,DIO_INPUT);
	DIO_voidEnablePullUp(TEST_INT0_PORT,TEST_INT0_PIN);
	EXTI_SetInterruptSenceCTRL(EXTI_INT0,EXTI_FALLING_EDGE);
	/* clear an old INT0 flag (write 1 to clear) */
	GIFR = (1<<GIFR_INTF0);
	Global_u8Int0Count = 0 ;
	EXTI_voidEnableINT(EXTI_INT0);
	TEST_voidPrintLine(FLASH_STR("MANUAL: EXTI - press the button on PD2 (10 s)"));
	while((Local_u16Ticks < TEST_MANUAL_TIMEOUT_TICKS) && (Global_u8Int0Count == 0)){
		TEST_voidTick();
		Local_u16Ticks++;
	}
	EXTI_voidDisableINT(EXTI_INT0);
	TEST_voidCheck(Global_u8Int0Count > 0, FLASH_STR("EXTI"), FLASH_STR("INT0 falling edge interrupt"));
}

static void TEST_voidKpad(){
	u16 Local_u16Ticks = 0 ;
	u8 Local_u8Keys = 0 ;
	u8 Local_u8Key ;
	u8 Local_u8ColsHigh = 1 ;
	KPAD_voidInit();
	TEST_voidPrintLine(FLASH_STR("MANUAL: KPAD - press 4 keys (20 s). Each key is printed here and on the LCD;"));
	TEST_voidPrintLine(FLASH_STR("        check that the printed key is the key you pressed."));
	while((Local_u16Ticks < TEST_KPAD_TIMEOUT_TICKS) && (Local_u8Keys < TEST_KPAD_KEYS)){
		Local_u8Key = KPAD_u8GetKeyPressed();
		if(Local_u8Key != 0xff){
			Local_u8Keys++;
			TEST_voidPrint(FLASH_STR("       key = "));
			USART_voidSend(Local_u8Key);
			TEST_voidNewLine();
			CLCD_voidSendData(Local_u8Key);
			/* after a scan all column outputs must be back HIGH */
			if((DIO_u8GetPinValue(KPAD_COL_PORT,KPAD_COL_PIN0) == DIO_PIN_LOW) ||
			   (DIO_u8GetPinValue(KPAD_COL_PORT,KPAD_COL_PIN1) == DIO_PIN_LOW) ||
			   (DIO_u8GetPinValue(KPAD_COL_PORT,KPAD_COL_PIN2) == DIO_PIN_LOW) ||
			   (DIO_u8GetPinValue(KPAD_COL_PORT,KPAD_COL_PIN3) == DIO_PIN_LOW)){
				Local_u8ColsHigh = 0 ;
			}
		}
		TEST_voidTick();
		Local_u16Ticks++;
	}
	TEST_voidCheck(Local_u8Keys == TEST_KPAD_KEYS, FLASH_STR("KPAD"), FLASH_STR("4 key presses detected"));
	if(Local_u8Keys > 0){
		TEST_voidCheck(Local_u8ColsHigh, FLASH_STR("KPAD"), FLASH_STR("all columns back HIGH after a key"));
	}else{
		TEST_voidSkip(FLASH_STR("KPAD"), FLASH_STR("column check needs a key press"));
	}
}

static void TEST_voidAdcLive(){
	TEST_voidPrintLine(FLASH_STR("MANUAL: ADC - change both LM35 values now (10 readings , 1 per second)"));
	for(u8 i = 0 ; i < TEST_ADC_LIVE_READINGS ; i++){
		TEST_voidPrint(FLASH_STR("       ADC0 = "));
		TEST_voidAdcPrintReading(ADC_u16StartConversion(ADC_CHANNEL_0));
		TEST_voidPrint(FLASH_STR("   ADC1 = "));
		TEST_voidAdcPrintReading(ADC_u16StartConversion(ADC_CHANNEL_1));
		TEST_voidNewLine();
		TEST_voidWaitTicks(1000 / TEST_TICK_MS);
	}
}

static void TEST_voidAdcAsync(){
	u16 Local_u16Ticks = 0 ;
	u16 Local_u16Sync ;
	u16 Local_u16Async ;
	u8 Local_u8Done ;
	ADC_SetCallBack(TEST_voidAdcCallback,&Global_u16AdcIsrData);
	Global_u8AdcDone = 0 ;
	ADC_voidStartConvertionAsyn(ADC_CHANNEL_0);
	while((Local_u16Ticks < 10) && (Global_u8AdcDone == 0)){
		TEST_voidTick();
		Local_u16Ticks++;
	}
	/* the driver has no function to switch the ADC interrupt off again ,
	 * and with ADIE on , the sync function would wait forever (ISR clears ADIF) */
	CLR_BIT(ADCSRA,ADCSRA_ADIE);
	Local_u8Done = Global_u8AdcDone ;
	Local_u16Async = Global_u16AdcAsyncValue ;   /* ADC interrupt off --> safe read */
	TEST_voidCheck(Local_u8Done, FLASH_STR("ADC"), FLASH_STR("async conversion calls the callback"));
	Local_u16Sync = ADC_u16StartConversion(ADC_CHANNEL_0);
	TEST_voidCheck(Local_u8Done && (((Local_u16Async > Local_u16Sync) ? (Local_u16Async - Local_u16Sync) : (Local_u16Sync - Local_u16Async)) <= 4),
			FLASH_STR("ADC"), FLASH_STR("async value = sync value (+-4)"));
}

int main(void){
	DIO_voidSetPinDirection(TEST_LED_PORT,TEST_LED_PIN,DIO_PIN_OUTPUT);
	USART_voidInit();
	TEST_voidNewLine();
	TEST_voidPrintLine(FLASH_STR("===== test_base : Phase 0 smoke test of the existing drivers ====="));
	TEST_voidPrintLine(FLASH_STR("Status LED: slow blink = running , solid = all passed , fast blink = a check did not pass"));

	TEST_voidDio();
	TEST_voidUsartRegisters();
	TEST_voidTimer0AndGie();
	TEST_voidTimer0Pwm();
	TEST_voidAdcSync();
	TEST_voidTwiPcf8574();
	TEST_voidClcd();
	TEST_voidUsartReceive();
	TEST_voidExti();
	TEST_voidKpad();
	TEST_voidAdcLive();
	TEST_voidAdcAsync();
	TEST_voidSkip(FLASH_STR("SPI"), FLASH_STR("empty stub , nothing to test"));
	TEST_voidSkip(FLASH_STR("SSG"), FLASH_STR("does not fit the 7447 pin map , replaced in Phase 2"));

	TEST_voidPrint(FLASH_STR("SUMMARY: "));
	TEST_voidPrintNum(Global_u8PassCount);
	TEST_voidPrint(FLASH_STR(" passed , "));
	TEST_voidPrintNum(Global_u8FailCount);
	TEST_voidPrint(FLASH_STR(" failed , "));
	TEST_voidPrintNum(Global_u8SkipCount);
	TEST_voidPrint(FLASH_STR(" skipped --> "));
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
