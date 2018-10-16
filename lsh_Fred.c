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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h"
#include <string.h>
#include <syscall.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

/*
 * TO DO:
 * background processes should not be affected by "CTRL-C"					: DONE
 * upon exit every process started by the shell should be killed			: DONE
 * implement piping in more stages !IMPORTANT! 									: DONE
 * implement correct error handling when using inavlid commands in pipes: almost done,
 *
 *
 * use "watch -n 1 ps -t 0" in another terminal in shell
 */

/*
 * Function declarations
 */

void PrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);
void exec_cmd(Command *);
void execRedirect (Command *);
void sigint_handler(int sig);
void exec_pgm(Pgm *);
void redirect_prcIO ( Command *cmd);

/* When non-zero, this global means the user is done using this program. */
int done = 0;
pid_t shell_pid;

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
  shell_pid = getpid();											// store shell process pid for sigint_handler
  signal(SIGINT, sigint_handler);							// install sigint_handler
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
/*
				if (!strcmp (cmd->pgm, "exit"))
		  		break;
*/
         //PrintCommand(n, &cmd);
         if (n == 1) exec_cmd(&cmd);
      }
    }
    
    if(line) {
      free(line);
    }
  }
  return 0;
}

/*
 * Name: exec_cmd
 *
 * Description: Commands and specifications handled by the lsh shell such as 
 *	-- simpler commands such as ls, date, who
 *	-- Commands can be executed as background processes
 * 
 */

void
exec_cmd (Command *cmd){
	if (!strcmp (*cmd->pgm->pgmlist, "exit")){         					// Detects if the command is exit
		printf ("\nExiting shell\n");
		kill(-shell_pid, SIGTERM);													// Kill processes created by shell
      done = 1;																		// Exits shell process
	}else if (!strcmp (*cmd->pgm->pgmlist, "cd") || !strcmp (*cmd->pgm->pgmlist, "cwd")){
		long size;
		char *buf;
		char *ptr;
		const char *newdir;
		size = pathconf(".",_PC_PATH_MAX);										// prepares ptr for getcwd() output
		if ((buf = (char *)malloc((size_t)size)) != NULL)					// creates a correct sized buf
			ptr = getcwd(buf,(size_t)size);										// get current directory
		if (!strcmp (*cmd->pgm->pgmlist, "cd")){								// if (Command = cd)
			int ret;
			char **pl = cmd->pgm->pgmlist;
			char *pl_arg[10] = { NULL };
			int pl_opt = 0;
			while (*pl){
				pl_arg[pl_opt++] = *pl++;
			}
			if (!strcmp(pl_arg[1],"..")){											// cm ..
				if (chdir("..") == 0) { 											// changed directory successfully
					printf("\nchanged directory successfully\n");
				}
			}else{				
				ptr = strcat(ptr,"/");
				newdir = strcat(ptr,pl_arg[1]);									// concatenate cwd with new directory
				if (chdir(newdir) == 0) { 											// changed directory successfully
					printf("\nCurrent directory is \"%s\"\n", newdir);
				}else{
					printf("cd: %s: No such file or directory\n", newdir);
				}
			}
			
		}else{																			// if (Command = cwd)
			 printf("\nCurrent directory is \"%s\"\n", ptr);				// output current directory
		}
	}else{
		char **pgmlist_cmd = cmd->pgm->pgmlist;
		pid_t pid_c = fork();														// fork the initial child
		if (pid_c < 0)
		   fprintf(stderr, "Fork Failed\n");
		else if (pid_c == 0) {														// Child process							
			if (cmd->bakground){
				pid_t pid_gc = fork();
				if (pid_gc < 0)
		   		fprintf(stderr, "Fork Failed\n");
				else if (pid_gc == 0) {
					signal(SIGINT, SIG_IGN);			// ignore "CTRL-C" signal for background processes			 			
					redirect_prcIO(cmd);
					exec_pgm(cmd->pgm);
				}else{// pid_gc > 0														
					exit(0);																//close dummy process
				}
			}else{// !bakground	
				redirect_prcIO(cmd);
				exec_pgm(cmd->pgm);
			}
		}else{// pid_c > 0	
			waitpid(pid_c,NULL,0);
		}				
	}	 
}

void sigint_handler(int sig) {
	if(sig == SIGINT){
		pid_t self = getpid();
		if (shell_pid != self)  { // closes all foreground processes except the shell process
		_exit(0);
		}
	}
}

/* ---------------------------------------------------------------------------------------------------------------
 * Name: exec_pgm
 *
 * Description: executes the command in the pgmlist of the struct Pgm
 *              and pipes the commands if necessary
 *
 */
 
void exec_pgm ( Pgm *p) {
	if (p->next) { 										//check if pipe is required
		if (p == NULL){
      	return;
		}else{
			char **pl = p->pgmlist;
			char *pl_arg[10] = { NULL };
			int pl_opt = 0;
			int pp[2];
			pipe(pp);
			pid_t pid = fork();
			if (pid < 0) 
				fprintf(stderr, "Fork Failed\n");
			else if (pid == 0){ //child
				close(1);      							// close output of process stdout == 1
				close(pp[0]);  							// close read end of pipe
				dup2(pp[1], 1); 							// redirect output of process into pipe
				exec_pgm(p->next);			// Check if another pipe is needed and/or execute
			}else{ //parent
				waitpid(pid,NULL,0);
				close(0);									// close input of process stdin == 0
				close(pp[1]);								// close write end of pipe
				dup2(pp[0], 0);							// redirect input of process into pipe
				while (*pl) {
					pl_arg[pl_opt++] = *pl++;
				}
				if (errno <= 0){							// controls if there is a previous error message
					execvp(pl_arg[0],pl_arg);
					perror("ERROR: ");
				}else{
					perror("ERROR: ");
				}	
				exit(0);
			}
		}
	}else{
		char **pl = p->pgmlist;
		char *pl_arg[10] = { NULL };
		int pl_opt = 0;
		while (*pl) {
			pl_arg[pl_opt++] = *pl++;
		}
		if (errno <= 0){									// controls if there is a previous error message
			execvp(pl_arg[0],pl_arg);
			perror("ERROR: ");
		}else{
			perror("ERROR: ");
		}			
		exit(0);
	}
}
// ---------------------------------------------------------------------------------------------------------------

/* ---------------------------------------------------------------------------------------------------------------
 * Name: redirect_prcIO
 *
 * Description: redirects output and input of the process
 *
 */
 
void redirect_prcIO ( Command *cmd) {
	int flags;
	mode_t mode;
	char * filename;
	if(cmd->rstdout){										// redirect output of process
		flags = O_WRONLY | O_CREAT | O_TRUNC;
		mode = S_IRUSR;
		filename = cmd->rstdout;
		int fdopen = open(filename,flags,mode);
		dup2(fdopen,1);
		close(fdopen);
	}
	if(cmd->rstdin){										// redirect input of process
		flags = O_RDONLY;
		filename = cmd->rstdin;
		int fdopen = open(cmd->rstdin,flags);
		dup2(fdopen,0);
		close(fdopen);
	}
}
// ---------------------------------------------------------------------------------------------------------------

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
