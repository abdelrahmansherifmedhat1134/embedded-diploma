#include<stdio.h>
#include<math.h>

int perfect_sqr(float x);
int operation(char x,int num1,int num2);
int is_alpha(char x);
char lower_to_upper(char x);
int is_multiple(int x,int y);
int heating_time(int x);
int floor_float_sum(float x,float y);

int main(){
    printf("%d\n",perfect_sqr(15));
    printf("%d\n",operation('*',10,2));
    printf("%d\n",is_alpha('A'));
    printf("%c\n",lower_to_upper('a'));
    printf("%d\n",is_multiple(10,5));
    printf("%d\n",heating_time(15));
    printf("%d\n",floor_float_sum(15.32,16.98));
  
}




int perfect_sqr(float x){
        if(sqrt(x)==floor(sqrt(x))){
            return 1;
        }
        else{
            return 0;
        }
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


int is_alpha(char x){
    int y= x;    
    if((y>=65 && y<=90)||(y>=97 && y<=122)){
        return 1;
    }
    else {
        return 0;
    }
}

char lower_to_upper(char x){
    int y= x;    
    if(y>=97 && y<=122){
        return (char)(y-32);
            }
    else 
        return 0;
}

int is_multiple(int x,int y){
    if((x%y)==0){
        return 1;
    }
    else 
    return 0;
}


int heating_time(int x){
    if (x >= 0 && x <= 30) {
    return 7;
} else if (x > 30 && x <= 60) {
    return 5;
} else if (x > 60 && x <= 90) {
    return 3;
} else if (x>90 && x<100) {
    return 1;
}   else{
    printf("invalid input");
    return 0;
    }
}



int floor_float_sum(float x,float y){
    return (int)(x+y);
}
    