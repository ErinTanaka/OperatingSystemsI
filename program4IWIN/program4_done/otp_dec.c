#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

void error(const char *msg) { fprintf(stderr, msg); exit(0); } // Error function used for reporting issues

void GetCiphertextAndKey(char* textfile, char* keyfile, char** ciphertext, char** key){ //function to read data in from provided files
        FILE * curfile;
	long filesize;
   	char * filename;
	char* tmp;
	filename=calloc((8+strlen(textfile)),sizeof(char));
        
	//get line of text from ciphertext file
	sprintf(filename, "./%s\0", textfile);
        curfile=fopen(filename, "r"); //open for reading
        if(curfile==NULL) error("error opening file"); //error check opening file
        //get size of file
	fseek(curfile, 0, SEEK_END);
	filesize=ftell(curfile);
	fseek(curfile, 0, SEEK_SET);
	//reallocate memory if needed
	if(63<filesize){
	   	free(*ciphertext);
	   	*ciphertext=calloc(filesize+1, sizeof(char));
	}
	fgets(*ciphertext, filesize ,curfile);
        fclose(curfile); //close file
        free(filename);
	
	//get line of text from key file
	filename=calloc(8+strlen(keyfile),sizeof(char));
	sprintf(filename, "./%s\0", keyfile);
	curfile=fopen(filename, "r"); //open for reading
        if(curfile==NULL) error("error opening file"); //error check opening file
        //get file's size
	fseek(curfile, 0, SEEK_END);
	filesize=ftell(curfile);
        fseek(curfile,0, SEEK_SET);	
	//reallocate memory if needed
	if(63<filesize){
	   	free(*key);
	   	*key=calloc(filesize+1, sizeof(char));
	}
	fgets(*key, filesize ,curfile); 
        fclose(curfile); //close file	
	free(filename);
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

bool GoodKeyLength(char*ciphertext, char* key){ //function to verify that key is of equal or greater length than ciphertext
   	if(strlen(ciphertext)>strlen(key)){
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
    
	if (argc < 4) { fprintf(stderr,"USAGE: %s ciphertext key port\n", argv[0]); exit(0); } // Check usage & args

	//err check key and plain text
        char * ciphertext=malloc(64*sizeof(char));
	memset(ciphertext, '\0', 64);
        char * key=malloc(64*sizeof(char));
	memset(key, '\0', 64);

	GetCiphertextAndKey(argv[1], argv[2], &ciphertext, &key);
        RemoveNewLineChar(ciphertext, key);

	if(CheckChars(ciphertext)==false){
	   	fprintf(stderr,"Invalid characters found in ciphertext file");
	   	exit(1);
	}
	if(CheckChars(key)==false){
	   	fprintf(stderr, "Invalid characters found in key file");
	   	exit(1);
	}
	if(GoodKeyLength(ciphertext, key)==false){
	   	fprintf(stderr,"Key doesn't meet length requirements");
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

	
	sprintf(buffer, "%d%s%s$$\0",strlen(ciphertext), ciphertext, key);
	
	charsWritten = send(socketFD, buffer, strlen(buffer), 0);
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");	
	
	// Get return message from server
	memset(buffer, '\0', 150000); // Clear out the buffer again for reuse
	char readbuffer[150000];
        
	while(1){
	   	memset(readbuffer, '\0', 150000);
	   	if(strstr(buffer, "@@")!=NULL || strstr(buffer, "$$")!=NULL){
	      		break;
	   	}
	   	charsRead = recv(socketFD, readbuffer, 150000, 0); // Read data from the socket, leaving \0 at end
	   	strcat(buffer, readbuffer);
	   	if (charsRead < 0) {error("CLIENT: ERROR reading from socket"); break;}
	   	if (charsRead ==0) {printf("recieved none\n"); break;}
	}

	if(strncmp(buffer, "***", 3)==0){ //encryption didn't happen or something went wrong
	   	close(socketFD); // Close the socket
           	free(buffer);	
	   	free(key);
	   	free(ciphertext);
	   	return 0;
             	   
	}
       //remove end code from received string
	char * pch;
	pch=strstr(buffer, "$$");
	if(pch!=NULL){
	   	strncpy(pch,"\0\0", 2);
	}

	//write to stdout
        fflush(stdout);
	fprintf(stdout, "%s\n", buffer);
	
	close(socketFD); // Close the socket
  //      printf("socket closed\n");
	free(buffer);	
	free(key);
	free(ciphertext);
	return 0;
}
