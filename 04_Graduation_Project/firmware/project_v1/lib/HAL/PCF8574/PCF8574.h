/*
 * PCF8574.h
 *
 *  Created on: Sep 28, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef HAL_PCF8574_PCF8574_H_
#define HAL_PCF8574_PCF8574_H_

/* PCF8574 7-bit address = 0b0100 A2 A1 A0 (0x20 .. 0x27)
 * PCF8574A             = 0b0111 A2 A1 A0 (0x38 .. 0x3F) */
#define PCF8574_BASE_ADDRESS     0x20
#define PCF8574A_BASE_ADDRESS    0x38

void PCF8574_voidInit();
/* return TWI_OK or a TWI error code */
u8   PCF8574_u8WritePort(u8 Copy_u8Address, u8 Copy_u8Value);
u8   PCF8574_u8ReadPort (u8 Copy_u8Address, u8 * Copy_pu8Value);
/* several port values in ONE I2C transaction (START , address , bytes , STOP) */
u8   PCF8574_u8WriteBytes(u8 Copy_u8Address, const u8 * Copy_pu8Data, u8 Copy_u8Length);

#endif /* HAL_PCF8574_PCF8574_H_ */
