/*
 * FMT.h
 *
 *  Created on: Oct 3, 2026
 *      Author: eslam
 */

#ifndef SERVICE_FMT_FMT_H_
#define SERVICE_FMT_FMT_H_

#define FMT_NUMBER_TEXT_SIZE    6      /* "65535" + '\0' : buffer size for FMT_u8NumberToText */

u8 FMT_u8NumberToText(u16 Copy_u16Number, c8 * Copy_pc8Text);                      /* writes digits + '\0' , returns the length */
u8 FMT_u8TextToNumber(const c8 * Copy_pc8Text, u16 * Copy_pu16Number);             /* 1 = text was a valid number (digits only , <= 65535) */

#endif /* SERVICE_FMT_FMT_H_ */
