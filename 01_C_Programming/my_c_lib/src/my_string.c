#include<stdio.h>



void print_string(const char* str){
    char* temp=str;
    puts(*temp++);
}