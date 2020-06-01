#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
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


int main()
{
    char tmp[40]; //maximum lenght of a single command

    char command[80] = "cat file1.txt   | lol | less | yourmumgay < hahagay.lul";
    
    int len = strlen(command);

    char **c = malloc(30); // we can have up to 30 commands piping together

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
        stripping_whitespace(c[x],trimmed_temp);
        printf("Command: %s with the length is %ld \n",c[x],strlen(c[x]));

    }
    
    

    return 0;
}