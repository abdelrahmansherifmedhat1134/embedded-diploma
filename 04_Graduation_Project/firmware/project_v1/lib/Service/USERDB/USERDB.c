/*
 * USERDB.c
 *
 *  Created on: Oct 3, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../std_types.h"
#include "../ESTORE/ESTORE.h"
#include "../ESTORE/ESTORE_cfg.h"
#include "USERDB.h"
#include "USERDB_cfg.h"

#define USERDB_NO_SLOT        0xFF
#define USERDB_CHAR_FIRST     0x21                     /* printable , no space : '!' .. '~' */
#define USERDB_CHAR_LAST      0x7E
#define USERDB_TOO_LONG       (USERDB_NAME_SIZE + 1)   /* "length" of a text longer than a field */

static const __flash c8 USERDB_ADMIN_NAME_ARR[] = ESTORE_DEFAULT_ADMIN_NAME ;
static const __flash c8 USERDB_ADMIN_PASS_ARR[] = ESTORE_DEFAULT_ADMIN_PASS ;

/* REQ-SEC-07 : accounts are read-only until the admin logs in */
static u8 Global_u8WriteAccess = 0 ;

/* length of a C string , counted only up to 9 (anything longer than a field is "too long") */
static u8 USERDB_u8TextLength(const c8 * Copy_pc8Text){
	u8 Local_u8Length = 0 ;
	while((Copy_pc8Text[Local_u8Length] != '\0') && (Local_u8Length < USERDB_TOO_LONG)){
		Local_u8Length++;
	}
	return Local_u8Length ;
}

/* 1 = the length is inside min..max and every character is allowed */
static u8 USERDB_u8IsTextValid(const c8 * Copy_pc8Text, u8 Copy_u8Min, u8 Copy_u8Max, u8 Copy_u8DigitsOnly){
	u8 Local_u8Length = USERDB_u8TextLength(Copy_pc8Text);
	u8 i ;
	if((Local_u8Length < Copy_u8Min) || (Local_u8Length > Copy_u8Max)){
		return 0 ;
	}
	for(i = 0 ; i < Local_u8Length ; i++){
		if(Copy_u8DigitsOnly == 1){
			/* REQ-SEC-02 : the keypad has digits only */
			if((Copy_pc8Text[i] < '0') || (Copy_pc8Text[i] > '9')){
				return 0 ;
			}
		}else{
			if((Copy_pc8Text[i] < USERDB_CHAR_FIRST) || (Copy_pc8Text[i] > USERDB_CHAR_LAST)){
				return 0 ;
			}
		}
	}
	return 1 ;
}

/* Compares one 8-byte field of the image with a C string.
 * The field is padded with 0x00 and has NO terminator when it is 8 characters long ,
 * so all 8 positions are compared (text shorter than 8 counts as 0x00 from its end on). */
static u8 USERDB_u8IsFieldEqual(u8 Copy_u8Address, const c8 * Copy_pc8Text){
	u8 Local_u8Length = USERDB_u8TextLength(Copy_pc8Text);
	u8 Local_u8Char ;
	u8 i ;
	if(Local_u8Length > USERDB_NAME_SIZE){
		return 0 ;
	}
	for(i = 0 ; i < USERDB_NAME_SIZE ; i++){
		Local_u8Char = (i < Local_u8Length) ? (u8)Copy_pc8Text[i] : 0x00 ;
		if(ESTORE_u8ReadByte(Copy_u8Address + i) != Local_u8Char){
			return 0 ;
		}
	}
	return 1 ;
}

/* writes name + password as ONE record = one EEPROM page (D-6 : never half done) */
static void USERDB_voidWriteRecord(u8 Copy_u8Address, const c8 * Copy_pc8Name, const c8 * Copy_pc8Pass){
	u8 Local_u8Record[USERDB_RECORD_SIZE] ;
	u8 Local_u8NameLength = USERDB_u8TextLength(Copy_pc8Name);
	u8 Local_u8PassLength = USERDB_u8TextLength(Copy_pc8Pass);
	u8 i ;
	for(i = 0 ; i < USERDB_NAME_SIZE ; i++){
		Local_u8Record[USERDB_NAME_OFFSET + i] = (i < Local_u8NameLength) ? (u8)Copy_pc8Name[i] : 0x00 ;
		Local_u8Record[USERDB_PASS_OFFSET + i] = (i < Local_u8PassLength) ? (u8)Copy_pc8Pass[i] : 0x00 ;
	}
	ESTORE_voidWriteBlock(Copy_u8Address,Local_u8Record,USERDB_RECORD_SIZE);
}

static u8 USERDB_u8SlotCount(u8 Copy_u8List){
	if(Copy_u8List == USERDB_LIST_REMOTE){
		return USERDB_REMOTE_SLOTS ;
	}else if(Copy_u8List == USERDB_LIST_KEYPAD){
		return USERDB_KEYPAD_SLOTS ;
	}
	return 0 ;                                         /* unknown list : no slots , nothing matches */
}

static u8 USERDB_u8SlotAddress(u8 Copy_u8List, u8 Copy_u8Slot){
	if(Copy_u8List == USERDB_LIST_REMOTE){
		return ESTORE_ADDR_REMOTE_USERS + (Copy_u8Slot * USERDB_RECORD_SIZE) ;
	}
	return ESTORE_ADDR_KEYPAD_USERS + (Copy_u8Slot * USERDB_RECORD_SIZE) ;
}

/* ASSUMPTION: D-6 , no valid flag : a first name byte of 0x00 or 0xFF means "empty slot" */
static u8 USERDB_u8IsSlotUsed(u8 Copy_u8Address){
	u8 Local_u8First = ESTORE_u8ReadByte(Copy_u8Address + USERDB_NAME_OFFSET);
	if((Local_u8First == 0x00) || (Local_u8First == 0xFF)){
		return 0 ;
	}
	return 1 ;
}

/* slot of a user name inside one list , USERDB_NO_SLOT when it is not there */
static u8 USERDB_u8FindUser(u8 Copy_u8List, const c8 * Copy_pc8Name){
	u8 Local_u8Count = USERDB_u8SlotCount(Copy_u8List);
	u8 Local_u8Address ;
	u8 i ;
	for(i = 0 ; i < Local_u8Count ; i++){
		Local_u8Address = USERDB_u8SlotAddress(Copy_u8List,i);
		if(USERDB_u8IsSlotUsed(Local_u8Address) && USERDB_u8IsFieldEqual(Local_u8Address + USERDB_NAME_OFFSET,Copy_pc8Name)){
			return i ;
		}
	}
	return USERDB_NO_SLOT ;
}

void USERDB_voidInit(){
	u8 Local_u8First = ESTORE_u8ReadByte(ESTORE_ADDR_ADMIN + USERDB_NAME_OFFSET);
	c8 Local_c8Name[USERDB_NAME_SIZE + 1] ;
	c8 Local_c8Pass[USERDB_PASS_SIZE + 1] ;
	u8 i ;
	Global_u8WriteAccess = 0 ;
	/* REQ-SEC-09 : there must always be an admin , or nobody could ever log in.
	 * (0x00 and 0xFF are outside the printable range , so one test covers "empty" too) */
	if((Local_u8First < USERDB_CHAR_FIRST) || (Local_u8First > USERDB_CHAR_LAST)){
		for(i = 0 ; i <= USERDB_NAME_SIZE ; i++){
			Local_c8Name[i] = '\0' ;
			Local_c8Pass[i] = '\0' ;
		}
		for(i = 0 ; (i < USERDB_NAME_SIZE) && (USERDB_ADMIN_NAME_ARR[i] != '\0') ; i++){
			Local_c8Name[i] = USERDB_ADMIN_NAME_ARR[i] ;
		}
		for(i = 0 ; (i < USERDB_PASS_SIZE) && (USERDB_ADMIN_PASS_ARR[i] != '\0') ; i++){
			Local_c8Pass[i] = USERDB_ADMIN_PASS_ARR[i] ;
		}
		/* a repair , not a user action : it does not go through the write gate */
		USERDB_voidWriteRecord(ESTORE_ADDR_ADMIN,Local_c8Name,Local_c8Pass);
	}
}

/* REQ-SEC-01 */
u8 USERDB_u8CheckAdmin(const c8 * Copy_pc8Name, const c8 * Copy_pc8Pass){
	if(USERDB_u8IsFieldEqual(ESTORE_ADDR_ADMIN + USERDB_NAME_OFFSET,Copy_pc8Name)
			&& USERDB_u8IsFieldEqual(ESTORE_ADDR_ADMIN + USERDB_PASS_OFFSET,Copy_pc8Pass)){
		return 1 ;
	}
	return 0 ;
}

/* REQ-SEC-02 : each list is searched on its own */
u8 USERDB_u8CheckUser(u8 Copy_u8List, const c8 * Copy_pc8Name, const c8 * Copy_pc8Pass){
	/* only used slots are searched , so an empty name can never match an empty slot */
	u8 Local_u8Slot = USERDB_u8FindUser(Copy_u8List,Copy_pc8Name);
	if(Local_u8Slot == USERDB_NO_SLOT){
		return 0 ;
	}
	return USERDB_u8IsFieldEqual(USERDB_u8SlotAddress(Copy_u8List,Local_u8Slot) + USERDB_PASS_OFFSET,Copy_pc8Pass) ;
}

/* REQ-SEC-03 */
u8 USERDB_u8AddUser(u8 Copy_u8List, const c8 * Copy_pc8Name, const c8 * Copy_pc8Pass){
	u8 Local_u8Count = USERDB_u8SlotCount(Copy_u8List);
	u8 Local_u8DigitsOnly = (Copy_u8List == USERDB_LIST_KEYPAD) ? 1 : 0 ;
	u8 i ;
	/*1. REQ-SEC-07 : only the admin may change accounts */
	if(Global_u8WriteAccess == 0){
		return USERDB_ERR_READ_ONLY ;
	}
	/*2. length and characters */
	if((Local_u8Count == 0) || (USERDB_u8IsTextValid(Copy_pc8Name,USERDB_NAME_MIN,USERDB_NAME_MAX,Local_u8DigitsOnly) == 0)){
		return USERDB_ERR_BAD_NAME ;
	}
	if(USERDB_u8IsTextValid(Copy_pc8Pass,USERDB_PASS_MIN,USERDB_PASS_MAX,Local_u8DigitsOnly) == 0){
		return USERDB_ERR_BAD_PASS ;
	}
	/*3. unique inside its list ; a remote user may not take the admin's name either */
	if(USERDB_u8FindUser(Copy_u8List,Copy_pc8Name) != USERDB_NO_SLOT){
		return USERDB_ERR_EXISTS ;
	}
	if((Copy_u8List == USERDB_LIST_REMOTE) && USERDB_u8IsFieldEqual(ESTORE_ADDR_ADMIN + USERDB_NAME_OFFSET,Copy_pc8Name)){
		return USERDB_ERR_EXISTS ;
	}
	/*4. first empty slot */
	for(i = 0 ; i < Local_u8Count ; i++){
		if(USERDB_u8IsSlotUsed(USERDB_u8SlotAddress(Copy_u8List,i)) == 0){
			USERDB_voidWriteRecord(USERDB_u8SlotAddress(Copy_u8List,i),Copy_pc8Name,Copy_pc8Pass);   /* REQ-SEC-04 */
			return USERDB_OK ;
		}
	}
	return USERDB_ERR_FULL ;
}

/* REQ-SEC-03 */
u8 USERDB_u8RemoveUser(u8 Copy_u8List, const c8 * Copy_pc8Name){
	u8 Local_u8Slot ;
	if(Global_u8WriteAccess == 0){
		return USERDB_ERR_READ_ONLY ;
	}
	Local_u8Slot = USERDB_u8FindUser(Copy_u8List,Copy_pc8Name);
	if(Local_u8Slot == USERDB_NO_SLOT){
		return USERDB_ERR_NOT_FOUND ;
	}
	/* the whole record becomes 0x00 : name gone (slot empty) and no old password left behind */
	USERDB_voidWriteRecord(USERDB_u8SlotAddress(Copy_u8List,Local_u8Slot),"","");
	return USERDB_OK ;
}

u8 USERDB_u8GetUserName(u8 Copy_u8List, u8 Copy_u8Slot, c8 * Copy_pc8Name){
	u8 Local_u8Address ;
	u8 i ;
	Copy_pc8Name[0] = '\0' ;
	if(Copy_u8Slot >= USERDB_u8SlotCount(Copy_u8List)){
		return 0 ;
	}
	Local_u8Address = USERDB_u8SlotAddress(Copy_u8List,Copy_u8Slot);
	if(USERDB_u8IsSlotUsed(Local_u8Address) == 0){
		return 0 ;
	}
	/* the stored name has no terminator when it is 8 characters long : add one here */
	for(i = 0 ; i < USERDB_NAME_SIZE ; i++){
		Copy_pc8Name[i] = (c8)ESTORE_u8ReadByte(Local_u8Address + USERDB_NAME_OFFSET + i);
	}
	Copy_pc8Name[USERDB_NAME_SIZE] = '\0' ;
	return 1 ;
}

u8 USERDB_u8SetAdminPassword(const c8 * Copy_pc8Pass){
	u8 Local_u8Length ;
	u8 i ;
	if(Global_u8WriteAccess == 0){
		return USERDB_ERR_READ_ONLY ;
	}
	if(USERDB_u8IsTextValid(Copy_pc8Pass,USERDB_PASS_MIN,USERDB_PASS_MAX,0) == 0){
		return USERDB_ERR_BAD_PASS ;
	}
	Local_u8Length = USERDB_u8TextLength(Copy_pc8Pass);
	for(i = 0 ; i < USERDB_PASS_SIZE ; i++){
		ESTORE_voidWriteByte(ESTORE_ADDR_ADMIN + USERDB_PASS_OFFSET + i,(i < Local_u8Length) ? (u8)Copy_pc8Pass[i] : 0x00);
	}
	return USERDB_OK ;
}

void USERDB_voidSetWriteAccess(u8 Copy_u8State){
	if(Copy_u8State == 1){
		Global_u8WriteAccess = 1 ;
	}else{
		Global_u8WriteAccess = 0 ;
	}
}
