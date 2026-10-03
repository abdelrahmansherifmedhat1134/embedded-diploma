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
/* HIGH = buzzer on. The pin drives the base of an NPN transistor (1k) that switches the buzzer :
 * the buzzer takes far more current (5 V / 12 R in the Proteus model) than an AVR pin may give. */
#define BUZZER_ON_LEVEL      DIO_PIN_HIGH

#endif /* HAL_BUZZER_BUZZER_CFG_H_ */
