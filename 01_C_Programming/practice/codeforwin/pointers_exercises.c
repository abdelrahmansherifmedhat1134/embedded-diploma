#include<stdio.h>


/*
int main()
{
    int x=5;
    int y=10;
    int z=6;
    
    int *ptr1=&x;
    int *ptr2=&y;
    int *ptr3=&z;
    
    
    printf("ptr1: %p\n",ptr1);
    printf("ptr1: %p\n",ptr2);
    printf("ptr1: %p\n",ptr3);
    printf("difference betn 1 and 2 is: %d",ptr2-ptr1);
    printf("difference betn 1 and 3 is: %d",ptr3-ptr1);
    
}
//i want to add the two adresses of a pointer by adding them using bitwise operations on the address value
*/
/*
int main(){
    int x=1;
    int y=5;
    int *ptr1=&x;
    int *ptr2=&y;
    int address1=ptr1;
    int address2=ptr2;
    
    int **ptr3=&ptr1;
    int **ptr4=&ptr2;
    
    int sum = address1 + address2 ;
    
    printf("address first ptr: %p\n",ptr1);
    printf("address scnd ptr: %p\n",ptr2);
    printf("sum is: %d",sum);
    
    
    
    printf("address in first ptr: %p\n",ptr1);
    printf("value in first ptr: %d\n",*ptr1);
    printf("address in scnd ptr: %p\n",ptr3);
    printf("value in scnd ptr: %d\n",**ptr3);
    
}
*/
/*
void swap(int *x,int *y){
    int temp=*x;
    *x=*y;
    *y=temp;
}
int main(){
    int e=10;
    int b=15;
    void (*fptr)(int *,int *);
    fptr=swap;
    printf("e value: %d\n",e);
    printf("b value: %d\n",b);

    fptr(&e,&b);
    printf("e value: %d\n",e);
    printf("b value: %d\n",b);
    
}*/

/*

int temp=0;
int main(){
    int arr[5][5];
    for(int i=0;i<5;i++){
        
        for(int j=0;j<5;j++){
            arr[i][j]=temp;
            temp++;
        }
    }
    
    for(int m=0;m<5;m++){
        
        for(int d=0;d<5;d++){
            printf("%d  ",arr[m][d]);
        }
        printf("\n");
    }
}*/
