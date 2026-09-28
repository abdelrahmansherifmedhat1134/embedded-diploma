#include<stdio.h>

void swap(int*a,int*b){
    *a=*a^*b;
    *b=*a^*b;
    *a=*a^*b;
}
/*
void swap(int*arr,int size){
    for(int i=0;i<size(arr);i++){
        int min=0;
        for(int m=0;m<size(arr);m++){
            if min<arr9        }
        
    }
}
*/
void binary_search(int *arr,int start,int end,int element){
    if(element==arr[(int)(start+end/2)]){
        return ();
    }
    else if(element<arr[end/2]){
        return end/2;
    }
    

}

int main(){
    int arr[4]={1,2,3,4};
    printf("element in index %d\n",binary_search(0,3,3));

}