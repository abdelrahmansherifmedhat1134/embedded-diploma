#include<stdio.h>
#include"../my_c_lib/inc/config.h"

int main(){
    #if DIR == L
    puts("left");
    #elif DIR == R
    puts("right");
    #warning "khaly balaaaaaaak"
    #else 
    #error "erroooooor"
    #endif

    printf("in %s in %s in %s: line %d:erroooooor\n",__DATE__,__TIME__,__FILE__,__LINE__);
    return 0;
}