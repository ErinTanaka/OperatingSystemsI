#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct room {
   char* name;
   int num_connections;
   char** connection_list;
   char* type;
};

void InitialSetup(struct room *);
bool IsGraphFull(struct room*);
void AddRandomConnection(struct room*);
struct room* GetRandomRoom(struct room*);
bool CanAddConnectionFrom(struct room*);
void ConnectRooms(struct room*, struct room*);
bool IsSameRoom(struct room*, struct room*);
void Connect (struct room*, struct room*);
bool ConnectionAlreadyExists (struct room*, struct room*);
void WriteFile(struct room*);

int main(){
   srand (time(NULL));
   
   //allocate 7 structs, one for each room
   struct room* roomarray= (struct room *)malloc(7*sizeof(struct room));
   
   //give each "room" a name, type and initialize connections
   InitialSetup(roomarray);

   //add connections
   while (IsGraphFull(roomarray)==false){
      AddRandomConnection(roomarray);
   }

   //make room files
   WriteFile(roomarray);
   
   //free malloc'd stuff
   int i,j;
   for(i=0; i<7; i++){
      for(j=0; j<6; j++){
	free(roomarray[i].connection_list[j]);
      }
      free(roomarray[i].connection_list);
   }
   free(roomarray);
   
return 0;
}
/***************************************************
 * Function Name:InitialSetup
 * Description: gives each of the 7 rooms a name and type by randomly generating a number between 0 and 6.
 * Parameters:pointer to beginning of array of struct rooms
 * ************************************************/
void InitialSetup(struct room * roomarray){  
   char* rm_name_list[10]= {"Waterfall\0", "Pond\0", "Forest\0", "Meadow\0", "Desert\0", "Cliff\0", "Cave\0", "Mountain\0", "Ocean\0", "Farmland\0"};
   int namecheck[10]; //array for checking off which room names have been used
   int i, j;
   for (i=0; i<10; i++){//zerofill name check
      namecheck[i]=0;
   }
   //setting room names, initializing num_connections to zero, and allocating space for connection list
   for (i=0; i<7; i++){
      int nameindex= rand() % 10;
      if (namecheck[nameindex]==0){
         roomarray[i].name=rm_name_list[nameindex];
	 namecheck[nameindex]=1;
	 roomarray[i].num_connections=0;
	 roomarray[i].connection_list=malloc(6*sizeof(char*));
	 for(j=0; j<6; j++){
	    roomarray[i].connection_list[j]=malloc(10*sizeof(char));
	 }
      }
      else i--;   
   }
   //setting room types
   int start= rand() % 7;
   int end;
   do{
      end=rand() % 7;
   }while (end==start);

   for(i=0; i<7; i++){
      if(i==start){
         roomarray[i].type="START_ROOM";
      }
      else if (i==end){
         roomarray[i].type="END_ROOM";
      }
      else{
         roomarray[i].type="MID_ROOM";
      } 
   }
}
/***************************************************
 * Function Name: IsGraphFull
 * Description: iterates through list of rooms, if all rooms have at least 3 connections the fnction returns true, if any rooms have less than three rooms the function returns false
 * parameters: list of rooms
 * *************************************************/
bool IsGraphFull(struct room* list){
   int i;
   for (i=0; i<7; i++){
      if (list[i].num_connections < 3){
	 return false;
      }
   }
return true;
}
/******************************************************
 * Function: AddRandomConnection
 * Description: randomly chooses two rooms and connects them
 * Parameters: list of rooms
 * ***************************************************/ 
void AddRandomConnection(struct room* list){
   struct room* A;
   struct room* B;
   //call function to get a rando room and continue to call until room is able to add another connection
   while(true){
      A=GetRandomRoom(list);
      if (CanAddConnectionFrom(A)==true){
         break;
      }
   }
   //call function to get second rand room cont call while b is the same room as a, they already have a connection or b cant add another connection
   do{
      B=GetRandomRoom(list);
   }while(CanAddConnectionFrom(B)==false || IsSameRoom(A,B)==true || ConnectionAlreadyExists(A,B)==true);
   //call function to connect the two rooms
   ConnectRooms(A,B);
}
/**********************************************
 * Function: GetRandomRoom
 * Description: Generates "random" number between 0 and 6, returns room in list at index of random num
 * Parameters: list of rooms
 * *********************************************/
struct room* GetRandomRoom(struct room* list){
   int x= rand() % 7;
   struct room * temp = &list[x];
   return temp; 
}
/********************************************
 * Function: CanAddConnectionFrom
 * Description: returns true if room has less than 6 connections
 * parameters: room
 * ******************************************/
bool CanAddConnectionFrom(struct room* x){
   //can add another connection
   if(x->num_connections<6){
      return true;
   }
   //no more connections
   return false;
}
/**************************************************
 * Function: ConnectRooms
 * Description: connects two rooms in list, updaes number of connections and adds room nme ot connection list
 * Parameters: two rooms
 * **************************************************/
void ConnectRooms(struct room* x,struct room* y){
   //add room y's name to x's connection list and inc x's connection count
   strcpy(x->connection_list[x->num_connections], y->name);
   x->num_connections++;   

   //add room x's name to y's connection list and inc y's connection count
   strcpy(y->connection_list[y->num_connections], x->name);
   y->num_connections++;
}
/**********************************************
 * Function: IsSameRoom 
 * Description: compares names of two rooms, returns true if fooms have same name
 * Parameters: pointers to two rooms
 * ******************************************/
bool IsSameRoom(struct room* x, struct room* y){
   //x and y are the same room
   if (strcmp(x->name, y->name)==false){
      return true;
   }
   //rooms are different
   return false;
}
/*************************************************
 * Function: ConnectionAlreadyExists
 * Description: returns true if room x and y are already connected
 * parameters: pointers to two rooms
 * ***********************************************/
bool ConnectionAlreadyExists(struct room* x, struct room* y){
   int i;
   for(i=0; i< x->num_connections; i++){//iterate through x's connection list
      if (strcmp(x->connection_list[i], y->name)==0){//rooms already connected?
         return true;
      }
   }
   //y is not in x's connection list
   return false;

} 

/*********************************************
 * Function: WriteFile
 * Description: makes new directory with proces id and generates files based off of list of rooms;
 * parameters: list of rooms
 * ******************************************/
void WriteFile(struct room * list){
   int x, i;
   //make directory with process id
   char dirname[50];
   char* prefix = "tanakae.rooms.";
   int process_id = getpid();
   sprintf(dirname, "%s%d", prefix, process_id);
   int result = mkdir(dirname, 0775);
   
   //setup string of stuff to write to file
   for(x=0; x<7; x++){
      //create and open file
      char filename[50];
      sprintf (filename, "./%s/%s_room", dirname, list[x].name);
      FILE * fileptr;
      fileptr = fopen(filename,"w");
     
      //write roomname to file
      char lineone[50];
      sprintf (lineone, "ROOM NAME: %s\n", list[x].name);  
      fwrite(lineone, sizeof(char)*strlen(lineone), 1, fileptr);
      
      //write connections to file  
      char connections[50];
      for (i=0; i<list[x].num_connections; i++){
         sprintf(connections, "CONNECTION %d: %s\n", (i+1), list[x].connection_list[i]);
         fwrite(connections, sizeof(char)*strlen(connections), 1, fileptr);
      }
      //write type to file
      char type[50];
      sprintf(type, "ROOM TYPE: %s\n", list[x].type);
      fwrite(type, sizeof(char)*strlen(type), 1, fileptr);
           
      fclose(fileptr);
   }
}
