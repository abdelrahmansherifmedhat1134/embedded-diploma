/*
 * CLCD.h
 *
 *  Created on: Dec 29, 2025
 *      Author: eslam
 */

#ifndef HAL_CLCD_CLCD_H_
#define HAL_CLCD_CLCD_H_
void CLCD_voidInit();
void CLCD_voidSendCommand(u8 Copy_u8Cmd);
void CLCD_voidSendData	 (u8 Copy_u8Data);
void CLCD_voidSetCursorPosition(u8 Copy_u8x, u8 Copy_u8y);
void CLCD_voidSendString(char * str);
void CLCD_voidSendFlashString(const __flash c8 * Copy_pc8Str);
void CLCD_voidClearDisp();
/* result of the last I2C write : 0 = OK , else a TWI error code (always 0 in the parallel modes) */
u8   CLCD_u8GetStatus();

void CLCD_voidCreatSpecialChar(u8 Copy_u8Index,u8 * Copypu8Array);
#endif /* HAL_CLCD_CLCD_H_ */
