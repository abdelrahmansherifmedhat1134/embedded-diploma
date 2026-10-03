/*
 * SEVEN_SEG.c
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */
#include "../../Service/std_types.h"
#include "../../Service/Bit_math.h"
#include "../../MCAL/DIO/DIO.h"
#include "SEVEN_SEG.h"
#include "SEVEN_SEG_cfg.h"

#define SEVEN_SEG_DIGIT_OFF_LEVEL   (!SEVEN_SEG_DIGIT_ON_LEVEL)

#if SEVEN_SEG_BLANK_TICKS >= SEVEN_SEG_TICKS_PER_DIGIT
#error "SEVEN_SEG_BLANK_TICKS must be smaller than SEVEN_SEG_TICKS_PER_DIGIT"
#endif

/* Shared with the tick ISR (SEVEN_SEG_voidRefresh) : single bytes , so every access is atomic.
 * The number is kept as packed BCD (tens in the high nibble) so one write changes both digits. */
static volatile u8 Global_u8Bcd = 0 ;
static volatile u8 Global_u8Enabled = 0 ;
/* used by the ISR only */
static u8 Global_u8Ticks = 0 ;
static u8 Global_u8NextDigit = 0 ;      /* 0 = tens , 1 = units */

static void SEVEN_SEG_voidBothOff(){
	DIO_voidSetPinValue(SEVEN_SEG_DIGIT_PORT,SEVEN_SEG_TENS_PIN ,SEVEN_SEG_DIGIT_OFF_LEVEL);
	DIO_voidSetPinValue(SEVEN_SEG_DIGIT_PORT,SEVEN_SEG_UNITS_PIN,SEVEN_SEG_DIGIT_OFF_LEVEL);
}
static void SEVEN_SEG_voidPutBcd(u8 Copy_u8Digit){
	DIO_voidSetPinValue(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_A,GET_BIT(Copy_u8Digit,0));
	DIO_voidSetPinValue(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_B,GET_BIT(Copy_u8Digit,1));
	DIO_voidSetPinValue(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_C,GET_BIT(Copy_u8Digit,2));
	DIO_voidSetPinValue(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_D,GET_BIT(Copy_u8Digit,3));
}

void SEVEN_SEG_voidInit(){
	/*0. PC2..PC5 are JTAG pins : release them (pin_map C-3) */
	DIO_voidDisableJTAG();
	/*1. Digit enables : off first , then output (no flash at start-up) */
	SEVEN_SEG_voidBothOff();
	DIO_voidSetPinDirection(SEVEN_SEG_DIGIT_PORT,SEVEN_SEG_TENS_PIN ,DIO_PIN_OUTPUT);
	DIO_voidSetPinDirection(SEVEN_SEG_DIGIT_PORT,SEVEN_SEG_UNITS_PIN,DIO_PIN_OUTPUT);
	/*2. BCD pins output */
	DIO_voidSetPinDirection(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_A,DIO_PIN_OUTPUT);
	DIO_voidSetPinDirection(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_B,DIO_PIN_OUTPUT);
	DIO_voidSetPinDirection(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_C,DIO_PIN_OUTPUT);
	DIO_voidSetPinDirection(SEVEN_SEG_BCD_PORT,SEVEN_SEG_BCD_PIN_D,DIO_PIN_OUTPUT);
	/*3. Blank display. From now on only SEVEN_SEG_voidRefresh (ISR) writes these pins. */
	Global_u8Bcd = 0 ;
	Global_u8Enabled = 0 ;
	Global_u8Ticks = 0 ;
	Global_u8NextDigit = 0 ;
}
void SEVEN_SEG_voidSetNumber(u8 Copy_u8Number){
	if(Copy_u8Number > 99){
		Copy_u8Number = 99 ;
	}
	Global_u8Bcd = (u8)(((Copy_u8Number / 10) << 4) | (Copy_u8Number % 10)) ;
}
void SEVEN_SEG_voidEnable(){
	Global_u8Enabled = 1 ;
}
void SEVEN_SEG_voidDisable(){
	/* the ISR switches the digits off , so the main loop never touches the port */
	Global_u8Enabled = 0 ;
}
void SEVEN_SEG_voidRefresh(){
	if(Global_u8Enabled == 0){
		SEVEN_SEG_voidBothOff();
		Global_u8Ticks = 0 ;
		return ;
	}
	/* every SEVEN_SEG_TICKS_PER_DIGIT ticks : show the other digit */
	if(Global_u8Ticks < SEVEN_SEG_BLANK_TICKS){
		/*1. dead time : both digits off , so the old digit (slow PNP) is really dark
		 *   before the new BCD appears (no ghosting) */
		SEVEN_SEG_voidBothOff();
	}else if(Global_u8Ticks == SEVEN_SEG_BLANK_TICKS){
		u8 Local_u8Bcd = Global_u8Bcd ;
		/*2. BCD of the digit to show */
		if(Global_u8NextDigit == 0){
			SEVEN_SEG_voidPutBcd(Local_u8Bcd >> 4);
			DIO_voidSetPinValue(SEVEN_SEG_DIGIT_PORT,SEVEN_SEG_TENS_PIN,SEVEN_SEG_DIGIT_ON_LEVEL);
			Global_u8NextDigit = 1 ;
		}else{
			SEVEN_SEG_voidPutBcd(Local_u8Bcd & 0x0F);
			DIO_voidSetPinValue(SEVEN_SEG_DIGIT_PORT,SEVEN_SEG_UNITS_PIN,SEVEN_SEG_DIGIT_ON_LEVEL);
			Global_u8NextDigit = 0 ;
		}
	}
	Global_u8Ticks++;
	if(Global_u8Ticks >= SEVEN_SEG_TICKS_PER_DIGIT){
		Global_u8Ticks = 0 ;
	}
}
