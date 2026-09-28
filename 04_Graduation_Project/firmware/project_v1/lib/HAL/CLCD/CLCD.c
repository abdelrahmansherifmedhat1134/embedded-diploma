/*
 * CLCD.c
 *
 *  Created on: Dec 29, 2025
 *      Author: eslam
 */
#include "util/delay.h"
#include "../../Service/std_types.h"
#include "../../Service/Bit_math.h"
#include "../../MCAL/DIO/DIO.h"
#include "CLCD.h"
#include "CLCD_cfg.h"
#if CLCD_MODE == CLCD_I2C_MODE
#include "../PCF8574/PCF8574.h"

/* Send one nibble through the PCF8574 : E high then E low.
 * Each I2C write takes ~0.3 ms at 100 kHz , longer than the LCD needs
 * for E pulse (450 ns) and for a normal command (37 us). */
static void I2C_voidSendNibble(u8 Copy_u8Nibble, u8 Copy_u8RS){
	u8 Local_u8Data = 0 ;
	/* back light always ON , RW always LOW (write only) */
	SET_BIT(Local_u8Data,CLCD_I2C_BL_BIT);
	if(Copy_u8RS == DIO_PIN_HIGH){
		SET_BIT(Local_u8Data,CLCD_I2C_RS_BIT);
	}
	if(GET_BIT(Copy_u8Nibble,0)){ SET_BIT(Local_u8Data,CLCD_I2C_D4_BIT); }
	if(GET_BIT(Copy_u8Nibble,1)){ SET_BIT(Local_u8Data,CLCD_I2C_D5_BIT); }
	if(GET_BIT(Copy_u8Nibble,2)){ SET_BIT(Local_u8Data,CLCD_I2C_D6_BIT); }
	if(GET_BIT(Copy_u8Nibble,3)){ SET_BIT(Local_u8Data,CLCD_I2C_D7_BIT); }

	PCF8574_u8WritePort(CLCD_I2C_ADDRESS,Local_u8Data | (1<<CLCD_I2C_E_BIT));
	PCF8574_u8WritePort(CLCD_I2C_ADDRESS,Local_u8Data);
}
static void I2C_voidSendByte(u8 Copy_u8Byte, u8 Copy_u8RS){
	I2C_voidSendNibble(Copy_u8Byte>>4,Copy_u8RS);
	I2C_voidSendNibble(Copy_u8Byte,Copy_u8RS);
}
#else
static void SetHalfPort(u8 Copy_u8Data){
	DIO_voidSetPinValue(CLCD_DATA_PORT,CLCD_DATA_PIN_0,GET_BIT(Copy_u8Data,0));
	DIO_voidSetPinValue(CLCD_DATA_PORT,CLCD_DATA_PIN_1,GET_BIT(Copy_u8Data,1));
	DIO_voidSetPinValue(CLCD_DATA_PORT,CLCD_DATA_PIN_2,GET_BIT(Copy_u8Data,2));
	DIO_voidSetPinValue(CLCD_DATA_PORT,CLCD_DATA_PIN_3,GET_BIT(Copy_u8Data,3));
}
static void SendEnablePulse(){
	DIO_voidSetPinValue(CLCD_CTRL_PORT,CLCD_E_PIN,DIO_PIN_HIGH);
	_delay_ms(10);
	DIO_voidSetPinValue(CLCD_CTRL_PORT,CLCD_E_PIN,DIO_PIN_LOW);
}
#endif
void CLCD_voidInit(){
#if CLCD_MODE == CLCD_I2C_MODE
	/*1. Init I2C bus */
	PCF8574_voidInit();
	/*2. delay 40 ms (power up , init only) */
	_delay_ms(40);
	/*3. Reset sequence then switch to 4 bit : 0x3 , 0x3 , 0x3 , 0x2 */
	I2C_voidSendNibble(0b0011,DIO_PIN_LOW);
	_delay_ms(5);
	I2C_voidSendNibble(0b0011,DIO_PIN_LOW);
	_delay_ms(1);
	I2C_voidSendNibble(0b0011,DIO_PIN_LOW);
	I2C_voidSendNibble(0b0010,DIO_PIN_LOW);
	/*4. Function Set : 4 bit , 2 lines , 5x8 */
	CLCD_voidSendCommand(0b00101000);
#else
	/**Data port Output >*/
	DIO_voidSetPortDirection(CLCD_DATA_PORT,DIO_PORT_OUTPUT);
	/**RS Output >*/
	DIO_voidSetPinDirection(CLCD_CTRL_PORT,CLCD_RS_PIN,DIO_PIN_OUTPUT);
	/**RW Output >*/
	DIO_voidSetPinDirection(CLCD_CTRL_PORT,CLCD_RW_PIN,DIO_PIN_OUTPUT);
	/**E  Output >*/
	DIO_voidSetPinDirection(CLCD_CTRL_PORT,CLCD_E_PIN, DIO_PIN_OUTPUT);


	/*delay 40 ms*/
	_delay_ms(40);
#if CLCD_MODE == CLCD_8_BIT_MODE
	/*Send Command Function Set */
	CLCD_voidSendCommand(0b00111000);
#elif CLCD_MODE == CLCD_4_BIT_MODE
	SetHalfPort(0b0010);
	SendEnablePulse();
	SetHalfPort(0b0010);
	SendEnablePulse();
	SetHalfPort(0b1000);
	SendEnablePulse();
#else

#endif
#endif
	/*Send Command on / off control  */
	CLCD_voidSendCommand(0b00001111);
	/*Send Command Clear  */
	CLCD_voidSendCommand(1);
}
void CLCD_voidSendCommand(u8 Copy_u8Cmd){
#if CLCD_MODE == CLCD_I2C_MODE
	/* RS = LOW for command */
	I2C_voidSendByte(Copy_u8Cmd,DIO_PIN_LOW);
	/* Clear (1) and Return Home (2) need 1.52 ms */
	if(Copy_u8Cmd <= 2){
		_delay_ms(2);
	}
#else
	/*2. RS=========> LOW */
	DIO_voidSetPinValue(CLCD_CTRL_PORT,CLCD_RS_PIN,DIO_PIN_LOW);
	/*2. RW=========> LOW */
	DIO_voidSetPinValue(CLCD_CTRL_PORT,CLCD_RW_PIN,DIO_PIN_LOW);


#if CLCD_MODE == CLCD_8_BIT_MODE
	/*1. Data on Data Port */
	DIO_voidSetPortValue(CLCD_DATA_PORT,Copy_u8Cmd);
	/*2. Send enable Pulse */
	DIO_voidSetPinValue(CLCD_CTRL_PORT,CLCD_E_PIN,DIO_PIN_HIGH);
	_delay_ms(10);
	DIO_voidSetPinValue(CLCD_CTRL_PORT,CLCD_E_PIN,DIO_PIN_LOW);
#elif CLCD_MODE == CLCD_4_BIT_MODE
	SetHalfPort(Copy_u8Cmd>>4);
	SendEnablePulse();

	SetHalfPort(Copy_u8Cmd);
	SendEnablePulse();
#else

#endif
#endif
}
void CLCD_voidSendData	 (u8 Copy_u8Data){
#if CLCD_MODE == CLCD_I2C_MODE
	/* RS = HIGH for data */
	I2C_voidSendByte(Copy_u8Data,DIO_PIN_HIGH);
#else

	/*2. RS=========> High */
	DIO_voidSetPinValue(CLCD_CTRL_PORT,CLCD_RS_PIN,DIO_PIN_HIGH);
	/*2. RW=========> LOW */
	DIO_voidSetPinValue(CLCD_CTRL_PORT,CLCD_RW_PIN,DIO_PIN_LOW);
	/*2. Send enable Pulse */
#if CLCD_MODE == CLCD_8_BIT_MODE
	/*1. Data on Data Port */
	DIO_voidSetPortValue(CLCD_DATA_PORT,Copy_u8Data);

	DIO_voidSetPinValue(CLCD_CTRL_PORT,CLCD_E_PIN,DIO_PIN_HIGH);
	_delay_ms(10);
	DIO_voidSetPinValue(CLCD_CTRL_PORT,CLCD_E_PIN,DIO_PIN_LOW);
#elif CLCD_MODE == CLCD_4_BIT_MODE
	SetHalfPort(Copy_u8Data>>4);
	SendEnablePulse();

	SetHalfPort(Copy_u8Data);
	SendEnablePulse();

#else

#endif
#endif
}
void CLCD_voidClearDisp(){
	CLCD_voidSendCommand(1);

}

void CLCD_voidSetCursorPosition(u8 Copy_u8x, u8 Copy_u8y){
	u8 Address = Copy_u8x+(Copy_u8y*0x40);
	SET_BIT(Address,7);
	CLCD_voidSendCommand(Address);
}


void CLCD_voidSendString(char * str){
	while(*str != '\0'){
		CLCD_voidSendData(*str++);
	}
}

void CLCD_voidCreatSpecialChar(u8 Copy_u8Index,u8 * Copypu8Array){
	/* send command wrtie on CGRam */
	u8 Local_u8Address = Copy_u8Index * 8 ;
	SET_BIT(Local_u8Address,6);
	CLCD_voidSendCommand(Local_u8Address);
	/* send array */
	for(u8 i = 0 ; i < 8 ; i++){
		CLCD_voidSendData(Copypu8Array[i]);
	}
}






















