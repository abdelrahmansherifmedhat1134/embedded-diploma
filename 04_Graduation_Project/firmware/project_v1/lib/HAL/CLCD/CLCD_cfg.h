/*
 * CLCD_cfg.h
 *
 *  Created on: Dec 29, 2025
 *      Author: Eng. Eslam Hefny (AMIT embedded systems diploma)
 *    Modified: Abdelrahman Sherif Medhat (graduation project fixes, docs/architecture.md Section 2)
 */

#ifndef HAL_CLCD_CLCD_CFG_H_
#define HAL_CLCD_CLCD_CFG_H_
#define CLCD_4_BIT_MODE    0
#define CLCD_8_BIT_MODE    1
#define CLCD_I2C_MODE      2
#define CLCD_MODE 			CLCD_I2C_MODE

/* Display control command : 0b00001DCB (D = display on , C = cursor , B = blink) */
#define CLCD_DISPLAY_CTRL   0b00001100   /* display on , cursor off , blink off */

/* The Configuration is Valid In Case I2C Mode only (LCD behind a PCF8574) */
/* 0x27 = PCF8574 with A2 A1 A0 = 1 1 1 */
#define CLCD_I2C_ADDRESS    0x27
/* PCF8574 pin (P0..P7) connected to each LCD pin */
#define CLCD_I2C_RS_BIT     0
#define CLCD_I2C_RW_BIT     1
#define CLCD_I2C_E_BIT      2
#define CLCD_I2C_BL_BIT     3
#define CLCD_I2C_D4_BIT     4
#define CLCD_I2C_D5_BIT     5
#define CLCD_I2C_D6_BIT     6
#define CLCD_I2C_D7_BIT     7


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
