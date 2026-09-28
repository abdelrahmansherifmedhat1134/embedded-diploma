#include<stdio.h>
#include"../my_c_lib/inc/my_string.h"

/*
string is an array of characters 
it differs than a normal array because it has a null character that 
the compiler adds at the end automatically 

while performing operations that are meant for strings for normal 
arrays the compiler is going to act strangely because for strings 
it depends that there is a null character   
*/
/*
int main(){
    int arr[5]={1,2,3,4};
    int** arrdp=&arr;
    *arrdp++;
    printf("%d",*arr);
}
    */

int main(){
    char str[]="abdelrahman";
    print_string(str);
    }