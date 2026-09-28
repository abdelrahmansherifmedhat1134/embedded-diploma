#include "GIE.h"
void GIE_voidEnableGlobalInterrupt(){
	__asm("SEI");
}
void GIE_voidDisableGlobalInterrupt(){
	__asm("CLI");
}
