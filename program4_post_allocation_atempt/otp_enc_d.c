#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues
//encryption fns

void ProcessData(char* buffer, char * plaintext, char * key){ //function to process incoming data
	char * moretext=NULL;
	//get length of plaintext/integer at beginning of buffer
	int i=0;
	char lenbuffer[16];
	memset(lenbuffer, '\0', 16);
	while(isdigit(buffer[i])){
	   strncat(lenbuffer, &buffer[i], 1);		     
	   i++;		  
	}		  
	int mssglen=atoi(lenbuffer);
//	printf("mssg len: %d\n", mssglen);
			  
	//get plaintext
	int x;
	for(x=strlen(lenbuffer); x<mssglen+strlen(lenbuffer); x++){
	   strncat(plaintext, &buffer[x], 1);
	}
	
	//calculate startingpoint for key
	int a=strlen(lenbuffer)+mssglen;
	
	//calculate stopping location
	int y=(mssglen+mssglen+strlen(lenbuffer));

	for(x=a; x<y; x++){
	   strncat(key, &buffer[x],1);
	}
	strncat(key, "\0",1);
//printf("key: %s\n", key);
//printf("%s :: %s \n", plaintext, key);
}

char* ReplaceSpace(char* str){ //function to replace all spaces " " with open bracket "[" to make calc easier
	if(str==NULL){ //error check str contains something
	   return str;
	}
	if(memchr(str, ' ', strlen(str))==NULL){ //"err check" no spaces exist in str
	   return str;
	}
	int i;
	for(i=0; i< strlen(str); i++){ //iterate through string char by char
	   if(strncmp(&str[i], " ", 1)==0){ //replace " " w/ "["
	      strncpy(&str[i], "[", 1);
	   }
	}
	return str;
}

char* ReplaceBracket(char* str){ //function to replace all "["s with " "s
	if(str==NULL){ //check if str contains anything
	   return str;
	}
	if(memchr(str, '[', strlen(str))==NULL){ //chech if any "["s occur in str
	   return str;
	}
	int i;
	for(i=0; i< strlen(str); i++){ //iterate through str char by char
	   if(strncmp(&str[i], "[", 1)==0){ //replace "[" w/ " "
	      strncpy(&str[i], " ", 1);
	   }
	}
	return str;

}
void EncodeMssg(char* plaintext, char* key, char* dest){ //function to encrypt plain text into cipher text
	int i;
	for(i=0; i<strlen(plaintext); i++){ //do calculations one character at a time
	   //get numerical val 1-27 for A-Z and "space"
	   //add the two numbers(one from plaintext, one from key) together
	   //mod the sum by 27
	   //add 65(ascii vall of 'A') to modded sum to get Capital Letter or ascii val 91
	   //the 91 gets switched to a space in the RePlaceBracket fn
	   dest[i]=(((plaintext[i]-65)+(key[i]-65))%27)+65; 
//printf("%d : %d : %c\n", i, dest[i], dest[i]);
	}
}

int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char * buffer=malloc(75000*sizeof(char));
	struct sockaddr_in serverAddress, clientAddress;

	
	char *  plaintext=malloc(75000*sizeof(char));
	memset(plaintext,'\0',75000);
	char * key=malloc(75000*sizeof(char));
	memset(key,'\0',75000);

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
        

        /*makeith the loopy thingy here?*/
	while(1){
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("ERROR on accept");

        
		//getting stuff from client
		char * moretext =NULL;
		char *readbuffer=malloc(75000*sizeof(char));
		memset(buffer, '\0', sizeof(buffer));
		while(strstr(readbuffer, "@@")==NULL || strstr(readbuffer, "$$")==NULL){
			//memset(readbuffer, '\0', sizeof(readbuffer));
			charsRead = recv(establishedConnectionFD, readbuffer, strlen(readbuffer)-1, 0); //read clients mssg from socket
		//need to think about reallocation here	
			strcat(buffer, readbuffer);
			if (charsRead<0) error("ERROR on accept"); break;
			if (charsRead==0) printf("received none\n");break;
			printf("looping\n");
		}
//printf("CLIENT: buffer: %s\n", buffer);
fflush(stdout);
//exit(1);

        	//***fork process to do encryption
        	pid_t spawnPid=-3;
		int childexitstatus=-3;
        	spawnPid=fork();
		switch (spawnPid){
			case -1:{
			   	perror("Hull Breach, fork failed\n");
			   	return 1;
			   	exit(1);
			}
			case 0:{ //child process, encrypt message here
                          
			  	//verify speaking to otp_enc: end code will be "@@" for messages from otp_enc and "$$" for otp_dec
			  	if(strstr(buffer, "@@")==NULL){
			     		perror("ERROR: Not communicating with otp_enc");
			     		return 3;
			     		exit (3);
			  	}
	                  
			  	//separate string of recieved info into plaintext and key 
	//printf("buffer:%s \n", buffer);
				ProcessData(buffer, plaintext, key);
	//printf("key: %s\n ",key);
	//replacspaces in plaintext and  key with ascii 91 or [ to make calculation easier
			  	ReplaceSpace(plaintext);
			  	ReplaceSpace(key);
	//printf("%s :: %s \n", plaintext, key);
			  
			  	//encrypt plaintext
			  	memset(buffer, '\0', strlen(buffer));
			  	EncodeMssg(plaintext, key, buffer);
	//printf("done with encoding: %s \n");
			  
			  	//switch all ['s back in to spaces'
			  	ReplaceBracket(buffer);
			  
	//printf("done with replace bracket: %s \n", buffer);
			  	//add end code to buffer
			  	strcat(buffer, "@@");
			  
			  	//send encrypted text back to client
			  	charsRead = send(establishedConnectionFD, buffer, strlen(buffer), 0); // Send encrypted mssg back
			  	if (charsRead < 0) error("ERROR writing to socket");
			  	break;
			  	exit(0);
		       }
			default:{
			   	pid_t actualpid=waitpid(spawnPid, &childexitstatus,0);//wait for encryption to finish
	                   	if (childexitstatus!=0){//something went wrong in child process
			      	   if(childexitstatus==3){
				      charsRead=send(establishedConnectionFD, "***error wrong server@@", 21, 0);
				   }
				   else{
				      charsRead=send(establishedConnectionFD, "***error@@", 8,0); //send "error" notification
				   }   
				}

			   	close(establishedConnectionFD); // Close the existing socket which is connected to the client

			   	break;
			   	exit(0);
			}
		}	
	}
	close(listenSocketFD); // Close the listening socket
	return 0; 
}
