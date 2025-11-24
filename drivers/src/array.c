#include<stdio.h>
//you just need to include the .h file and dont need to include the .c file in the .h file nor the .h file in the .c file 
// any function you intend to use in the library file you should add the library that contains this function like stdio.h for printf here
#include<stdint.h>

int get_array(int arr[],int size){
    for(int i=0;i<size;i++){
        scanf("%d",&arr[i]);
    }

}


int display_array(int arr[],int size){
    for(int i=0;i<size;i++){
        printf("%d\t",arr[i]);
    }
    printf("\n");
}

//first
int array_max(int arr[],int size){
    int max=0;
    for(int i=0;i<size;i++){
        if(max<arr[i]){
            max=arr[i];
        }
    }
    return max;
}
int array_sec_max(int arr[],int size){
    int max=0;
    int scnd_max=0;
    for(int i=0;i<size;i++){
        if(max<arr[i]){
            max=arr[i];
        }
        else{
            if(scnd_max<arr[i])
                scnd_max=arr[i];
        }
    }
    return scnd_max;
}
int array_min(int arr[],int size){
    int min=2147483647;
    for(int i=0;i<size;i++){
        if(min>arr[i]){
            min=arr[i];
        }
    }
    return min;
}
int array_sum(int arr[],int size){
    int sum=0;
    for(int i=0;i<size;i++){
        sum+=arr[i];
    }
    return sum;
}
int array_Count_odd(int arr[],int size){
    int sum_odd=0;
    for(int i=0;i<size;i++){
        if((arr[i]%2)!=0){
            sum_odd+=arr[i];
        }
        else{
            continue;
        }
    }
    return sum_odd;
}
int array_Count_Even(int arr[],int size){
    int sum_even=0;
    for(int i=0;i<size;i++){
        if((arr[i]%2)==0){
            sum_even+=arr[i];
        }
        else{
            continue;
        }
    }
    return sum_even;
}
int array_find_at(int arr[],int size,int elem){
    for(int i=0;i<size;i++){
        if(arr[i]==elem){
            return i;
        }
        else{
            continue;
        }
    }
    return -1;
}
int array_Count_elem(int arr[],int size,int elem){
    int sum=0; 
    for(int i=0;i<size;i++){
        if(arr[i]==elem){
            sum+=1;
        }
        else{
            continue;
        }
    }
    return sum;
}








//pro
int array_cmp(int arr1[],int arr2[],int size){
    for(int i=0;i<size;i++){
        if(arr1[i]!=arr2[i])
        return 0;
    }
    return 1;
}

int array_copy(int arr1[],int arr2[],int size){
    for(int i=0;i<size;i++){
        arr2[i]=arr1[i];
    }

}
//arr[]={1,2,3,4,5} after shift arr[]={2,3,4,5,1}
int array_rotate_left(int arr[],int size){
    int temp;
    for(int i=0;i<(size-1);i++){
        temp=arr[i];
        arr[i]=arr[i+1];
        arr[i+1]=temp;
    }

}

int array_rotate_right(int arr[],int size){
    int temp;
    for(int i=(size-1);i>1;i--){
        temp=arr[i];
        arr[i]=arr[i-1];
        arr[i+1]=temp;
    }
}