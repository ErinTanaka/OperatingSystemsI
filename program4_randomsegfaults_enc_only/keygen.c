#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
int main(int argc, char** argv){
   srand(time(NULL));
   int i;   
   int keylen= atoi(argv[1]);
   //printf("keylen= %d\n", keylen);
   char* key= malloc(keylen * sizeof(char));
   //char key[keylen];
   for (i=0; i<keylen; i++){
      int temp=rand() % 27 + 65;
      if (temp==91){//use space, not ascii
         key[i]=32;
      }
      else{
         key[i]=temp;
      }
   }
   //printf("length of key: %d\n", strlen(key)+1);
   fflush(stdout);
   fprintf(stdout, "%s\n", key);
   free(key);
   return 0;
}
