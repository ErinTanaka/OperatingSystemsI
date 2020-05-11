#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/ioctl.h>
void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

void GetPlaintextAndKey(char* textfile, char* keyfile, char** plaintext, char** key){ //function to read data in from provided files
        FILE * curfile;
	long filesize;
   	char * filename;
	char* tmp;
//printf("getplntxtandkeyfn\n");
	filename=calloc((8+strlen(textfile)),sizeof(char));
        
	//get line of text from plaintext file
	sprintf(filename, "./%s\0", textfile);
        curfile=fopen(filename, "r"); //open for reading
        if(curfile==NULL) error("error opening file"); //error check opening file
        fseek(curfile, 0, SEEK_END);
	filesize=ftell(curfile);
	fseek(curfile, 0, SEEK_SET);
	if(63<filesize){
	   free(*plaintext);
	   *plaintext=calloc(filesize+1, sizeof(char));
	}
	fgets(*plaintext, filesize ,curfile);
        fclose(curfile); //close file
        free(filename);
	
	//get line of text from key file
	filename=calloc(8+strlen(keyfile),sizeof(char));
	sprintf(filename, "./%s\0", keyfile);
	curfile=fopen(filename, "r"); //open for reading
        if(curfile==NULL) error("error opening file"); //error check opening file
        fseek(curfile, 0, SEEK_END);
	filesize=ftell(curfile);
        fseek(curfile,0, SEEK_SET);	
	if(63<filesize){
	   //printf("reallocating for key\n");
	   free(*key);
	   *key=calloc(filesize+1, sizeof(char));
	   //*key=tmp;
	}
	fgets(*key, filesize ,curfile); 
        fclose(curfile); //close file	
	free(filename);
//	printf("plaintext: %d\nkey: %d\n", strlen(*plaintext),strlen( *key));
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
	    return false;
      }
      else if(islower(string[i])){ //letter is lowercase
            return false;	 
      }
   }
   return true; //entire string passed test
}

bool GoodKeyLength(char*plaintext, char* key){ //function to verify that key is of equal or greater length than plaintext
   if(strlen(plaintext)>strlen(key)){
      return false;
   }
   return true;
}



int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char* buffer=calloc(150000, sizeof(char));
    
	if (argc < 4) { fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]); exit(0); } // Check usage & args

	//err check key and plain text
        char * plaintext=malloc(70000*sizeof(char));
	memset(plaintext, '\0', 70000);
        char * key=malloc(70500*sizeof(char));
	memset(key, '\0', 70500);

	GetPlaintextAndKey(argv[1], argv[2], &plaintext, &key);
        RemoveNewLineChar(plaintext, key);

	if(CheckChars(plaintext)==false){
	   perror("Invalid characters found in plaintext file");
	   exit(1);
	}
	if(CheckChars(key)==false){
	   perror("Invalid characters found in key file");
	   exit(1);
	}
	if(GoodKeyLength(plaintext, key)==false){
	   perror("Key doesn't meet length requirements");
	   exit(1);
	}
	printf("client: passed checks for length and chars\n");
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

//	int sizebuffer=256;
	//format data for sending
//	if(256<strlen(key)+strlen(plaintext)+16){
//	   free(buffer);
//	   buffer=calloc(strlen(key)+strlen(plaintext)+16, sizeof(char));
//	   sizebuffer=strlen(key)+strlen(plaintext)+16;
//	}
	
	sprintf(buffer, "%d%s%s@@\0",strlen(plaintext), plaintext, key);
//    printf("buffer: %s\n", buffer);
        
	//tell server how many chars to expect
//	char* lenbuff=calloc(16, sizeof(char));
//	fflush(stdout);
//	sprintf(lenbuff, "%d@@\0", strlen(buffer));
printf("CLIENT: sending shit\n");
	
	charsWritten = send(socketFD, buffer, strlen(buffer), 0);
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");
printf("CLIENT: passed send\n");
	int checkSend = -5;
	do{
	   ioctl(socketFD, TIOCOUTQ, &checkSend);

	}while(checkSend>0);
	if(checkSend<0)error("ioctl error");
	
	
	//printf("charswritten: %d", charsWritten);	
printf("CLIENT: time to recieve...\n");	
	// Get return message from server
	memset(buffer, '\0', 500000); // Clear out the buffer again for reuse
	char readbuffer[500000];
	while(1){
	   if(strstr(buffer, "@@")!=NULL){
	      printf("CLIENT:break the recv loop");
	      break;
	   }
	   memset(readbuffer, '\0', sizeof(readbuffer));
	   charsRead = recv(socketFD, readbuffer, sizeof(readbuffer) - 1, 0); // Read data from the socket, leaving \0 at end
	   strcat(buffer, readbuffer);
	   if (charsRead < 0) error("CLIENT: ERROR reading from socket"); break;
	   if (charsRead ==0) printf("recieved none\n"); break;
	}
printf("CLIENT: recved shit back\n");
        //remove end code from received string
	char * pch;
	if(pch!=NULL){
	pch=strstr(buffer, "@@");
	strncpy(pch,"\0\0", 2);
	}

	//write to stdout
fflush(stdout);
	fprintf(stdout, "%s\n", buffer);
	
	close(socketFD); // Close the socket
        free(buffer);	
	free(key);
	free(plaintext);
	return 0;
}
