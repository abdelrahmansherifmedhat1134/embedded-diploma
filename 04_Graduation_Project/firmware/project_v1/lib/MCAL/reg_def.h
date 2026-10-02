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
#define MCUCSR_JTD               7

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
#define TIMSK_TOIE1				2
#define TIMSK_OCIE1B			3
#define TIMSK_OCIE1A			4
#define TIMSK_TICIE1			5
#define TIMSK_TOIE2				6
#define TIMSK_OCIE2				7
#define TIFR    				*(volatile u8 *)(0x58)
#define TIFR_TOV0				0
#define TIFR_OCF0				1
#define TIFR_TOV1				2
#define TIFR_OCF1B				3
#define TIFR_OCF1A				4
#define TIFR_ICF1				5
#define TIFR_TOV2				6
#define TIFR_OCF2				7
#define OCR0    				*(volatile u8 *)(0x5C)


/**************** TIMER 1 ******/
//$2F ($4F) TCCR1A COM1A1 COM1A0 COM1B1 COM1B0 FOC1A FOC1B WGM11 WGM10
//$2E ($4E) TCCR1B ICNC1 ICES1 - WGM13 WGM12 CS12 CS11 CS10
//$2D ($4D) TCNT1H / $2C ($4C) TCNT1L
//$2B ($4B) OCR1AH / $2A ($4A) OCR1AL
//$29 ($49) OCR1BH / $28 ($48) OCR1BL
//$27 ($47) ICR1H  / $26 ($46) ICR1L
#define TCCR1A  				*(volatile u8 *)(0x4F)
#define TCCR1A_WGM10			0
#define TCCR1A_WGM11			1
#define TCCR1A_FOC1B			2
#define TCCR1A_FOC1A			3
#define TCCR1A_COM1B0			4
#define TCCR1A_COM1B1			5
#define TCCR1A_COM1A0			6
#define TCCR1A_COM1A1			7
#define TCCR1B  				*(volatile u8 *)(0x4E)
#define TCCR1B_CS10				0
#define TCCR1B_CS11				1
#define TCCR1B_CS12				2
#define TCCR1B_WGM12			3
#define TCCR1B_WGM13			4
#define TCCR1B_ICES1			6
#define TCCR1B_ICNC1			7
#define TCNT1H  				*(volatile u8 *)(0x4D)
#define TCNT1L  				*(volatile u8 *)(0x4C)
#define OCR1AH  				*(volatile u8 *)(0x4B)
#define OCR1AL  				*(volatile u8 *)(0x4A)
#define OCR1BH  				*(volatile u8 *)(0x49)
#define OCR1BL  				*(volatile u8 *)(0x48)
#define ICR1H   				*(volatile u8 *)(0x47)
#define ICR1L   				*(volatile u8 *)(0x46)


/**************** TIMER 2 ******/
//$25 ($45) TCCR2 FOC2 WGM20 COM21 COM20 WGM21 CS22 CS21 CS20
//$24 ($44) TCNT2 Timer/Counter2 (8 Bits)
//$23 ($43) OCR2  Timer/Counter2 Output Compare Register
#define TCCR2  					*(volatile u8 *)(0x45)
#define TCCR2_CS20				0
#define TCCR2_CS21				1
#define TCCR2_CS22				2
#define TCCR2_WGM21				3
#define TCCR2_COM20				4
#define TCCR2_COM21				5
#define TCCR2_WGM20				6
#define TCCR2_FOC2				7
#define TCNT2  					*(volatile u8 *)(0x44)
#define OCR2  					*(volatile u8 *)(0x43)


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


/************** TWI Register ****************/
/*$00 ($20) TWBR Two-wire Serial Interface Bit Rate Register 178
$01 ($21) TWSR TWS7 TWS6 TWS5 TWS4 TWS3 - TWPS1 TWPS0 180
$02 ($22) TWAR TWA6 TWA5 TWA4 TWA3 TWA2 TWA1 TWA0 TWGCE 181
$03 ($23) TWDR Two-wire Serial Interface Data Register 180
$36 ($56) TWCR TWINT TWEA TWSTA TWSTO TWWC TWEN - TWIE 178*/

#define TWBR  				*(volatile u8 *)(0x20)
#define TWSR  				*(volatile u8 *)(0x21)
#define TWSR_TWPS0			0
#define TWSR_TWPS1			1
#define TWAR  				*(volatile u8 *)(0x22)
#define TWDR  				*(volatile u8 *)(0x23)
#define TWCR  				*(volatile u8 *)(0x56)
#define TWCR_TWIE			0
#define TWCR_TWEN			2
#define TWCR_TWWC			3
#define TWCR_TWSTO			4
#define TWCR_TWSTA			5
#define TWCR_TWEA			6
#define TWCR_TWINT			7


#endif
