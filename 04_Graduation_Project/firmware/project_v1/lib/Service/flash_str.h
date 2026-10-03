/*
 * flash_str.h
 *
 *  Created on: Oct 3, 2026
 *      Author: eslam
 */

#ifndef SERVICE_FLASH_STR_H_
#define SERVICE_FLASH_STR_H_

/* Keep text in flash , not RAM (2 KB RAM only). Same idea as avr-libc PSTR() ,
 * but with the GCC "__flash" keyword , so no <avr/pgmspace.h> is needed
 * (that header pulls <avr/io.h> , which clashes with reg_def.h).
 * Include std_types.h first (c8). Use : TERM_voidPutFlash(FLASH_STR("text")); */
#define FLASH_STR(str)    (__extension__({ static const __flash c8 Local_c8Text[] = (str); &Local_c8Text[0]; }))

#endif /* SERVICE_FLASH_STR_H_ */
