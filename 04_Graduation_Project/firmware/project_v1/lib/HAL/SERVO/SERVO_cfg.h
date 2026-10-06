/*
 * SERVO_cfg.h
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef HAL_SERVO_SERVO_CFG_H_
#define HAL_SERVO_SERVO_CFG_H_

#define SERVO_PORT            DIO_PORTD
#define SERVO_PIN             DIO_PIN_5                /* OC1A */
#define SERVO_MIN_PULSE_US    1000UL                   /* 0 degrees   */
#define SERVO_MAX_PULSE_US    2000UL                   /* 180 degrees */
#define SERVO_MAX_ANGLE       180

#endif /* HAL_SERVO_SERVO_CFG_H_ */
