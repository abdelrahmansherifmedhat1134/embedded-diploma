/*
 * EXT_EEPROM_cfg.h
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */

#ifndef HAL_EXT_EEPROM_EXT_EEPROM_CFG_H_
#define HAL_EXT_EEPROM_EXT_EEPROM_CFG_H_

#define EXT_EEPROM_I2C_ADDRESS   0x50                  /* 1010 A2 B1 B0 , A2 = GND ; B1 B0 = address bits 9:8 */
#define EXT_EEPROM_PAGE_SIZE     16
#define EXT_EEPROM_SIZE          1024
#define EXT_EEPROM_BLOCK_SIZE    256                   /* one I2C address (B1 B0) covers 256 bytes */

#endif /* HAL_EXT_EEPROM_EXT_EEPROM_CFG_H_ */
