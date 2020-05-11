#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues
//encryption fns

void ProcessData(char* buffer, char ** plaintext, char ** key){ //function to process incoming data
	//get length of plaintext/integer at beginning of buffer
	int i=0;
	char lenbuffer[16];
	memset(lenbuffer, '\0', 16);
	while(isdigit(buffer[i])){
	   strncat(lenbuffer, &buffer[i], 1);		     
	   i++;		  
	}		  
	int mssglen=atoi(lenbuffer);
	printf("mssg len: %d\n", mssglen);
			  
	//get plaintext
	//
//	if(63<mssglen){
//	   free(*plaintext);
//	   *plaintext=calloc(mssglen+8, sizeof(char));
//	}
//	else{
	
	   memset(*plaintext, '\0', 70000);
//	}
	//strncpy(plaintext, &buffer[i], mssglen);
	int x;
	for(x=strlen(lenbuffer); x<mssglen+strlen(lenbuffer); x++){
	   strncat(*plaintext, &buffer[x], 1);
	}
	strcat(*plaintext, "\0");
	printf("server plaintext done\n",*plaintext);
        
	//get key
	//getsizeof key
	int keysize=strlen(buffer)-mssglen-strlen(lenbuffer)-2;
//	if(63<keysize){
//	  free(*key);
//	  *key=calloc(keysize+8,sizeof(char));
//	}
//	else{
	   memset(*key, '\0', sizeof(*key));
//	}
	//printf("key size: %d\n", keysize);
	char* pch;
	pch=strstr(buffer, "@@");
	int keyit=mssglen+strlen(lenbuffer);
	while(&buffer[keyit]!=pch){ //append key one character at a time unitll end code is reached
	   //printf("key: %s\n", *key);
	   strncat(*key, &buffer[keyit],1);
	   keyit++;
	}
	strcat(*key,"\0");
	printf("done with process data\n");
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
	}
}

int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char * buffer=calloc(150000,sizeof(char));
	struct sockaddr_in serverAddress, clientAddress;

	//erins vars
	char * plaintext=calloc(70000, sizeof(char));
	char * key=calloc(70500, sizeof(char));

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

		char* tmp;
		//getting stuff from client
		char readbuffer[150000];
		buffer=realloc(buffer,(150000)*sizeof(char));
		memset(buffer, '\0', 150000);
		
		//while(strstr(buffer, "@@")==NULL || strstr(buffer, "$$")==NULL){
		while(1){
		   char* getout=strstr(buffer, "@@");
		   if(getout!=NULL){
		      printf("server break recv loop\n");
		      break;
		   }
		//printf("while loop\n");	
		   memset(readbuffer, '\0', 256);
			charsRead = recv(establishedConnectionFD, readbuffer, 255, 0); //read clients mssg from socket
			//buffercontains=buffercontains+strlen(readbuffer);
//			printf("size:%d contains:%d\n", buffersize, buffercontains);
			strcat(buffer, readbuffer);
			//printf("buffer; %s\n",buffer);
			if (charsRead<0) {error("ERROR on accept"); break;}
			if (charsRead==0) {printf("received none\n");break;}
		//printf("bottomofloop\n");
		}
                strcat(buffer, "\0");
printf("server outta the recv loopy\n");
		//printf("SERVER: buffer: %s\n", buffer);
        	
		
		
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
                          
				 printf("childprocess\n");
			  	//verify speaking to otp_enc: end code will be "@@" for messages from otp_enc and "$$" for otp_dec
			  	if(strstr(buffer, "@@")==NULL){
			     		perror("ERROR: Not communicating with otp_enc");
			     		return 3;
			     		exit (3);
			  	}
	                  
			printf("process data here...\n");
			  	//separate string of recieved info into plaintext and key 
			  	ProcessData(buffer, &plaintext, &key);

			  	//replacspaces in plaintext and  key with ascii 91 or [ to make calculation easier
			  	ReplaceSpace(plaintext);
			  	ReplaceSpace(key);
	printf("finished replace space");
			  
			  	//encrypt plaintext
			  	memset(buffer, '\0', strlen(buffer));
			  	EncodeMssg(plaintext, key, buffer);
	printf("done with encoding\n");
			  
			  	//switch all ['s back in to spaces'
			  	ReplaceBracket(buffer);
			  
	printf("done with replace bracket \n");
			  	//add end code to buffer
			  	strcat(buffer, "@@\0");
//			  printf("buffer to send back to client: %s\n",buffer);
			  	//send encrypted text back to client
			  	charsRead = send(establishedConnectionFD, buffer, strlen(buffer), 0); // Send encrypted mssg back
			  	if (charsRead < 0) error("ERROR writing to socket");
			  	if (charsRead<strlen(buffer)){
				   printf("SERVER: warning not all data written to socket\n");
				}
				int checkSend=-3;
				do{
				   ioctl(establishedConnectionFD, TIOCOUTQ, &checkSend);
				}while(checkSend>0);
				if(checkSend <0) error("ioctl error");
				printf("SERVER childprocess done\n");
				return 0;
			  	exit(0);
		       }
			default:{
				   
			   	printf("SERVER: parentprocess waiting for my child");
				   pid_t actualpid=waitpid(spawnPid, &childexitstatus,0);//wait for encryption to finish
	                   	printf("SERVER: childfinished\n");
				   if (childexitstatus!=0){//something went wrong in child process
			      	   charsRead=send(establishedConnectionFD, "***error@@", 8,0); //send "error" notification
			   	}

			   	close(establishedConnectionFD); // Close the existing socket which is connected to the client

			   	break;
			   	exit(0);
			}
		}	
	}
	close(listenSocketFD); // Close the listening socket
	free(buffer);
	free(key);
	free(plaintext);
	return 0; 
}
