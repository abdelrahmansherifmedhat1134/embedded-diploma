/*
 * HEATER_cfg.h
 *
 *  Created on: Oct 3, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef APP_HEATER_HEATER_CFG_H_
#define APP_HEATER_HEATER_CFG_H_

/* set temperature in C (REQ-HTR-02 , REQ-HTR-03) */
#define HEATER_SET_MIN              35
#define HEATER_SET_MAX              75
#define HEATER_SET_STEP             5
#define HEATER_SET_DEFAULT          60
/* REQ-HTR-09 : heat below set - band , cool above set + band */
#define HEATER_BAND                 5
/* REQ-HTR-12 : setting mode ends after this many 100 ms tasks without Up / Down */
#define HEATER_SETTING_TIMEOUT      50                 /* 50 x 100 ms = 5 s */

#endif /* APP_HEATER_HEATER_CFG_H_ */
