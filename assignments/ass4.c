#include<stdio.h>
#include"../my_lib/array.h" //you just need to include the .h file and dont need to include the .c file in the .h file nor the .h file in the .c file 


int operation(char x,int num1,int num2);
int cube(int x);
int prime_interval_display(int n); 

int main(){
    //printf("%d\n",operation('*',10,5));
    //printf("%d\n",cube(5));

    int arr1[]={1,2,3,4,5};
    int arr2[]={1,2,3,4,5};
    int x= array_cmp(arr1,arr2,5);
    printf("are they the same? %d\n",x);
    
    array_rotate_left(arr1,5);
    display_array(arr1,5);
    
    
    array_rotate_right(arr2,5);
    display_array(arr2,5);
}

int cube(int x){
    return x*x*x;
}
int operation(char x,int num1,int num2){
    switch(x){
        case'+':
        return num1+num2;
        break;
        case'-':
        return num1-num2;
        break;
        case'*':
        return num1*num2;
        break;
        case'/':
        return num1/num2;
        break;
    }
}

int prime_interval_display(int n){
    if(n=2)
    for(int i=0;i<1;i++){

    }
}