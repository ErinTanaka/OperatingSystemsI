#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <stdbool.h>
void* MyFunction(int num){
   printf("myfunction %d\n", num);
};



int main(){
   char * daysofwk[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
   char* monthsofyr[12]= {"January","February", "March","April", "May", "June", "July", "August", "September", "October", "November", "December"};
   time_t t =time(NULL);
   struct tm cur= *localtime(&t);
   char * label;
   int hours=cur.tm_hour;

   if (hours>12){
      hours=hours-12;
      label="pm";
   }
   else{
      label="am";
   }
   printf("%d:%d%s, %s, %s %d, %d \n", hours, cur.tm_min, label, daysofwk[cur.tm_wday], monthsofyr[cur.tm_mon], cur.tm_mday, cur.tm_year+1900);

return 0;
}
