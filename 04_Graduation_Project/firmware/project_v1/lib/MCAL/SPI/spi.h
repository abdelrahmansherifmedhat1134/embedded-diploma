/*
 * spi.h
 *
 *  Created on: Feb 25, 2026
 *      Author: Eng. Eslam Hefny (AMIT embedded systems diploma)
 */

#ifndef MCAL_SPI_SPI_H_
#define MCAL_SPI_SPI_H_

void SPI_voidMasterInit();
void SPI_voidMasterSend(u8 Copy_u8Data);

void SPI_voidSlaveInit();
u8 SPI_u8SlaveRecive();


#endif /* MCAL_SPI_SPI_H_ */
