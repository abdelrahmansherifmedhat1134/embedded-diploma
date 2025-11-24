#include<stdio.h>
/*
int main(){
    int x;
    char* p1;
    short* p2;
    int* p3;
    float* p4;

    printf("%d\n",sizeof(p1));
    printf("%d\n",sizeof(p2));
    printf("%d\n",sizeof(p3));
    printf("%d\n",sizeof(p4));
}
*/
/*
func(){
    int a=10;
    return &a;
}
void print(int a){
    printf("hello %d",a);
}
int main(){
    
    long long x= 0xffaabbccddeeff77;
    unsigned char *p1=&x;
    unsigned short*p2=&x;
    unsigned int* p3=&x;
    long long* p4=&x;
    printf("char %x\n",*p1);
    printf("short %x\n",*p2);        
    printf("int %x\n",*p3);
    printf("long %x\n",*p4);

    int *m1=NULL; //null pointer
    int* m2; //wild pointer can corrupt data or write in an unautherized address so os can close the application and give segmentation error
    int *m3=func();  //dangling pointer bec scope of address has lifetime of function only
    void *m4; //void pointer saving space only but you need to cast when derefrencing

    int* s1=100;
    printf("%x\n",++s1); // when i add to the address of the pointer i add based on the size of this pointer data type

    int* s2=200;
    printf("%d\n",--s2);

    printf("%d\n",s2-s1);

    int *ptr(int);
    ptr=print(10);

}
*/
/*
int main(){
    //adding two numbers
    int x=10;
    int y=20;
    int *p1=&x;
    int *p2=&y;
    //printf("%d\n",*p1 + *p2);
    //swapping 
    int temp=0;
    int *p3=&temp;
    *p3=*p1;
    *p1=*p2;
    *p2=*p3;
    
   // printf("x is %d\n",x);
    //printf("y is %d\n",y);
    //input and print array elements using pointer 
    int arr[]={1,2,3,4,5};
    int *s1=arr;
    printf("arr 1\n");
    for(int i=0;i<5;i++){
        printf("%d\n",*(s1+i));
        (i==4)? (s1-i):NULL;
    }
    printf("arr2\n");
    int arr2[5]={0};
    for(int i=4;i>-1;i--){
        arr2[i]=*s1++;
    }
    s1=arr;
    for(int i=0;i<5;i++){
        printf("%d\n",arr2[i]);
    }
    int *s2=arr2;
    int *s3=&temp;
    for(int i=0;i<5;i++){
        *(s3)=*(s1+i);
        *(s1+i)=*(s2+i);
        *(s2+i)=*(s3);
    }
    printf("arr1 arr2 after swapping\n");
    for(int i=0;i<5;i++){
        printf("%d      %d\n",arr[i],arr2[i]);
    }
}
    */


    int main(){
        printf("hello");
    }
