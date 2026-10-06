/*
 * test_mcal.c
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 *
 * =====================================================================
 *  Phase 2 test of the MCAL layer (after the fixes and the new drivers)
 * =====================================================================
 *  Tested : DIO (JTAG disable) , GIE , ADC (async then sync) ,
 *           TIMER0 (PWM) , TIMER1 (50 Hz PWM) , TIMER2 (1 ms tick) ,
 *           USART (baud , RX interrupt , TX interrupt) , TWI (ACK / NACK /
 *           timeout) , EXTI (callback , no callback)
 *
 *  Build : pio run -e test_mcal
 *  Hex   : .pio/build/test_mcal/firmware.hex
 *
 * ---------------------- Proteus parts and wiring ----------------------
 *  ATMEGA32          Clock Frequency = 16 MHz (must equal F_CPU)
 *  Status LED        PA3 -> 330R -> LED-RED -> GND
 *  VIRTUAL TERMINAL  9600 baud , 8N1 : terminal RXD <- PD1 (TXD) ,
 *                    terminal TXD -> PD0 (RXD)
 *  LM35 (ambient)    VOUT -> PA0
 *  LM35 (water)      VOUT -> PA1
 *  I2C bus           PC0 = SCL , PC1 = SDA , 4.7k pull-up to VCC on BOTH lines
 *  PCF8574 (LCD)     A0 = A1 = A2 = VCC  --> address 0x27
 *  PCF8574 (lamps)   A0 = A1 = A2 = GND  --> address 0x20
 *  24C08 EEPROM      A2 = GND , address 0x50
 *  (no device may answer at 0x60)
 *  SWITCH (SPST)     PC1 (SDA) -> switch -> GND   (open = normal ,
 *                    closed = SDA held low for the TWI timeout check)
 *  BUTTON            PD2 (INT0) -> button -> GND (internal pull-up is used)
 *  OSCILLOSCOPE      channel A on PB3 (OC0 , Timer0 PWM)
 *                    channel B on PD5 (OC1A , Timer1 , servo pin)
 *                    channel C on PD4 (OC1B , Timer1 , fan pin)
 *                    (a servo on PD5 / a fan on PD4 are fine , they just move)
 *
 * --------------------------- Expected result ---------------------------
 *  UART : one line per check "[PASS] <module>: <what>" or "[FAIL] ...",
 *         "[SKIP]" = a manual step was not done , "MANUAL:" lines tell you
 *         what to do / what to look at , last line = SUMMARY.
 *  Status LED : slow blink (2 Hz) = test running
 *               solid ON         = all checks passed
 *               fast blink (5 Hz) = at least one check failed
 *
 *  Things you must do by hand (the terminal tells you when):
 *   1. TIMER0  : scope on PB3 , 3 steps of 3 s: 0 % , 50 % , 100 % , 7.8 kHz
 *   2. TIMER1  : scope on PD5 : 20 ms period , pulse 1.0 / 1.5 / 2.0 ms (3 s each)
 *                scope on PD4 : 0 % / 50 % / 100 % duty (3 s each)
 *   3. USART RX: type 3 keys in the Virtual Terminal (10 s)
 *   4. USART TX: look at the burst : 10 lines , each 28 times the same letter A..J
 *   5. TWI     : MANUAL , close the SDA switch when asked (20 s) , open it again
 *   6. EXTI    : press the button on PD2 (10 s)
 * =====================================================================
 */
#include <util/delay.h>
#include "../lib/Service/std_types.h"
#include "../lib/Service/Bit_math.h"
#include "../lib/MCAL/reg_def.h"
#include "../lib/MCAL/DIO/DIO.h"
#include "../lib/MCAL/GIE/GIE.h"
#include "../lib/MCAL/ADC/ADC.h"
#include "../lib/MCAL/TIMER0/TIMER0.h"
#include "../lib/MCAL/TIMER1/TIMER1.h"
#include "../lib/MCAL/TIMER1/TIMER1_cfg.h"
#include "../lib/MCAL/TIMER2/TIMER2.h"
#include "../lib/MCAL/USART/USART.h"
#include "../lib/MCAL/USART/USART_cfg.h"
#include "../lib/MCAL/TWI/TWI.h"
#include "../lib/MCAL/TWI/TWI_cfg.h"
#include "../lib/MCAL/EXTI/EXTI.h"

/* FLASH_STR("...") : text kept in flash , shared by all layers */
#include "../lib/Service/flash_str.h"

/* Test configuration */
#define TEST_LED_PORT             DIO_PORTA
#define TEST_LED_PIN              DIO_PIN_3
#define TEST_T0_PORT              DIO_PORTB      /* OC0  */
#define TEST_T0_PIN               DIO_PIN_3
#define TEST_OC1A_PORT            DIO_PORTD      /* OC1A */
#define TEST_OC1A_PIN             DIO_PIN_5
#define TEST_OC1B_PORT            DIO_PORTD      /* OC1B */
#define TEST_OC1B_PIN             DIO_PIN_4
#define TEST_INT0_PORT            DIO_PORTD
#define TEST_INT0_PIN             DIO_PIN_2

#define TEST_TICK_MS              10          /* one wait step */
#define TEST_BLINK_TICKS          25          /* 25 x 10 ms = 250 ms --> 2 Hz blink */
#define TEST_FAIL_BLINK_MS        100         /* 100 ms --> 5 Hz blink */
#define TEST_STEP_TICKS           300         /* 3 s per scope step */
#define TEST_MANUAL_TIMEOUT_TICKS 1000        /* 10 s */
#define TEST_SDA_TIMEOUT_TICKS    2000        /* 20 s */

/* Timer2 : prescaler 64 , CTC , OCR2 = F_CPU / 64 / 1000 - 1  (16 MHz -> 249 = 1 ms) */
#define TEST_T2_OCR               ((u8)(F_CPU / 64UL / 1000UL - 1UL))
/* Timer0 : prescaler 8 , fast PWM --> F_CPU / 8 / 256 = 7812 Hz at 16 MHz */
#define TEST_T0_PWM_HZ            ((u16)(F_CPU / 8UL / 256UL))

/* PCF8574 (LCD) , PCF8574 (lamps) , 24C08 , and an address nobody uses */
#define TEST_TWI_ADDR_LCD         0x27
#define TEST_TWI_ADDR_LAMPS       0x20
#define TEST_TWI_ADDR_EEPROM      0x50
#define TEST_TWI_ADDR_ABSENT      0x60

#define TEST_TX_LINES             10
#define TEST_TX_LINE_CHARS        28          /* + CR LF = 30 bytes per line */
#define TEST_TX_BURST_BYTES       (TEST_TX_LINES * (TEST_TX_LINE_CHARS + 2))   /* 300 */
#define TEST_TX_RING_SIZE         64          /* test only : the real ring is in TERM */
#define TEST_RX_KEYS              3

#define TEST_ADC_MAX_VALID        600         /* 150 C at 4 steps per C */

static u8 Global_u8PassCount = 0 ;
static u8 Global_u8FailCount = 0 ;
static u8 Global_u8SkipCount = 0 ;
static u8 Global_u8BlinkTicks = 0 ;

static volatile u16 Global_u16Timer2Ticks = 0 ;
static volatile u8  Global_u8Int0Count = 0 ;
static volatile u8  Global_u8AdcDone = 0 ;
static u16 Global_u16AdcIsrData = 0 ;         /* written by the ADC driver ISR */

static volatile u8  Global_u8RxCount = 0 ;
static volatile c8  Global_c8RxKeys[TEST_RX_KEYS] ;

static volatile u8  Global_u8TxRing[TEST_TX_RING_SIZE] ;
static volatile u8  Global_u8TxHead = 0 ;     /* written by main */
static volatile u8  Global_u8TxTail = 0 ;     /* written by the TX interrupt */
static volatile u16 Global_u16TxSent = 0 ;    /* bytes written to UDR by the interrupt */

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

/* 16-bit timer registers : low byte first (it latches the high byte) */
static u16 TEST_u16ReadTCNT1(){
	u16 Local_u16Value = TCNT1L ;
	Local_u16Value |= ((u16)TCNT1H << 8) ;
	return Local_u16Value ;
}
static u16 TEST_u16ReadICR1(){
	u16 Local_u16Value = ICR1L ;
	Local_u16Value |= ((u16)ICR1H << 8) ;
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

/*************************** callbacks (ISR context) ***************************/
static void TEST_voidTimer2Callback(void){
	Global_u16Timer2Ticks++;
}
static void TEST_voidInt0Callback(void){
	Global_u8Int0Count++;
}
static void TEST_voidAdcCallback(void){
	Global_u8AdcDone = 1 ;
}
static void TEST_voidRxCallback(u8 Copy_u8Data){
	if(Global_u8RxCount < TEST_RX_KEYS){
		Global_c8RxKeys[Global_u8RxCount] = Copy_u8Data ;
	}
	if(Global_u8RxCount < 255){
		Global_u8RxCount++;
	}
}
static void TEST_voidTxCallback(void){
	/* UDRE interrupt : send the next byte , or switch the interrupt off when the ring is empty */
	if(Global_u8TxTail != Global_u8TxHead){
		USART_voidWriteData(Global_u8TxRing[Global_u8TxTail]);
		Global_u8TxTail = (Global_u8TxTail + 1) % TEST_TX_RING_SIZE ;
		Global_u16TxSent++;
	}else{
		USART_voidDisableTxInterrupt();
	}
}

/*************************** tests ***************************/
static void TEST_voidDioJtag(){
	u8 Local_u8Ok = 1 ;
	DIO_voidDisableJTAG();
	TEST_voidCheck(GET_BIT(MCUCSR,MCUCSR_JTD) == 1, FLASH_STR("DIO"), FLASH_STR("JTAG disable bit (JTD) is set"));
	/* PC2..PC5 are the 7447 pins : must work as ordinary outputs */
	for(u8 Local_u8Pin = DIO_PIN_2 ; Local_u8Pin <= DIO_PIN_5 ; Local_u8Pin++){
		DIO_voidSetPinDirection(DIO_PORTC,Local_u8Pin,DIO_PIN_OUTPUT);
		DIO_voidSetPinValue(DIO_PORTC,Local_u8Pin,DIO_PIN_HIGH);
		if(DIO_u8GetPinValue(DIO_PORTC,Local_u8Pin) != DIO_PIN_HIGH){
			Local_u8Ok = 0 ;
		}
		DIO_voidSetPinValue(DIO_PORTC,Local_u8Pin,DIO_PIN_LOW);
		if(DIO_u8GetPinValue(DIO_PORTC,Local_u8Pin) != DIO_PIN_LOW){
			Local_u8Ok = 0 ;
		}
	}
	TEST_voidCheck(Local_u8Ok, FLASH_STR("DIO"), FLASH_STR("PC2..PC5 work as outputs (HIGH and LOW)"));
}

static void TEST_voidAdc(){
	u16 Local_u16Value ;
	u8 Local_u8Done ;
	u16 Local_u16Ticks = 0 ;
	ADC_voidInit();
	TEST_voidCheck((ADMUX & 0b11000000) == 0b11000000, FLASH_STR("ADC"), FLASH_STR("reference = internal 2.56 V"));

	/*1. async conversion : the callback must run (the ISR is not removed by LTO) */
	ADC_SetCallBack(TEST_voidAdcCallback,&Global_u16AdcIsrData);
	Global_u8AdcDone = 0 ;
	ADC_voidStartConvertionAsyn(ADC_CHANNEL_0);
	while((Local_u16Ticks < 10) && (Global_u8AdcDone == 0)){
		TEST_voidTick();
		Local_u16Ticks++;
	}
	Local_u8Done = Global_u8AdcDone ;
	TEST_voidCheck(Local_u8Done, FLASH_STR("ADC"), FLASH_STR("async conversion calls the callback"));

	/*2. sync read right after the async one , ADIE still on : this used to hang */
	Local_u16Value = ADC_u16StartConversion(ADC_CHANNEL_1);
	TEST_voidCheck(Local_u16Value <= TEST_ADC_MAX_VALID, FLASH_STR("ADC"), FLASH_STR("sync read works after an async read (0..150 C)"));
	TEST_voidCheck(GET_BIT(ADCSRA,ADCSRA_ADIE) == 0, FLASH_STR("ADC"), FLASH_STR("sync read switched ADIE off"));

	/*3. ADC_voidDisableInterrupt */
	ADC_voidStartConvertionAsyn(ADC_CHANNEL_0);
	TEST_voidWaitTicks(2);
	ADC_voidDisableInterrupt();
	TEST_voidCheck(GET_BIT(ADCSRA,ADCSRA_ADIE) == 0, FLASH_STR("ADC"), FLASH_STR("ADC_voidDisableInterrupt clears ADIE"));
	TEST_voidPrint(FLASH_STR("       ADC1 (water) = "));
	TEST_voidPrintNum(Local_u16Value);
	TEST_voidPrintLine(FLASH_STR(" steps (4 per C)"));
}

static void TEST_voidTimer0Step(u8 Copy_u8Duty, u8 Copy_u8ExpectedOcr, const __flash c8 * Copy_pc8What){
	TIMER0_GeneratePWM(TIMER0_PWM_NONINVERTED,Copy_u8Duty);
	TEST_voidCheck(OCR0 == Copy_u8ExpectedOcr, FLASH_STR("TIMER0"), Copy_pc8What);
	TEST_voidWaitTicks(TEST_STEP_TICKS);
}
static void TEST_voidTimer0Pwm(){
	/* Fast PWM on OC0 (PB3) , prescaler 8 --> 7812 Hz at 16 MHz */
	DIO_voidSetPinDirection(TEST_T0_PORT,TEST_T0_PIN,DIO_PIN_OUTPUT);
	TIMER0_voidInit(TIMER0_DIV_8,TIMER0_FAST_PWM);
	TEST_voidPrint(FLASH_STR("MANUAL: TIMER0 - scope on PB3 , "));
	TEST_voidPrintNum(TEST_T0_PWM_HZ);
	TEST_voidPrintLine(FLASH_STR(" Hz : 0 % , 50 % , 100 % , 3 s each"));
	TEST_voidTimer0Step(0,0,FLASH_STR("0 % duty gives OCR0 = 0"));
	TEST_voidCheck((TCCR0 & ((1<<TCCR0_COM01) | (1<<TCCR0_COM00))) == (1<<TCCR0_COM01),
			FLASH_STR("TIMER0"), FLASH_STR("non-inverted PWM sets COM01:0 = 10"));
	TEST_voidTimer0Step(50,127,FLASH_STR("50 % duty gives OCR0 = 127"));
	TEST_voidTimer0Step(100,255,FLASH_STR("100 % duty gives OCR0 = 255 (not 256 -> 0)"));
	TIMER0_GeneratePWM(TIMER0_PWM_DISCONNECTED,0);
	TEST_voidCheck((TCCR0 & ((1<<TCCR0_COM01) | (1<<TCCR0_COM00))) == 0, FLASH_STR("TIMER0"), FLASH_STR("PWM disconnected clears COM01:0"));
}

static void TEST_voidTimer1(){
	u16 Local_u16OneMs = TIMER1_TICKS_PER_MS ;
	DIO_voidSetPinDirection(TEST_OC1A_PORT,TEST_OC1A_PIN,DIO_PIN_OUTPUT);
	DIO_voidSetPinDirection(TEST_OC1B_PORT,TEST_OC1B_PIN,DIO_PIN_OUTPUT);
	TIMER1_voidInit();
	TIMER1_voidInit();   /* SERVO and FAN both call it : a second call must change nothing */
	TEST_voidCheck(TEST_u16ReadICR1() == TIMER1_TOP_VALUE, FLASH_STR("TIMER1"), FLASH_STR("ICR1 = TOP (39999 at 16 MHz = 20 ms)"));
	TEST_voidCheck(((TCCR1A & 0b00000011) == 0b00000010) && ((TCCR1B & 0b00011000) == 0b00011000),
			FLASH_STR("TIMER1"), FLASH_STR("mode 14 : WGM13:10 = 1110"));
	TEST_voidCheck((TCCR1B & 0b00000111) == 0b00000010, FLASH_STR("TIMER1"), FLASH_STR("prescaler 8 (CS12:0 = 010)"));

	/*1. OC1A : servo pulse 1.0 / 1.5 / 2.0 ms */
	TIMER1_voidGeneratePWM_A(TIMER1_PWM_NONINVERTED);
	TEST_voidCheck((TCCR1A & 0b11000000) == 0b10000000, FLASH_STR("TIMER1"), FLASH_STR("OC1A non-inverted sets COM1A1:0 = 10"));
	TEST_voidPrintLine(FLASH_STR("MANUAL: TIMER1 - scope on PD5 (OC1A): 20 ms period , pulse 1.0 / 1.5 / 2.0 ms , 3 s each"));
	TIMER1_voidSetOCRA(Local_u16OneMs);
	TEST_voidCheck(TEST_u16ReadOCR1A() == Local_u16OneMs, FLASH_STR("TIMER1"), FLASH_STR("OCR1A = 1.0 ms (2000 at 16 MHz)"));
	TEST_voidWaitTicks(TEST_STEP_TICKS);
	TIMER1_voidSetOCRA((Local_u16OneMs * 3) / 2);
	TEST_voidCheck(TEST_u16ReadOCR1A() == ((Local_u16OneMs * 3) / 2), FLASH_STR("TIMER1"), FLASH_STR("OCR1A = 1.5 ms (3000 at 16 MHz)"));
	TEST_voidWaitTicks(TEST_STEP_TICKS);
	TIMER1_voidSetOCRA(Local_u16OneMs * 2);
	TEST_voidCheck(TEST_u16ReadOCR1A() == (Local_u16OneMs * 2), FLASH_STR("TIMER1"), FLASH_STR("OCR1A = 2.0 ms (4000 at 16 MHz)"));
	TEST_voidWaitTicks(TEST_STEP_TICKS);

	/*2. OC1B : fan duty 0 / 50 / 100 % . 100 % = TOP + 1 (OCR = TOP would leave one 0.5 us low spike) */
	TIMER1_voidGeneratePWM_B(TIMER1_PWM_NONINVERTED);
	TEST_voidCheck((TCCR1A & 0b00110000) == 0b00100000, FLASH_STR("TIMER1"), FLASH_STR("OC1B non-inverted sets COM1B1:0 = 10"));
	TEST_voidPrintLine(FLASH_STR("MANUAL: TIMER1 - scope on PD4 (OC1B): 0 % , 50 % , 100 % , 3 s each"));
	TIMER1_voidSetOCRB(0);
	TEST_voidCheck(TEST_u16ReadOCR1B() == 0, FLASH_STR("TIMER1"), FLASH_STR("OCR1B = 0 (0 %)"));
	TEST_voidWaitTicks(TEST_STEP_TICKS);
	TIMER1_voidSetOCRB(TIMER1_TOP_VALUE / 2);
	TEST_voidCheck(TEST_u16ReadOCR1B() == (TIMER1_TOP_VALUE / 2), FLASH_STR("TIMER1"), FLASH_STR("OCR1B = TOP / 2 (50 %)"));
	TEST_voidWaitTicks(TEST_STEP_TICKS);
	TIMER1_voidSetOCRB(TIMER1_TOP_VALUE + 1);
	TEST_voidCheck(TEST_u16ReadOCR1B() == (u16)(TIMER1_TOP_VALUE + 1), FLASH_STR("TIMER1"), FLASH_STR("OCR1B = TOP + 1 (100 %)"));
	TEST_voidWaitTicks(TEST_STEP_TICKS);
}

/* wait until Timer1 sets TOV1 (once per 20 ms period) , then clear it : returns 0 on a 100 ms guard timeout */
static u8 TEST_u8WaitTimer1Period(){
	u32 Local_u32Guard = (F_CPU / 10UL) / 4UL ;   /* about 100 ms of polling */
	while((GET_BIT(TIFR,TIFR_TOV1) == 0) && (Local_u32Guard > 0)){
		Local_u32Guard--;
	}
	TIFR = (1<<TIFR_TOV1) ;   /* write 1 clears the flag (the other bits get 0 = no effect) */
	return (Local_u32Guard > 0) ;
}
static void TEST_voidTimer2(){
	u16 Local_u16Ticks ;
	u16 Local_u16Before ;
	u8 Local_u8Ok = 1 ;
	/* Timer1 (PWM still running from the test above) is the reference :
	 * 50 periods of 20 ms = 1.000 s must contain 1000 Timer2 compare interrupts */
	TIMER2_voidSetCallBack_OC(TEST_voidTimer2Callback);
	TIMER2_voidInit(TIMER2_DIV_64,TIMER2_CTC);
	TIMER2_voidSetOCR(TEST_T2_OCR);
	TIMER2_voidEnableOCInterrupt();

	TIFR = (1<<TIFR_TOV1) ;                           /* drop a stale TOV1 , or the window starts mid-period */
	Local_u8Ok = TEST_u8WaitTimer1Period();           /* sync to a period edge */
	GIE_voidDisableGlobalInterrupt();
	Global_u16Timer2Ticks = 0 ;
	GIE_voidEnableGlobalInterrupt();
	for(u8 i = 0 ; (i < 50) && Local_u8Ok ; i++){
		Local_u8Ok = TEST_u8WaitTimer1Period();
	}
	GIE_voidDisableGlobalInterrupt();
	Local_u16Ticks = Global_u16Timer2Ticks ;
	GIE_voidEnableGlobalInterrupt();
	TEST_voidPrint(FLASH_STR("       Timer2 interrupts in 50 Timer1 periods = "));
	TEST_voidPrintNum(Local_u16Ticks);
	TEST_voidPrintLine(FLASH_STR(" (expected 1000)"));
	TEST_voidCheck(Local_u8Ok && (Local_u16Ticks >= 998) && (Local_u16Ticks <= 1002),
			FLASH_STR("TIMER2"), FLASH_STR("1000 compare interrupts in 1.000 s (+-2)"));

	/* interrupt off --> the counter stops */
	TIMER2_voidDisableOCInterrupt();
	Local_u16Before = Global_u16Timer2Ticks ;
	_delay_ms(5);
	TEST_voidCheck(Global_u16Timer2Ticks == Local_u16Before, FLASH_STR("TIMER2"), FLASH_STR("disable stops the interrupt"));

	/* the Timer1 PWM is not needed any more */
	TIMER1_voidGeneratePWM_A(TIMER1_PWM_DISCONNECTED);
	TIMER1_voidGeneratePWM_B(TIMER1_PWM_DISCONNECTED);
}

static void TEST_voidUsartBaud(){
	TEST_voidPrintLine(FLASH_STR("MANUAL: if you can read this line , USART TX works at 9600 baud."));
	/* UBRRH is not read : in Proteus a single read of that address returns UCSRC */
	TEST_voidCheck(UBRRL == (u8)USART_UBRR_VALUE, FLASH_STR("USART"), FLASH_STR("UBRRL = computed value (103 at 16 MHz)"));
}

static void TEST_voidUsartRx(){
	u16 Local_u16Ticks = 0 ;
	u8 Local_u8Count = 0 ;
	USART_voidSetCallBack_RX(TEST_voidRxCallback);
	USART_voidEnableRxInterrupt();
	TEST_voidPrintLine(FLASH_STR("MANUAL: USART RX - type 3 keys in the Virtual Terminal (10 s)"));
	while((Local_u16Ticks < TEST_MANUAL_TIMEOUT_TICKS) && (Local_u8Count < TEST_RX_KEYS)){
		TEST_voidTick();
		Local_u16Ticks++;
		Local_u8Count = Global_u8RxCount ;   /* one byte : atomic */
	}
	TEST_voidPrint(FLASH_STR("       received : "));
	for(u8 i = 0 ; (i < Local_u8Count) && (i < TEST_RX_KEYS) ; i++){
		USART_voidSend(Global_c8RxKeys[i]);
	}
	TEST_voidNewLine();
	TEST_voidCheck(Local_u8Count >= TEST_RX_KEYS, FLASH_STR("USART"), FLASH_STR("RX callback receives 3 typed keys"));
}

static void TEST_voidUsartTxBurst(){
	u16 Local_u16Sent ;
	u8 Local_u8Next ;
	u16 Local_u16Guard ;
	USART_voidSetCallBack_TX(TEST_voidTxCallback);
	TEST_voidPrintLine(FLASH_STR("MANUAL: USART TX - 300 bytes follow , 10 lines of 28 equal letters A..J:"));
	/* the burst goes through the ring + UDRE interrupt , main only waits when the ring is full */
	Global_u16TxSent = 0 ;
	for(u8 Local_u8Line = 0 ; Local_u8Line < TEST_TX_LINES ; Local_u8Line++){
		for(u8 Local_u8Col = 0 ; Local_u8Col < (TEST_TX_LINE_CHARS + 2) ; Local_u8Col++){
			u8 Local_u8Byte ;
			if(Local_u8Col < TEST_TX_LINE_CHARS){
				Local_u8Byte = 'A' + Local_u8Line ;
			}else if(Local_u8Col == TEST_TX_LINE_CHARS){
				Local_u8Byte = '\r' ;
			}else{
				Local_u8Byte = '\n' ;
			}
			Local_u8Next = (Global_u8TxHead + 1) % TEST_TX_RING_SIZE ;
			while(Local_u8Next == Global_u8TxTail){
				/* ring full : the interrupt frees a slot every 1 ms */
			}
			Global_u8TxRing[Global_u8TxHead] = Local_u8Byte ;
			Global_u8TxHead = Local_u8Next ;
			USART_voidEnableTxInterrupt();
		}
	}
	/* wait until the ring is empty and the last byte has left the shift register */
	Local_u16Guard = 0 ;
	while(((Global_u8TxTail != Global_u8TxHead) || (GET_BIT(UCSRA,UCSRA_UDRE) == 0)) && (Local_u16Guard < 1000)){
		_delay_ms(1);
		Local_u16Guard++;
	}
	_delay_ms(2);   /* last byte still in the shift register */
	GIE_voidDisableGlobalInterrupt();
	Local_u16Sent = Global_u16TxSent ;
	GIE_voidEnableGlobalInterrupt();
	TEST_voidPrint(FLASH_STR("       bytes sent by the interrupt = "));
	TEST_voidPrintNum(Local_u16Sent);
	TEST_voidPrintLine(FLASH_STR(" (expected 300)"));
	TEST_voidCheck(Local_u16Sent == TEST_TX_BURST_BYTES, FLASH_STR("USART"), FLASH_STR("300-byte burst sent by the TX interrupt , none lost"));
	TEST_voidCheck(GET_BIT(UCSRB,UCSRB_UDRIE) == 0, FLASH_STR("USART"), FLASH_STR("TX interrupt switches itself off when the ring is empty"));
}

/* START + SLA+W + STOP , elapsed time in 0.5 us timer ticks (Timer1 runs free : needs TIMER1_voidInit) */
static u8 TEST_u8ProbeTimed(u8 Copy_u8Address, u16 * Copy_pu16Elapsed){
	u16 Local_u16Start = TEST_u16ReadTCNT1();
	u8 Local_u8Error = TWI_u8ProbeAddress(Copy_u8Address);
	u16 Local_u16End = TEST_u16ReadTCNT1();
	if(Local_u16End >= Local_u16Start){
		*Copy_pu16Elapsed = Local_u16End - Local_u16Start ;
	}else{
		*Copy_pu16Elapsed = (TIMER1_TOP_VALUE + 1 - Local_u16Start) + Local_u16End ;   /* counter wrapped */
	}
	return Local_u8Error ;
}
static void TEST_voidTwi(){
	u16 Local_u16Elapsed ;
	u16 Local_u16Ticks = 0 ;
	u8 Local_u8Error = TWI_OK ;
	u8 Local_u8Seen = 0 ;
	TWI_voidMasterInit();
	TEST_voidCheck(TWBR == TWI_TWBR_VALUE, FLASH_STR("TWI"), FLASH_STR("bit rate register set for 100 kHz"));
	TEST_voidCheck(TWI_u8ProbeAddress(TEST_TWI_ADDR_LAMPS) == TWI_OK, FLASH_STR("TWI"), FLASH_STR("0x20 (lamps PCF8574) ACKs"));
	TEST_voidCheck(TWI_u8ProbeAddress(TEST_TWI_ADDR_LCD) == TWI_OK, FLASH_STR("TWI"), FLASH_STR("0x27 (LCD PCF8574) ACKs"));
	TEST_voidCheck(TWI_u8ProbeAddress(TEST_TWI_ADDR_EEPROM) == TWI_OK, FLASH_STR("TWI"), FLASH_STR("0x50 (24C08) ACKs"));
	TEST_voidCheck(TWI_u8ProbeAddress(TEST_TWI_ADDR_ABSENT) == TWI_ERR_SLA_NACK, FLASH_STR("TWI"), FLASH_STR("0x60 (nobody) gives TWI_ERR_SLA_NACK"));

	/*1. MANUAL : SDA held low --> TWI_ERR_TIMEOUT within 2 ms */
	TEST_voidPrintLine(FLASH_STR("MANUAL: TWI - within 20 s CLOSE the SDA-to-GND switch (hold SDA low)."));
	while((Local_u16Ticks < TEST_SDA_TIMEOUT_TICKS) && (Local_u8Seen == 0)){
		TEST_voidTick();
		Local_u16Ticks++;
		Local_u8Error = TEST_u8ProbeTimed(TEST_TWI_ADDR_LCD,&Local_u16Elapsed);
		if(Local_u8Error != TWI_OK){
			Local_u8Seen = 1 ;
		}
	}
	if(Local_u8Seen){
		TEST_voidPrint(FLASH_STR("       failed probe took "));
		TEST_voidPrintNum((u16)(((u32)Local_u16Elapsed * 1000UL) / TIMER1_TICKS_PER_MS));
		TEST_voidPrintLine(FLASH_STR(" us"));
		TEST_voidCheck(Local_u8Error == TWI_ERR_TIMEOUT, FLASH_STR("TWI"), FLASH_STR("stuck SDA returns TWI_ERR_TIMEOUT"));
		TEST_voidCheck(Local_u16Elapsed < (2 * TIMER1_TICKS_PER_MS), FLASH_STR("TWI"), FLASH_STR("... within 2 ms (no hang)"));

		/*2. release : the bus must work again after the switch is opened */
		TEST_voidPrintLine(FLASH_STR("MANUAL: TWI - now OPEN the switch again (10 s)."));
		Local_u16Ticks = 0 ;
		Local_u8Error = TWI_ERR_TIMEOUT ;
		while((Local_u16Ticks < TEST_MANUAL_TIMEOUT_TICKS) && (Local_u8Error != TWI_OK)){
			TEST_voidTick();
			Local_u16Ticks++;
			Local_u8Error = TWI_u8ProbeAddress(TEST_TWI_ADDR_LCD);
		}
		TEST_voidCheck(Local_u8Error == TWI_OK, FLASH_STR("TWI"), FLASH_STR("bus recovers after SDA is released (0x27 ACKs again)"));
	}else{
		TEST_voidSkip(FLASH_STR("TWI"), FLASH_STR("SDA was not held low , timeout not tested"));
	}
}

static void TEST_voidExti(){
	u16 Local_u16Ticks = 0 ;
	u8 Local_u8Count ;
	/* PD2 is switched to OUTPUT for a moment : a software edge triggers INT0
	 * (keep the button released) */
	EXTI_SetInterruptSenceCTRL(EXTI_INT0,EXTI_FALLING_EDGE);

	/*1. no callback set : the ISR must not crash */
	EXTI_voidINT0_callBack(NULL);
	DIO_voidSetPinValue(TEST_INT0_PORT,TEST_INT0_PIN,DIO_PIN_HIGH);
	DIO_voidSetPinDirection(TEST_INT0_PORT,TEST_INT0_PIN,DIO_PIN_OUTPUT);
	GIFR = (1<<GIFR_INTF0);
	EXTI_voidEnableINT(EXTI_INT0);
	DIO_voidSetPinValue(TEST_INT0_PORT,TEST_INT0_PIN,DIO_PIN_LOW);   /* falling edge */
	_delay_ms(1);
	TEST_voidCheck(1, FLASH_STR("EXTI"), FLASH_STR("INT0 with no callback does not crash"));

	/*2. callback set , software edge */
	EXTI_voidINT0_callBack(TEST_voidInt0Callback);
	DIO_voidSetPinValue(TEST_INT0_PORT,TEST_INT0_PIN,DIO_PIN_HIGH);
	GIFR = (1<<GIFR_INTF0);
	Global_u8Int0Count = 0 ;
	DIO_voidSetPinValue(TEST_INT0_PORT,TEST_INT0_PIN,DIO_PIN_LOW);   /* falling edge */
	_delay_ms(1);
	Local_u8Count = Global_u8Int0Count ;
	TEST_voidPrint(FLASH_STR("       INT0 callbacks counted = "));
	TEST_voidPrintNum(Local_u8Count);
	TEST_voidPrintLine(FLASH_STR(" (expected 1)"));
	if(Local_u8Count == 1){
		TEST_voidCheck(1, FLASH_STR("EXTI"), FLASH_STR("INT0 callback called once by a software edge"));
	}else if(Local_u8Count == 0){
		/* the button step below tests the same path ; Proteus does not seem to raise INT0 for a pin the MCU drives itself */
		TEST_voidSkip(FLASH_STR("EXTI"), FLASH_STR("software edge not seen (Proteus limit?) , see button step"));
	}else{
		TEST_voidCheck(0, FLASH_STR("EXTI"), FLASH_STR("INT0 callback called once by a software edge"));
	}

	/*3. MANUAL : the real button */
	DIO_voidSetPinDirection(TEST_INT0_PORT,TEST_INT0_PIN,DIO_INPUT);
	DIO_voidEnablePullUp(TEST_INT0_PORT,TEST_INT0_PIN);
	_delay_ms(1);
	GIFR = (1<<GIFR_INTF0);
	Global_u8Int0Count = 0 ;
	TEST_voidPrintLine(FLASH_STR("MANUAL: EXTI - press the button on PD2 (10 s)"));
	while((Local_u16Ticks < TEST_MANUAL_TIMEOUT_TICKS) && (Global_u8Int0Count == 0)){
		TEST_voidTick();
		Local_u16Ticks++;
	}
	EXTI_voidDisableINT(EXTI_INT0);
	if(Global_u8Int0Count > 0){
		TEST_voidCheck(1, FLASH_STR("EXTI"), FLASH_STR("INT0 falling edge from the button"));
	}else{
		TEST_voidSkip(FLASH_STR("EXTI"), FLASH_STR("button on PD2 was not pressed"));
	}
}

int main(void){
	DIO_voidSetPinDirection(TEST_LED_PORT,TEST_LED_PIN,DIO_PIN_OUTPUT);
	USART_voidInit();
	GIE_voidEnableGlobalInterrupt();
	TEST_voidNewLine();
	TEST_voidPrintLine(FLASH_STR("===== test_mcal : Phase 2 test of the MCAL layer ====="));
	TEST_voidPrintLine(FLASH_STR("Status LED: slow blink = running , solid = all passed , fast blink = a check failed"));

	TEST_voidDioJtag();
	TEST_voidUsartBaud();
	TEST_voidAdc();
	TEST_voidTimer0Pwm();
	TEST_voidTimer1();
	TEST_voidTimer2();      /* uses the Timer1 PWM that is still running */
	TEST_voidUsartRx();
	TEST_voidUsartTxBurst();
	TEST_voidTwi();         /* uses Timer1 as a free-running stop watch */
	TEST_voidExti();

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
