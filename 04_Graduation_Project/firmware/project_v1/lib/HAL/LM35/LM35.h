/*
 * LM35.h
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef HAL_LM35_LM35_H_
#define HAL_LM35_LM35_H_

#define LM35_AMBIENT    0
#define LM35_WATER      1

void LM35_voidInit();                                  /* ADC_voidInit */
/* one conversion (~110 us) , unit = 0.25 C (the raw ADC value is the temperature in quarter degrees) */
u16  LM35_u16ReadTempX4(u8 Copy_u8SensorID);

#endif /* HAL_LM35_LM35_H_ */
