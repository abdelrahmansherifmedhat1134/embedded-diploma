/*
 * RINGBUF.h
 *
 *  Created on: Oct 3, 2026
 *      Author: eslam
 */

#ifndef SERVICE_RINGBUF_RINGBUF_H_
#define SERVICE_RINGBUF_RINGBUF_H_

#define RINGBUF_OK       0
#define RINGBUF_FULL     1
#define RINGBUF_EMPTY    2

/* One producer and one consumer , 8-bit indexes : safe between an ISR and the
 * main loop WITHOUT disabling interrupts (each side writes only its own index). */
typedef struct{
	volatile u8 * Buffer ;      /* volatile : a byte written by an ISR is never read early (LTO) */
	u8   Size ;                 /* storage bytes , capacity = Size - 1 */
	volatile u8 Head ;          /* written only by the producer */
	volatile u8 Tail ;          /* written only by the consumer */
}RINGBUF_t;

void RINGBUF_voidInit(RINGBUF_t * rb, volatile u8 * Copy_pu8Storage, u8 Copy_u8Size);
u8   RINGBUF_u8Put(RINGBUF_t * rb, u8 Copy_u8Data);            /* RINGBUF_OK or RINGBUF_FULL */
u8   RINGBUF_u8Get(RINGBUF_t * rb, u8 * Copy_pu8Data);         /* RINGBUF_OK or RINGBUF_EMPTY */
u8   RINGBUF_u8GetCount(RINGBUF_t * rb);                       /* bytes stored */
u8   RINGBUF_u8GetFree (RINGBUF_t * rb);                       /* bytes that still fit */

#endif /* SERVICE_RINGBUF_RINGBUF_H_ */
