/*
 * MAVG_cfg.h
 *
 *  Created on: Oct 3, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef SERVICE_MAVG_MAVG_CFG_H_
#define SERVICE_MAVG_MAVG_CFG_H_

/* REQ-HTR-08 : average of the last 10 readings.
 * The sum is a u16 : MAVG_WINDOW x biggest sample must fit in 65535 (10 x 1023 ADC steps = 10230). */
#define MAVG_WINDOW    10

#endif /* SERVICE_MAVG_MAVG_CFG_H_ */
