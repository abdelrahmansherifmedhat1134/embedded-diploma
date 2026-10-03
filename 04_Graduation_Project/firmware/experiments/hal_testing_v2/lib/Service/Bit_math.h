/*************** Sevice **************
 * File Name: std_types
 * Date     : 15 DEC 2025
 * version   : 1.0.0
 * Auther    : ESlam El Hefny
 * Description:
 **************************************/
#ifndef  BIT_MATH_H_
#define  BIT_MATH_H_


 #define SET_BIT(reg,bit)  (reg|=1<<bit)
 #define CLR_BIT(reg,bit)  (reg&=~(1<<bit))
 #define GET_BIT(reg,bit)  ((reg>>bit)&1)
 #define TOG_BIT(reg,bit)  (reg^=1<<bit)

#endif