/* 
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file 
 * you will need to modify Makefile to compile
 * your additional functions.
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Submit the entire lab1 folder as a tar archive (.tgz).
 * Command to create submission archive: 
      $> tar cvf lab1.tgz lab1/
 *
 * All the best 
 */

 
 // https://stackoverflow.com/questions/33912024/shell-program-with-pipes-in-c
 // Above link is to set up structure that seperates 
 //arguments in Main, then calls either docmd or 
 //dopipes depending on if it is supposed to be piped or not
 
 // https://stackoverflow.com/questions/8389033/implementation-of-multiple-pipes-in-c
 // Above link is to loop through commands and check how many 
 // pipes there are, therefore works with multiple pipes.
 
 //
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h"
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <errno.h>

#define BUFFER_SIZE 25
#define READ_END 0
#define WRITE_END 1



/*
 * Function declarations
 */

void PrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);
void sigint_handler(int sig);
void bakgrunding(Command *c);
void redirect(Command *c);

/* When non-zero, this global means the user is done using this program. */
int done = 0;
pid_t shell_pid;
pid_t bakgrund;
static int exitstatus = 54;

/*
 * Name: main
 *
 * Description: Gets the ball rolling...
 *
 */
int main(void)
{
  Command cmd;
  int n;
  shell_pid = getpid();	
  signal(SIGINT, sigint_handler);
  
  while (!done) {

    char *line;
    line = readline("> ");

    if (!line) {
      /* Encountered EOF at top level */
      done = 1;
    }
    else {
      /*
       * Remove leading and trailing whitespace from the line
       * Then, if there is anything left, add it to the history list
       * and execute it.
       */
      stripwhite(line);

      if(*line) {
        add_history(line);
        /* execute it */
        n = parse(line, &cmd);
        PrintCommand(n, &cmd);
		
		//printf("%d\n", n);
		
		if (n == 1){
			
			//Command * c = &cmd;
			
			/*if (c->bakground){
				pid_t shell;
				shell = fork();				// initial child
				if (shell < 0 ){
					perror("Fork");
					_exit(0);
				}
				else if (shell == 0)
				{
					bakgrunding(&cmd);
				}
				else{
					waitpid(shell,NULL,0);
					//exit(0);					//-- here we exit the shell completely :(
				}
				
				
			}		
			else{	*/	
				doFunction(&cmd);
			//}
		}
      }
    }
    
    if(line) {
      free(line);
    }
  }
  return 0;
}

/*
 * Name: PrintCommand
 *
 * Description: Prints a Command structure as returned by parse on stdout.
 *
 */
void
PrintCommand (int n, Command *cmd)
{
  printf("Parse returned %d:\n", n);
  printf("   stdin : %s\n", cmd->rstdin  ? cmd->rstdin  : "<none>" );
  printf("   stdout: %s\n", cmd->rstdout ? cmd->rstdout : "<none>" );
  printf("   bg    : %s\n", cmd->bakground ? "yes" : "no");
  PrintPgm(cmd->pgm);
}

/*
 * Name: PrintPgm
 *
 * Description: Prints a list of Pgm:s
 *
 */
void
PrintPgm (Pgm *p)
{
  if (p == NULL) {
    return;
  }
  else {
    char **pl = p->pgmlist;

    /* The list is in reversed order so print
     * it reversed to get right
     */
    PrintPgm(p->next);
    printf("    [");
    while (*pl) {
      printf("%s ", *pl++);
    }
    printf("]\n");
  }
}


void bakgrunding(Command* c)
{

	bakgrund = fork();
		
	if (bakgrund == -1){
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (bakgrund == 0 ){
		//close(READ_END); // close child's stdin
		//fopen("/dev/null", "r");
		signal(SIGINT, SIG_IGN);
		doFunction(c);
	}
	else{
		//waitpid(bakgrund, NULL, -1);
		_exit(0);
	}	
}
/*
 * Name: stripwhite
 *
 * Description: Strip whitespace from the start and end of STRING.
 */
void
stripwhite (char *string)
{
  register int i = 0;

  while (isspace( string[i] )) {
    i++;
  }
  
  if (i) {
    strcpy (string, string + i);
  }

  i = strlen( string ) - 1;
  while (i> 0 && isspace (string[i])) {
    i--;
  }

  string [++i] = '\0';
}


void sigint_handler(int sig) {  // handle foreground processes, do not exit with C^
	if(sig == SIGINT){
		pid_t self = getpid();
		if (shell_pid != self)  { 
		_exit(0);
		}
	}
	//else if(sig == SIGCHLD) {
	//	waitpid(bakgrund, NULL, 0);
	//}
}

int redirection(Command * c){ // Checks if do redirect
	if (c->rstdin != NULL){
		//STDIN
		int fdin; /*file descriptor to the file we will redirect ls's output*/
				
		if((fdin = open(c->rstdin, O_RDONLY))==-1){ /*open the file */
			perror("open read");
			return 1;
		}
		
		dup2(fdin, STDIN_FILENO);
		dup2(fdin, STDERR_FILENO); /* same, for the standard error */
		close(fdin);
	}
		
	if (c->rstdout != NULL){     
		//STDOUT
		int fd; /*file descriptor to the file we will redirect ls's output*/
			
		if((fd = open(c->rstdout, O_RDWR | O_CREAT, S_IRWXU))==-1){ /*open the file */
			perror("open write");
			return 1;
		}
		dup2(fd,STDOUT_FILENO); /*copy the file descriptor fd into standard output*/
		dup2(fd,STDERR_FILENO); /* same, for the standard error */
		close(fd); /* close the file descriptor as we don't need it more  */
	}
	
	return 0;
}

int doFunction (Command * c){
	
	int status;
	Pgm *p = c->pgm;
	
	  if (p == NULL) {
		return 0;
	  }
	  else {
		Pgm *pcopy = c->pgm;
		int pipingIsTrue = 0;
		char **pl = p->pgmlist;
		int ret;
		
		if (strcmp(pl[0], "exit") == 0){// kill all children, exit
			kill(-shell_pid, SIGTERM);	  
			done = 1;
		}else if(strcmp(pl[0],"cd") == 0){
			ret = chdir(pl[1]);
			if (ret == -1){
				//exit(EXIT_FAILURE);
				printf("\nCould not find directory \n");
				perror("CD");
			}	
		}else{		
		
			pid_t pid;
			int pipefd[2];
			if (pipe(pipefd) == -1){ //Set up pipe
				fprintf(stderr, "Pipe Failed\n");
				perror("pipe");
				return 1;
			}
			pid = fork();
			
			if (pid == -1){
				perror("fork");
				//THINGS WENT WRONG
				exit(EXIT_FAILURE);
			}
			
			if (pid == 0 ){
									//I AM CHILD

				if (pcopy->next != NULL){		//Check if commands are to be piped
					Pgm *pcopynext = pcopy->next;
					
					if (c->bakground){ //Check if it's supposed to be a background process
						pid_t pidbak;
						pidbak = fork();
						if (pidbak < 0){
							perror("fork");
						}else if (pidbak == 0){ //Child
							signal(SIGINT, SIG_IGN);//CTRL+C does not terminate background processes
							doPipe(c, pcopy); //Start the recursion of pipes
							exit(0);
						}else{ //Parent
							_exit(0);
						}
						
					}else{
						doPipe(c, pcopy);	
					}
					
					
				}else{
					if (redirection(c)){ //Checks and "activates" RSTDIN and RSTDOUT
						perror("redirect");
						exit(0);
					}
					if (c->bakground){ //Check if background process
						
						pid_t pidbak;
						pidbak = fork();
						if (pidbak < 0){
							perror("fork");
						}else if (pidbak == 0){ //Child
							signal(SIGINT, SIG_IGN);//CTRL+C does not terminate background processes
							execvp(pl[0], pl); //Execute
							printf("\nCould not find command\n");
							perror("Not found");
							exit(0);
						}else{//Parent
							//waitpid(pidbak, NULL, 0);
							_exit(0);
						}
						
					}else{
						execvp(pl[0], pl); //Execute
						printf("\nCould not find command\n");
						perror("Not found");
						exit(0);
					}
				}
				/*while (*pl) {
					printf("%s ", *pl++); 
				}*/
				
			}			
			
			if (pid > 0){
								//I AM PARENT
				
				printf("\nPID = %d\n", pid);
				char **plcopy = pcopy->pgmlist;
				waitpid(-1, &status, 0);				

			}			
		}
		/* The list is in reversed order so print
		 * it reversed to get right
		 */
		//PrintPgm(p->next);
		
	  }
}

int doPipe (Command * c, Pgm* pgm_p){
	
	if (pgm_p == NULL){			// sista i listan
		return;
	}
	else{
		//Piping
		int pipefd[2];
		pid_t pid;
		
		char **plcopy = pgm_p->pgmlist;
		int ret;
		int status;
				
		if (pipe(pipefd) == -1){
			printf("\nPipe Failed\n");
			perror("pipe");
			return 1;
		}
		pid = fork();
		
		if (pid < 0) {
			fprintf(stderr, "Fork Failed\n");	
			perror("fork");
		}
		else if (pid == 0){		// CHILD
			close(WRITE_END);	
			close(pipefd[READ_END]);
			dup2(pipefd[WRITE_END], STDOUT_FILENO);/*copy the file descriptor fd into standard output*/
			doPipe(c, pgm_p->next); //Continue the recursion
			
			_exit(0);
		}
		else{	//Parent
			waitpid(pid,&status,0);
			//pid_t wpid;
			//while((wpid = wait(&status)) > 0);
			
			if(WEXITSTATUS(status) != exitstatus){ //If exitstatus is okay
				close(READ_END);		// close input of process stdin == 0
				close(pipefd[WRITE_END]);		// close write end of pipe
				dup2(pipefd[READ_END], STDIN_FILENO);
				printf("\n %s\n", plcopy[0]);
				if (errno != ENOENT && errno <= 0){ 
				// controls if there is any previous error messages
						if (redirection(c)){ //If redirections goes wrong, exit
							perror("redirect");
							exit(0);
						}
						execvp(plcopy[0], plcopy);
						perror("Not Found");
						_exit(exitstatus);
					}else{
						perror("error");
					}
			}
			
			printf("Could not complete piped command\n");
			//perror("Not found");
			_exit(exitstatus);
		}
		
	}
}

