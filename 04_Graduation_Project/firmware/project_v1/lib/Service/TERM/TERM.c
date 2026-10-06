/*
 * TERM.c
 *
 *  Created on: Oct 3, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../std_types.h"
#include "../Bit_math.h"
#include "../RINGBUF/RINGBUF.h"
#include "../FMT/FMT.h"
#include "../../MCAL/GIE/GIE.h"
#include "../../MCAL/USART/USART.h"
#include "TERM.h"
#include "TERM_cfg.h"

#define TERM_CHAR_BACKSPACE    0x08
#define TERM_CHAR_DELETE       0x7F                    /* some terminals send this for the backspace key */
#define TERM_CHAR_FIRST        0x20                    /* printable range : space .. '~' */
#define TERM_CHAR_LAST         0x7E

/* ring storage : one byte more than the cfg size , because a ring holds Size - 1 bytes */
static volatile u8 Global_u8RxStorage[TERM_RX_BUFFER_SIZE + 1] ;
static volatile u8 Global_u8TxStorage[TERM_TX_BUFFER_SIZE + 1] ;
static RINGBUF_t Global_RxRing ;                       /* producer = RX ISR , consumer = main loop */
static RINGBUF_t Global_TxRing ;                       /* producer = main loop , consumer = UDRE ISR */
static volatile u8 Global_u8RxActive = 0 ;             /* set by the RX ISR */

/* line editor (main loop only) */
static c8 Global_c8Line[TERM_LINE_MAX + 1] ;
static u8 Global_u8LineLength = 0 ;
static u8 Global_u8TooLong = 0 ;                       /* 1 = this line overflowed , ignore it up to its end */
static u8 Global_u8LastWasCR = 0 ;                     /* 1 = the LF of a CR LF pair must be skipped */
static u8 Global_u8Echo = TERM_ECHO_DEFAULT ;

/* RX complete ISR callback : one byte into the ring , nothing else (REQ-RUI-01) */
static void TERM_voidRxCallback(u8 Copy_u8Data){
	/* a full ring drops the byte : nothing useful can be done inside the ISR */
	RINGBUF_u8Put(&Global_RxRing,Copy_u8Data);
	Global_u8RxActive = 1 ;
}

/* data register empty ISR callback : next byte , or stop the interrupt when the ring is empty */
static void TERM_voidTxCallback(void){
	u8 Local_u8Data ;
	if(RINGBUF_u8Get(&Global_TxRing,&Local_u8Data) == RINGBUF_OK){
		USART_voidWriteData(Local_u8Data);
	}else{
		USART_voidDisableTxInterrupt();
	}
}

void TERM_voidInit(){
	/*1. rings first : the callbacks use them */
	RINGBUF_voidInit(&Global_RxRing,Global_u8RxStorage,TERM_RX_BUFFER_SIZE + 1);
	RINGBUF_voidInit(&Global_TxRing,Global_u8TxStorage,TERM_TX_BUFFER_SIZE + 1);
	Global_u8RxActive = 0 ;
	Global_u8LineLength = 0 ;
	Global_u8TooLong = 0 ;
	Global_u8LastWasCR = 0 ;
	Global_u8Echo = TERM_ECHO_DEFAULT ;
	/*2. UART : 9600 8N1 , baud from F_CPU (USART_cfg.h) */
	USART_voidInit();
	USART_voidSetCallBack_RX(TERM_voidRxCallback);
	USART_voidSetCallBack_TX(TERM_voidTxCallback);
	/*3. RX interrupt on ; the TX interrupt is switched on when there is something to send */
	USART_voidEnableRxInterrupt();
}

u8 TERM_u8TxFree(){
	return RINGBUF_u8GetFree(&Global_TxRing) ;
}

void TERM_voidPutChar(c8 Copy_c8Char){
	if(RINGBUF_u8Put(&Global_TxRing,(u8)Copy_c8Char) == RINGBUF_OK){
		/* the byte is in the ring before the interrupt is enabled , so it can not be missed */
		USART_voidEnableTxInterrupt();
	}else{
		/* ring full : the character is dropped , the loop never waits for the UART */
	}
}

void TERM_voidPutFlash(const __flash c8 * Copy_pc8Text){
	while(*Copy_pc8Text != '\0'){
		TERM_voidPutChar(*Copy_pc8Text++);
	}
}

void TERM_voidPutRam(const c8 * Copy_pc8Text){
	while(*Copy_pc8Text != '\0'){
		TERM_voidPutChar(*Copy_pc8Text++);
	}
}

void TERM_voidPutNumber(u16 Copy_u16Number){
	c8 Local_c8Text[FMT_NUMBER_TEXT_SIZE] ;
	FMT_u8NumberToText(Copy_u16Number,Local_c8Text);
	TERM_voidPutRam(Local_c8Text);
}

void TERM_voidNewLine(){
	TERM_voidPutChar('\r');
	TERM_voidPutChar('\n');
}

void TERM_voidSetEcho(u8 Copy_u8Mode){
	if(Copy_u8Mode <= TERM_ECHO_MASKED){
		Global_u8Echo = Copy_u8Mode ;
	}else{
		//error
	}
}

/* Line editor : takes the bytes waiting in the RX ring until one line is complete.
 * CR , LF and CR LF all end ONE line. */
u8 TERM_u8GetLine(c8 * Copy_pc8Line){
	u8 Local_u8Char ;
	u8 i ;
	while(RINGBUF_u8Get(&Global_RxRing,&Local_u8Char) == RINGBUF_OK){
		/*1. the LF right after a CR belongs to the line that already ended */
		if((Local_u8Char == '\n') && (Global_u8LastWasCR == 1)){
			Global_u8LastWasCR = 0 ;
			continue ;
		}
		Global_u8LastWasCR = (Local_u8Char == '\r') ? 1 : 0 ;

		/*2. end of line */
		if((Local_u8Char == '\r') || (Local_u8Char == '\n')){
			if(Global_u8Echo != TERM_ECHO_OFF){
				TERM_voidNewLine();
			}
			if(Global_u8TooLong == 1){
				Global_u8TooLong = 0 ;
				Global_u8LineLength = 0 ;
				return TERM_LINE_TOO_LONG ;
			}
			for(i = 0 ; i < Global_u8LineLength ; i++){
				Copy_pc8Line[i] = Global_c8Line[i] ;
			}
			Copy_pc8Line[Global_u8LineLength] = '\0' ;
			Global_u8LineLength = 0 ;
			return TERM_LINE_READY ;
		}

		/*3. the rest of a too long line is ignored */
		if(Global_u8TooLong == 1){
			continue ;
		}

		/*4. backspace : remove the last character , on the screen too */
		if((Local_u8Char == TERM_CHAR_BACKSPACE) || (Local_u8Char == TERM_CHAR_DELETE)){
			if(Global_u8LineLength > 0){
				Global_u8LineLength--;
				if(Global_u8Echo != TERM_ECHO_OFF){
					TERM_voidPutChar(TERM_CHAR_BACKSPACE);
					TERM_voidPutChar(' ');
					TERM_voidPutChar(TERM_CHAR_BACKSPACE);
				}
			}
			continue ;
		}

		/*5. a normal character (other control codes are ignored) */
		if((Local_u8Char >= TERM_CHAR_FIRST) && (Local_u8Char <= TERM_CHAR_LAST)){
			if(Global_u8LineLength < TERM_LINE_MAX){
				Global_c8Line[Global_u8LineLength++] = (c8)Local_u8Char ;
				if(Global_u8Echo == TERM_ECHO_NORMAL){
					TERM_voidPutChar((c8)Local_u8Char);
				}else if(Global_u8Echo == TERM_ECHO_MASKED){
					TERM_voidPutChar('*');             /* password stays hidden on the terminal */
				}
			}else{
				Global_u8TooLong = 1 ;
			}
		}
	}
	return TERM_LINE_NONE ;
}

u8 TERM_u8IsLineEmpty(){
	if((Global_u8LineLength == 0) && (Global_u8TooLong == 0)){
		return 1 ;
	}
	return 0 ;
}

u8 TERM_u8IsRxActive(){
	u8 Local_u8Active ;
	/* read-and-clear must not be cut by the RX ISR (it sets the flag) */
	GIE_voidDisableGlobalInterrupt();
	Local_u8Active = Global_u8RxActive ;
	Global_u8RxActive = 0 ;
	GIE_voidEnableGlobalInterrupt();
	return Local_u8Active ;
}
