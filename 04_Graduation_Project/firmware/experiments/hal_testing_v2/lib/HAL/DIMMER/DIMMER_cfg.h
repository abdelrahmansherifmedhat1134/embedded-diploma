/*
 * DIMMER_cfg.h
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */

#ifndef HAL_DIMMER_DIMMER_CFG_H_
#define HAL_DIMMER_DIMMER_CFG_H_

#define DIMMER_PORT              DIO_PORTB
#define DIMMER_PIN               DIO_PIN_3             /* OC0 */
#define DIMMER_TIMER_PRESCALER   TIMER0_DIV_8          /* F_CPU / 8 / 256 = 7812 Hz : easy to filter to 0-5 V */

#endif /* HAL_DIMMER_DIMMER_CFG_H_ */
