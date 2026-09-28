#include<stdio.h>
//excercise 1
/*
int main(){
	int number1,number2;
	printf("enter number 1: ");
	scanf("%d",&number1);
	printf("enter number 2: ");
	scanf("%d",&number2);
	number1 > number2 ? printf("the max is %d",number1):printf("the max is %d",number2);
	return 0;
}
*/

//excercise 4
/*
int main(){
	int number1,number2,number3;
	printf("enter number1: ");
	scanf("%d",number1);
	printf("enter number2: ");
	scanf("%d",number2);
	printf("enter number3: ");
	scanf("%d",number3);
	
	if(number1>number2 && number1>number3)
		printf("the biggest number is: %d",number1);
	if(number2>number1 && number2>number3)
		printf("the biggest number is: %d",number1);
	if(number3>number1 && number3>number2)
		printf("the biggest number is: %d",number1);
	return 0;
}
*/
//excercise 5
/*
int main(){
	int year;
	printf("input year: ");
	scanf("%d",&year);
	if(year%4==0 && year%100!=0)
		printf("%d is a leap year",year);
	else if(year%400==0)
		printf("%d is a leap year",year);
	else
		printf("%d is not a leap year",year);
}
*/

int main(){
	char character;
	printf("enter character: ");
	scanf("%c",&character);
	if(int(character)>64 && int(character)<91 || int(character)>96 && int(character)<123)
		printf("it is ALPHABET");
	else 
		print("it is not ALPHABET");
	return 0;
}