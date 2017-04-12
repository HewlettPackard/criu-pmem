#include <stdio.h>
#include <unistd.h>

void main( )  {

 int counter = 0;
 while(1) {
   printf("%d\n",counter++);
   sleep(1);
 }
}
