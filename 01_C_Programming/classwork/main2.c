#include"../my_c_lib/inc/my_lib.h"   //this path inclusion using relative path 
//i can use the absolute path and write "D:/embedded diploma/code/my_lib/my_lib.h"
#include<stdio.h>

//i must pass any .c file in my application to the gcc so it compiles it 
// to pass multiple files to gcc leave a space between them 
//if there's a name of c file passed to gcc that has a space you can put it in a string so the gcc doesnt think it is two separate files and give you an error 
// you must consider what file you are in when passing functions to gcc so as to pass the relative path of the file so the gcc can find it 


int recursion_sum(int n);
int recursion_sum_even(int n);

int main(){
    /*char x;
    printf("enter a number");
    scanf("%c",&x);
    int y = x;
    printf("x is %c\n",x);
    printf("y is %d\n",y);
*/
    int x=5;
    printf("sum is = %d\n",recursion_sum(x));
    printf("sum is = %d\n",recursion_sum_even(x));
    
}


int recursion_sum(int n){
    if (n==1){
        return 1;
    }
    else {
    return n + recursion_sum(n-1);
    }  
}

int recursion_sum_even(int n){
    if (n==1){
        return 2;
    }
    else if(n%2!=0){
        return recursion_sum_even(n-1);
    }
    else {
    return n + recursion_sum(n-2);
    }  
}
