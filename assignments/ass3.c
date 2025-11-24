#include<stdio.h>
#include<math.h>

void display_alpha();
int is_prime(int x);
int calc_power(int x, int n);
int reverse_num(int x);
void half_pyramid(int rows);
void half_pyramid_inverted(int rows);
void full_pyramid(int rows);
void x_pyramid(int rows);

int main(){
    printf("%d",is_prime(17));
    display_alpha();
    printf("%d\n",calc_power(2,5));
    printf("%d\n",reverse_num(55067));
    half_pyramid(5);
    half_pyramid_inverted(5);
    full_pyramid(5);
}




int is_prime(int x){
    for(int i=x-1;i>sqrt(x);i--){
        if( x%i == 0)
        return 0;
    }
    return 1;
}

void display_alpha(){
    for(int i=65;i<91;i++){
        printf("%c\n",(char)i);
    }
}

int calc_power(int x, int n){
    int pow =1;
    for(int i=n;i>0;i--){
        pow*=x;
    }
    return pow;
}

int reverse_num(int x){
    int remainder;
    int reverse; 
    while(x!=0){
        remainder=x%10;
        x/=10;
        reverse*=10;
        reverse+=remainder;
    }
    return reverse;
    }
void half_pyramid(int rows){
    for(int i=1;i<rows+1;i++){
        for(int m=i;m>0;m--){
        printf("*");
    }
    printf("\n");
    }
}

void half_pyramid_inverted(int rows){
    for(int i=rows;i>=0;i--){
        for(int m=0;m<i;m++){
        printf("*");
    }
    printf("\n");
    }
}

void full_pyramid(int rows){
    int mid=((2*rows)-2)/2;
    for(int i=1;i<rows+1;i++){
        for(int m=0;m<(2*rows)-1;m++){
            if(m>(mid-i) && m<(mid+i)){
                printf("*");    
            }
            else
            printf(" ");
        }
        printf("\n");

    }
}

void x_pyramid(int rows){
    
        for(int m=0;m<(2*rows)-1;m++){
            if(m==i || m==((2*rows)-2-i)){
                printf("*");    
            }
            else
            printf(" ");
        }
        printf("\n");

}