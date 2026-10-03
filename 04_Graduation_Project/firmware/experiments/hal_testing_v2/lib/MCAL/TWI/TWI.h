/*
 * TWI.h
 *
 *  Created on: Sep 28, 2026
 */

#ifndef MCAL_TWI_TWI_H_
#define MCAL_TWI_TWI_H_

/* Error status returned by the TWI functions */
#define TWI_OK               0
#define TWI_ERR_START        1
#define TWI_ERR_SLA_NACK     2
#define TWI_ERR_DATA_NACK    3
#define TWI_ERR_ARB_LOST     4
#define TWI_ERR_BUS          5
#define TWI_ERR_TIMEOUT      6

/* TWI Status Codes (TWSR masked with 0xF8) */
#define TWI_STATUS_MASK              0xF8
#define TWI_STATUS_START             0x08
#define TWI_STATUS_REPEATED_START    0x10
#define TWI_STATUS_MT_SLA_ACK        0x18
#define TWI_STATUS_MT_SLA_NACK       0x20
#define TWI_STATUS_MT_DATA_ACK       0x28
#define TWI_STATUS_MT_DATA_NACK      0x30
#define TWI_STATUS_ARB_LOST          0x38
#define TWI_STATUS_MR_SLA_ACK        0x40
#define TWI_STATUS_MR_SLA_NACK       0x48
#define TWI_STATUS_MR_DATA_ACK       0x50
#define TWI_STATUS_MR_DATA_NACK      0x58

void TWI_voidMasterInit();
void TWI_voidDisable();
u8   TWI_u8GetStatus();

u8   TWI_u8SendStartCondition();
u8   TWI_u8SendRepeatedStart();
void TWI_voidSendStopCondition();
/* START + SLA+W + STOP : TWI_OK = device ACKs , TWI_ERR_SLA_NACK = absent or busy (24C08 write cycle) */
u8   TWI_u8ProbeAddress(u8 Copy_u8SlaveAddress);

u8   TWI_u8SendSlaveAddressWrite(u8 Copy_u8SlaveAddress);
u8   TWI_u8SendSlaveAddressRead (u8 Copy_u8SlaveAddress);

u8   TWI_u8MasterWriteDataByte  (u8 Copy_u8Data);
u8   TWI_u8MasterReadDataByteAck (u8 * Copy_pu8Data);
u8   TWI_u8MasterReadDataByteNack(u8 * Copy_pu8Data);

/* Full transfers : START , address , data , STOP */
u8   TWI_u8MasterTransmit (u8 Copy_u8SlaveAddress, const u8 * Copy_pu8Data, u8 Copy_u8Length);
u8   TWI_u8MasterReceive  (u8 Copy_u8SlaveAddress, u8 * Copy_pu8Data, u8 Copy_u8Length);
u8   TWI_u8MasterWriteRead(u8 Copy_u8SlaveAddress,
                           const u8 * Copy_pu8WriteData, u8 Copy_u8WriteLength,
                           u8 * Copy_pu8ReadData, u8 Copy_u8ReadLength);

#endif /* MCAL_TWI_TWI_H_ */
