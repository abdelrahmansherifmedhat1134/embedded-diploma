/*
 * BUTTON_cfg.h
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef HAL_BUTTON_BUTTON_CFG_H_
#define HAL_BUTTON_BUTTON_CFG_H_

#define BUTTON_COUNT              3
#define BUTTON_ONOFF_PORT         DIO_PORTD
#define BUTTON_ONOFF_PIN          DIO_PIN_6
#define BUTTON_UP_PORT            DIO_PORTD
#define BUTTON_UP_PIN             DIO_PIN_7
#define BUTTON_DOWN_PORT          DIO_PORTB
#define BUTTON_DOWN_PIN           DIO_PIN_5
/* buttons connect the pin to GND : the internal pull-up keeps the pin HIGH when released */
#define BUTTON_PRESSED_LEVEL      DIO_PIN_LOW
#define BUTTON_DEBOUNCE_SAMPLES   3                    /* 3 x 10 ms = 30 ms */

#endif /* HAL_BUTTON_BUTTON_CFG_H_ */
