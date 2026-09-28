#include "../../Service/std_types.h"
#include "../../Service/Bit_math.h"
#include "../reg_def.h"
#include "EXTI.h"
void (*INT0_p)() = NULL;
void (*INT1_p)() = NULL;
void (*INT2_p)() = NULL;
void EXTI_voidINT0_callBack(void (*p)()){
	INT0_p = p ;
}
void EXTI_voidINT1_callBack(void (*p)()){
	INT1_p = p ;
}
void EXTI_voidINT2_callBack(void (*p)()){
	INT2_p = p ;
}
void EXTI_SetInterruptSenceCTRL(u8 Copy_u8INT_Id, u8 Copy_u8SenceCTRL){
	switch(Copy_u8INT_Id){
	case EXTI_INT0:
		switch(Copy_u8SenceCTRL){
		case EXTI_LOW_LEVEL:
			CLR_BIT(MCUCR,MCUCR_ISC00);
			CLR_BIT(MCUCR,MCUCR_ISC01);
			break;
		case EXTI_ANY_CHANGE:
			SET_BIT(MCUCR,MCUCR_ISC00);
			CLR_BIT(MCUCR,MCUCR_ISC01);
			break;
		case EXTI_FALLING_EDGE:
			CLR_BIT(MCUCR,MCUCR_ISC00);
			SET_BIT(MCUCR,MCUCR_ISC01);
			break;
		case EXTI_RISING_EDGE:
			SET_BIT(MCUCR,MCUCR_ISC00);
			SET_BIT(MCUCR,MCUCR_ISC01);
			break;
		}
		break ;
		case EXTI_INT1:
			switch(Copy_u8SenceCTRL){
			case EXTI_LOW_LEVEL:
				CLR_BIT(MCUCR,MCUCR_ISC10);
				CLR_BIT(MCUCR,MCUCR_ISC11);
				break;
			case EXTI_ANY_CHANGE:
				SET_BIT(MCUCR,MCUCR_ISC10);
				CLR_BIT(MCUCR,MCUCR_ISC11);
				break;
			case EXTI_FALLING_EDGE:
				CLR_BIT(MCUCR,MCUCR_ISC10);
				SET_BIT(MCUCR,MCUCR_ISC11);
				break;
			case EXTI_RISING_EDGE:
				SET_BIT(MCUCR,MCUCR_ISC10);
				SET_BIT(MCUCR,MCUCR_ISC11);
				break;
			}
			break ;
			case EXTI_INT2:
				switch(Copy_u8SenceCTRL){
				case EXTI_FALLING_EDGE:
					CLR_BIT(MCUCSR,MCUCSR_ISC2);
					break ;
				case EXTI_RISING_EDGE:
					SET_BIT(MCUCSR,MCUCSR_ISC2);
					break ;

				}
				break ;
	}
}
void EXTI_voidEnableINT (u8 Copy_u8INT_Id){
	switch(Copy_u8INT_Id){
	case EXTI_INT0: SET_BIT(GICR,GICR_INT0);break;
	case EXTI_INT1: SET_BIT(GICR,GICR_INT1);break;
	case EXTI_INT2: SET_BIT(GICR,GICR_INT2);break;
	}
}
void EXTI_voidDisableINT(u8 Copy_u8INT_Id){
	switch(Copy_u8INT_Id){
	case EXTI_INT0: CLR_BIT(GICR,GICR_INT0);break;
	case EXTI_INT1: CLR_BIT(GICR,GICR_INT1);break;
	case EXTI_INT2: CLR_BIT(GICR,GICR_INT2);break;
	}
}

void __vector_1 (){
INT0_p();

}

void __vector_2 (){
INT1_p();

}

void __vector_3 (){
INT2_p();

}
