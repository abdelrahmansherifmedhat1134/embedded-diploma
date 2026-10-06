/*
 * FMT.c
 *
 *  Created on: Oct 3, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../std_types.h"
#include "FMT.h"

#define FMT_U16_MAX_DIV_10    6553     /* 65535 / 10 */
#define FMT_U16_MAX_MOD_10    5        /* 65535 % 10 */

/* no printf : it would cost about 1.5 KB of flash */
u8 FMT_u8NumberToText(u16 Copy_u16Number, c8 * Copy_pc8Text){
	c8 Local_c8Digits[FMT_NUMBER_TEXT_SIZE - 1] ;
	u8 Local_u8Count = 0 ;
	u8 Local_u8Length = 0 ;
	/*1. digits come out last one first */
	do{
		Local_c8Digits[Local_u8Count++] = '0' + (Copy_u16Number % 10) ;
		Copy_u16Number /= 10 ;
	}while(Copy_u16Number != 0);
	/*2. copy them in reading order */
	while(Local_u8Count > 0){
		Copy_pc8Text[Local_u8Length++] = Local_c8Digits[--Local_u8Count] ;
	}
	Copy_pc8Text[Local_u8Length] = '\0' ;
	return Local_u8Length ;
}

u8 FMT_u8TextToNumber(const c8 * Copy_pc8Text, u16 * Copy_pu16Number){
	u16 Local_u16Value = 0 ;
	u8 Local_u8Digit ;
	/*1. empty text is not a number */
	if(*Copy_pc8Text == '\0'){
		return 0 ;
	}
	while(*Copy_pc8Text != '\0'){
		/*2. digits only : no sign , no space */
		if((*Copy_pc8Text < '0') || (*Copy_pc8Text > '9')){
			return 0 ;
		}
		Local_u8Digit = *Copy_pc8Text - '0' ;
		/*3. would value * 10 + digit pass 65535 ? checked BEFORE the multiply overflows */
		if((Local_u16Value > FMT_U16_MAX_DIV_10) || ((Local_u16Value == FMT_U16_MAX_DIV_10) && (Local_u8Digit > FMT_U16_MAX_MOD_10))){
			return 0 ;
		}
		Local_u16Value = (Local_u16Value * 10) + Local_u8Digit ;
		Copy_pc8Text++;
	}
	*Copy_pu16Number = Local_u16Value ;
	return 1 ;
}
