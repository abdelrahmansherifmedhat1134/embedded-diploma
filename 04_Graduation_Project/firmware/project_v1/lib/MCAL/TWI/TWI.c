/*
 * TWI.c
 *
 *  Created on: Sep 28, 2026
 */
#include "../../Service/std_types.h"
#include "../../Service/Bit_math.h"
#include "../reg_def.h"
#include "TWI.h"
#include "TWI_cfg.h"

/* Write TWCR (TWINT + TWEN + extra flags) then wait until TWINT is set again.
 * Waits at most TWI_TIMEOUT_LOOPS : a stuck bus (SDA or SCL held low) returns
 * TWI_ERR_TIMEOUT and the bus is released instead of hanging the loop. */
static u8 TWI_u8SendCommand(u8 Copy_u8Flags){
	u8 Local_u8Error = TWI_OK ;
	u16 Local_u16Timeout = TWI_TIMEOUT_LOOPS ;
	TWCR = (1<<TWCR_TWINT) | (1<<TWCR_TWEN) | Copy_u8Flags ;
	/* wait on TWINT (about 90 us per byte at 100 kHz) */
	while(GET_BIT(TWCR,TWCR_TWINT) == 0){
		if(Local_u16Timeout == 0){
			/*1. release the bus */
			TWCR = (1<<TWCR_TWINT) | (1<<TWCR_TWEN) | (1<<TWCR_TWSTO);
			/*2. switch the unit off and on : clears any half-finished transfer */
			TWCR = 0 ;
			TWCR = (1<<TWCR_TWEN);
			Local_u8Error = TWI_ERR_TIMEOUT ;
			break ;
		}
		Local_u16Timeout-- ;
	}
	return Local_u8Error ;
}

void TWI_voidMasterInit(){
	/*1. Set prescaler bits */
	TWSR = (TWI_PRESCALER & 0b00000011);
	/*2. Set bit rate */
	TWBR = TWI_TWBR_VALUE ;
	/*3. Enable TWI */
	TWCR = (1<<TWCR_TWEN);
}

void TWI_voidDisable(){
	TWCR = 0 ;
	TWSR = 0 ;
	TWBR = 0 ;
}

u8 TWI_u8GetStatus(){
	return (TWSR & TWI_STATUS_MASK);
}

u8 TWI_u8SendStartCondition(){
	u8 Local_u8Error = TWI_u8SendCommand(1<<TWCR_TWSTA);
	if((Local_u8Error == TWI_OK) && (TWI_u8GetStatus() != TWI_STATUS_START)){
		Local_u8Error = TWI_ERR_START ;
	}
	return Local_u8Error ;
}

u8 TWI_u8SendRepeatedStart(){
	u8 Local_u8Error = TWI_u8SendCommand(1<<TWCR_TWSTA);
	u8 Local_u8Status ;
	if(Local_u8Error == TWI_OK){
		Local_u8Status = TWI_u8GetStatus();
		if((Local_u8Status != TWI_STATUS_REPEATED_START) && (Local_u8Status != TWI_STATUS_START)){
			Local_u8Error = TWI_ERR_START ;
		}
	}
	return Local_u8Error ;
}

void TWI_voidSendStopCondition(){
	/* STOP does not set TWINT , no wait */
	TWCR = (1<<TWCR_TWINT) | (1<<TWCR_TWEN) | (1<<TWCR_TWSTO);
}

u8 TWI_u8SendSlaveAddressWrite(u8 Copy_u8SlaveAddress){
	u8 Local_u8Error ;
	/* SLA+W : 7-bit address shifted left , R/W bit = 0 */
	TWDR = (u8)(Copy_u8SlaveAddress<<1);
	Local_u8Error = TWI_u8SendCommand(0);
	if(Local_u8Error != TWI_OK){
		return Local_u8Error ;
	}
	switch(TWI_u8GetStatus()){
	case TWI_STATUS_MT_SLA_ACK : Local_u8Error = TWI_OK ; break ;
	case TWI_STATUS_MT_SLA_NACK: Local_u8Error = TWI_ERR_SLA_NACK ; break ;
	case TWI_STATUS_ARB_LOST   : Local_u8Error = TWI_ERR_ARB_LOST ; break ;
	default                    : Local_u8Error = TWI_ERR_BUS ; break ;
	}
	return Local_u8Error ;
}

u8 TWI_u8SendSlaveAddressRead(u8 Copy_u8SlaveAddress){
	u8 Local_u8Error ;
	/* SLA+R : 7-bit address shifted left , R/W bit = 1 */
	TWDR = (u8)((Copy_u8SlaveAddress<<1) | 1);
	Local_u8Error = TWI_u8SendCommand(0);
	if(Local_u8Error != TWI_OK){
		return Local_u8Error ;
	}
	switch(TWI_u8GetStatus()){
	case TWI_STATUS_MR_SLA_ACK : Local_u8Error = TWI_OK ; break ;
	case TWI_STATUS_MR_SLA_NACK: Local_u8Error = TWI_ERR_SLA_NACK ; break ;
	case TWI_STATUS_ARB_LOST   : Local_u8Error = TWI_ERR_ARB_LOST ; break ;
	default                    : Local_u8Error = TWI_ERR_BUS ; break ;
	}
	return Local_u8Error ;
}

u8 TWI_u8MasterWriteDataByte(u8 Copy_u8Data){
	u8 Local_u8Error ;
	TWDR = Copy_u8Data ;
	Local_u8Error = TWI_u8SendCommand(0);
	if(Local_u8Error != TWI_OK){
		return Local_u8Error ;
	}
	switch(TWI_u8GetStatus()){
	case TWI_STATUS_MT_DATA_ACK : Local_u8Error = TWI_OK ; break ;
	case TWI_STATUS_MT_DATA_NACK: Local_u8Error = TWI_ERR_DATA_NACK ; break ;
	case TWI_STATUS_ARB_LOST    : Local_u8Error = TWI_ERR_ARB_LOST ; break ;
	default                     : Local_u8Error = TWI_ERR_BUS ; break ;
	}
	return Local_u8Error ;
}

u8 TWI_u8MasterReadDataByteAck(u8 * Copy_pu8Data){
	u8 Local_u8Error = TWI_OK ;
	/* TWEA = 1 --> send ACK , slave continues sending */
	Local_u8Error = TWI_u8SendCommand(1<<TWCR_TWEA);
	if(Local_u8Error != TWI_OK){
		return Local_u8Error ;
	}
	if(TWI_u8GetStatus() == TWI_STATUS_MR_DATA_ACK){
		*Copy_pu8Data = TWDR ;
	}else{
		Local_u8Error = TWI_ERR_BUS ;
	}
	return Local_u8Error ;
}

u8 TWI_u8MasterReadDataByteNack(u8 * Copy_pu8Data){
	u8 Local_u8Error = TWI_OK ;
	/* TWEA = 0 --> send NACK , this is the last byte */
	Local_u8Error = TWI_u8SendCommand(0);
	if(Local_u8Error != TWI_OK){
		return Local_u8Error ;
	}
	if(TWI_u8GetStatus() == TWI_STATUS_MR_DATA_NACK){
		*Copy_pu8Data = TWDR ;
	}else{
		Local_u8Error = TWI_ERR_BUS ;
	}
	return Local_u8Error ;
}

u8 TWI_u8ProbeAddress(u8 Copy_u8SlaveAddress){
	u8 Local_u8Error = TWI_u8SendStartCondition();
	if(Local_u8Error == TWI_OK){
		Local_u8Error = TWI_u8SendSlaveAddressWrite(Copy_u8SlaveAddress);
	}
	TWI_voidSendStopCondition();
	return Local_u8Error ;
}

/* read Copy_u8Length bytes : ACK all bytes except the last one */
static u8 TWI_u8ReadBytes(u8 * Copy_pu8Data, u8 Copy_u8Length){
	u8 Local_u8Error = TWI_OK ;
	for(u8 i = 0 ; (i < Copy_u8Length) && (Local_u8Error == TWI_OK) ; i++){
		if(i < (Copy_u8Length - 1)){
			Local_u8Error = TWI_u8MasterReadDataByteAck(&Copy_pu8Data[i]);
		}else{
			Local_u8Error = TWI_u8MasterReadDataByteNack(&Copy_pu8Data[i]);
		}
	}
	return Local_u8Error ;
}

u8 TWI_u8MasterTransmit(u8 Copy_u8SlaveAddress, const u8 * Copy_pu8Data, u8 Copy_u8Length){
	u8 Local_u8Error = TWI_u8SendStartCondition();
	if(Local_u8Error == TWI_OK){
		Local_u8Error = TWI_u8SendSlaveAddressWrite(Copy_u8SlaveAddress);
	}
	for(u8 i = 0 ; (i < Copy_u8Length) && (Local_u8Error == TWI_OK) ; i++){
		Local_u8Error = TWI_u8MasterWriteDataByte(Copy_pu8Data[i]);
	}
	TWI_voidSendStopCondition();
	return Local_u8Error ;
}

u8 TWI_u8MasterReceive(u8 Copy_u8SlaveAddress, u8 * Copy_pu8Data, u8 Copy_u8Length){
	u8 Local_u8Error = TWI_OK ;
	if(Copy_u8Length == 0){
		return TWI_OK ;
	}
	Local_u8Error = TWI_u8SendStartCondition();
	if(Local_u8Error == TWI_OK){
		Local_u8Error = TWI_u8SendSlaveAddressRead(Copy_u8SlaveAddress);
	}
	if(Local_u8Error == TWI_OK){
		Local_u8Error = TWI_u8ReadBytes(Copy_pu8Data, Copy_u8Length);
	}
	TWI_voidSendStopCondition();
	return Local_u8Error ;
}

u8 TWI_u8MasterWriteRead(u8 Copy_u8SlaveAddress,
                         const u8 * Copy_pu8WriteData, u8 Copy_u8WriteLength,
                         u8 * Copy_pu8ReadData, u8 Copy_u8ReadLength){
	/*1. Write phase */
	u8 Local_u8Error = TWI_u8SendStartCondition();
	if(Local_u8Error == TWI_OK){
		Local_u8Error = TWI_u8SendSlaveAddressWrite(Copy_u8SlaveAddress);
	}
	for(u8 i = 0 ; (i < Copy_u8WriteLength) && (Local_u8Error == TWI_OK) ; i++){
		Local_u8Error = TWI_u8MasterWriteDataByte(Copy_pu8WriteData[i]);
	}
	/*2. Repeated START then read phase */
	if((Local_u8Error == TWI_OK) && (Copy_u8ReadLength > 0)){
		Local_u8Error = TWI_u8SendRepeatedStart();
		if(Local_u8Error == TWI_OK){
			Local_u8Error = TWI_u8SendSlaveAddressRead(Copy_u8SlaveAddress);
		}
		if(Local_u8Error == TWI_OK){
			Local_u8Error = TWI_u8ReadBytes(Copy_pu8ReadData, Copy_u8ReadLength);
		}
	}
	TWI_voidSendStopCondition();
	return Local_u8Error ;
}
