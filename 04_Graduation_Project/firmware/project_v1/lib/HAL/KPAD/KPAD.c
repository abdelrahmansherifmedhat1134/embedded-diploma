#include "../../Service/std_types.h"
#include "../../Service/Bit_math.h"
#include "../../MCAL/DIO/DIO.h"
#include "KPAD.h"
#include "KPAD_cfg.h"
static u8 KPAD_MAT[4][4] = {
		{'7','8','9','/'},
		{'4','5','6','*'},
		{'1','2','3','-'},
		{'C','0','=','+'}

};
static u8 KPAD_COL_ARR[4] = {KPAD_COL_PIN0,KPAD_COL_PIN1,KPAD_COL_PIN2,KPAD_COL_PIN3};
static u8 KPAD_ROW_ARR[4] = {KPAD_ROW_PIN0,KPAD_ROW_PIN1,KPAD_ROW_PIN2,KPAD_ROW_PIN3};

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
