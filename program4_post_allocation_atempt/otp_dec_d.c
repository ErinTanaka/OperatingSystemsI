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

void ProcessData(char* buffer, char * ciphertext, char * key){ //function to process incoming data
	//get length of ciphertext/integer at beginning of buffer
	int i=0;
	char lenbuffer[16];
	memset(lenbuffer, '\0', 16);
	while(isdigit(buffer[i])){
	   strncat(lenbuffer, &buffer[i], 1);		     
	   i++;		  
	}		  
	int mssglen=atoi(lenbuffer);
	
			  
	//get ciphertext
	if(sizeof(ciphertext)<mssglen){
	   ciphertext=realloc(ciphertext, (mssglen+8)*sizeof(char));
	}
	memset(ciphertext, '\0', sizeof(ciphertext));
	//strncpy(ciphertext, &buffer[i], mssglen);
	for(i=strlen(lenbuffer); i< mssglen+strlen(lenbuffer); i++){
	   strncat(ciphertext, &buffer[i], 1);
	}
	//get key
	//char* pch;
	//pch=strstr(buffer, "$$");
	//int keyit=mssglen+strlen(lenbuffer);
	if(sizeof(key)<strlen(buffer)-(mssglen+strlen(lenbuffer))){
	   key=realloc(key,(strlen(buffer)-(mssglen+strlen(lenbuffer))+8*sizeof(char)));
	}
	memset(key, '\0', sizeof(key));
	int x;
	for(x=(mssglen+strlen(lenbuffer)); x<mssglen+strlen(lenbuffer)+strlen(key); x++){ //append key one character at a time unitll end code is reached
	   strncat(key, &buffer[x],1);
	}
	strncat(key, "\0",1);
        	
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
void DecodeMssg(char* ciphertext, char* key, char* dest){ //function to encrypt plain text into cipher text
	int i;
	for(i=0; i<strlen(ciphertext); i++){ //do calculations one character at a time
	   //get numerical val 1-27 for A-Z and "space"
	   //subtract the two numbers(one from ciphertext, one from key)
	   //if number is negative add 27 (moding for negative number)
	   //add 65(ascii vall of 'A') to difference to get Capital Letter or ascii val 91
	   //the 91 gets switched to a space in the RePlaceBracket fn
		dest[i]=(ciphertext[i]-key[i]);
	   	if(dest[i]<0){
	     		dest[i]=dest[i]+27;
	  	} 
	  	dest[i]=dest[i]+65;
	}
}

int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char * buffer=malloc(75000*sizeof(char));
	struct sockaddr_in serverAddress, clientAddress;

	
	char * ciphertext=malloc(64*sizeof(char));
	memset(ciphertext,'\0',64);
	char * key=malloc(64*sizeof(char));
	memset(key,'\0',64);

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
		char * readbuffer=malloc(75000* sizeof(char));
		memset(buffer, '\0', sizeof(readbuffer));
/*this is different*/		while(strstr(readbuffer, "@@")==NULL || strstr(readbuffer, "$$")==NULL){
			memset(readbuffer, '\0', sizeof(readbuffer));
			charsRead = recv(establishedConnectionFD, readbuffer, sizeof(readbuffer)-1, 0); //read clients mssg from socket
			if(strlen(buffer)>=sizeof(buffer-16)){
			   buffer=realloc(buffer, (sizeof(buffer)*2)*sizeof(char));
			}
			strcat(buffer, readbuffer);
			if (charsRead<0) error("ERROR on accept"); break;
			if (charsRead==0) printf("received none\n");break;
		}
printf("buffer: %s \n", buffer);
fflush(stdout);
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
				  if(strstr(buffer, "$$")==NULL){
			     		perror("ERROR: Not communicating with otp_dec");
			     		return 3;
			     		exit (3);
			  	}
	                  
			  	//separate string of recieved info into ciphertext  and key 
			  	ProcessData(buffer, ciphertext, key);
			  	//replacspaces in ciphertext and  key with ascii 91 or [ to make calculation easier
			  	ReplaceSpace(ciphertext);
			  	ReplaceSpace(key);
			  
			  	//decrypt ciphertext
			  	memset(buffer, '\0', strlen(buffer));
			  	DecodeMssg(ciphertext, key, buffer);
			  
			  	//switch all ['s back in to spaces'
			  	ReplaceBracket(buffer);
			  
			  	//add end code to buffer
			  	strcat(buffer, "$$");
			  
			  	//send encrypted text back to client
			  	charsRead = send(establishedConnectionFD, buffer, strlen(buffer), 0); // Send encrypted mssg back
			  	if (charsRead < 0) error("ERROR writing to socket");
			  	break;
			  	exit(0);
		       }
			default:{
			   	pid_t actualpid=waitpid(spawnPid, &childexitstatus,0);//wait for encryption to finish
	                   	if (childexitstatus!=0){//something went wrong in child process
	                   	   if (childexitstatus==3){//something went wrong in child process
				      charsRead=send(establishedConnectionFD, "***error wrong server$$", 21, 0); //send "error" notification 
				   }
				   else{
				      charsRead=send(establishedConnectionFD, "***error$$", 8,0); //send "error" notification
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
