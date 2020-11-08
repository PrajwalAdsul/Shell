#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#include <limits.h>
#include <signal.h>
#include "circularDLL.h"
 
// for colouring purpose
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

// args[0] will be the actual command name
char* args[512];
int argsCount = 0, command_pipe[2]; // store args count for current command
// required to implement history command
int histIndex, maxHistIndex;
char **history;
pid_t pid;
char line[128];
Node* root = NULL;


// Initialize shell
void shellInit(){
	histIndex = 0; 
	maxHistIndex = 16;
	history  = malloc(maxHistIndex * sizeof(char*));
}	
// Handler function
void ctrlc(){
	kill(pid, SIGKILL);
}
// Handler function
void ctrlz(){
	insert(&root, pid, args[0], "STOPPED");
	kill(pid, SIGSTOP);
}
// to change directory
int cd(char **args){
  	if (args[1] == NULL) 
    	fprintf(stderr, "cd expects a argument \n");
  	else 
  	{
  		// chdir is used to change directory, returns 0 on success
    	if (chdir(args[1]) != 0) 
      		perror("Bash: cd ");
  	}
  	return 1;
}
// helping the user to know all the commands
int help(){
  	printf("A star (*) next to a name means that the command is disabled.\n job_spec [&]\n              history [-c] [-d offset] [n] or hist>(( expression ))                        if COMMANDS; then COMMANDS; [ elif C>\n. filename [arguments]                  jobs [-lnprs] [jobspec ...] or jobs >\n:                                       kill [-s sigspec | -n signum | -sigs>\n[ arg... ]                              let arg [arg ...]\n[[ expression ]]                        local [option] name[=value] ...\n alias [-p] [name[=value] ... ]          logout [n]\n bg [job_spec ...]                       mapfile [-d delim] [-n count] [-O or>\n bind [-lpsvPSVX] [-m keymap] [-f file>  popd [-n] [+N | -N]\n break [n]                               printf [-v var] format [arguments]\n builtin [shell-builtin [arg ...]]       pushd [-n] [+N | -N | dir]\n caller [expr]                           pwd [-LP]\n case WORD in [PATTERN [| PATTERN]...)>  read [-ers] [-a array] [-d delim] [->\n cd [-L|[-P [-e]] [-@]] [dir]            readarray [-n count] [-O origin] [-s>\n command [-pVv] command [arg ...]        readonly [-aAf] [name[=value] ...] o>\n compgen [-abcdefgjksuv] [-o option] [>  return [n]\n complete [-abcdefgjksuv] [-pr] [-DE] >  select NAME [in WORDS ... ;] do COMM>\n compopt [-o|+o option] [-DE] [name ..>  set [-abefhkmnptuvxBCHP] [-o option->\n continue [n]                            shift [n]\n coproc [NAME] command [redirections]    shopt [-pqsu] [-o] [optname ...]\n declare [-aAfFgilnrtux] [-p] [name[=v>  source filename [arguments]\n dirs [-clpv] [+N] [-N]                  suspend [-f]\n disown [-h] [-ar] [jobspec ... | pid >  test [expr]\n echo [-neE] [arg ...]                   time [-p] pipeline\n enable [-a] [-dnps] [-f filename] [na>  times\n eval [arg ...]                          trap [-lp] [[arg] signal_spec ...]\n exec [-cl] [-a name] [command [argume>  true\n exit [n]                                type [-afptP] name [name ...]\n export [-fn] [name[=value] ...] or ex>  typeset [-aAfFgilnrtux] [-p] name[=v>\n false                                   ulimit [-SHabcdefiklmnpqrstuvxPT] [l>\n fc [-e ename] [-lnr] [first] [last] o>  umask [-p] [-S] [mode]\n fg [job_spec]                           unalias [-a] name [name ...]\n for NAME [in WORDS ... ] ; do COMMAND>  unset [-f] [-v] [-n] [name ...]\n for (( exp1; exp2; exp3 )); do COMMAN>  until COMMANDS; do COMMANDS; done\n function name { COMMANDS ; } or name >  variables - Names and meanings of so>\n getopts optstring name [arg]            wait [-n] [id ...]\n hash [-lr] [-p pathname] [-dt] [name >  while COMMANDS; do COMMANDS; done\n help [-dms] [pattern ...]               { COMMANDS ; }\n"); 
  	return 1;
}

// to print history (previous commands)
int hist(){
	int i = 0;
	for(i = 0; i < histIndex; i++)
		printf(" %d %s \n", i, history[i]);
	return 1;
}
// executing the command
int command(int input, int command_type){
	int pipefd[2];

 	// pipe() returns -1 on failure
 	if(pipe(pipefd) == -1){
		return -1;
	}

	// create a new process by duplicating the calling process
	pid = fork();
	// fork returns zero on successfully creating a child process 
	// STDOUT_FILENO is an integer file descriptor for write sys call and STDIN_FILENO for read sys call
	if (pid == 0) {
	
		// dup(int fd) creates copy of file descriptors, 
		// dup2(int oldfd, int newfd) uses the new file descriptor specified by the user

		// command_type is 0 for first command
		if (command_type == 0 && input == 0) {
			dup2(pipefd[1], STDOUT_FILENO);
		} 
		else if (command_type == 1 && input != 0) {  // command_type is 1 for middle command
			dup2(input, STDIN_FILENO);
			dup2(pipefd[1], STDOUT_FILENO);
		} 
		else {                                       // command_type is 2 for last command
			dup2(input, STDIN_FILENO);
			int i, j;
			// Handling redirections 
			for(i = 0; i < argsCount - 1; i++){
				// output redirection
				if(strcmp(args[i], ">") == 0){
					char file[128];
					// args[i + 1] will be the filename specified by the user 
					strcpy(file, args[i + 1]);
					// we dont need argument args[i] now and make it NULL for correct execution of execvp
					args[i] = (char*)NULL;
					int fd = open(file, O_RDWR | O_CREAT | O_SYNC, S_IRUSR | S_IWUSR);					
					// instead of writing to STDOUT, write output to a file, hence duplicating
					dup2(fd, 1);
					// first argument for execvp(cmd, args) is actual command and next is required arguments
					execvp(args[0], args);
					return 1;
				}
				if(strcmp(args[i], "<") == 0){
					char file[128];					
					// args[i + 1] will be the filename specified by the user
					strcpy(file, args[i + 1]);
					// all other arguments from and after '<' are not required now and should be NULL compulsorily for execution using execvp
					for(j = i; j < argsCount; j++){
						args[j] = (char*)NULL;
					}
					int fd = open(file, O_RDONLY, S_IRUSR | S_IWUSR);
					// error in opening a defined file 
					if(fd < 0){
						perror(file);
						return -1;
					}
					// instead of reading form STDIN, read from a file, hence duplicating
					dup2(fd, 0);
					close(fd);
					// first argument for execvp(cmd, args) is actual command and next is required arguments
					execvp(args[0], args);
					return 1;
				}
			}
		}
		if (execvp(args[0], args) == -1){
			printf("%s: command not found\n", args[0]);
			_exit(EXIT_FAILURE); 
		}
	}
 	if (input != 0) 
		close(input);
 	close(pipefd[1]);
 	if (command_type == 2)
		close(pipefd[0]);
 	return pipefd[0];
}
// function to skip white spaces 
char* whitespaces(char* s)
{
	// skip all the elements of character array which are consecutive white spaces
	while (isspace(*s)) 
		s++;	
	// return once there is no white space at present index
	return s;
}

// function break cmd into respective words  
void tokenize(char* cmd)
{
	// strchr locates first occurance of a character in string
	// Here we find whitespace and skip the whitespace and only take non space charaters and tokenize them
	// eg :-   "grep    word  file" will tokenize to {"grep", "word", "file"} with argsCount as 3 i.e total 
	// number of arguments

	cmd = whitespaces(cmd);
	char* next = strchr(cmd, ' ');
	int i = 0;
 	argsCount = 0;
	while(next != NULL) {
		next[0] = '\0';
		args[i] = cmd;
		++i;
		argsCount++;
		cmd = whitespaces(next + 1);
		next = strchr(cmd, ' ');
	}

	if (cmd[0] != '\0') {
		args[i] = cmd;
		next = strchr(cmd, '\n');
		next[0] = '\0';
		++i; 
		argsCount++;
	}
	// from i index we dont need anything, making ith index NULL helps execvp in execution
	args[i] = NULL;
}
// Print all the stopped jobs
int jobs(){
	printDLL(root);
	return 0;
}
// foreground process
int fg(){
	signal(SIGINT, ctrlc);
	// If atleast one process was stopped 
	if(!isEmpty(root)){
		// If only fg is typed 
		if(argsCount == 1)
		{
			// get the pid of the last stopped process i.e LIFO system
			pid = root -> prev -> pid;
			delete(&root, pid);
			// send SIGCONT (continue) signal to the stopped process
			kill(pid, SIGCONT);
			// wait till processing is done by user
			wait(NULL);
		}
		else{
			// If second argument is specified i.e %srno is given
			if(args[1][0] == '%'){
				int i = 0;
				for(i = 1; i < strlen(args[1]); i++){
					args[1][i - 1] = args[1][i];
				}
				args[1][strlen(args[1]) - 1] = '\0';
				// get the pid using srno 
				pid = gettPid(root, atoi(args[1]));
				delete(&root, pid);
				// send SIGCONT (continue) signal to the stopped process
				kill(pid, SIGCONT);
				// wait till processing is done by user
				wait(NULL);
			}
		}
	}
	else{ // If there is no stopped process
		printf("bash: fg: current: no such job\n");
	}
   return 0;
}
// Background process
int bg(){
	// If atleast one process was stopped 
	if(!isEmpty(root)){
		int pfg = root -> prev -> pid;
		delete(&root, pfg);
		// send SIGCONT (continue) signal to the stopped process
		kill(pfg, SIGCONT);	
		// dont wait in bg command, just continue the execution of the stopped process
	}
	else{ // If there is no stopped process
		printf("bash: fg: current: no such job\n");
	}
}
// run the command 
int run(char* cmd, int input, int command_type)
{
	// tokenize the cmd, i.e break it into words, separating the main command and its arguments
	
	tokenize(cmd);
	// there atleast one literal in cmd
	if (args[0] != NULL) {
		// check if command is different i.e wrote separate code for below commands
		if (strcmp(args[0], "exit") == 0) 
			exit(0);
		else if(strcmp(args[0], "cd") == 0)
			return cd(args);
		else if(strcmp(args[0], "help") == 0)
			return help();
		else if(strcmp(args[0], "jobs") == 0)
			return jobs();
		else if(strcmp(args[0], "fg") == 0)
			return fg();
		else if(strcmp(args[0], "bg") == 0)
			return bg();
		else if(strcmp(args[0], "history") == 0)
			return hist();
		else
			return command(input, command_type);  // run the command
	}
	return 0;
}

// function to get present working directory
char* pwd(){
	char cwd[PATH_MAX]; 
	return getcwd(cwd, sizeof(cwd)); 
}

// function to input command and process it further
void command_line(){
	// getenv("USER") is used to get the user_name from your linux system
	printf("%s%s@:%s~%s$ %s", KGRN, getenv("USER"), KBLU, pwd(), KWHT);
	// flushes all open output streams
	fflush(NULL);
	// Using fgets because fgets will read a string with white spaces too, scanf fails to read strings with white spaces
	if (!fgets(line, 128, stdin)) 
		return;		
	int input = 0; 
	int command_type = 0;
	// cmd will broken further for individual execution of commands
	// line char array will be used to save command in history array	
	char* cmd = line;
	char* next = strchr(cmd, '|'); // Find first '|' 

	// for pipelining multiple commands, commands are to be separated with |
	// strchr() function returns a pointer to the first occurrence of the character c in the string 
	while (next != NULL) {
		// 'next' points to '|' 
		*next = '\0';

		// Put the present command in execution
		// In pipes output of first command is given as input to next command, hence we keep the fd and use it for next command
		input = run(cmd, input, command_type);
			
			// getting next command in pipes 
		cmd = next + 1;
		next = strchr(cmd, '|'); // Find next '|'

		// once first command is executed 
		//first = 0;
		command_type = 1;
	}

	// running the last command 
	command_type = 2;
	input = run(cmd, input, command_type);

	// adding commmand line to history
	if(histIndex >= maxHistIndex){
		maxHistIndex = maxHistIndex * 2;
		history  = realloc(history, maxHistIndex * sizeof(char*));
	}
	history[histIndex] = (char*)malloc((int)strlen(line) * sizeof(char));
    strcpy(history[histIndex++], line);
}
void loop(){
	while(1){
		// print and take input
		command_line();
		if(pid) {
			//wait till child has changed state
			waitpid(pid, 0, WUNTRACED);
			pid = 0;
		}		
	}
}
// Driver function
int main()
{
	signal(SIGINT, ctrlc);
	signal(SIGTSTP, ctrlz);
	signal(SIGCHLD, SIG_DFL);	
	shellInit();
	loop();
	return 0;
}
