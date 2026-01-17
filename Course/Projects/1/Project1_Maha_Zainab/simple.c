#include <stdio.h>
#include <math.h>

int main(){
double numbers[10]={27,56,88,12,35,5,61,68,24,10};
double sum=0.0;
double average;
int i;
for(i=0; i<10;i++){
sum +=sqrt(numbers[i]);
}
average =sum/10;
printf("Final average of the square root numbers=%f\n", average);
return 0;
}
