/*
 * SEVEN_SEG.c
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../../Service/std_types.h"
#include "../../MCAL/TWI/TWI.h"
#include "../PCF8574/PCF8574.h"
#include "SEVEN_SEG.h"
#include "SEVEN_SEG_cfg.h"

/* Pattern -> pin level : the tables below are written for a LOW = ON (common anode) wiring */
#if SEVEN_SEG_ON_LEVEL == 0
#define SEVEN_SEG_LEVEL(pattern)    ((u8)(pattern))
#else
#define SEVEN_SEG_LEVEL(pattern)    ((u8)~(pattern))
#endif
#define SEVEN_SEG_BLANK             SEVEN_SEG_LEVEL(0xFF)

/* Digit 0..9 : bit0 = a ... bit6 = g , bit7 = dp (always off) , bit = 0 lights the segment.
 *   a
 * f   b       0xC0 = a b c d e f on , g off
 *   g
 * e   c
 *   d                                                                    */
static const __flash u8 SEVEN_SEG_TABLE[10] = {
		0xC0,   /* 0 */
		0xF9,   /* 1 */
		0xA4,   /* 2 */
		0xB0,   /* 3 */
		0x99,   /* 4 */
		0x92,   /* 5 */
		0x82,   /* 6 */
		0xF8,   /* 7 */
		0x80,   /* 8 */
		0x90    /* 9 */
};

static u8 Global_u8Number = 0 ;
static u8 Global_u8Enabled = 0 ;
/* what the two chips show now ; Global_u8KnownTens / Units = 0 means "unknown , write it" */
static u8 Global_u8ShownTens = 0 ;
static u8 Global_u8ShownUnits = 0 ;
static u8 Global_u8KnownTens = 0 ;
static u8 Global_u8KnownUnits = 0 ;
static u8 Global_u8Status = 0 ;

/* write one digit only when its pattern is not on the chip yet ; returns the I2C status */
static u8 SEVEN_SEG_u8WriteDigit(u8 Copy_u8Address, u8 Copy_u8Pattern, u8 * Copy_pu8Shown, u8 * Copy_pu8Known){
	u8 Local_u8Status = TWI_OK ;
	if((*Copy_pu8Known == 0) || (*Copy_pu8Shown != Copy_u8Pattern)){
		Local_u8Status = PCF8574_u8WritePort(Copy_u8Address,Copy_u8Pattern);
		if(Local_u8Status == TWI_OK){
			*Copy_pu8Shown = Copy_u8Pattern ;
			*Copy_pu8Known = 1 ;
		}else{
			/* the chip may have missed it : try again at the next call */
			*Copy_pu8Known = 0 ;
		}
	}
	return Local_u8Status ;
}
/* bring both chips to the wanted state */
static void SEVEN_SEG_voidShow(){
	u8 Local_u8Tens = SEVEN_SEG_BLANK ;
	u8 Local_u8Units = SEVEN_SEG_BLANK ;
	u8 Local_u8Status ;
	if(Global_u8Enabled == 1){
		Local_u8Tens  = SEVEN_SEG_LEVEL(SEVEN_SEG_TABLE[Global_u8Number / 10]) ;
		Local_u8Units = SEVEN_SEG_LEVEL(SEVEN_SEG_TABLE[Global_u8Number % 10]) ;
	}
	Global_u8Status = TWI_OK ;
	Local_u8Status = SEVEN_SEG_u8WriteDigit(SEVEN_SEG_TENS_ADDRESS,Local_u8Tens,&Global_u8ShownTens,&Global_u8KnownTens);
	if(Local_u8Status != TWI_OK){
		Global_u8Status = Local_u8Status ;
	}
	Local_u8Status = SEVEN_SEG_u8WriteDigit(SEVEN_SEG_UNITS_ADDRESS,Local_u8Units,&Global_u8ShownUnits,&Global_u8KnownUnits);
	if(Local_u8Status != TWI_OK){
		Global_u8Status = Local_u8Status ;
	}
}

void SEVEN_SEG_voidInit(){
	PCF8574_voidInit();
	Global_u8Number = 0 ;
	Global_u8Enabled = 0 ;
	Global_u8KnownTens = 0 ;
	Global_u8KnownUnits = 0 ;
	SEVEN_SEG_voidShow();
}
void SEVEN_SEG_voidSetNumber(u8 Copy_u8Number){
	if(Copy_u8Number > 99){
		Copy_u8Number = 99 ;
	}
	Global_u8Number = Copy_u8Number ;
	SEVEN_SEG_voidShow();
}
void SEVEN_SEG_voidEnable(){
	Global_u8Enabled = 1 ;
	SEVEN_SEG_voidShow();
}
void SEVEN_SEG_voidDisable(){
	Global_u8Enabled = 0 ;
	SEVEN_SEG_voidShow();
}
u8 SEVEN_SEG_u8GetStatus(){
	return Global_u8Status ;
}
