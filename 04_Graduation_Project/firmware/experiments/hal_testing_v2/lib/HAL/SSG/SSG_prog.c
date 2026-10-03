#include "../../Service/std_types.h"
#include "../../Service/Bit_math.h"
#include "../../MCAL/DIO/DIO.h"
#include "SSG_CFG.h"
#include "SSG_int.h"

const u8 WriteNumber[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66,
    0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

void SSG_voidEnable(SSG_t * ssg){
	DIO_voidSetPinValue(ssg->EnablePort,ssg->EnablePin ,DIO_PIN_HIGH);
}
void SSG_voidDisable(SSG_t * ssg){
	DIO_voidSetPinValue(ssg->EnablePort,ssg->EnablePin ,DIO_PIN_LOW);

}
void SSG_voidWriteNumber(SSG_t * ssg , u8 Copy_u8Num){
	DIO_voidSetPortValue(ssg->DataPort, WriteNumber[Copy_u8Num]);
}
