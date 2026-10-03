/*
 * EXT_EEPROM.h
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */

#ifndef HAL_EXT_EEPROM_EXT_EEPROM_H_
#define HAL_EXT_EEPROM_EXT_EEPROM_H_

#define EXT_EEPROM_OK       0
#define EXT_EEPROM_ERROR    1

void EXT_EEPROM_voidInit();                            /* TWI_voidMasterInit */
/* blocking , 90 us per byte : boot only */
u8   EXT_EEPROM_u8ReadBlock(u16 Copy_u16Address, u8 * Copy_pu8Data, u8 Copy_u8Length);
/* 1..16 bytes inside ONE page , returns after sending ; the chip is then busy (write cycle 5..10 ms) */
u8   EXT_EEPROM_u8WritePage(u16 Copy_u16Address, const u8 * Copy_pu8Data, u8 Copy_u8Length);
/* ONE ACK poll : 1 = write cycle finished , 0 = still busy (or chip absent). Never waits. */
u8   EXT_EEPROM_u8IsReady();

#endif /* HAL_EXT_EEPROM_EXT_EEPROM_H_ */
