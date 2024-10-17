
// Name: Ife Olusola
// UID# U93163494
// Netid: ifeoluwao

/* this program is a simple Unix shell, it is a CLI that responds to user prompts 
by creating child processes and executing commands.*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h> 
#include <fcntl.h>
#include <ctype.h> 

//max user input as stated in project prompt
#define USER_INPUT 255
#define MAX_PATHS 255

//error message
char error_message[30] = "An error has occurred\n";
char *paths[MAX_PATHS]; //path array
int path_count=1; 
char *default_path = "/bin"; //default path


// this function is used to split the input string based on whitespace and tabs
char **split_input(char *input, int *arg_count) {
    char **args = malloc(USER_INPUT * sizeof(char *));
    //if args is NULL, throw error
    if (!args) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(EXIT_FAILURE);
    }

    *arg_count = 0;

    //tokenize strings based on white spae/tab/newline
    char *token = strtok(input, " \t\n\a\r");
    //check if token is NULL, if not accept it to the args array, increment and keep tokenizing
    while (token != NULL) {
        args[(*arg_count)++] = token;
        token = strtok(NULL, " \t\n\a\r");
    }

    // If no valid tokens are found (only whitespace), return NULL to indicate an empty input
    if (*arg_count == 0) {
        free(args);
        return NULL;
    }

    args[*arg_count] = NULL; // Null-terminate the list of arguments
    return args;
}


//function to parse user input and execute all commands (built-in, parallel, path, redirection, e.t.c)
void parse_execute(char *input) {
    //array of commands and count check, check for & and tokenize in case of parallel commands
    char *commands[USER_INPUT];
    int count = 0;
    char *command = strtok(input, "&");

    //check that command is not NULL (parallel commands mainly)
    while (command != NULL) {
        // Trim leading and trailing whitespace
        while (isspace(*command)) command++;
        char *end = command + strlen(command) - 1;
        while (end > command && isspace(*end)) *end-- = '\0';

        commands[count++] = command;
        command = strtok(NULL, "&");
    }

    commands[count] = NULL;// null terminate commands

    pid_t pids[count];//process id array for child processes

    //loop through commands to handle execution
    for (int i = 0; i < count; i++) {
        int arg_count = 0;
        char **args = split_input(commands[i], &arg_count);//chop up input 

        //check if chopped up/split commands still NULL
        if (args == NULL) {
            continue;
        }

        // Check for built-in commands, exit handled in main
        //check for cd
        if (strcmp(args[0], "cd") == 0) {
            //ensure when cd is input, it contains only 1 argument to its right
            if (arg_count != 2) {
                write(STDERR_FILENO, error_message, strlen(error_message));
            } else {
                if (chdir(args[1]) != 0) { //if cd fails, throw error
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
            }
        } 
        //check for path
        else if (strcmp(args[0], "path") == 0) {
            path_count = 0; //keep path count to erase path when new path is input
            //obtain new path
            for (int j = 1; j < arg_count; j++) {
                if (path_count < MAX_PATHS) {
                    paths[path_count++] = strdup(args[j]);
                }
            }
        }
         //if not built-in function (regular commands), proceed as:
        else {
            pid_t pid = fork(); //fork new process
            if (pid == 0) {
                // Child process
                // Handle redirection to file
                for (int j = 0; j < arg_count; j++) {
                    if (strcmp(args[j], ">") == 0) {
                        //if redirection has no argument to its write or more than one, throw error
                        if ((j + 1 >= arg_count) || (args[j + 1] == NULL) || (j + 2 < arg_count)) {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            exit(1);
                        }
                        //open file for redirection
                        int fd = open(args[j + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        //if opening fails, throw error
                        if (fd == -1) {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            exit(1);
                        }
                        /* use dup2 to make file descriptor of fd refer to file descriptor of standard output(terminal),
                         now output redirected to fd*/
                        if (dup2(fd, STDOUT_FILENO) < 0) {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            close(fd);
                            exit(1);
                        }
                        close(fd); //close file
                        args[j] = NULL; // Null-terminate args for exec
                        break; //break because redirection should only occur once
                    }
                }

                // Execute the command
                char executable[MAX_PATHS];
                for (int x = 0; x < path_count; x++) {
                    snprintf(executable, sizeof(executable), "%s/%s", paths[x], args[0]); //write command path and command unto executable buffer
                    if (access(executable, X_OK) != -1) { //check accesibilty of executable
                        execv(executable, args); // replace the process on the executable with args
                    }
                }
                // if code control flow gets here, execv failed
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1);
            } else if (pid > 0) {
                // Parent process
                pids[i] = pid;
            } else {
                // Fork failed
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
        }
        free(args); //free args
    }   

    // Wait for all child processes to complete
    for (int i = 0; i < count; i++) {
        waitpid(pids[i], NULL, 0);
    }
}


int main(int argc, char *argv[]){
    // check that initialize input  is "./rush" (1 count only)
    if (argc > 1) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 1;
    }

    //set up for getline
    char *input = NULL;
    size_t n = 0;

    // Initialize path with default value
    paths[0] = default_path;

    // While loop to print prompt
    while(1){
        printf("rush> ");
        fflush(stdout); //flush standard output

        //getline to read input from terminal
        if(getline(&input, &n, stdin) == -1)  {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }

        // Remove trailing newline
        input[strcspn(input, "\n")] = '\0';

        // Skip empty input
        if (strlen(input) == 0) {
            continue;
        }

        // Exit command
        if(strcmp(input,"exit") == 0){
            free(input);
            exit(0);
        }

        // Parse and execute commands
        parse_execute(input);
    }

    free(input);//free input 
    return 0;
}