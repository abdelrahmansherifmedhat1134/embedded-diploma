/*
 * RELAY.h
 *
 *  Created on: Oct 2, 2026
 *      Author: eslam
 */

#ifndef HAL_RELAY_RELAY_H_
#define HAL_RELAY_RELAY_H_

#define RELAY_HEATING   0
#define RELAY_COOLING   1

void RELAY_voidInit();                 /* both OFF */
void RELAY_voidOn (u8 Copy_u8RelayID);
void RELAY_voidOff(u8 Copy_u8RelayID);

#endif /* HAL_RELAY_RELAY_H_ */
