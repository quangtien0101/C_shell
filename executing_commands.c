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
//int number_of_commands = 1; // the total number of commands
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
    if (strlen(un_trimmed) < 1){
        return 0;
    }
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

char **command_tokenize(char *command) //format the command so it can be use in execvp
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
void execute(char **c, int number_of_commands, char *infile, char *outfile, int background)
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
        //printf("In file exist!\n");
        //we need to check the existence of the infile too
        fdin = open(infile,O_RDONLY); //open a file with read only flag 
    }
    else
    {
        fdin=dup(tmpin);
    }
    
    int child;
    int status;
    int fdout;
    for (int i=0; i <number_of_commands; i++)
    {
        //printf("%d\n",i);
        dup2(fdin, 0);
        close(fdin);

        if (i == number_of_commands-1)
        {
            if(outfile)
            {
                //printf("Executing OUTPUT redirect!\n");
                //getchar();
                fdout = open(outfile,O_WRONLY | O_CREAT); //open a file with write only flag 
            }
        
            else
            {
                //printf("Executing else\n");
                fdout = dup(tmpout);
            }
        
        }
        else //Not last command --> pipe
            {
                //printf("Piping\n");
                int fdpipe[2];
                if (pipe(fdpipe) != 0)
                {
                    //printf("failed to create pipe");
                }
                else 
                {
                    fdout = fdpipe[1];
                    fdin = fdpipe[0];
                    //printf("Finish creat pipe\n");
                }
                
            }
        dup2(fdout, 1);
        close(fdout);

        child = fork();
        //printf("Finish fork?");
        if (child == 0)
        {
            //here we will start to tokenize our command
    
            char ** argv = command_tokenize(c[i]);
            //printf("Trying to fork\n");
            execvp(argv[0],argv); //it should be (cmd[i].argv[0],cmd[i].argv)
            perror("Failed when trying to fork\n");
            exit(1);
        }
        dup2(tmpin, 0);
        dup2(tmpout, 1);
    }
    close(tmpin);
    close(tmpout);


//    dup2(tmpin, 0);
//    dup2(tmpout, 1);
//    close(tmpin);
//    close(tmpout);

    if (!background) //commands run in the background
    {
        waitpid(child, &status, 0);
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

    //printf("the command is: %s\n",command);
    //printf("the length of the command is: %d\n",len);

    int infile_bit = 0; //this will be set if our parsing is reading the infile redirection
    int outfile_bit = 0; // this will be set if our parsing is reading the outfile redirection

    int old_command_index = 0; //keep track of the index of the last command
    //now we need to find the index of the pipe character
    for (int i = 0; i < len; i++ )
    {
        if(command[i] == 124 || command[i] == 62 || command[i] == 60) //the "|",">","<" character and the end of the comamnd
        {
            strncpy (tmp, command, i);
            tmp[i] = '\0';         //add NULL to terminate the copied string
            //printf("scanning through: %s\n", tmp);

            if (outfile_bit == 0 && infile_bit == 0) // parsing command
            {
                c[command_index] = malloc(strlen(tmp));
                
                getSubString(command,c[command_index],old_command_index,i-1);
                //printf("The extract command is: %s \n",c[command_index]);
                
                command_index ++;
                old_command_index = i+1;
            }

            if (infile_bit == 1) // reading infile descriptor
            {
                infile = malloc(strlen(tmp));
                getSubString(command,infile,old_command_index,i-1);
                    //trimmimg the command
                    //printf("The extract infile descriptor is: %s \n",infile);
                old_command_index = i+1; //update since now we have a new command
                infile_bit = 0; //reset the bit since we're done reading the infile descriptor
            }

            if (outfile_bit == 1) // reading outfile descriptor
            {
                outfile = malloc(strlen(tmp));
                getSubString(command,outfile,old_command_index,i-1);
                //printf("The extract outfile descriptor is: %s \n",outfile);
               
                old_command_index = i+1; //update since now we have a new command / file descriptor
                outfile_bit = 0; //reset the bit since we're done reading outfile descriptor
            }

            if (command[i] == 60) // Saw the < character, inform the shell that the next argument will be the infile descriptor 
            {
                //printf("Find infile!\n");
                infile_bit = 1;
                outfile_bit = 0;
            }


            else if (command[i] == 62)// Saw the > character, inform the shell that the next argument will be the infile descriptor
            {
                //printf("Find outfile!\n");
                outfile_bit = 1;
                infile_bit = 0;
            }        
        }
        
        if (i == len - 1) // reach the end of the string
        {
            strncpy (tmp, command, i+1);
            tmp[i+1] = '\0';//add NULL to terminate the copied string
            //printf("scanning through: %s\n", tmp);

            if (infile_bit == 1)
            {
                infile = malloc(strlen(tmp));
                if (getSubString(command,infile,old_command_index,i) == 0)
                {
                    //trimmimg the command
                    //printf("The extract infile descriptor is: %s \n",infile);
                }
                old_command_index = i+1; //update since now we have a new command
                infile_bit = 0;
            }
            else if (outfile_bit == 1)
            {
                outfile = malloc(strlen(tmp));
                if (getSubString(command,outfile,old_command_index,i) == 0)
                {
                    //trimmimg the command
                    //printf("The extract outfile descriptor is: %s \n",outfile);
                }
                old_command_index = i+1;
                outfile_bit = 0;
            }
            
            else 
            {
                c[command_index] = malloc(strlen(tmp));
                if (getSubString(command,c[command_index],old_command_index,i) == 0)
                {
                    //trimmimg the command

                    //printf("The extract command is: %s \n",c[command_index]);
                }
                command_index ++;
                old_command_index = i+1;
            }
        }

    }

    //printf("Command index is: %d\n",command_index);
    //printf("Printing the parsing commands: \n");
    char *trimmed_temp; //a temporary string
    for (int x = 0; x < command_index ;x++)
    {
        //printf("Stripping white spaces\n");
        stripping_whitespace(c[x],trimmed_temp);        

        printf("Command: %s with the length is %ld \n",c[x],strlen(c[x]));
    }

    stripping_whitespace(infile,trimmed_temp);
    stripping_whitespace(outfile,trimmed_temp);


    execute(c,command_index,infile,outfile,background);
    

}


int main()
{
    char command[MAX_LENGTH];

	char **c = malloc(40); // we can have up to 40 commands

	int should_run = 1;

	while (should_run) 
    {
        printf("ssh>>");

		fflush(stdout);
        memset(&command[0], 0, sizeof(command)); // clean the command
        gets(command);
        
		command[strcspn(command,"\n")]='\0'; // add the null byte 

		// exit the shell
		if (strcmp(command,"exit")==0)
        {
			should_run=0;
			continue;
		}
		
		//Parse commands and execute 
		parse_and_run(command, c);
    }
    
    return 0;
}
