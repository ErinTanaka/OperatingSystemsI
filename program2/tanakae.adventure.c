#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

struct room{
   char name[10];
   int num_connections;
   char connection_list[6][11];
   char type[11];
};
//global mutex because passing betewwn functions wasn't working
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

void FindMostRecentDir(char*);
void GetRoomData(struct room*, char *);
void PlayGame(struct room*);
struct room* GetStartingRoom(struct room*);
bool CheckEndGame(struct room *);
bool UserInputGood(struct room *, char*);
struct room* ChangeRoomTo(char*, struct room *);
void FinishGame(int, char**);
void* WriteTime();
void printtime();

int main(){
   char directory[64];
   //get most recently created directory name
   FindMostRecentDir(&directory[0]);
   
   //allocate memory for list of rooms
   struct room* roomlist=malloc(7*sizeof(struct room));
   int i,j,k;
   for (i=0; i<7; i++){
     memset(roomlist[i].type, '\0',11);
     memset(roomlist[i].name, '\0',11);
      for(j=0; j<6; j++){
         memset(roomlist[i].connection_list[j],'\0',11); 
      }
   }
   //game setup
   GetRoomData(roomlist, directory);
   //game interface and functionality
   PlayGame(roomlist);

   free(roomlist);
   return 0;
}

void FindMostRecentDir(char* savedir){
   int newestDirTime = -1;
   char targetDirPrefix[32] = "tanakae.rooms.";
   char newestDirName[64];
   memset(newestDirName, '\0', sizeof(newestDirName));//fill string with null char

   DIR* dirToCheck;
   struct dirent *fileInDir;
   struct stat dirAttributes;
   dirToCheck = opendir(".");

   if (dirToCheck>0){
      while((fileInDir=readdir(dirToCheck))!= NULL){  //while there are directories in current dir
         if (strstr(fileInDir->d_name, targetDirPrefix)!= NULL){  //if directory exists with prefix "tanakae.rooms"
	    stat(fileInDir->d_name, &dirAttributes);  //get directory's information
	    if((int)dirAttributes.st_mtime > newestDirTime){  //current directorywas created more recently than previously recorded 
	       newestDirTime = (int)dirAttributes.st_mtime;
	       memset(newestDirName, '\0', sizeof(newestDirName));
	       strcpy(newestDirName, fileInDir->d_name);
	    }
	 }
      }
   }
   closedir(dirToCheck);
   strcpy(savedir, newestDirName);
}
void GetRoomData(struct room* list, char * dirname){
   int i;
   char buffer[64];
   sprintf(buffer, "./%s", dirname);
   DIR *workingdir;
   struct dirent *fileindir;
   workingdir=opendir(buffer);
   //make a list of filenames in working directory
   char* filelist[7];
   for(i=0; i<7;i++){
      filelist[i]=malloc(32*sizeof(char));
   }
   int fileindex=0;
   while ((fileindir=readdir (workingdir))!=NULL){
      if(strcmp(fileindir->d_name,".") && strcmp(fileindir->d_name, "..")){
         strcpy(filelist[fileindex], fileindir->d_name);
	 fileindex++;
      }
   }
   
   closedir(workingdir);
  //open each file in file list created above and read contents into struct
  for(i=0; i<7; i++){
      //setup string for filename to be opened
      char curfilename[128];
      memset(curfilename, '\0', 128);
      sprintf(curfilename, "./%s/%s", dirname, filelist[i]);
      char myline[50];
      char* pch;
      char temp[20];
      memset(temp, '\0', 20);

      FILE *curfile;
      curfile = fopen(curfilename, "r");
      if (curfile==NULL) perror("error openingfile");
      else{
         int connection_count=0;
         while(fgets(myline, 32, curfile)!=NULL){  //grab one line from file
            memset(temp, '\0', 20);
            if (strcmp(myline, "ROOM NAME:  ")>0){
	       if(strcmp(myline, "ROOM TYPE:")>0){  //set room type
		  strncpy(temp, &myline[11], 11);
		  pch = memchr(temp, '\n', strlen(temp));
	          strncpy(list[i].type,temp, pch-temp);
	       }
	       else{  //set room name
	          strncpy(temp, &myline[11], 10);
		  pch = memchr(temp, '\n', strlen(temp));
		  strncpy(list[i].name, temp, pch-temp);
	       }
            }
            else if(strcmp(myline, "CONNECTION  ")>0){ //add connection to connection list and increment count
	       strncpy(temp, &myline[14], 11);
	       pch = memchr(temp, '\n', strlen(temp));
	       strncpy(list[i].connection_list[connection_count], temp, pch-temp);
               connection_count++;
               list[i].num_connections = connection_count;	       
            }
            else{
               printf("done goofed\n");
            }  
         }  
         fclose(curfile);  
      }
      free(filelist[i]);
   }
   
}
void PlayGame(struct room* list){

   int resultint;
   pthread_t timethread;
     
   pthread_mutex_lock(&mutex);
   resultint=pthread_create(&timethread, NULL, WriteTime, NULL);
   
   //variables for gameplay
   int i;
   struct room* curr_room;
   int steps=0;
   char* path[100];
   for(i=0;i<100;i++){
      path[i]=malloc(11*sizeof(char));
      memset(path[i], '\0', 11);
   }
   bool reqtime=0; 
   bool end_game;
   bool good_input;
   char user_input[100];
   memset(user_input, '\0', 100);
   curr_room=GetStartingRoom(list);
   //loop for gameplay
   while(true){
      end_game=CheckEndGame(curr_room);
      if (end_game){
	 break;
      }
      //print prompts
      if (reqtime==0){
      printf("CURRENT LOCATION: %s\n", curr_room->name);
      printf("POSSIBLE CONNECTIONS: ");
      for(i=0; i< curr_room->num_connections; i++){
	  if(i<(curr_room->num_connections-1)){
            printf("%s, ", curr_room->connection_list[i]);
         }
         else{
            printf("%s.\n", curr_room->connection_list[i]);  
         }
      }
      }
      reqtime=0;
      printf("WHERE TO? >");
      scanf("%s", user_input);
      
      //check userinput
      good_input=UserInputGood(curr_room, user_input);
       if (good_input){
	 //user requested time
	 if(strcmp(user_input, "time")==0){	    
	   pthread_mutex_unlock(&mutex);//let time thread run
           sleep(1);//give timethread a chance to finish
	   printf("\n");
	   printtime();
	   reqtime=1;
	 }
	 //user requested valid room name
	 else{
         curr_room=ChangeRoomTo(user_input, list);
         strncpy(path[steps], user_input, strlen(user_input));
	 steps=steps+1;
	 memset(user_input, '\0', strlen(user_input));
	 printf("\n");
	 }
      }
      else{
	 //user didnt request valid room name or time
         printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN\n\n");
	 memset(user_input, '\0', strlen(user_input));
      }
  }
  //call endgame  
  FinishGame(steps, path);
  //free path list
  for(i=0; i<100; i++){
     free(path[i]);
  }
}
struct room* GetStartingRoom(struct room* list){
   int i;
   //iterate through roomlist and return list with type start
   for(i=0; i<7; i++){
      if(strncmp(list[i].type, "START", 5)==0){ 
	 return &list[i];
      }
   }
   return NULL;
}
bool CheckEndGame(struct room *curr){
   //check if room is end room
   if(strncmp(curr->type, "END", 3)==0){
      return true;
   }
   //room is not of type end
   return false;
}
bool UserInputGood(struct room * curr, char* input){
   int i;
   //user requested time
      if(strcmp(input, "time")==0){
         return true;
      }
   //iterate through room's connections and check if user req matches any
   for(i=0; i<curr->num_connections; i++){
      if (strncmp(curr->connection_list[i], input, strlen(input))==0){
	 return true;
      }
   }
   //user didn't request time or valid connection
   return false;
}
struct room* ChangeRoomTo(char* roomname, struct room * list){
   int i;
   //iterate through list of rooms and return address of room with name passed in
   for (i=0; i< 7; i++){
      if(strncmp(list[i].name, roomname, strlen(roomname))==0){
         return &list[i];
      }
   }
}
void FinishGame(int steps, char** path){
   int i;
   printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
   printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n",steps);
   //iterate through path list and print each room name
   for (i=0; i<steps; i++){
      printf("%s\n", path[i]);
   }
}
void* WriteTime(){
   
   pthread_mutex_lock(&mutex);
   //days of week and months of year
   char* daylist[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
   char* monthlist[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
   
   while(true){

      FILE * fileptr;
      fileptr = fopen("./currentTime.txt", "w");
      
      //get current time
      time_t t =time(NULL);
      struct tm cur=*localtime(&t);
      
      //am or pm and change 24 hour to 12
      char* label;
      int hours=cur.tm_hour;
      if(hours>12){
         hours=hours-12;
         label="pm";
      }   
      else{
         label="am";
      }
      int minutes= cur.tm_min;
   
      char currenttime[64];
      memset(currenttime, '\0', 64);
      
      //need 0 infront of single digit minutes
      if (minutes<10){
         sprintf(currenttime, "%d:0%d%s, %s, %s %d, %d \n", hours, cur.tm_min, label, daylist[cur.tm_wday], monthlist[cur.tm_mon], cur.tm_mday, cur.tm_year+1900);
      }
      //minutes are two digits
      else{
         sprintf(currenttime, "%d:%d%s, %s, %s %d, %d \n", hours, cur.tm_min, label, daylist[cur.tm_wday], monthlist[cur.tm_mon], cur.tm_mday, cur.tm_year+1900);   
      }
      
      //write time to file
      fwrite(currenttime, sizeof(char)*strlen(currenttime), 1, fileptr);
      
      //close file
      fclose(fileptr);
      
      //lock this thread
      pthread_mutex_lock(&mutex);

   }
}
void printtime(){
   //open time file
   FILE * ptr;
   ptr=fopen("./currentTime.txt", "r");
   
   //read time from opened file and print to screen
   char timestring[64];
   memset(timestring, '\0', 64);
   fgets(timestring, 64, ptr);
   printf("%s\n", timestring);
   
   //close file
   fclose(ptr);
}
