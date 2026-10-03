/*
 * EXT_EEPROM.c
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */
#include "../../Service/std_types.h"
#include "../../MCAL/TWI/TWI.h"
#include "EXT_EEPROM.h"
#include "EXT_EEPROM_cfg.h"

/* 24C08 : address bits 9:8 travel in the two low bits of the I2C slave address */
static u8 EXT_EEPROM_u8SlaveAddress(u16 Copy_u16Address){
	return EXT_EEPROM_I2C_ADDRESS | (u8)((Copy_u16Address >> 8) & 0b11) ;
}

void EXT_EEPROM_voidInit(){
	TWI_voidMasterInit();
}
u8 EXT_EEPROM_u8ReadBlock(u16 Copy_u16Address, u8 * Copy_pu8Data, u8 Copy_u8Length){
	u8 Local_u8Result = EXT_EEPROM_ERROR ;
	if((Copy_u8Length > 0) && (((u32)Copy_u16Address + Copy_u8Length) <= EXT_EEPROM_SIZE)){
		Local_u8Result = EXT_EEPROM_OK ;
		/* a read is split at 256-byte block borders : each block has its own I2C address */
		while((Copy_u8Length > 0) && (Local_u8Result == EXT_EEPROM_OK)){
			u8 Local_u8WordAddress = (u8)Copy_u16Address ;
			u16 Local_u16Room = EXT_EEPROM_BLOCK_SIZE - Local_u8WordAddress ;
			u8 Local_u8Chunk = (Copy_u8Length < Local_u16Room) ? Copy_u8Length : (u8)Local_u16Room ;
			/* write the word address , repeated START , read */
			if(TWI_u8MasterWriteRead(EXT_EEPROM_u8SlaveAddress(Copy_u16Address),&Local_u8WordAddress,1,Copy_pu8Data,Local_u8Chunk) != TWI_OK){
				Local_u8Result = EXT_EEPROM_ERROR ;
			}
			Copy_u16Address += Local_u8Chunk ;
			Copy_pu8Data += Local_u8Chunk ;
			Copy_u8Length -= Local_u8Chunk ;
		}
	}else{
		//error
	}
	return Local_u8Result ;
}
u8 EXT_EEPROM_u8WritePage(u16 Copy_u16Address, const u8 * Copy_pu8Data, u8 Copy_u8Length){
	u8 Local_u8Result = EXT_EEPROM_ERROR ;
	u8 Local_au8Frame[EXT_EEPROM_PAGE_SIZE + 1] ;
	/* the data must stay inside one 16-byte page , else the chip wraps around inside the page */
	if((Copy_u8Length > 0) && (Copy_u8Length <= EXT_EEPROM_PAGE_SIZE)
			&& (((Copy_u16Address % EXT_EEPROM_PAGE_SIZE) + Copy_u8Length) <= EXT_EEPROM_PAGE_SIZE)
			&& (Copy_u16Address < EXT_EEPROM_SIZE)){
		/* frame = word address , then the data bytes */
		Local_au8Frame[0] = (u8)Copy_u16Address ;
		for(u8 i = 0 ; i < Copy_u8Length ; i++){
			Local_au8Frame[i + 1] = Copy_pu8Data[i] ;
		}
		if(TWI_u8MasterTransmit(EXT_EEPROM_u8SlaveAddress(Copy_u16Address),Local_au8Frame,Copy_u8Length + 1) == TWI_OK){
			Local_u8Result = EXT_EEPROM_OK ;
		}
	}else{
		//error
	}
	return Local_u8Result ;
}
u8 EXT_EEPROM_u8IsReady(){
	/* the chip does not acknowledge its address while it is inside a write cycle (ACK polling) */
	return (TWI_u8ProbeAddress(EXT_EEPROM_I2C_ADDRESS) == TWI_OK) ;
}
