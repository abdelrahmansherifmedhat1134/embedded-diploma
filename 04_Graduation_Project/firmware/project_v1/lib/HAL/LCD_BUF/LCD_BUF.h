/*
 * LCD_BUF.h
 *
 *  Created on: Oct 2, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef HAL_LCD_BUF_LCD_BUF_H_
#define HAL_LCD_BUF_LCD_BUF_H_

/* Non-blocking LCD : writers only touch a RAM copy ("wanted") , LCD_BUF_voidUpdate
 * sends the differences to the LCD , at most ONE byte per call. */
void LCD_BUF_voidInit();                                /* CLCD_voidInit (blocking , boot only) + clear buffers */
void LCD_BUF_voidClear();                               /* RAM only : fills the wanted text with spaces */
void LCD_BUF_voidWriteChar  (u8 Copy_u8Row, u8 Copy_u8Col, c8 Copy_c8Char);
void LCD_BUF_voidWriteFlash (u8 Copy_u8Row, u8 Copy_u8Col, const __flash c8 * Copy_pc8Text);
void LCD_BUF_voidWriteRam   (u8 Copy_u8Row, u8 Copy_u8Col, const c8 * Copy_pc8Text);
void LCD_BUF_voidUpdate();                              /* call every 5 ms : sends at most ONE byte to the LCD */

#endif /* HAL_LCD_BUF_LCD_BUF_H_ */
