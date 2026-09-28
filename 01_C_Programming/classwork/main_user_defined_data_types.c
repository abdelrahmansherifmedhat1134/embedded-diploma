#include<stdio.h>
#include<stdlib.h>
#include<string.h>

/*
struct point{
    float x;
    float y;
    
}p1,p2;

void display_point(struct point p){
    printf("(%f,%f)",p.x,p.y);
}


int main(){
    
    p1.x=10;
    p1.y=10;
    struct point p2={.x=1, .y=1};
    struct point p3={20,20};
    
    printf("point1 :(%f,%f)\n",p1.x,p1.y);
    printf("point2 :");
    display_point(p2);
       
}
*/
/*
struct student{
    char id[8];
    char name[20];
    int age;
    float grade; 
};

//dont initialize a string with the same size as what you wnat to store in it 
//because there will be no room left for the null terminator 


//nested structs 

struct nested_parent{
    int m1;
    struct nested_child{
        int m2;
        char c2;
    };
};
struct child{
    int x;
    int y;
};
struct parent{
    int esmo;
    struct child a;
};

int main(){
    struct student s1={"2300100","abdelrahman sherif",20,3.66};
    printf("name: %s and age: %d id: %s done\n",s1.name,s1.age,s1.id);
    struct student s2={.age=10,.id="2300500",.name="hassan"};
    printf("name: %s and age: %d id: %s done\n",s2.name,s2.age,s2.id);
    struct nested_parent sx={100};
    struct nested_child sy;
    sy.m2=44;
    printf("parent: %d\n",sy.m2);
    struct parent p1={100,50,25};
    printf("p1.esmo: %d p1.a.x: %d p1.a.y: %d\n",p1.esmo,p1.a.x,p1.a.y);

    
    



}
*/
//size of structs
struct str1{
    char a;
    int i;
    char x;
};
struct str2{
    char c;
    int i;
    char x;
} __attribute((packed))__;

int main(){
    printf("struct 1 size: %d\n",sizeof(struct str1));
    
    printf("struct 2 size: %d\n",sizeof(struct str2));
}