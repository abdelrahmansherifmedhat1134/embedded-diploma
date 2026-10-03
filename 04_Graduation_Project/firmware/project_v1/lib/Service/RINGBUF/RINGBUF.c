/*
 * RINGBUF.c
 *
 *  Created on: Oct 3, 2026
 *      Author: eslam
 */
#include "../std_types.h"
#include "RINGBUF.h"

void RINGBUF_voidInit(RINGBUF_t * rb, volatile u8 * Copy_pu8Storage, u8 Copy_u8Size){
	rb->Buffer = Copy_pu8Storage ;
	rb->Size = Copy_u8Size ;
	rb->Head = 0 ;
	rb->Tail = 0 ;
}

u8 RINGBUF_u8Put(RINGBUF_t * rb, u8 Copy_u8Data){
	u8 Local_u8Head = rb->Head ;
	u8 Local_u8Next = Local_u8Head + 1 ;
	if(Local_u8Next >= rb->Size){
		Local_u8Next = 0 ;
	}
	/* one slot always stays empty : Head == Tail means "empty" , never "full" */
	if(Local_u8Next == rb->Tail){
		return RINGBUF_FULL ;
	}
	/*1. store the byte first */
	rb->Buffer[Local_u8Head] = Copy_u8Data ;
	/*2. then publish it (one 8-bit write : the consumer sees the old or the new index) */
	rb->Head = Local_u8Next ;
	return RINGBUF_OK ;
}

u8 RINGBUF_u8Get(RINGBUF_t * rb, u8 * Copy_pu8Data){
	u8 Local_u8Tail = rb->Tail ;
	if(Local_u8Tail == rb->Head){
		return RINGBUF_EMPTY ;
	}
	*Copy_pu8Data = rb->Buffer[Local_u8Tail] ;
	Local_u8Tail++;
	if(Local_u8Tail >= rb->Size){
		Local_u8Tail = 0 ;
	}
	rb->Tail = Local_u8Tail ;
	return RINGBUF_OK ;
}

u8 RINGBUF_u8GetCount(RINGBUF_t * rb){
	/* each index is read once : an interrupt in between only makes the answer a little old */
	u8 Local_u8Head = rb->Head ;
	u8 Local_u8Tail = rb->Tail ;
	if(Local_u8Head >= Local_u8Tail){
		return Local_u8Head - Local_u8Tail ;
	}
	return rb->Size - Local_u8Tail + Local_u8Head ;
}

u8 RINGBUF_u8GetFree(RINGBUF_t * rb){
	return (rb->Size - 1) - RINGBUF_u8GetCount(rb) ;
}
