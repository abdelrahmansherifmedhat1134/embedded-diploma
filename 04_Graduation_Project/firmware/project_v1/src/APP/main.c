#include <avr/delay.h>
//#include <avr/interrupt.h>
#include "Service/std_types.h"
#include "MCAL/DIO/DIO.h"
#include "MCAL/ADC/ADC.h"
#include <MCAL/GIE/GIE.h>
#include "MCAL/TIMER0/TIMER0.h"
#include "MCAL/USART/USART.h"
#include "MCAL/TWI/twi_slave.h"
#include "HAL/CLCD/CLCD.h"
u8 arr[100];

static uint8_t last_received_byte = 0;

void echo_rx_cb(void)
{
    /* Read all received bytes */
    while (TWI_slave_rx_available()) {
        last_received_byte = TWI_slave_rx_read();
		sprintf(arr,"recieved %d\r\n",last_received_byte);
		USART_voidSendString(arr);
    }
}

void echo_tx_cb(void)
{
	USART_voidSendString("Transmit\r\n");
    /* Send back the last received byte */
    TWI_slave_tx_write(last_received_byte);
}

/*===========================================================================
 * main — choose which example to run
 *===========================================================================*/

int main(void)
{
	USART_voidInit();
    TWI_slave_init(0x55, echo_rx_cb, echo_tx_cb);

    GIE_voidEnableGlobalInterrupt();

    while (1)
    {

    }
	return 0 ;
}




