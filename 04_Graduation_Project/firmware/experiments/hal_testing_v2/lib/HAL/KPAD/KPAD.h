/*
 * KPAD.h
 *
 *  Created on: Jan 5, 2026
 *      Author: Eslam
 */

#ifndef HAL_KPAD_KPAD_H_
#define HAL_KPAD_KPAD_H_

#define KPAD_NO_KEY    0xFF

void KPAD_voidInit();
/* blocking : waits for the key release (kept for test_base) */
u8 KPAD_u8GetKeyPressed();
/* non-blocking : one matrix scan , call every 10 ms */
void KPAD_voidUpdate();
/* returns each accepted key ONCE , then KPAD_NO_KEY until a new key is accepted */
u8 KPAD_u8GetKey();

#endif /* HAL_KPAD_KPAD_H_ */
