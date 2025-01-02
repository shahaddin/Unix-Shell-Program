//Shahaddin Gafarov
//A simple shell program that imitates real shell environment. My shell program recognizes the builtin commands such as "path", "cd", "exit" and does the corresponding operations for the shell.
//When a general command is given such as pwd,ls,cal etc. the call gets redirected using execv() functiuon which checks all the available path where the command could run. And if there is no path for the command to run, or in case of invalid command the shell program returns an error message.
//There is also special care for redirection operations. For example, echo  5   >   os.txt, will make sure that the tabs or spaces are ignored and will succesfully write 5 into os.txt file, and add it to the current directory from path list.
//Another important implementation is the use of parallel commands, which makes it available for the shell to run more than one commands at the same time, rather than waiting for the child process to finish. The fastest gromgram to finish will also be the one which will get displayed first. Command example is ls & ls, ls         &   pwd     & echo 5

//This is the list of libraries we need for our functions to work 
//One example from each such as execv(), printf(), malloc(), strlen(), wait(), file manipulation(line 139), isspace(0)
#include "unistd.h"
#include "stdio.h"
#include <stdlib.h>
#include "string.h"
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h> 

//xxx is for the while loop, which acts as a forever ongoing loop
int xxx = 1;
//Default error message
char error_message[30] = "An error has occurred\n";
//Since character limit is 255, there can theoretically be 255 paths at most
char *path_list[255];
//we will need this for redirection(>) and ampersand(&) later on
char *end;

//we will need this for keeping track of number of paths
//by default we have /bin, which makes the count 1
int paths_count=1;

//when the user inputs cd to rush> the code will redirect to the cd function
int cdfunction(char **commands, int command_count)
{
    //cd needs strictly two commands
    if(command_count !=2)
    {
        //printf("command_count is not 2\n");
        return 0;
    }

    //if chdir(0 fails for some reason, then there is some error going on with directory change)
    if (chdir(commands[1]) == -1)
    {
        //printf("coomads[1] == -1 \n");
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 0;
    }

    //if everything works we return a 1, for success
    return 1;
}

//when the user inputs path to rush> the code will redirect to the custom path function
void path_function(char **commands, int command_count)
{
    //I have set the number of paths to be 255 at max, if there are more than 255 paths on a single line, then we likely exceed the 255 character limit
    if(command_count - 1 > 255)
    {
        //printf("command_count > 255\n");
        write(STDERR_FILENO, error_message, strlen(error_message));
    }

    //simply "cleans" the path by nullifying every string in the path list when path is called at rush>
    for(int i=0;i<255;i++)
    {
        path_list[i] = NULL;
    }

    //path_count will help to keep the track of number of paths
    paths_count = 0;
    //the reason we start from 1 is commands[0] is the word path itself instead of some path directory
    for (int i=1; i< command_count; i++)
    {
        path_list[paths_count++] = commands[i];
    }
    
    // //Testing what are the pathes at path_list
    // for(int i=0; i< paths_count; i++)
    // {
    //     printf("Path List: %s\n", path_list[i]);
    // }
    // printf("\n");
    
}

//if the input is not a builtin command(path, cd, exit), we will be using execv()
void execute_function(char **commands, int command_count, char *final_file)
{
    //full_path will be explained below(line 97), it basically as itt name suggest gives the full_path by merging the command and the path
    static char full_path[255];
    //check is for understanding whether at least a single command runs with the paths from path_list
    int check = 0;
    //the command is always first input
    char *command = commands[0];

    //go through every path list
    for(int i=0; i<paths_count; i++)
    {
        //combine the command with the path_list's path
        //for example if rush> ls and path is /bin, full_path will be bin/ls
        snprintf(full_path, sizeof(full_path), "%s/%s", path_list[i], command);
        //printf("%s", full_path);
        //this line checks whether the fully combined path is accessible or not
        if (access(full_path, X_OK) == 0) 
        {
            //if the code works with at least one of the path at path_list, that's all we need to display
            check = 1; 
            break;
        }
    }

    //if the command did not work with any of the paths given, print an error
    //Which means-->either the command itself was wrong, or the paths code currently has doesn't have access for the particular command
    if(check == 0)
    {
        //printf("check=0\n");
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(0);
    }

    //if check==1, the command worked at a path from path_list
    else
    {
        //we crate a new process with fork()
        pid_t pid = fork();

        //if we are at parent directory, we wait child to finish
        if(pid > 0)
        {
            //printf("parent\n");
            wait(NULL);
        }

        //if we are at the child directory we call the execv()
        else if(pid == 0)
        {
            //printf("child\n");
            //some of the commands such as ls > .txt or echo 5 > .txt need to make sure a file exists to write the output in the first place
            if(final_file != NULL)
            {
                //make sure we have the access to the file
                int fd = open(final_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                //if there is an error, exit
                if(fd < 0)
                {
                    //write(STDERR_FILENO, error_message, strlen(error_message));
                    exit(1);
                }

                //creates the file if it does not exist, makes it ready for the execv()
                else
                {
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }
            }

            //given the full paths to the command, and the command itself we will execute the command
            execv(full_path, commands);
            exit(1);

        }

        //An error with the fork() occured
        else
        {
            //printf("pid = -1, fork failed\n");
            write(STDERR_FILENO, error_message, strlen(error_message));

        }
    }
}

//The code has to be declared in a separate void funvtion for the code to work parallel and not sequential
//if the code below is run at main(), it will lead to a sequantial run, which is not what the program needs.
void parallelizer(char **commands_parallel, int parallel_count)
{
    //keeps the list of processes
    //Since there is the 255 character limit, no more than 255 pids can exist
    pid_t pid_list[255];
    //number of child processes
    int children_number = 0;

    //cpn = current parallel number
    for (int cpn = 0; cpn < parallel_count; cpn++) 
    {
        //commands parallel keeps the list of parallel commands, which is defined at main()
        //retrieve first of te parallel command to run
        char *string = commands_parallel[cpn];
        char *final_file = NULL;

        //this line looks for > command in the string
        char *after_bigger_sign = strchr(string, '>');
        
        //if there is an >, then it is a ls >.txt or echo .. >.txt type operation
        //the code below is dedicated wholely for redirection case(>)
        if(after_bigger_sign != NULL)
        {
            //point a separate string to the beginin of the cpn-th parallel command
            char *bigger=string;

            int bigger_count=0;

            //this is basically for skipping all the tabs or empty spaces at the start
            //I used to define these using strcmp and ' ', however it was not working for tab(\t)
            //isspace() is a great alternative as a C library command that checks empty spcaes(whether ' ' or '\t' for me)
            while(isspace((unsigned char)*bigger))
            {
                bigger++;
            }

            //bigger helps the case 22 on Gradescope, which checks for illegal operations such as > existing but no preceeding command:
            //rush>     > text.txt is an illegal operattion
            //if the first non-space character of *bigger is >, then that means there is no preceeding combat --->an error
            if(*bigger == '>')
            {
                //printf("the > does not have a preceeding command\n");
                write(STDERR_FILENO, error_message, strlen(error_message));
                continue;
            }

            //this takes a count of redirections(>)
            //if there are more than 2 redirections then the input will return an error:
            //rush> ls > tmp.txt > tmp.txt or ls >> text.txt ---> error
            for(;*bigger;bigger++)
            {
                if(*bigger == '>')
                {
                    bigger_count++;
                }
            }

            //if there is more than 1 redirection, then there is an error with input, as explained above
            if(bigger_count!=1)
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
                continue;
            }

            //For example, ls > tmp.txt turns into ----> ls \0 tmp.txt
            *after_bigger_sign = '\0';
            //file name is basically whatever comes after the >, for example:
            //ls > tmp.txt ->  tmp.txt 
            char *file_name = after_bigger_sign + 1;

            //this is for skipping the empty part at the start of the file name
            while(isspace((unsigned char)*file_name))
            {
                file_name++;
            }
            //printf("File name with spaces before removed:%s\n", final_file);  

            
            //if the file name is empty, we can not write contents of ls there
            //which is basically same as inaccessable file 
            char *testtt = strchr(file_name, ' ');
            //testtt checks if the strchr can still find empty spaces in between commands
            if(*file_name == '\0' || testtt != NULL)
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
                continue;
            }

            //we put the file_name into final_file which will be what execution will look for
            final_file = strdup(file_name);
            //and we skip the spaces at the end of the file name just tlike we did for the beginning
            end = final_file + strlen(final_file) - 1;
            while(end > final_file && isspace((unsigned char)*end))
            {
                end--;
            }
            //if the file name is hello\t\t\t\t... -> it becomes hello
            //but we still need endline for a proper string declaration so we add the \0 to the end
            //"hello" -> "hello\0"
            *(end + 1) = '\0';
        }

        //the code below is for dividing thte into into command list
        int i = 0;
        //hold each string until a ' ' is found(This also includes \t because of line 393 )
        char *separate;
        //commands array holds all lists of commands(again the 255-rule)
        char **commands = malloc(255 * sizeof(char *));
        //keep the track of command count
        int command_count = 0;

        //https://c-for-dummies.com/blog/?p=1769
        //strsep(&string," ") divides the string based on the empty space character until reaching the end(!+NULL)
        while((separate = strsep(&string," ")) != NULL)
        {
            //if the separeted string is ont null character, then skip it
            if(strcmp(separate, "\0") == 0) 
            {
                continue;
            }
            //if not store the separated string in commands list,and go to the next storage in the list with i++
            //printf("%s \n", commands[i]);
            commands[i] = separate;
            i++;
            //printf("%s \n", separate);
            //technically command_count and i is the same number, but the difference was because of an earlier prototype
            command_count++;
        }
        //since i++ will always be one ahead(we increment from 0), put the ith iteration to be null as a way of knowing when the command list ends similar to what \0 does
        commands[i] = NULL;  

        //if there is no commands(press enter or with empty space)
        //then do nothing
        if(command_count == 0) 
        {
            free(commands);
            free(string);
            continue;
        }
        
        else
        {
            //handling builtin commands
            //the first builtin command is exit
            //we make sure exit comes first in the command list
            if(strcmp(commands[0], "exit") == 0)
            {
                //the call sould have only "exit", if typed rush> exit ls for example, then return an error
                if(command_count > 1)
                {
                    //printf("EXIT WITH Too many commands%s", error_message);
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    //continue;
                }

                //if everything is proper, free the memory, and exit
                else
                {
                    // Clean up and exit
                    free(commands);
                    //free(final_file);
                    //free(string);
                    exit(0);                        
                }
            }
            //if first command is cd(built-in):
            else if(strcmp(commands[0], "cd") == 0)
            {
                //we run the custom cd command, 0 is a sign of error for various reasons ecplained in the cd function
                if(cdfunction(commands, command_count) == 0)
                {
                    //printf("sss%s", error_message);
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                //if cd ==1, then everything is already handles by the cd function properly, no need for other changes
            }
            //if the first command is path(built-in)
            else if(strcmp(commands[0], "path") == 0)
            {
                //path function will handle the commands_array's input and the number of commands
                //it will free up all the previous path, and put the new ones on path_list to be used by execution function later on
                path_function(commands, command_count);
            }

            //lastly if the first command is not necessarily a built-in command, then we execute whatever the command is based on given path at the moment
            else
            {
                //create a new process
                pid_t pid = fork();
                //if we are at the child process, execute the command and exit
                if (pid == 0) 
                {
                    execute_function(commands, command_count, final_file);
                    exit(0);
                } 
                //if we are at the parent ptocess, we add the pid to the respective pid_list to keep a record on child procces
                //also increment the children_number for that at the same time, which we will need in line 389
                else if (pid > 0) 
                {
                    // Parent process
                    pid_list[children_number++] = pid;
                } 

                //if we are in neither the parent nor the child process, then it means that fork() has failed for some reason
                else 
                {
                    //printf("eska%s", error_message);
                    write(STDERR_FILENO, error_message, strlen(error_message));

                }
            }
            //like other cases, free up the memory
            free(commands);
            //free(string);
            //free(final_file);
        }
        free(string);
    }
    //Although the code is parallel and not sequantial, we still have to wait for all them to finish to not have a chaotic output that would trample basic commands like rush>
    for (int i = 0; i < children_number; i++) 
    {
        //wait for each pid to finish based on their order(again this doesn;t mean the code is sequential)
        waitpid(pid_list[i], NULL, 0);
    }
}

//main function, argc will be useful for initial ./rush command only running with nothing else
int main(int argc, char *argv[])
{
    //if ./rush runs with something else then return an error
    //This line also fixes the infinite loop issue of my very first submission, where the grader was showing time fault(more than 10 minutes to run)
    if (argc>1) 
    {
        //printf("argc error");
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    //default path(s)
    path_list[0] = "/bin";
    //basically runs forver, since xxx is defined to be 1
    while(xxx == 1)
    {
        //string and len will store the string of the input and the size of input
        char *string = NULL;
        size_t len = 0;
        ssize_t input;

        printf("rush> ");
        fflush(stdout);
        //input stores the result of getline(), this value will be used at line 434
        input = getline(&string, &len, stdin);
        //turn tabs into empty spaces
        for (int i = 0; string[i]; i++) 
        {
            if (string[i] == '\t') 
            {
                string[i] = ' ';
            }
        }

        //if inputt is -1, or simpply less than 0, then there was some problem with the getline() operation
        if(input < 0)
        {
            //printf("%s", error_message);
            write(STDERR_FILENO, error_message, strlen(error_message));
            free(string);
            continue;
        }
        
        //if there is no error, input is basically the number of characters read by getline()
        //since we don't want the newline character we replace it with null character
        //the if case is to make sure if newline exits in first place(it might not have existed if the input is empty, too short, etc.)
        if (string[input - 1] == '\n') 
        {
            string[input - 1] = '\0';
        }

        //command_parallel as its name suggests keeps the list of parallel command
        //if inputt is ls & ls & pwd, it will store ls, ls, and pwd(without spaces of course)
        //this array size is also 255 by the "255 rule", check line 25 and 54 for further explanation
        char *commands_parallel[255];

        //parallel count is for keeping the track on numbers of parallel commands
        int parallel_count = 0;
        //we need save_string for strtok for saving the token info
        char *save_string;
        
        //ampersand will hold the istring until first & is encountered
        //if rush>      ls > text.txt   & pwd & cal then ampersand holds"      ls > text.txt    "
        char *ampersand = strtok_r(string, "&", &save_string);
        while (ampersand != NULL) 
        {
            //if more than 255 parallel command is called, then we have likely breached thte 255 character limit
            if(parallel_count > 255)
            {
                //printf("There are more than 255 parallel commands running");
                //printf("%s", error_message);
                write(STDERR_FILENO, error_message, strlen(error_message));
                continue;
            }

            //using isspace fixes \t issues
            //the same logic as the redirection(>)
            //we first get rid of empty spaces at the start
            while (isspace((unsigned char)*ampersand))
            {
                ampersand++;
            }

            //then we get rid of empty spaces from the end 
            char *end= ampersand + strlen(ampersand) - 1;
            while (end>ampersand && isspace((unsigned char)*end))
            {
                end--;
            }
            //and lastly but null character at the end, for a proper string use(so we can now where it ends)
            *(end + 1) = '\0';

            //if the input is not empty, we store the empty space filtered parallel command in its respective array
            //we don't need to anything for an empty ampersand rush > & ---> rush>
            if (strlen(ampersand) > 0) 
            {
                commands_parallel[parallel_count] = ampersand;
                parallel_count++;
            }
            
            //get to next string before an ampersand(next token basically which the save_string holds the info of)
            ampersand = strtok_r(NULL, "&", &save_string);
        }

        //now that we have all the parallel commands, we can now call the parallelizer(which will do the rest)
        //usually the codes inside parallelizer were part of main(), but that caused the code to be sequential, so I moved it up as a separate void command
        //based on the commands_parallel[] list and their own respective commands[] list all operations will be completed with a success
        parallelizer(commands_parallel, parallel_count);
    }
    //and we finish tthe program!
    return 0;
}