#ifndef EXTI_H_
#define EXTI_H_

#define EXTI_INT0    0
#define EXTI_INT1    1
#define EXTI_INT2    2

#define EXTI_LOW_LEVEL      0
#define EXTI_ANY_CHANGE     1
#define EXTI_FALLING_EDGE   2
#define EXTI_RISING_EDGE    3
void EXTI_SetInterruptSenceCTRL(u8 Copy_u8INT_Id, u8 Copy_u8SenceCTRL);
void EXTI_voidEnableINT (u8 Copy_u8INT_Id);
void EXTI_voidDisableINT(u8 Copy_u8INT_Id);
void __vector_1 () __attribute__ ((signal, used, externally_visible)) ;
void __vector_2 () __attribute__ ((signal, used, externally_visible)) ;
void __vector_3 () __attribute__ ((signal, used, externally_visible)) ;
#endif
