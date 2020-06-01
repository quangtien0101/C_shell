#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/wait.h>

//for open()
//https://www.man7.org/linux/man-pages/man2/open.2.html
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define MAX_LENGTH 80 // The maximum length of the commands
#define BUF_SIZE MAX_LENGTH/2 + 1


char *infile = "infile.txt";
char *outfile = "test_out_redirect.txt"; // this will  
int numsimplecommands = 1; // the total number of commands
int *background = 1; //this will be set when the user append the ampersand (&) to make the process running in the background

//testing the command $: cat < infile.txt >test_out_redirect.txt

void execute(char *argv[])
{
    //= how our commands should be parsed =======
    //char *argv[2];	
    //argv[0] = "cat";
    //argv[1] = NULL;
    //============================

    int tmpin=dup(0);
    int tmpout=dup(1);

    int fdin;
    if(infile)
    {
        //we need to check the existence of the infile too
        fdin = open(infile,O_RDONLY); //open a file with read only flag 
    }
    else
    {
        fdin=dup(tmpin);
    }
    
    int ret;
    int status;
    int fdout;
    for (int i=0; i <numsimplecommands; i++)
    {
        dup2(fdin, 0);
        close(fdin);

        if (i == numsimplecommands-1)
        {
            if(outfile)
            {
                printf("Executing OUTPUT redirect");
                fdout = open(outfile,O_WRONLY | O_CREAT); //open a file with write only flag 
            }
        
            else
            {
                printf("Executing else");
                fdout = dup(tmpout);
            }
        
        }
        else //Not last command --> pipe
            {
                printf("Piping");
                int fdpipe[2];
                pipe(fdpipe);
                fdout = fdpipe[1];
                fdin = fdpipe[0];
            }
        dup2(fdout, 1);
        close(fdout);

        ret = fork();
        if (ret == 0)
        {
            execvp(argv[0],argv); //it should be (cmd[i].argv[0],cmd[i].argv)
            perror("Failed when trying to fork");
            exit(1);
        }
    }

    dup2(tmpin, 0);
    dup2(tmpout, 1);
    close(tmpin);
    close(tmpout);

    if (!background)
    {
        waitpid(ret, &status,NULL);
    }
}

void parse_command(char command[], char* argv[], int *background)
{
	for (unsigned i=0; i<BUF_SIZE; i++) argv[i]=NULL;

	if (command[strlen(command)-1]=='&')
    {
		*background=1;
		command[strlen(command)-1]='\0';
	}
	
    else *background=0;

	const char *delim=" ";
	unsigned i=0;
	char *token=strtok(command, delim);
	while(token!=NULL)
    {
		argv[i++]=token;
		token=strtok(NULL,delim);
	}

	argv[i]=NULL;
}


int main()
{
    char command[MAX_LENGTH];

	char *args[BUF_SIZE]; // MAximum 40 argments

	int should_run = 1;

	while (should_run) {
		printf("ssh>>");
		fflush(stdout);

		while (fgets(command, MAX_LENGTH, stdin)==NULL){
			perror("You did not write anythign!");
			fflush(stdin);
		}
		command[strcspn(command,"\n")]='\0';

		// Youn can enter quit or \q to quit shell
		if (strcmp(command,"quit")==0 || strcmp(command,"\\q")==0){
			should_run=0;
			continue;
		}
		
		//Parse command and arguments.
		parse_command(command, args, &background);

        execute(args);
    }
    
    return 0;
}