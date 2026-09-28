#ifndef _BIT_MATH_H_ 
#define _BIT_MATH_H_

#define SET_BIT(reg,bit)  (reg|=(1<<bit))
#define CLR_BIT(reg,bit)  (reg&=(~(1<<bit)))
#define TOGGLE_BIT(reg,bit) (reg^=(1<<bit))
#define GET(reg,bit) ((reg>>bit)&1)

/*
#define ROT_R(reg,bit)
#define ROT_L(reg,bit)
#define SW_NIP(reg,bit)
#define ASS_BIT(reg,bit,value)


*/

#endif