/*
 * LCD_BUF.c
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */
#include "../../Service/std_types.h"
#include "../../Service/Bit_math.h"
#include "../CLCD/CLCD.h"
#include "LCD_BUF.h"
#include "LCD_BUF_cfg.h"

#define LCD_BUF_CELLS           (LCD_BUF_ROWS * LCD_BUF_COLS)
#define LCD_BUF_CURSOR_UNKNOWN  0xFF

static c8 Global_c8Wanted[LCD_BUF_CELLS] ;     /* what the application wants on the screen */
static c8 Global_c8Shown [LCD_BUF_CELLS] ;     /* what the LCD really shows */
static u8 Global_u8Cursor = LCD_BUF_CURSOR_UNKNOWN ;   /* cell index where the next data byte lands */
static u8 Global_u8Scan = 0 ;                  /* cell where the search for a difference starts */
static u16 Global_u16Wait = 0 ;                /* updates left to wait after an I2C error */

void LCD_BUF_voidInit(){
	/*1. Blocking LCD init (boot only) : it also clears the LCD , cursor is at home */
	CLCD_voidInit();
	/*2. Both copies are blank */
	for(u8 i = 0 ; i < LCD_BUF_CELLS ; i++){
		Global_c8Wanted[i] = ' ' ;
		Global_c8Shown [i] = ' ' ;
	}
	Global_u8Cursor = 0 ;
	Global_u8Scan = 0 ;
	Global_u16Wait = 0 ;
}
void LCD_BUF_voidClear(){
	for(u8 i = 0 ; i < LCD_BUF_CELLS ; i++){
		Global_c8Wanted[i] = ' ' ;
	}
}
void LCD_BUF_voidWriteChar(u8 Copy_u8Row, u8 Copy_u8Col, c8 Copy_c8Char){
	if((Copy_u8Row < LCD_BUF_ROWS) && (Copy_u8Col < LCD_BUF_COLS)){
		Global_c8Wanted[(Copy_u8Row * LCD_BUF_COLS) + Copy_u8Col] = Copy_c8Char ;
	}else{
		//error
	}
}
void LCD_BUF_voidWriteFlash(u8 Copy_u8Row, u8 Copy_u8Col, const __flash c8 * Copy_pc8Text){
	/* text longer than the row is cut */
	while((*Copy_pc8Text != '\0') && (Copy_u8Col < LCD_BUF_COLS)){
		LCD_BUF_voidWriteChar(Copy_u8Row,Copy_u8Col++,*Copy_pc8Text++);
	}
}
void LCD_BUF_voidWriteRam(u8 Copy_u8Row, u8 Copy_u8Col, const c8 * Copy_pc8Text){
	while((*Copy_pc8Text != '\0') && (Copy_u8Col < LCD_BUF_COLS)){
		LCD_BUF_voidWriteChar(Copy_u8Row,Copy_u8Col++,*Copy_pc8Text++);
	}
}
void LCD_BUF_voidUpdate(){
	u8 Local_u8Cell = Global_u8Scan ;
	u8 Local_u8Found = 0 ;
	/*1. After an I2C error stay quiet for a while (the LCD may be unplugged) */
	if(Global_u16Wait > 0){
		Global_u16Wait--;
		return ;
	}
	/*2. Find the next cell that is different (round robin , at most one lap) */
	for(u8 i = 0 ; i < LCD_BUF_CELLS ; i++){
		if(Global_c8Wanted[Local_u8Cell] != Global_c8Shown[Local_u8Cell]){
			Local_u8Found = 1 ;
			break ;
		}
		Local_u8Cell++;
		if(Local_u8Cell >= LCD_BUF_CELLS){
			Local_u8Cell = 0 ;
		}
	}
	if(Local_u8Found == 0){
		return ;
	}
	/*3. Cursor not there yet : this call only sets the cursor */
	if(Global_u8Cursor != Local_u8Cell){
		CLCD_voidSetCursorPosition(Local_u8Cell % LCD_BUF_COLS, Local_u8Cell / LCD_BUF_COLS);
		if(CLCD_u8GetStatus() == 0){
			Global_u8Cursor = Local_u8Cell ;
		}else{
			Global_u8Cursor = LCD_BUF_CURSOR_UNKNOWN ;
			Global_u16Wait = LCD_BUF_RETRY_UPDATES ;
		}
		Global_u8Scan = Local_u8Cell ;
		return ;
	}
	/*4. Cursor is there : this call sends the character */
	CLCD_voidSendData(Global_c8Wanted[Local_u8Cell]);
	if(CLCD_u8GetStatus() == 0){
		Global_c8Shown[Local_u8Cell] = Global_c8Wanted[Local_u8Cell] ;
		/* the LCD moves its cursor right ; at the end of a row it leaves the visible area */
		if(((Local_u8Cell + 1) % LCD_BUF_COLS) == 0){
			Global_u8Cursor = LCD_BUF_CURSOR_UNKNOWN ;
		}else{
			Global_u8Cursor = Local_u8Cell + 1 ;
		}
		Local_u8Cell++;
		if(Local_u8Cell >= LCD_BUF_CELLS){
			Local_u8Cell = 0 ;
		}
		Global_u8Scan = Local_u8Cell ;
	}else{
		/* byte may be lost : keep "shown" as it is so it is sent again later */
		Global_u8Cursor = LCD_BUF_CURSOR_UNKNOWN ;
		Global_u16Wait = LCD_BUF_RETRY_UPDATES ;
	}
}
