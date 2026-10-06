/*
 * LAMP_cfg.h
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef HAL_LAMP_LAMP_CFG_H_
#define HAL_LAMP_LAMP_CFG_H_

#define LAMP_COUNT          5
#define LAMP_I2C_ADDRESS    0x20                       /* PCF8574 , A2 A1 A0 = 0 0 0 */
#define LAMP_FIRST_BIT      0                          /* lamp 1 = P0 ... lamp 5 = P4 */
#define LAMP_ON_LEVEL       0                          /* active low : +5 V -> 220 R -> LED -> pin (pin_map.md C-2) */

#endif /* HAL_LAMP_LAMP_CFG_H_ */
