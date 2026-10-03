/*
 * SEC.h
 *
 *  Created on: Oct 3, 2026
 *      Author: eslam
 */

#ifndef APP_SEC_SEC_H_
#define APP_SEC_SEC_H_

#define SEC_SOURCE_REMOTE    0                         /* UART terminal */
#define SEC_SOURCE_LOCAL     1                         /* keypad + LCD */
#define SEC_SOURCE_COUNT     2

#define SEC_ROLE_NONE        0                         /* logged out */
#define SEC_ROLE_USER        1
#define SEC_ROLE_ADMIN       2

/* results of SEC_u8Login */
#define SEC_LOGIN_USER       1
#define SEC_LOGIN_ADMIN      2
#define SEC_LOGIN_FAILED     3                         /* wrong name or wrong password : the same answer for both */
#define SEC_LOGIN_LOCKED     4                         /* lockdown (this attempt was the last one , or the system is already locked) */

void SEC_voidInit();
u8   SEC_u8Login(u8 Copy_u8Source, const c8 * Copy_pc8Name, const c8 * Copy_pc8Pass);
void SEC_voidLogout(u8 Copy_u8Source);
u8   SEC_u8GetRole(u8 Copy_u8Source);
u8   SEC_u8GetAttemptsLeft(u8 Copy_u8Source);
u8   SEC_u8IsLocalAllowed();                           /* REQ-SEC-08 */
void SEC_voidSetLocalAllowed(u8 Copy_u8State);         /* effective only while the admin is logged in */

#endif /* APP_SEC_SEC_H_ */
