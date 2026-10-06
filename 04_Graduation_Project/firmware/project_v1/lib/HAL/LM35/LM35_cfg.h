/*
 * LM35_cfg.h
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef HAL_LM35_LM35_CFG_H_
#define HAL_LM35_LM35_CFG_H_

#define LM35_AMBIENT_CHANNEL   ADC_CHANNEL_0
#define LM35_WATER_CHANNEL     ADC_CHANNEL_1
/* 10 mV/C , ADC reference 2.56 V , 10 bit --> 2.5 mV per step --> 4 steps per C */
#define LM35_STEPS_PER_C       4

#endif /* HAL_LM35_LM35_CFG_H_ */
