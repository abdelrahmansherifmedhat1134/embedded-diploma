/*
 * ESTORE.h
 *
 *  Created on: Oct 3, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef SERVICE_ESTORE_ESTORE_H_
#define SERVICE_ESTORE_ESTORE_H_

#define ESTORE_OK            0
#define ESTORE_FIRST_BOOT    1      /* defaults were written at this boot */
#define ESTORE_FAULT         2      /* chip not answering : RAM image only */

void ESTORE_voidInit();                                /* blocking read of the image (boot only) ; defaults if magic/version wrong */
void ESTORE_voidUpdate();                              /* call every 10 ms : write-behind state machine */
u8   ESTORE_u8ReadByte (u8 Copy_u8Address);
void ESTORE_voidReadBlock (u8 Copy_u8Address, u8 * Copy_pu8Data, u8 Copy_u8Length);
void ESTORE_voidWriteByte (u8 Copy_u8Address, u8 Copy_u8Data);                      /* no effect and no wear if the value is unchanged */
void ESTORE_voidWriteBlock(u8 Copy_u8Address, const u8 * Copy_pu8Data, u8 Copy_u8Length);
void ESTORE_voidLoadDefaults();                        /* factory reset : defaults + all pages marked for writing */
u8   ESTORE_u8IsBusy();                                /* 1 = something still waits to be written */
u8   ESTORE_u8GetStatus();
u16  ESTORE_u16GetPageWrites();                        /* diagnostic (test_service) : page writes started since init */

#endif /* SERVICE_ESTORE_ESTORE_H_ */
