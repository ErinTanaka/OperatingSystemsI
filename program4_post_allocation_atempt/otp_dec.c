#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

void GetText(char* textfile, char* ciphertext){ //function to read data in from provided files
   	long sizefile;     
        char * filename;
	FILE * curfile;
        //getfilename and open
	filename=malloc((2+strlen(textfile))*sizeof(char));
        memset(filename, '\0', sizeof(filename));
	sprintf(filename, "./%s", textfile);
        curfile=fopen(filename, "r"); //open for reading
        if(curfile==NULL) perror("error opening file"); //error check opening file
        fseek(curfile,0,SEEK_END);
	sizefile=ftell(curfile);
	ciphertext=realloc(ciphertext, (sizefile+8*sizeof(char)));
	fseek(curfile, 0, SEEK_SET);
	fgets(ciphertext, sizeof(ciphertext)-1 ,curfile);
	fclose(curfile); //close file
  //      free(filename);
/*	
	//get line of text from key file
	filename=malloc((2+strlen(keyfile)*sizeof(char)));
	memset(filename, '\0', sizeof(filename));
	sprintf(filename, "./%s", keyfile);
	curfile=fopen(filename, "r"); //open for reading
        if(curfile==NULL) perror("error opening file"); //error check opening file
        //while(strstr(key, "\n")==NULL){
	  // memset(key, '\0', sizeof(key));
	   fgets(key, sizeof(key)-1 ,curfile);
	   //moretext=realloc(key, sizeof(key)*2);
	   //key=moretext;
	//}
        fclose(curfile); //close file	
	free(filename);
*/
}

void RemoveNewLineChar(char* str_a, char* str_b){ //function to remove newline charater at end of string
   int i;
   char* pch;
   pch=memchr(str_a, '\n', strlen(str_a)); //locate \n
   if(pch!=NULL){
      memset(pch, '\0',1); //replace \n with \0
   }
   pch=memchr(str_b, '\n', strlen(str_b)); 
   if(pch!=NULL){
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

bool GoodKeyLength(char*plaintext, char* key){ //function to verify that key is of equal or greater length than ciphertext
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
	char * buffer=malloc(75000*sizeof(char));
    
	if (argc < 4) { fprintf(stderr,"USAGE: %s ciphertext key port\n", argv[0]); exit(0); } // Check usage & args

	//err check key and ciphertext
        char * ciphertext=malloc(75000*sizeof(char));
	memset(ciphertext, '\0', sizeof(ciphertext));
        char * key=malloc(75000*sizeof(char));
	memset(key, '\0', sizeof(key));

	GetText(argv[1], ciphertext);
	GetText(argv[2], key);
        RemoveNewLineChar(ciphertext, key);

	if(CheckChars(ciphertext)==false){
	   perror("Invalid characters found in ciphertext file");
	   exit(1);
	}
	if(CheckChars(key)==false){
	   perror("Invalid characters found in key file");
	   exit(1);
	}
	if(GoodKeyLength(ciphertext, key)==false){
	   perror("Key doesn't meet length requirements");
	   exit(1);
	}
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

       //printf("sending to server\n");
	 if (sizeof(buffer)<strlen(ciphertext)+strlen(key)+8){
	    buffer=realloc(buffer, (strlen(ciphertext)+strlen(key)+64));
	 }
	memset(buffer, '\0', sizeof(buffer));
	sprintf(buffer, "%d%s%s$$",strlen(ciphertext), ciphertext, key);
	
	charsWritten = send(socketFD, buffer, sizeof(buffer), 0);
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");
	
	
	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	char readbuffer[sizeof(buffer)];
	while(strstr(readbuffer, "$$")==NULL){
	   memset(readbuffer, '\0', sizeof(readbuffer));
	   charsRead = recv(socketFD, readbuffer, sizeof(readbuffer) - 1, 0); // Read data from the socket, leaving \0 at end
	   strcat(buffer, readbuffer);
	   if (charsRead < 0) error("CLIENT: ERROR reading from socket"); break;
	   if (charsRead ==0) printf("recieved none\n"); break;
	}
        //printf("CLIENT: got something from server: %s\n", buffer);
        if(strncmp(buffer, "***error", 8)==0){
	  if(strcmp(buffer, "***error wong server$$")==0){
	     error("wrong server");
	  }
	  else{
	   perror("decryption failed");
	  }
	}
	else{
	//remove end code from received string
	char * pch;
	pch=strstr(buffer, "$$");
	strncpy(pch,"\0\0", 2);
	
	//write to stdout
	fprintf(stdout, "%s\n", buffer);
	}
	close(socketFD); // Close the socket
	free(buffer);
	free(key);
	free(ciphertext);
	return 0;
}
