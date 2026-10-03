/*
 * USERDB.h
 *
 *  Created on: Oct 3, 2026
 *      Author: eslam
 */

#ifndef SERVICE_USERDB_USERDB_H_
#define SERVICE_USERDB_USERDB_H_

#define USERDB_LIST_REMOTE      0
#define USERDB_LIST_KEYPAD      1

#define USERDB_OK               0
#define USERDB_ERR_FULL         1
#define USERDB_ERR_EXISTS       2
#define USERDB_ERR_NOT_FOUND    3
#define USERDB_ERR_BAD_NAME     4      /* length , character set , digits-only rule for keypad users */
#define USERDB_ERR_BAD_PASS     5
#define USERDB_ERR_READ_ONLY    6      /* write access is closed (REQ-SEC-07) */

#define USERDB_NAME_TEXT_SIZE   9      /* 8 characters + '\0' : buffer for USERDB_u8GetUserName */

/* names and passwords are passed as normal C strings ('\0' at the end) */
void USERDB_voidInit();                                                                 /* repairs an invalid admin record with the default */
u8   USERDB_u8CheckAdmin(const c8 * Copy_pc8Name, const c8 * Copy_pc8Pass);             /* 1 = match */
u8   USERDB_u8CheckUser (u8 Copy_u8List, const c8 * Copy_pc8Name, const c8 * Copy_pc8Pass);
u8   USERDB_u8AddUser   (u8 Copy_u8List, const c8 * Copy_pc8Name, const c8 * Copy_pc8Pass);
u8   USERDB_u8RemoveUser(u8 Copy_u8List, const c8 * Copy_pc8Name);
u8   USERDB_u8GetUserName(u8 Copy_u8List, u8 Copy_u8Slot, c8 * Copy_pc8Name);           /* 1 = slot is used */
u8   USERDB_u8SetAdminPassword(const c8 * Copy_pc8Pass);
void USERDB_voidSetWriteAccess(u8 Copy_u8State);       /* 1 only while the admin is logged in */

#endif /* SERVICE_USERDB_USERDB_H_ */
