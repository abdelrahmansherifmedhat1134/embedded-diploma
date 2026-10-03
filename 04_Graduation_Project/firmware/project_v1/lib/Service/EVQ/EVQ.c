/*
 * EVQ.c
 *
 *  Created on: Oct 3, 2026
 *      Author: eslam
 */
#include "../std_types.h"
#include "../RINGBUF/RINGBUF.h"
#include "EVQ.h"
#include "EVQ_cfg.h"

#define EVQ_BYTES_PER_EVENT    2                       /* event ID + argument */
/* a ring holds Size - 1 bytes : + 1 so that EVQ_MAX_EVENTS events really fit */
#define EVQ_STORAGE_SIZE       ((EVQ_BYTES_PER_EVENT * EVQ_MAX_EVENTS) + 1)

static volatile u8 Global_u8Storage[EVQ_STORAGE_SIZE] ;
static RINGBUF_t Global_Ring ;
static u8 Global_u8Muted = 0 ;

void EVQ_voidInit(){
	RINGBUF_voidInit(&Global_Ring,Global_u8Storage,EVQ_STORAGE_SIZE);
	Global_u8Muted = 0 ;
}

void EVQ_voidPost(u8 Copy_u8Event, u8 Copy_u8Arg){
	if(Global_u8Muted == 1){
		/* UIREM runs its own command : that change is already answered with [OK] */
	}else if(RINGBUF_u8GetFree(&Global_Ring) < EVQ_BYTES_PER_EVENT){
		/* full : the new event is dropped , the older ones stay in order */
	}else{
		/* both bytes or none : an event is never stored half */
		RINGBUF_u8Put(&Global_Ring,Copy_u8Event);
		RINGBUF_u8Put(&Global_Ring,Copy_u8Arg);
	}
}

u8 EVQ_u8Get(u8 * Copy_pu8Event, u8 * Copy_pu8Arg){
	if(RINGBUF_u8GetCount(&Global_Ring) < EVQ_BYTES_PER_EVENT){
		return 0 ;
	}
	RINGBUF_u8Get(&Global_Ring,Copy_pu8Event);
	RINGBUF_u8Get(&Global_Ring,Copy_pu8Arg);
	return 1 ;
}

void EVQ_voidSetMute(u8 Copy_u8State){
	if(Copy_u8State == 0){
		Global_u8Muted = 0 ;
	}else{
		Global_u8Muted = 1 ;
	}
}
