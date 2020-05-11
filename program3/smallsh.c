#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <fcntl.h>
#include <signal.h>

//function declarations
char * MainReadLine();
char** MainSplitUp(char*);
int MainExecuteSomething(char**, int, char*);

void BuiltInExit(char**, char*);
void BuiltInStatus(int, bool);
void BuiltInCd(char**);

int IsBuiltInCommand(char*);
bool IsBackgroundProcess(char**);
void StoreBackgroundPid(int );
bool IsCommentOrBlank(char*);

int NonBuiltInCommand(char**);
bool InputOutputRedir(char**, bool);
void CheckBackgroundFinished();
char* replace$$pid(char *, char*);

void catchSIGINT(int); 
void catchSIGTSTP(int);

//global variables
volatile bool inforegroundonlymode;
volatile bool termbysig;
volatile int signum;

int runningpids[5];

struct sigaction SIGINT_action={0};
struct sigaction SIGTSTP_action;
struct sigaction ignore_action={0};


int main(){
   //set up sigint struct 
   SIGINT_action.sa_handler=catchSIGINT;
   sigfillset(&SIGINT_action.sa_mask);
   SIGINT_action.sa_flags = 0;
   //seup sigtstp struct
   SIGTSTP_action.sa_handler=catchSIGTSTP;
   sigfillset(&SIGTSTP_action.sa_mask);
   SIGTSTP_action.sa_flags=0;
   //setup ignore struct
   ignore_action.sa_handler=SIG_IGN;
   //shell signal handlers
   sigaction(SIGINT, &ignore_action, NULL);
   sigaction(SIGTSTP, &SIGTSTP_action, NULL);
   
   //setting globals
   termbysig=false;
   inforegroundonlymode=false;
   int i;
   for (i=0; i<5; i++){
      runningpids[i]=0;
   }
   
   //vars to keep shell running
   int keeprunning=-3;
   char * userinput;
   char ** list;

   while (true){
      CheckBackgroundFinished(); //check for any completed processes
      userinput= MainReadLine(); //get user input via "cmd line"
      list= MainSplitUp(userinput); //"itemize" user input
      bool commentorblank=IsCommentOrBlank(list[0]);//check for comment or blankline
      if (commentorblank==false){//execute some command
         keeprunning=MainExecuteSomething(list, keeprunning, userinput);
      }
      //free any malloc'd memory  
      if (userinput!=NULL) free(userinput);
      if (list!= NULL) free(list);
   
   }

}
/*****************************************************************
 * FN: catchSIGINT
 * PARAMETERS: int signal number
 * RETURNS: nothing
 * DESCRIPTION: prints termination message; sets global variable
 * **************************************************************/
void catchSIGINT(int signo){
   signum=signo; //set global var for reference by builtinstatus
   termbysig=true;
   BuiltInStatus(signo, true); //print term by sig signo
}
/*****************************************************************
 * FN: catchSIGTSTP 
 * PARAMETERS: int signal number
 * RETURNS: none
 * DESCRIPTION: prints message regarding entrance or exit of 
 * foreground-only mode and changes global bool indicating to 
 * shell and processes if shell is in foreground-only mode
 * **************************************************************/
void catchSIGTSTP(int signo){
   if(inforegroundonlymode==false){ //enter fg only
      char * mssg = "Entering foreground-only mode (& is now ignored)\n";
      write(STDOUT_FILENO, mssg, 49);
      inforegroundonlymode=true;
   }
   else{ //exit fg only
      char* mssg = "Exiting foreground-only mode\n";
      write(STDOUT_FILENO, mssg, 29);
      inforegroundonlymode=false;
   }
}
/*****************************************************************
 * FN: MainReadLine 
 * PARAMETERS: none
 * RETURNS: pointer to string of user input
 * DESCRIPTION: prompts user for input, calls function to replace 
 * instances of $$, and returns pointer to string
 * **************************************************************/
char* MainReadLine(){
   char* buffer=NULL;
   size_t sizeBuffer=2048;
   char* temp=NULL;
   printf(": "); //cmd prompt
   fflush(stdout);
   getline(&buffer, &sizeBuffer, stdin); //get userinput
   buffer=replace$$pid(buffer, temp);
   return buffer; 
}

/*****************************************************************
 * FN: MainSplitUp 
 * PARAMETERS: pointer to space delineated string of user input
 * RETURNS: pointer to individual "tokens" of ui string
 * DESCRIPTION: uses strtok to take space delineated list and get 
 * pointers to beginning of each "word" 
 * **************************************************************/
char** MainSplitUp(char * varlist){
   int i=0;
   char** list = calloc(512, sizeof(char*));
   char* item; 

   item=strtok(varlist, " \n\r\t"); //get first string
   while(item!=NULL){
      list[i]=item; //store pointer to the current string
      i++;
      if (i>=512){
         printf("too many args!!\n");
	 fflush(stdout);
	 return NULL;
      }
      item=strtok(NULL, " \t\r\n"); //get next string
   }
   list[i]=NULL; //with the way loop is then it breaks before the null gets saved
   return list;
}
/*****************************************************************
 * FN: IsBuiltInCommand 
 * PARAMETERS: pointer to command to be executed
 * RETURNS: integer specifying which built in command was requested 
 * or 0 if none were
 * DESCRIPTION: checks user input for any of the three builtin 
 * commands; returns exit:1, status:2, cd:3 or 0 for non builtin command
 * **************************************************************/
int IsBuiltInCommand(char* item){
   if (strcmp(item, "exit")==0){
      return 1;
   }
   else if(strcmp(item, "status")==0){
      return 2;
   }
   else if(strcmp(item, "cd")==0){
      return 3;
   }
   else{ 
      return 0;
   }
}
/*****************************************************************
 * FN: IsCommentOrBlank 
 * PARAMETERS: pointer to first character/word of user input
 * RETURNS: true if userinput starts with # (indicates comment), 
 * or true if pointer is null: blankline; anything else returns false
 * DESCRIPTION: returns true if comment or blankline 
 * **************************************************************/
bool IsCommentOrBlank(char* item){
   if(item== NULL){ //blankline
      return true;
   }
   else if (strncmp(item, "#", 1)==0){ //comment
      return true;
   }
   else{ //anything else
      return false;
   }
}

/*****************************************************************
 * FN: MainExecuteSomething 
 * PARAMETERS: list of args, previous exitstatus/term sig, userinput string
 * RETURNS: int exxitstats or termination signal of child process if any
 * DESCRIPTION: Checks for built-in command and executes if found, 
 * or calls fn to fork child process
 * **************************************************************/
int MainExecuteSomething(char**list, int prevstatsig, char *userinput){
   int returnvalue;
   //check for builtin cmd
   int builtincmd = IsBuiltInCommand(list[0]);
   if (builtincmd!=0){
      if(builtincmd==1){ //exit shell
	 BuiltInExit(list, userinput);
      }
      else if(builtincmd==2){ //status
         BuiltInStatus(prevstatsig, termbysig);
	 return prevstatsig;
      }
      else{ //cd
	 BuiltInCd(list);
	 return prevstatsig;
      }
   }
   else{ //not a built in command
      return NonBuiltInCommand(list);
   }
}
/*****************************************************************
 * FN: BuiltInExit 
 * PARAMETERS: userinput string, and list of pointers to each space 
 * delineated word in ui
 * RETURNS: nothing 
 * DESCRIPTION: kills an background porcesses, frees malloc'd mem 
 * for user input and exits program
 * **************************************************************/
void BuiltInExit(char** list, char *ui){
   int i;
   //killing any background processes
   for(i=0; i<5; i++){
      if(runningpids[i]!=0){
         kill(runningpids[i], SIGTERM);
	 runningpids[i]=0;
      }
   }
 //freeing userinput  
   if(ui!=NULL){
      free(ui);
   }
   if (list!=NULL){
      free(list);
   }
   
   exit(0);
}
/*****************************************************************
 * FN: BuiltInStatus 
 * PARAMETERS: exitvalue/signal number and bool true if terminated by signal
 * RETURNS: none
 * DESCRIPTION: prints exit value val or terminating signal val 
 * depending on passed bool regarding if process was terminated by signal
 * **************************************************************/
void BuiltInStatus(int val, bool wassig){
   if (wassig==false){ //was not terminated by signal
      printf("exit value %d\n", val);
      fflush(stdout);
   }
   else{ //was terminated by signal
      printf("terminated by signal %d\n", val);
      fflush(stdout);
   }   
}
/*****************************************************************
 * FN: BuiltInCd 
 * PARAMETERS: list of cd cmd and optional argument
 * RETURNS: none
 * DESCRIPTION: changes working directory to home diretory or specified directory
 * **************************************************************/
void BuiltInCd(char** list){
   if (list[1]==NULL){ //change to home dir
      chdir(getenv("HOME"));
   }
   else{ //change to specified dir
      chdir(list[1]);
   }
}

/*****************************************************************
 * FN: NonBuiltInCommand 
 * PARAMETERS: list of program, args(if any), io redirection(if any) 
 * and background process indicator(if any)
 * RETURNS: integer exit status or terminating signal 
 * DESCRIPTION: forks off child process, calls function to perform 
 * i/o redirection, checks for background processes, handles sigint 
 * and sigtstp signals from keyboard
 * **************************************************************/
int NonBuiltInCommand(char** list){
   //'reset' global bool
   termbysig=false;  
   
   pid_t spawnPid=-3;
   int childexitstatus=-3;
   int backexitstatus=-3;
   bool isbackground=false; 
   bool iosuccess;

   spawnPid=fork();
   switch (spawnPid){
      case -1:{ //couldn't fork process
	         perror("Hull Breach\n");
		 return 1;
		 //break;
                 exit(1);
	      }
      case 0:{ //child process
		sigaction(SIGTSTP, &ignore_action, NULL); //ignore sigtstp signal
                
		isbackground=IsBackgroundProcess(list); //check if background process
		
		if (isbackground==true){
		   sigaction(SIGINT, &ignore_action, NULL); //bkgrnd ignores sigint
		}
		
		iosuccess=InputOutputRedir(list, isbackground); //perform i/o redirection if any
		
		if (iosuccess==false){ //something went wrong with i/o
		 exit(1);  
		}
		
		execvp(list[0], list); //perform command indicated by list[0]
		perror(list[0]); //there was an error performing command
		exit(1);
	     }
      default:{ //parent process
	         sigaction(SIGTSTP, &SIGTSTP_action, NULL); //handle sigtstp
		 sigaction(SIGINT, &ignore_action, NULL); //ignore sigint
		 
		 isbackground=IsBackgroundProcess(list); //check if forked process was background
		 if (isbackground==true){
		    printf("background pid is %d\n", spawnPid); //print background pid
		    fflush(stdout);
		    StoreBackgroundPid(spawnPid); //store background pid
		    return 0;
		 }
		 
		 else{ //forked process running in foreground
		    pid_t actualpid=waitpid(spawnPid, &childexitstatus, 0); //wait for foreground process
		    if (childexitstatus==0){ //program exited normally
		       return 0;
		    }
		    if (termbysig==true){ //program was terminated by signal
		       return signum;
		    }
		    return 1;
	      	}
		 break;
		 exit(1);
	      }
   }
}
/*****************************************************************
 * FN: IsBackgroundProcess 
 * PARAMETERS: list of cmds, args etc. 
 * RETURNS: bool true if process needs to be run in background
 * DESCRIPTION: checks passed args for & at end, if & is found and 
 * shell is not in foreground-only mode ture is returned
 * **************************************************************/
bool IsBackgroundProcess(char** list){
   int i=0;
   while (list[i]!=NULL){
      if(strcmp(list[i], "&")==0 && list[i+1]==NULL){ //& is last character
	 list[i]=NULL; //remove & from args
         if(inforegroundonlymode==true){ //ignore the &
	    return false;
	 }
	 return true;
      }
      i++;
   }
   return false;
   
}
/*****************************************************************
 * FN:StoreBackgroundPid 
 * PARAMETERS: pid to store
 * RETURNS: none
 * DESCRIPTION: adds passed pid to list of currently running background processes
 * **************************************************************/
void StoreBackgroundPid(int p){
   int i=0;
   for(i=0; i<5; i++){
      if (runningpids[i]==0){
         runningpids[i]=p;
	 break;
      }
   }
}
/*****************************************************************
 * FN: InputOutputRedir
 * PARAMETERS: list of arguments bool for if i/o is for a background process
 * RETURNS: bool t=successful io
 * DESCRIPTION: performs i/o redirection and removes "<", ">", and 
 * file names frm parameter list
 * **************************************************************/
bool InputOutputRedir(char **list, bool isback){
   int i=0;
   //loop as long as there are still arguments
   while(list[i]!=NULL){
      if(strcmp(list[i], "<")==0){ //do input redirection
         int filein=open(list[i+1], O_RDONLY);
         if(filein==-1){ //specified file cannot be opened
	    if(isback==true){ //this is a bkgrnd process redirect input from dev/null
	       int backfile=open("dev/null", O_RDONLY);
	       dup2(backfile, STDIN_FILENO);
	       close (backfile);
	       i++;
	       list[i-1]=NULL;
	    }
	    else{ //foreground process
	       printf("cannot open %s for input\n", list[i+1]);
	       return false;
	    }
	 }
	 else{ //sucessful file open in foreground or background
	    dup2(filein, STDIN_FILENO);
            close(filein);
            list++;
	    list[i-1]=NULL;
	    list++;
	    list[i-1]=NULL;
	 }	 
      }
      else if(strcmp(list[i], ">")==0){ // redirect output
	 int fileout=creat(list[i+1], 0644);
         if (fileout==-1){ //failed to open or create file
	    if(isback==false){ //foreground process print err mssg
	       printf("cannot open or create %s for output\n", list[i+1]);
	       return false;
	    }
	    else{ //background redir output to /dev/null
	       int backout=open("/dev/null", 0644);
               dup2(backout, STDOUT_FILENO);
	       close(fileout);
	       list++;
	       list[i-1]=NULL;
	    }
	 }
	 else{ //sucessful file open in foregrond or background
	      dup2(fileout, STDOUT_FILENO);
	      close(fileout); 
              list++;
	      list[i-1]=NULL;
	      list++;
	      list[i-1]=NULL;
	 }
      }
      else{ 
	 i++;
      }
   }
   return true;
}
/*****************************************************************
 * FN: CheckBackgroundFinished 
 * PARAMETERS: none
 * RETURNS: none
 * DESCRIPTION: Checks list of running background processes for any 
 * that have finished or been terminated by a signal. Prints exit 
 * status or signal terminated by if applicable
 * **************************************************************/
void CheckBackgroundFinished(){
   int i=0;
   int status;
   int retval;
   for (i=0; i<5; i++){
      if(runningpids[i]!=0){//check list for running background process
         retval=waitpid(runningpids[i], &status, WNOHANG);
	 if (retval!=0){//process has finished
	    printf("background pid %d is done:", runningpids[i], status);
	    fflush(stdout);
	    if(status==0 || status==1){
	       BuiltInStatus(status,false);//print exit status
	    }
	    else{
	       BuiltInStatus(status, true); //print termination status
	    }
	    runningpids[i]=0; //remove pid from list
	 }
      }
   }
}
/*****************************************************************
 * FN: replace$$pid 
 * PARAMETERS: pointer to original string and pointer to buffer for modified string 
 * RETURNS: string ready to be passed to splitup fn
 * DESCRIPTION: replaces all accounts of "%$$" if any in mystring 
 * **************************************************************/
char* replace$$pid(char * mystring, char* buffer){
   //no string was entered
   if (mystring==NULL){
      return mystring;
   }
   
   //$$ doesn't exist anywhere withing passed string
   if(strstr(mystring, "$$")==NULL){
      return mystring;
   }
   
   int shellpid=getpid();     
   char * pch;
   int i;

   //continue until no more $$ exist in string
   while (strstr(mystring, "$$")!=NULL){
      i=0;
      buffer=malloc(strlen(mystring)*(sizeof(char))+32* sizeof(char));
      memset(buffer, '\0', strlen(buffer));
      pch=strstr(mystring, "$$");      
      
      //append characters from original input until $$ to buffer
      while(&mystring[i]!=pch){
         strncat(buffer, &mystring[i], 1);
	 i++;
      }
      i=i+2;
      
      //concatenate buffer and pid  
      sprintf(buffer, "%s%d", buffer, shellpid);
      
      //append rest of original string
      while(i<strlen(mystring)){
         strncat(buffer, &mystring[i],1);
	 i++;
      }
      
      //add null terminator
      strcat(buffer, "\0");
      mystring=buffer;
   }
 return buffer;  
}
