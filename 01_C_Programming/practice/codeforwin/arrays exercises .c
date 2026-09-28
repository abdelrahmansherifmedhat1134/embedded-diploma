//ex 22

#include<stdio.h>

/*
int main(){
    int numbers[5];
    for(int i=0;i<5;i++)
    {
        printf("enter number %d: ",i);
        scanf("%d",&numbers[i]);
        
    }
    
    for(int i=0;i<5;i++)
    {
        printf("array element number %d is %d\n",i,numbers[i]);

    }
}
*/


/*
int main(){
    int arr[]={1,2,3,4};
    printf("%p \n",arr+2);
    printf("%p",&arr[2]);
}
*/
/*
int main(){
    int arr[10];
    for(int i=0;i<10;i++)
    {
        printf("enter number%d: ",i);
        scanf("%d",&arr[i]);
        
    }
    float sum=0;
    for(int i=0;i<10;i++)
    {
        sum+=arr[i];
        
    }
    printf("average is %f",sum/10);
}
*/
 /*
int main()
{
    double arr[]={1,2,3,4,5};
    printf("%p\n",arr);
    printf("%p",&arr[1]);
}
*/
void arr_swap(int *arr);
int main(){
    int arr[5]={10,20,30,40,50};
    arr_swap(arr);
        for(int i=0;i<5;i++)
    {
        printf("%d\n",arr[i]);
    }
    
}
void arr_swap(int *arr)
{
    for(int i=3;i>=0;i--)
    {
        for(int m=0;m<i+1;m++)
        {
            int temp=arr[m];
            arr[m]=arr[m+1];
            arr[m+1]=temp;
        }
        
    }

}
/*
int main(){
    int arr[5]={1,2,3,4,5};
    int *ptr=arr;
    printf("address: %p\n",ptr+1);
    printf("value: %d\n",*(ptr+1));
}*/