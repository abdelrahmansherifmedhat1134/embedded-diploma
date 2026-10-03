/*
 * BUZZER_cfg.h
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */

#ifndef HAL_BUZZER_BUZZER_CFG_H_
#define HAL_BUZZER_BUZZER_CFG_H_

#define BUZZER_PORT          DIO_PORTD
#define BUZZER_PIN           DIO_PIN_3
/* HIGH = buzzer on.
 * Proteus: the buzzer sits directly on the pin (model set to 3 V , 150 R).
 * Real hardware (TODO , pin_map.md C-9): the pin must drive an NPN transistor (1k to the base) that
 * switches the buzzer ; an AVR pin may not give the buzzer current. Nothing changes in the firmware. */
#define BUZZER_ON_LEVEL      DIO_PIN_HIGH

#endif /* HAL_BUZZER_BUZZER_CFG_H_ */
