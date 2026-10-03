/*
 * CLIMATE.h
 *
 *  Created on: Oct 3, 2026
 *      Author: eslam
 */

#ifndef APP_CLIMATE_CLIMATE_H_
#define APP_CLIMATE_CLIMATE_H_

void CLIMATE_voidInit();
void CLIMATE_voidTask100ms();                          /* one ambient LM35 reading -> CLIMATE_voidFeedSample */
/* one reading in 0.25 C units : average + hysteresis. The 100 ms task calls it ;
 * test_app also calls it directly to drive the logic without the sensor */
void CLIMATE_voidFeedSample(u16 Copy_u16TempX4);
u8   CLIMATE_u8IsAcOn();
u8   CLIMATE_u8IsTempValid();                          /* 0 during the first second */
u8   CLIMATE_u8GetRoomTemp();                          /* whole degrees */
void CLIMATE_voidForceOff();                           /* lockdown : fan off until reset */

#endif /* APP_CLIMATE_CLIMATE_H_ */
