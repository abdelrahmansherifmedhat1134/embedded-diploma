#include<stdio.h>
/*
int main(){

    //you have three ways of swapping variables 
    //creating a temporary variable 
    //three xors 
    int a=5;
    int b=10;
    a=a^b;
    b=a^b;
    a=a^b;
    printf("a is %d\n",a);
    printf("b is %d\n",b); 
    
    //addition and two subtractions
    a=a+b;
    b=a-b;
    a=a-b;
    printf("second swap\n");
    printf("a is %d\n",a);
    printf("b is %d\n",b);


}
*/
/*
int main(){
    int x=10;
    int *const p=x;//p++ pass , *p=10 fail
    const int* p2=x;//p++
    const int* const p3=x; 

}*/
int main(){
    int x=50;
    int *ptr=&x;
    printf("address x is %p\n",ptr);
    printf("x is %d\n",*ptr);
    ++*ptr;
    printf("address x is %p\n",ptr);
    printf("x is %d\n",*ptr++);
    ++*ptr;
     printf("address x is %p\n",ptr);
    printf("x is %d\n",*ptr++);
}