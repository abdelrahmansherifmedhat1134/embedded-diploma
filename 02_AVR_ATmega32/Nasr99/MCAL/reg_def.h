/*************** Sevice **************
 * File Name: std_types
 * Date     : 15 DEC 2025
 * version   : 1.0.0
 * Auther    : ESlam El Hefny
 * Description:
 **************************************/
#ifndef  REG_DEF_H_
#define  REG_DEF_H_

#define PORTA  *(volatile u8 *)(0x3b) 
#define DDRA   *(volatile u8 *)(0x3A)
#define PINA   *(volatile u8 *)(0x39)

#define PORTB  *(volatile u8 *)(0x38)
#define DDRB   *(volatile u8 *)(0x37)
#define PINB   *(volatile u8 *)(0x36)

#define PORTC  *(volatile u8 *)(0x35)
#define DDRC   *(volatile u8 *)(0x34)
#define PINC   *(volatile u8 *)(0x33)

#define PORTD  *(volatile u8 *)(0x32)
#define DDRD   *(volatile u8 *)(0x31)
#define PIND   *(volatile u8 *)(0x30)


/****                            EXTi                                  ***/
//$35 ($55) MCUCR SE SM2 SM1 SM0 ISC11 ISC10 ISC01 ISC00 32, 66
//$34 ($54) MCUCSR JTD ISC2 – JTRF WDRF BORF EXTRF PORF 40, 67, 228
//$3B ($5B) GICR INT1 INT0 INT2 – – – IVSEL IVCE 47, 67
//$3A ($5A) GIFR INTF1 INTF0 INTF2 – – – – – 68

#define MCUCR    *(volatile u8 *)(0x55)
#define MCUCR_ISC00             0
#define MCUCR_ISC01             1
#define MCUCR_ISC10             2
#define MCUCR_ISC11             3

#define MCUCSR   *(volatile u8 *)(0x54)
#define MCUCSR_ISC2              6

#define GIFR     *(volatile u8 *)(0x5A)

#define GIFR_INTF1             7
#define GIFR_INTF0             6
#define GIFR_INTF2             5


/****                            ADC                                   ***/
//$07 ($27) ADMUX REFS1 REFS0 ADLAR MU

#define GICR     *(volatile u8 *)(0x5B)
#define GICR_INT1               7
#define GICR_INT0               6
#define GICR_INT2               5
//$06 ($26) ADCSRA ADEN ADSC ADATE ADIF ADIE ADPS2 ADPS1 ADPS0 216
//$05 ($25) ADCH ADC Data Register High Byte 217
//$04 ($24) ADCL ADC Data Register Low Byte 217
//$30 ($50) SFIOR ADTS2 ADTS1 ADTS0– ACME PUD PSR2 PSR10 56,85,131,198,218
#define SFIOR  				*(volatile u8 *)(0x50)
#define ADMUX  				*(volatile u8 *)(0x27)
#define ADMUX_REFS1			7
#define ADMUX_REFS0			6
#define ADMUX_ADLAR			5

#define ADCSRA 				*(volatile u8 *)(0x26)
#define ADCSRA_ADEN			7
#define ADCSRA_ADSC			6
#define ADCSRA_ADATE		5
#define ADCSRA_ADIF			4
#define ADCSRA_ADIE			3
#define ADCSRA_ADPS2		2
#define ADCSRA_ADPS1		1
#define ADCSRA_ADPS0		0

#define ADCH   				*(volatile u8 *)(0x25)
#define ADCL   				*(volatile u8 *)(0x24)
#define ADC    				*(volatile u16*)(0x24)

/**************** TIMER 0 ******/
//$33 ($53) TCCR0 FOC0 WGM00 COM01 COM00 WGM01 CS02 CS01 CS00 80
//$32 ($52) TCNT0 Timer/Counter0 (8 Bits) 82
//$39 ($59) TIMSK OCIE2 TOIE2 TICIE1 OCIE1A OCIE1B TOIE1 OCIE0 TOIE0 82, 112, 130
//$38 ($58) TIFR OCF2 TOV2 ICF1 OCF1A OCF1B TOV1 OCF0 TOV0 83, 112, 130
//
#define TCNT0   				*(volatile u8 *)(0x52)
#define TCCR0   				*(volatile u8 *)(0x53)
#define TCCR0_CS00				0
#define TCCR0_CS01				1
#define TCCR0_CS02				2
#define TCCR0_WGM01				3
#define TCCR0_COM00				4
#define TCCR0_COM01				5
#define TCCR0_WGM00				6
#define TCCR0_FOC0				7
#define TIMSK   				*(volatile u8 *)(0x59)
#define TIMSK_TOIE0				0
#define TIMSK_OCIE0				1
#define TIFR    				*(volatile u8 *)(0x58)
#define TIFR_TOV0				0
#define TIFR_OCF0				1
#define OCR0    				*(volatile u8 *)(0x5C)


/************************* USART **************************/
/*
$0C ($2C) UDR USART I/O Data Register 159
$0B ($2B) UCSRA RXC TXC UDRE FE DOR PE U2X MPCM 160
$0A ($2A) UCSRB RXCIE TXCIE UDRIE RXEN TXEN UCSZ2 RXB8 TXB8 161
$09 ($29) UBRRL USART Baud Rate Register Low Byte 164
$40       UBRRH URSEL––– UBRR[11:8] 164
$40       UCSRC URSEL UMSEL UPM1 UPM0 USBS UCSZ1 UCSZ0 UCPOL 162
*/

#define UDR  		*(volatile u8 *)(0x2C)
#define UCSRA  		*(volatile u8 *)(0x2B)
#define UCSRA_RXC		7
#define UCSRA_TXC		6
#define UCSRA_UDRE		5
#define UCSRA_FE		4
#define UCSRA_DOR		3
#define UCSRA_PE		2
#define UCSRA_U2X		1
#define UCSRA_MPCM		0
#define UCSRB  		*(volatile u8 *)(0x2A)
#define UCSRB_RXCIE		7
#define UCSRB_TXCIE		6
#define UCSRB_UDRIE		5
#define UCSRB_RXEN		4
#define UCSRB_TXEN		3
#define UCSRB_UCSZ2		2
#define UCSRB_RXB8		1
#define UCSRB_TXB8		0
#define UCSRC  		*(volatile u8 *)(0x40)
#define UCSRC_URSEL		7
#define UCSRC_UMSEL		6
#define UCSRC_UPM1		5
#define UCSRC_UPM0		4
#define UCSRC_USBS		3
#define UCSRC_UCSZ1		2
#define UCSRC_UCSZ0		1
#define UCSRC_UCPOL		0
#define UBRRL  		*(volatile u8 *)(0x29)
#define UBRRH  		*(volatile u8 *)(0x40)



/************** SPI Register ****************/


/*$0F ($2F) SPDR SPI Data Register 138
$0E ($2E) SPSR SPIF WCOL––––– SPI2X 138
$0D ($2D) SPCR SPIE SPE DORD MSTR CPOL CPHA SPR1 SPR0 136*/


#define SPDR  				*(volatile u8 *)(0x2f)
#define SPSR  				*(volatile u8 *)(0x2e)
#define SPSR_SPIF			7
#define SPSR_WCOL			6
#define SPSR_SPI2X			0

#define SPCR  				*(volatile u8 *)(0x2d)
#define SPCR_SPIE			7
#define SPCR_SPE			6
#define SPCR_DORD			5
#define SPCR_MSTR			4
#define SPCR_CPOL			3
#define SPCR_CPHA			2
#define SPCR_SPR1			1
#define SPCR_SPR0			0


#endif
