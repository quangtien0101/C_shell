#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

//for open()
//https://www.man7.org/linux/man-pages/man2/open.2.html
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define MAX_LENGTH 80 // The maximum length of the commands
#define BUF_SIZE MAX_LENGTH/2 + 1


//char *infile = "infile.txt";
//char *outfile = "test_out_redirect.txt"; // this will  
//int numsimplecommands = 1; // the total number of commands
//int *background = 1; //this will be set when the user append the ampersand (&) to make the process running in the background

int  getSubString(char *source, char *target,int from, int to)
{
    //courtesy of https://www.includehelp.com/c-programs/extract-a-portion-of-string-substring-extracting.aspx
	int length=0;
	int i=0,j=0;
	
	//get length
	while(source[i++]!='\0')
		length++;
	
	if(from<0 || from>length){
		printf("Invalid \'from\' index\n");
		return 1;
	}
	if(to>length){
		printf("Invalid \'to\' index\n");
		return 1;
	}	
	
	for(i=from,j=0;i<=to;i++,j++){
		target[j]=source[i];
	}
	
	//assign NULL at the end of string
	target[j]='\0'; 
	
	return 0;	
}

void trim_trailing(char * str)
{
    int index, i;

    /* Set default index */
    index = -1;

    /* Find last index of non-white space character */
    i = 0;
    while(str[i] != '\0')
    {
        if(str[i] != ' ' && str[i] != '\t')
        {
            index= i;
        }

        i++;
    }

    str[index + 1] = '\0';
}


int stripping_whitespace(char *un_trimmed, char *trimmed) //this will trim all whitespaces of a command
{
    char ch;
    int index = 0;
    //first we need to trimmed the beginning spaces
    for (int i=0; i<strlen(un_trimmed);i++)
    {
        ch = un_trimmed[i];
        if (isspace(ch))
            index++;
        if (!isspace(ch))
            break;
    }
    if (getSubString(un_trimmed,trimmed,index,strlen(un_trimmed)) == 0)
    {
        //printf("The trimmed command: %s\n",trimmed);
        //copying the trimmed version to the un_trimmed
        strcpy(un_trimmed,trimmed);
    }

    trim_trailing(un_trimmed);
    
    return 0;
}

char **command_tokenize(char *command)
{
    char **temp = malloc(80*sizeof(char *));
    const char *delim=" ";
	unsigned i=0;
	char *token=strtok(command, delim);
	while(token!=NULL)
    {
		temp[i]=token;
        i++;
		token=strtok(NULL,delim);
	}

    temp[i]=NULL;
    return temp;
}

//testing the command $: cat < infile.txt >test_out_redirect.txt
void execute(char **c, int numsimplecommands, char *infile, char *outfile, int background)
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
        printf("In file exist!");
        getchar(); // this use to debug
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
                getchar();
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
            //here we will start to tokenize our command
    
            char ** argv = command_tokenize(c[i]);

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



void parse_and_run(char command[], char **c)
{
    char *infile = "\0";
    char *outfile = "\0";

    int background = 0;
	if (command[strlen(command)-1]=='&') // user want the process running in the background
    {
		background=1;
		command[strlen(command)-1]='\0';
	}
	
    char tmp[40]; //maximum lenght of a single command
    
    int len = strlen(command);

    int command_index = 0; //default there will be 1 command, and for every pipe character -> one more command

    printf("the command is: %s\n",command);
    printf("the length of the command is: %d\n",len);

    int old_command_index = 0; //keep track of the index of the last command
    //now we need to find the index of the pipe character
    for (int i = 0; i < len; i++ )
    {
        if(command[i] == 124 || command[i] == 62 || command[i] == 60) //the "|",">","<" character
        {
            strncpy (tmp, command, i);
            tmp[i] = '\0';         //add NULL to terminate the copied string
            printf("scanning through: %s\n", tmp);


            c[command_index] = malloc(strlen(tmp));
            if (getSubString(command,c[command_index],old_command_index,i-1) == 0)
            {
                //trimmimg the command

                printf("The extract command is: %s \n",c[command_index]);
            }
            command_index ++;
            old_command_index = i+1; //update since now we have a new command
        }

    }

    printf("Command index is: %d\n",command_index);
    printf("Printing the parsing commands: \n");
    char *trimmed_temp;
    for (int x = 0; x < command_index ;x++)
    {
        printf("Stripping white spaces\n");
        stripping_whitespace(c[x],trimmed_temp);
        printf("Command: %s with the length is %ld \n",c[x],strlen(c[x]));

    }
    printf("\nNow we executing the command:\n");

    //execute(c,command_index,infile,outfile,background);
    

}


int main()
{
    char command[MAX_LENGTH];

	char **c = malloc(50); // we can have up to 50 commands

	int should_run = 1;

	while (should_run) 
    {
        printf("ssh>>");

		fflush(stdout);
        if (fgets(command, 80, stdin) == NULL) 
        {
            fprintf (stderr, ("fgets failed"));
            return -1;
        }

		command[strcspn(command,"\n")]='\0';

		// Youn can enter quit or \q to quit shell
		if (strcmp(command,"quit")==0 || strcmp(command,"\\q")==0){
			should_run=0;
			continue;
		}
		
		//Parse command and arguments.
		parse_and_run(command, c);

        //execute(args);
    }
    
    return 0;
}