#include <util/delay.h>
#include "../../Service/std_types.h"
#include "../../Service/Bit_math.h"
#include "../../MCAL/DIO/DIO.h"
#include "KPAD.h"
#include "KPAD_cfg.h"
/* __flash : the tables stay in program memory (no RAM used) */
static const __flash u8 KPAD_MAT[4][4] = {
		{'7','8','9','/'},
		{'4','5','6','*'},
		{'1','2','3','-'},
		{'C','0','=','+'}

};
static const __flash u8 KPAD_COL_ARR[4] = {KPAD_COL_PIN0,KPAD_COL_PIN1,KPAD_COL_PIN2,KPAD_COL_PIN3};
static const __flash u8 KPAD_ROW_ARR[4] = {KPAD_ROW_PIN0,KPAD_ROW_PIN1,KPAD_ROW_PIN2,KPAD_ROW_PIN3};

void KPAD_voidInit(){
	/*make raw input */
	/*make raw pulled up */
	for(u8 i = 0 ; i< 4 ; i++){
		DIO_voidSetPinDirection(KPAD_ROW_PORT,KPAD_ROW_ARR[i],DIO_INPUT);
		DIO_voidEnablePullUp(KPAD_ROW_PORT,KPAD_ROW_ARR[i]);
	}
	/*make columns output */
	/*give it initial value High */
	for(u8 i = 0 ; i< 4 ; i++){
		DIO_voidSetPinDirection(KPAD_COL_PORT,KPAD_COL_ARR[i],DIO_PIN_OUTPUT);
		DIO_voidSetPinValue(KPAD_COL_PORT,KPAD_COL_ARR[i],DIO_PIN_HIGH);
	}
}
u8 KPAD_u8GetKeyPressed(){
	/*GIVE FIRST col val 0 */
	for(u8 i = 0 ; i<4;i++){

		DIO_voidSetPinValue(KPAD_COL_PORT,KPAD_COL_ARR[i],DIO_PIN_LOW);
		for(u8 j = 0 ; j< 4 ; j++){
			/*loop on each raw if raw was == 0 */
			if(DIO_u8GetPinValue(KPAD_ROW_PORT,KPAD_ROW_ARR[j]) == DIO_PIN_LOW){
				while(DIO_u8GetPinValue(KPAD_ROW_PORT,KPAD_ROW_ARR[j]) == DIO_PIN_LOW){
					// do nothing
				}
				/* give the column back its HIGH value before leaving */
				DIO_voidSetPinValue(KPAD_COL_PORT,KPAD_COL_ARR[i],DIO_PIN_HIGH);
				/* KPAD_MAT is written row by row --> mat[row j][col i] */
				return KPAD_MAT[j][i];
			}
		}
		DIO_voidSetPinValue(KPAD_COL_PORT,KPAD_COL_ARR[i],DIO_PIN_HIGH);
		/*continue looping with next col */
	}

	return 0xff;

}

/* ---------------- non-blocking scan (call KPAD_voidUpdate every 10 ms) ---------------- */
static u8 Global_u8Candidate = KPAD_NO_KEY ;   /* key seen in the last scans */
static u8 Global_u8SameScans = 0 ;             /* how many scans in a row saw the candidate */
static u8 Global_u8Stable = KPAD_NO_KEY ;      /* debounced key (KPAD_NO_KEY = nothing pressed) */
static u8 Global_u8Pending = KPAD_NO_KEY ;     /* accepted key not yet read by KPAD_u8GetKey */

/* One pass over the matrix : first key found , or KPAD_NO_KEY. Never waits for the key release. */
static u8 KPAD_u8ScanMatrix(){
	u8 Local_u8Key = KPAD_NO_KEY ;
	for(u8 i = 0 ; (i < 4) && (Local_u8Key == KPAD_NO_KEY) ; i++){
		DIO_voidSetPinValue(KPAD_COL_PORT,KPAD_COL_ARR[i],DIO_PIN_LOW);
		/* let the row pull-up level settle (pin synchronizer + wire) */
		_delay_us(5);
		for(u8 j = 0 ; j < 4 ; j++){
			if(DIO_u8GetPinValue(KPAD_ROW_PORT,KPAD_ROW_ARR[j]) == DIO_PIN_LOW){
				Local_u8Key = KPAD_MAT[j][i] ;
				break ;
			}
		}
		DIO_voidSetPinValue(KPAD_COL_PORT,KPAD_COL_ARR[i],DIO_PIN_HIGH);
	}
	return Local_u8Key ;
}
void KPAD_voidUpdate(){
	u8 Local_u8Raw = KPAD_u8ScanMatrix();
	/*1. Count how many scans in a row gave the same result */
	if(Local_u8Raw == Global_u8Candidate){
		if(Global_u8SameScans < KPAD_DEBOUNCE_SCANS){
			Global_u8SameScans++;
		}
	}else{
		Global_u8Candidate = Local_u8Raw ;
		Global_u8SameScans = 1 ;
	}
	/*2. Accept a new level after KPAD_DEBOUNCE_SCANS equal scans */
	if((Global_u8SameScans >= KPAD_DEBOUNCE_SCANS) && (Global_u8Candidate != Global_u8Stable)){
		Global_u8Stable = Global_u8Candidate ;
		if(Global_u8Stable != KPAD_NO_KEY){
			/* new key accepted : report it once (a held key gives no second event) */
			Global_u8Pending = Global_u8Stable ;
		}
	}
}
u8 KPAD_u8GetKey(){
	u8 Local_u8Key = Global_u8Pending ;
	Global_u8Pending = KPAD_NO_KEY ;
	return Local_u8Key ;
}
