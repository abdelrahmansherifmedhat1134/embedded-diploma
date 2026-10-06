/*
 * PCF8574.c
 *
 *  Created on: Sep 28, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../../Service/std_types.h"
#include "../../MCAL/TWI/TWI.h"
#include "PCF8574.h"

void PCF8574_voidInit(){
	/* the I2C bus is shared (LCD , lamps , EEPROM) , init twice is harmless */
	TWI_voidMasterInit();
}

u8 PCF8574_u8WritePort(u8 Copy_u8Address, u8 Copy_u8Value){
	/* one data byte sets the 8 quasi-bidirectional pins P0..P7 */
	return TWI_u8MasterTransmit(Copy_u8Address, &Copy_u8Value, 1);
}

u8 PCF8574_u8ReadPort(u8 Copy_u8Address, u8 * Copy_pu8Value){
	/* a pin used as input must be written HIGH before reading it */
	return TWI_u8MasterReceive(Copy_u8Address, Copy_pu8Value, 1);
}

u8 PCF8574_u8WriteBytes(u8 Copy_u8Address, const u8 * Copy_pu8Data, u8 Copy_u8Length){
	/* used by CLCD : one LCD byte = 4 port values in one transaction */
	return TWI_u8MasterTransmit(Copy_u8Address, Copy_pu8Data, Copy_u8Length);
}
