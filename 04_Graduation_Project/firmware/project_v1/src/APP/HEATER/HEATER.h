/*
 * HEATER.h
 *
 *  Created on: Oct 3, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef APP_HEATER_HEATER_H_
#define APP_HEATER_HEATER_H_

#define HEATER_ELEMENT_NONE       0
#define HEATER_ELEMENT_HEATING    1
#define HEATER_ELEMENT_COOLING    2

void HEATER_voidInit();                                /* OFF , set temperature from ESTORE */
void HEATER_voidTask10ms();                            /* buttons */
void HEATER_voidTask100ms();                           /* sampling , control , display , setting timeout */
void HEATER_voidTask500ms();                           /* blink phase */
/* one water reading in 0.25 C units : average + element control. The 100 ms task calls it ;
 * test_app also calls it directly to drive the logic without the sensor */
void HEATER_voidFeedSample(u16 Copy_u16TempX4);
u8   HEATER_u8IsOn();
u8   HEATER_u8GetElement();
u8   HEATER_u8GetWaterTemp();                          /* whole degrees */
u8   HEATER_u8GetSetTemp();
u8   HEATER_u8SetSetTemp(u8 Copy_u8Temp);              /* 1 = accepted and saved (REQ-HTR-14) */
void HEATER_voidForceOff();                            /* lockdown : off , buttons ignored until reset */

#endif /* APP_HEATER_HEATER_H_ */
