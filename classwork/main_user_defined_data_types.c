#include<stdio.h>
#include<stdlib.h>
#include<string.h>
struct point{
    float x;
    float y;
    
}p1,p2;

void display_point(struct point p){
    printf("(%f,%f)",p.x,p.y);
}

/*
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

struct student{
    int id;
    char name[20];
    int age;
    float grade; 
};
void display_student_info{
    printf(name:%s)
}

int main(){
    struct student s1={2300100,"abdelrahman sherif",20,3.66};
    struct student s2;
    s2.id=2300150;
    s2.name="habiba haytham";
    s2.age=20;


}