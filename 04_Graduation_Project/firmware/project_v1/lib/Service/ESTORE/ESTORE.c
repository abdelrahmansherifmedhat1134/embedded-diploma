/*
 * ESTORE.c
 *
 *  Created on: Oct 3, 2026
 *      Author: eslam
 */
#include "../std_types.h"
#include "../Bit_math.h"
#include "../EVQ/EVQ.h"
#include "../../HAL/EXT_EEPROM/EXT_EEPROM.h"
#include "ESTORE.h"
#include "ESTORE_cfg.h"

/* write-behind states (docs/architecture.md Section 5.9) */
#define ESTORE_STATE_IDLE       0
#define ESTORE_STATE_WRITING    1
#define ESTORE_STATE_FAULT      2

#define ESTORE_ALL_PAGES_DIRTY  ((u16)((1U << ESTORE_PAGE_COUNT) - 1))             /* bits 0..12 */
#define ESTORE_EMPTY_BYTE       0xFF                   /* reserved bytes , and what an erased chip holds */

static const __flash c8 ESTORE_ADMIN_NAME_ARR[] = ESTORE_DEFAULT_ADMIN_NAME ;
static const __flash c8 ESTORE_ADMIN_PASS_ARR[] = ESTORE_DEFAULT_ADMIN_PASS ;

/* REQ-EEP-01 , D-5 : the whole map lives in RAM , reads never touch the chip */
static u8  Global_u8Image[ESTORE_IMAGE_SIZE] ;
static u16 Global_u16Dirty = 0 ;                       /* bit n = page n differs from the chip */
static u8  Global_u8State = ESTORE_STATE_IDLE ;
static u8  Global_u8Polls = 0 ;
static u8  Global_u8Status = ESTORE_OK ;
static u16 Global_u16PageWrites = 0 ;

/* text from flash into a fixed field ; the rest of the field stays as it is (0x00 padding) */
static void ESTORE_voidCopyDefault(u8 Copy_u8Address, const __flash c8 * Copy_pc8Text, u8 Copy_u8Size){
	u8 i ;
	for(i = 0 ; (i < Copy_u8Size) && (Copy_pc8Text[i] != '\0') ; i++){
		Global_u8Image[Copy_u8Address + i] = (u8)Copy_pc8Text[i] ;
	}
}

static void ESTORE_voidEnterFault(){
	Global_u8State = ESTORE_STATE_FAULT ;
	Global_u8Status = ESTORE_FAULT ;
	EVQ_voidPost(EVQ_STORAGE_FAULT,0);
}

/* Factory defaults into the RAM image (docs/eeprom_map.md Section 4) */
static void ESTORE_voidFillDefaults(){
	u8 i ;
	/*1. pages 0 and 1 : reserved bytes are 0xFF */
	for(i = 0 ; i < ESTORE_ADDR_ADMIN ; i++){
		Global_u8Image[i] = ESTORE_EMPTY_BYTE ;
	}
	/*2. all 11 account records empty (0x00) */
	for(i = ESTORE_ADDR_ADMIN ; i < ESTORE_IMAGE_SIZE ; i++){
		Global_u8Image[i] = 0x00 ;
	}
	/*3. header , heater set temperature , default admin */
	Global_u8Image[ESTORE_ADDR_MAGIC] = ESTORE_MAGIC_VALUE ;
	Global_u8Image[ESTORE_ADDR_VERSION] = ESTORE_VERSION_VALUE ;
	Global_u8Image[ESTORE_ADDR_HEATER_SET] = ESTORE_DEFAULT_HEATER_SET ;            /* REQ-HTR-03 */
	ESTORE_voidCopyDefault(ESTORE_ADDR_ADMIN + USERDB_NAME_OFFSET,ESTORE_ADMIN_NAME_ARR,USERDB_NAME_SIZE);   /* REQ-SEC-09 */
	ESTORE_voidCopyDefault(ESTORE_ADDR_ADMIN + USERDB_PASS_OFFSET,ESTORE_ADMIN_PASS_ARR,USERDB_PASS_SIZE);
}

/* Boot only , before the scheduler starts : one blocking read of 208 bytes (about 19 ms) */
void ESTORE_voidInit(){
	Global_u16Dirty = 0 ;
	Global_u8State = ESTORE_STATE_IDLE ;
	Global_u8Polls = 0 ;
	Global_u8Status = ESTORE_OK ;
	Global_u16PageWrites = 0 ;
	EXT_EEPROM_voidInit();
	if(EXT_EEPROM_u8ReadBlock(0,Global_u8Image,ESTORE_IMAGE_SIZE) != EXT_EEPROM_OK){
		/*1. the chip does not answer : work from defaults in RAM , write nothing */
		ESTORE_voidFillDefaults();
		Global_u8State = ESTORE_STATE_FAULT ;
		Global_u8Status = ESTORE_FAULT ;
	}else if((Global_u8Image[ESTORE_ADDR_MAGIC] != ESTORE_MAGIC_VALUE) || (Global_u8Image[ESTORE_ADDR_VERSION] != ESTORE_VERSION_VALUE)){
		/*2. blank or foreign chip : first boot (REQ-SEC-09 , REQ-EEP-02) */
		ESTORE_voidLoadDefaults();
		Global_u8Status = ESTORE_FIRST_BOOT ;
	}else{
		/*3. valid data : the image is the chip (REQ-SEC-04 , REQ-HTR-04) */
	}
}

/* REQ-EEP-03 : every 10 ms ONE small step , never a wait.
 * start one page write (about 1.7 ms on the bus) or make one ACK poll (about 0.12 ms) */
void ESTORE_voidUpdate(){
	u8 Local_u8Page ;
	switch(Global_u8State){
	case ESTORE_STATE_IDLE:
		if(Global_u16Dirty != 0){
			/*1. highest dirty page first : on first boot the magic page 0 is written last */
			Local_u8Page = ESTORE_PAGE_COUNT - 1 ;
			while(GET_BIT(Global_u16Dirty,Local_u8Page) == 0){
				Local_u8Page--;
			}
			/*2. clear the bit BEFORE the write : a byte changed during the write cycle marks the page again */
			Global_u16Dirty &= ~((u16)1 << Local_u8Page) ;
			Global_u8Polls = 0 ;
			if(EXT_EEPROM_u8WritePage((u16)Local_u8Page * ESTORE_PAGE_SIZE,&Global_u8Image[Local_u8Page * ESTORE_PAGE_SIZE],ESTORE_PAGE_SIZE) == EXT_EEPROM_OK){
				Global_u16PageWrites++;
				Global_u8State = ESTORE_STATE_WRITING ;
			}else{
				ESTORE_voidEnterFault();
			}
		}
		break ;
	case ESTORE_STATE_WRITING:
		/* ACK polling : the chip answers its address again when the write cycle is over */
		if(EXT_EEPROM_u8IsReady() == 1){
			Global_u8State = ESTORE_STATE_IDLE ;
		}else{
			Global_u8Polls++;
			if(Global_u8Polls >= ESTORE_POLL_LIMIT){
				/* 50 ms without an answer : the chip is gone */
				ESTORE_voidEnterFault();
			}
		}
		break ;
	default:
		/* ESTORE_STATE_FAULT : no more writes , the RAM image keeps the system running */
		break ;
	}
}

u8 ESTORE_u8ReadByte(u8 Copy_u8Address){
	if(Copy_u8Address < ESTORE_IMAGE_SIZE){
		return Global_u8Image[Copy_u8Address] ;
	}else{
		//error
		return ESTORE_EMPTY_BYTE ;
	}
}

void ESTORE_voidReadBlock(u8 Copy_u8Address, u8 * Copy_pu8Data, u8 Copy_u8Length){
	u8 i ;
	for(i = 0 ; i < Copy_u8Length ; i++){
		Copy_pu8Data[i] = ESTORE_u8ReadByte(Copy_u8Address + i);
	}
}

void ESTORE_voidWriteByte(u8 Copy_u8Address, u8 Copy_u8Data){
	if(Copy_u8Address < ESTORE_IMAGE_SIZE){
		/* an unchanged byte costs no write cycle (endurance : 1 000 000 writes per page) */
		if(Global_u8Image[Copy_u8Address] != Copy_u8Data){
			Global_u8Image[Copy_u8Address] = Copy_u8Data ;
			Global_u16Dirty |= ((u16)1 << (Copy_u8Address / ESTORE_PAGE_SIZE)) ;
		}
	}else{
		//error
	}
}

void ESTORE_voidWriteBlock(u8 Copy_u8Address, const u8 * Copy_pu8Data, u8 Copy_u8Length){
	u8 i ;
	for(i = 0 ; i < Copy_u8Length ; i++){
		ESTORE_voidWriteByte(Copy_u8Address + i,Copy_pu8Data[i]);
	}
}

void ESTORE_voidLoadDefaults(){
	ESTORE_voidFillDefaults();
	/* every page goes to the chip , page 12 first and the magic page 0 last :
	 * a power failure in between leaves the magic missing , so the next boot starts over */
	Global_u16Dirty = ESTORE_ALL_PAGES_DIRTY ;
}

u8 ESTORE_u8IsBusy(){
	if(Global_u8State == ESTORE_STATE_FAULT){
		/* nothing will ever be written : nobody must wait for it */
		return 0 ;
	}
	if((Global_u8State == ESTORE_STATE_WRITING) || (Global_u16Dirty != 0)){
		return 1 ;
	}
	return 0 ;
}

u8 ESTORE_u8GetStatus(){
	return Global_u8Status ;
}

u16 ESTORE_u16GetPageWrites(){
	return Global_u16PageWrites ;
}
