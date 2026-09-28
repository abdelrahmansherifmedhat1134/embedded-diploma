/*
 * CLCD_cfg.h
 *
 *  Created on: Dec 29, 2025
 *      Author: eslam
 */

#ifndef HAL_CLCD_CLCD_CFG_H_
#define HAL_CLCD_CLCD_CFG_H_
#define CLCD_4_BIT_MODE    0
#define CLCD_8_BIT_MODE    1
#define CLCD_MODE 			CLCD_4_BIT_MODE


#define CLCD_DATA_PORT      DIO_PORTA
/* The Configuration is Valid In Case 4 Bit Mode only */
#define CLCD_DATA_PIN_0     DIO_PIN_4
#define CLCD_DATA_PIN_1     DIO_PIN_5
#define CLCD_DATA_PIN_2     DIO_PIN_6
#define CLCD_DATA_PIN_3     DIO_PIN_7



#define CLCD_CTRL_PORT      DIO_PORTB


#define CLCD_RS_PIN        DIO_PIN_1
#define CLCD_RW_PIN        DIO_PIN_0
#define CLCD_E_PIN         DIO_PIN_2

#endif /* HAL_CLCD_CLCD_CFG_H_ */
