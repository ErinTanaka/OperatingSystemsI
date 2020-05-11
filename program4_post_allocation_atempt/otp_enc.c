#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

void error(const char *msg) {fflush(stderr); perror(msg); exit(0); } // Error function used for reporting issues

void GetText(char* textfile, char* plaintext, int optional){ //function to read data in from provided files
	long sizefile;
	FILE * curfile;
	char * filename;
       //getting file name and opening 
	filename==malloc((2+strlen(textfile))*sizeof(char));
        memset(filename, '\0', strlen(filename));
        sprintf(filename, "./%s", textfile);
        curfile=fopen(filename, "r"); //open for reading
        if(curfile==NULL) perror("error opening file"); //error check opening file
        
	//getting num chars in file
	fseek(curfile, 0, SEEK_END);
	sizefile=ftell(curfile);
//printf("file has %d chars\n", sizefile);
	//if(strlen(plaintext)<sizefile){
	fseek(curfile, 0, SEEK_SET);
        if(optional==0){	  
       	   plaintext=realloc(plaintext, (sizefile+8)* sizeof(char));
	   fgets(plaintext, sizefile, curfile);
        }
	else{
       	   plaintext=realloc(plaintext, (optional+8)* sizeof(char));
	   fgets(plaintext, optional+7, curfile);
	   strcat(plaintext, "\0");
	}
fclose(curfile); //close file

}

void RemoveNewLineChar(char* str_a, char* str_b){ //function to remove newline charater at end of string
   int i;
   char* pch;
   pch=memchr(str_a, '\n', strlen(str_a)); //locate \n
   if (pch!=NULL){
      memset(pch, '\0',1); //replace \n with \0
   }
   pch=memchr(str_b, '\n', strlen(str_b)); 
   if(pch !=NULL){
      memset(pch, '\0',1);
   }
}

bool CheckChars(char* string){ // function to verify strings only contain capital letters or spaces
   int i;
   for(i=0; i<strlen(string); i++){
      if (isalpha(string[i])==false && strncmp(&string[i], " ", 1)!=0 ){ //current char is not a letter or space
//	 printf("not letter or space %d: %c : %d\n", i, string[i], string[i]);   
	 return false;
      }
      else if(islower(string[i])){ //letter is lowercase
//	 printf(" lowwer case %d: %c : %d\n", i, string[i], string[i]);   
            return false;	 
      }
   }
   return true; //entire string passed test
}

bool GoodKeyLength(char*plaintext, char* key){ //function to verify that key is of equal or greater length than plaintext
   if(strlen(plaintext)>strlen(key)){
//      printf("plaintext: %d key: %d \n", strlen(plaintext), strlen(key));
      return false;
   }
   return true;
}



int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char * buffer;
	char * key;
	char * plaintext;
    
	if (argc < 4) { fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]); exit(0); } // Check usage & args
	buffer=calloc(75000, sizeof(char));
        key=calloc(75000, sizeof(char));
	plaintext=calloc(75000, sizeof(char));
	GetText(argv[1], plaintext, 0);
	GetText(argv[2], key, strlen(plaintext));
//int i;
//	for(i=0; i< strlen(key);i++){
//	   printf("i=%d  ascii: %d  char: %c\n", i, key[i], key[i]);
//	}
//printf("plaintext: %s\n ", plaintext);
//printf("key: %s\n", key);
//	int i;
//	for(i=0; i< strlen(plaintext);i++){
//	   printf("i=%d  ascii: %d  char: %c\n", i, plaintext[i], plaintext[i]);
//	}
	RemoveNewLineChar(plaintext, key);
//        printf("past newlin fn\n");
	if(CheckChars(plaintext)==false){
	   sprintf(buffer, "invalid characters found in %s", argv[1]);
	   error(buffer);
	   //exit(1);
	}
	if(CheckChars(key)==false){
	   sprintf(buffer, "invalid characters found in %s", argv[2]);
	   error(buffer);
	   //exit(1);
	}
	if(GoodKeyLength(plaintext, key)==false){
	   error("Key doesn't meet length requirements");
	   //exit(1);
	}
//	printf("passed checks for length and chars\n");
        const char temphostname[]="localhost";	
	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname(temphostname); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");

	
	//format data for sending
	if(strlen(buffer)<strlen(plaintext)+strlen(key)+8){
	   buffer=realloc(buffer, (strlen(plaintext)+strlen(key)+64) * sizeof(char));
	}
	memset(buffer, '\0', sizeof(buffer));
	sprintf(buffer, "%d%s%s@@", strlen(plaintext), plaintext, key);

printf("buffer: %s\n", buffer);
	charsWritten = send(socketFD, buffer, strlen(buffer), 0);
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");
	
	
	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	char readbuffer[sizeof(buffer)];
	while(strstr(readbuffer, "@@")==NULL){
	   memset(readbuffer, '\0', strlen(readbuffer));
	   charsRead = recv(socketFD, readbuffer, strlen(readbuffer) - 1, 0); // Read data from the socket, leaving \0 at end
	   strcat(buffer, readbuffer);
	   if (charsRead < 0) error("CLIENT: ERROR reading from socket"); break;
	   if (charsRead ==0) printf("recieved none\n"); break;
	}

	if(strncmp(buffer,"***error",8)==0){
	   if(strncmp(buffer, "***error ", 9)==0){
	      error("Wrong Server");
	   }
	   else{
	      perror("encryption failed");
	   }
	}
        //remove end code from received string
	char * pch;
	pch=strstr(buffer, "@@");
	strncpy(pch,"\0\0", 2);
        	
	//write to stdout
	fflush(stdout);
	fprintf(stdout, "%s\n", buffer);
	fflush(stdout);
	close(socketFD); // Close the socket
	
	free(buffer);
	free(key);
	free(plaintext);
	return 0;
}
