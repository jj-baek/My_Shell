#include <stdio.h> //file i/o commands
#include <sys/stat.h> //checking file types of file descriptors
#include <sys/types.h> //fstat, other file i/o, wait
#include <sys/time.h> //for wait
#include <sys/resource.h> //for wait
#include <sys/wait.h> //for eait
#include <getopt.h> //for getopt
#include <unistd.h> //pipe command, parsing command line stuff, pause, forking
#include <stdlib.h> //malloc and stuff
#include <errno.h> //for the errno checking
#include <fcntl.h> //for O_CONSTANTS
#include <string.h> //fprintf, etc.
#include <signal.h> //for kill

#include "tokens.h" //custom tokenizer made for the class

#define MYSH_LINE 1024


typedef struct CmdSet {
    struct Cmd ** commands;
    int background;
    int numCmds;

} * cmdset;

 typedef struct Cmd {
    char** arguments;
    char* inputFile;
    char* outputFile;
    int numArgs;
    int appendOut;
    int pipeInput;
    int pipeOutput;


} * cmd;

typedef struct freelistnode {
    struct freelistnode *flink; //pointer to next free chunk node
    pid_t process_id; //size of current chunk
} * FreeListNode;

FreeListNode head;

void insert_node(FreeListNode n){
    FreeListNode temp = head; //background to check last node
    n->flink = 0; //clear out flink just in case
    if(head == 0) {
        head = n;
        return;
    }
    else {
        if(n < head){
            n->flink = head; //Make n's flink point to head
            head = n;
        return;
        }

        while(temp->flink != 0){ //List traversal
            if(n < temp->flink){
                n->flink = temp->flink;
                temp->flink = n;
            return;
            }
            temp = temp->flink;
        }
        temp->flink = n;
        return;
        //find last node
        //make last node next new node
    }

}

void remove_node(pid_t n) {

    if (head->process_id==n){ //if the head node is the one to remove
        head = head->flink;
        return;
    }

    FreeListNode temp = head; //for iterating over the list
    while(temp->flink != 0){ //List traversal: check middle nodes and last node
        if(temp->flink->process_id == n){ //if the next node (including the last node) is the one to remove
            temp->flink = temp->flink->flink; //jump over the next node in the linked list
            return;
        }
        temp = temp->flink; //else, continue to the next node
    }
    return;

}

// pid_t kill_list[1024];

int main( int argc, char *argv[] )
{
    while(1) {
        // commands = getCmd();

        // foreach command in commands: create succeeding pipe for I/O*
        // create child process open files for I/O*
        // redirect stdin from file or preceding pipe*
        // redirect stdoutto file or succeeding*
        // execute command in child
        // track childpid in parent
        // parent waits for foreground children*

        int error = 0;
        if(argc==2){
            if(!strcmp(argv[1],"-")){
                fprintf(stdout, ""); //don't print a prompt
            } else {
                fprintf(stdout, "%s ",argv[1]);
            }
        } else if (argc==1) {
            fprintf(stdout, "mysh: ");
        } else {
            fprintf(stderr,"Error: Usage: %s [prompt]\n", argv[0]);
            exit(-1);
        }

        char *linebuf = malloc(sizeof(char) * (MYSH_LINE));
        if(!linebuf) {
            fprintf(stderr, "Error: Out of memory\n");
            exit(-1);
        }

        char * result;
        errno = 0;
        do{
            result = fgets(linebuf, MYSH_LINE, stdin); //reads one line from input
        } while ((result==NULL)&&(errno==EINTR));
        //fgets returns 0 when it's at the end of a file or there was an error
        //fgets(string being read and stored (linebuf is being stored from stdin, max number of characters to be read, file stream that identifies where characters are read from)

        if((!result)||(strcmp(linebuf, "exit\n")==0)){ //if we got ctrl-D as input (EOF), or user typed in exit, quit
            pid_t kill_id;
            int status;
            fprintf(stdout, "Goodbye!\n");
            do{
                kill_id = wait3(&status, WNOHANG, NULL);
                if (kill_id > 0){
                    kill(kill_id,SIGTERM); //kill all background processes politely
                }
            } while(kill_id!=-1);
            return 0;
        }

        cmdset commandSet = calloc(1, sizeof(struct CmdSet));
        commandSet->numCmds = 1;
        commandSet->commands = calloc(1, sizeof(char*) * (commandSet->numCmds + 1));
        cmd command = calloc(1, sizeof(struct Cmd));
        command->arguments = calloc(1, sizeof(char **));
        commandSet->commands[0] = command;
        commandSet->commands[1] = NULL;
        
        char** breakup = get_tokens(linebuf);

        for( int i=0; breakup[i]; i++ ) {
            //fprintf(stdout, "My tokens: %s\n", breakup[i]);
            if(strcmp(breakup[i], "&") == 0) {
                if(breakup[i+1] != NULL) {
                    fprintf(stderr, "Error: \"&\" must be last token on command line\n");
                    error=1;
                    break;
                }
                else {
                    //fprintf(stdout, "Place commands into the background: after invoking the specified commands.‘&’ may only be specified as the final command line argument. %s\n", breakup[i]);
                    commandSet->background=1;
                }
            } else if(strcmp(breakup[i], "<") == 0) {
                    //fprintf(stdout, "Redirect the current command’s standard input stream from the file named immediately after the ‘<’ operator.\n");
                    i++; //increment i to the next token
                    //fprintf(stdout, "%s is our new input stream\n", breakup[i]);
                    if (command->inputFile){
                        fprintf(stderr,"Error: Ambiguous input redirection.\n");
                        error=1;
                        break;
                    }
                    if (breakup[i]){
                        command->inputFile = breakup[i];
                        //fprintf(stdout,"inputting from: %s\n",new_command->input);
                    } else {
                        fprintf(stderr,"Error: Missing filename for input redirection.\n");
                        error=1;
                        break;
                    }

            } else if(strcmp(breakup[i], ">") == 0) {
                    //fprintf(stdout, "Redirect the current command’s standard output stream to the file named immediately after the ‘>’ operator.\n");
                    i++; //increment i to the next token
                    //fprintf(stdout, "%s is our new output stream\n", breakup[i]);
                    if (command->outputFile){
                        fprintf(stderr,"Error: Ambiguous output redirection.\n");
                        error=1;
                        break;
                    }
                    if (breakup[i]){
                        struct stat buf;
                        int exists = lstat(breakup[i],&buf);
                        if (exists >= 0){
                            fprintf(stderr, "Error: open(\"%s\"): File exists\n", breakup[i]);
                            error = 1;
                            break;
                        }
                        command->outputFile = breakup[i];
                        //fprintf(stdout,"inputting from: %s\n",new_command->input);
                    } else {
                        fprintf(stderr,"Error: Missing filename for output redirection.\n");
                        error=1;
                        break;
                    }
            } else if(strcmp(breakup[i], ">>") == 0) {
                    //fprintf(stdout, "Redirect the current command’s standard output stream to the file named immediately after the ‘>>’ operator. Append to the file if it already exists.\n");
                    i++;
                    //fprintf(stdout, "%s is our new SPECIAL output stream\n", breakup[i]);
                    if (command->outputFile){
                        fprintf(stderr,"Error: Ambiguous output redirection.\n");
                        error=1;
                        break;
                    }
                    if (breakup[i]){
                        command->outputFile = breakup[i];
                        command->appendOut = 1;
                        //fprintf(stdout,"inputting from: %s\n",new_command->input);
                    } else {
                        fprintf(stderr,"Error: Missing filename for output redirection.\n");
                        error=1;
                        break;
                    }

            } else if(strcmp(breakup[i], "|") == 0) {
                    //fprintf(stdout, "Redirect the current command’s standard output stream to the standard input stream of the succeeding command. There may be any number of pipe-connected processes..\n");
                    //fprintf(stdout, "%s is the first command's output stream to be used for the standard input stream of the succeeding command\n", breakup[i-1]);
                    //fprintf(stdout, "%s is our command that will be using the other command's output as an input\n", breakup[i+1]);
                    if (!command->arguments){
                        fprintf(stderr,"Error: Invalid null command.\n");
                        error=1;
                        break;
                    }
                    commandSet->numCmds++;
                    commandSet->commands = realloc(commandSet->commands, sizeof(char*) * commandSet->numCmds + 1);
                    command->pipeOutput = 1;
                    cmd command2 = calloc(1, sizeof(struct Cmd));
                    command2->pipeInput = 1;
                    command2->arguments = calloc(1, sizeof(char **) * 2);
                    commandSet->commands[commandSet->numCmds - 1] = command2;
                    command = command2;
                    commandSet->commands[commandSet->numCmds] = NULL;

            } else {
                    //fprintf(stdout, "Here is the command.\n");
                    if(command->numArgs == 0) {
                        command->arguments[0] = breakup[i];
                        command->numArgs++;
                    } else {
                        command->numArgs++;
                        command->arguments = (char**) realloc(command->arguments, sizeof(char**) * (command->numArgs+1));
                        command->arguments[command->numArgs - 1] = breakup[i];
                        command->arguments[command->numArgs] = NULL;
                     }


                }

        }
        

    for(int c = 0; commandSet->commands[c]; c++) {
        cmd command = commandSet->commands[c];
        if(command->pipeOutput){ //this means that this will pipe out
            int pipefds[2] = {-1,-1}; //init new array for pipe file descriptors
            if(!command->arguments[0]) {
                fprintf(stderr,"Error: Invalid null command.\n");
                error=1;
                break;
            } else if(command->outputFile){
                fprintf(stderr,"Error: Ambiguous output redirection.\n");
                error=1;
                break;
            } else {
                if(pipe(pipefds)<0){
                    perror("Error: Cannot create pipe()");
                    error=1;
                    break;
                }
                command->pipeOutput = pipefds[1]; //set pipe it is writing to
            }
            if (commandSet->commands[c+1]){ //if there is a next command
                if(!commandSet->commands[c+1]->pipeInput){ //if the next command's pipeinput is not set, should not happen
                    fprintf(stderr,"Error: Destination not set to accept pipe.\n");
                    error=1;
                    break;
                } else if (!commandSet->commands[c+1]->arguments[0]){
                    fprintf(stderr,"Error: Invalid null command.\n");
                    error=1;
                    break;
                } else if (commandSet->commands[c+1]->inputFile){
                    fprintf(stderr,"Error: Ambiguous input redirection.\n");
                    error=1;
                    break;
                } else {
                    commandSet->commands[c+1]->pipeInput = pipefds[0]; //set pipe it is reading from 
                }
            } else { //there is no next command, should not happen
                fprintf(stderr,"Error: No destination for pipe.\n");
                error=1;
                break;
            }
        }

    }
    if (error) continue; //just bring back the prompt if there was an error anywhere in the process

    int stream;
    int status;
    pid_t process_id = 0;
    
    for(int c = 0; commandSet->commands[c]; c++) {
        cmd command = commandSet->commands[c];
        pid_t pid = fork();
        if(pid > 0) { //in the parent
            if(command->pipeOutput) { //close parent's writing pipe after fork
                close(command->pipeOutput);
            }
            if(command->pipeInput) { //close parent's reading pipe after fork
                close(command->pipeInput);
            }

        if(!commandSet->background){
            FreeListNode newNode = malloc(sizeof(struct freelistnode));
            newNode->process_id = pid;
            newNode->flink = NULL;
            insert_node(newNode);
        }

        } else if (pid == 0){ //in the child

            if(command->inputFile) {
                int fin = open(command->inputFile,O_RDONLY); 
                if(fin==-1){
                    fprintf(stderr, "Error: open(\"%s\"): %s\n", command->inputFile, strerror(errno));
                    exit(-1);
                }
                if(dup2(fin,0)==-1){ //redirect stdin
                    perror("Error: dup2");
                    exit(-1);
                }
                close(fin);
            }
            if(command->outputFile){
                int fout;
                if(command->appendOut){ //if we are just appending
                    fout = open(command->outputFile,
                                O_WRONLY|O_CREAT|O_APPEND, 
                                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); //redirect in child!!
                } else { //if we are creating a new file or exiting if one found
                    fout = open(command->outputFile,
                                O_CREAT|O_WRONLY,
                                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                }
                if(fout==-1){
                    fprintf(stderr, "Error: open(\"%s\"): %s\n", command->outputFile, strerror(errno));
                    exit(-1);
                }
                if(dup2(fout,1)==-1){ //redirect stdout
                    perror("Error: dup2()");
                    exit(-1);
                }
                close(fout);
            }
            if(command->pipeOutput){
                if(dup2(command->pipeOutput, 1) == -1) {
                    fprintf(stderr, "Error dup2 stdout, pipeOutput %d\n",command->pipeOutput);
                    exit(1);
                }
                close(command->pipeOutput);
            }
            if(command->pipeInput){
                if(dup2(command->pipeInput, 0) == -1) {
                    fprintf(stderr, "Error dup2 stdin, pipeInput %d\n",command->pipeInput);
                    exit(1);
                }
                close(command->pipeInput);
            }
            //fprintf(stderr,"Running command %s\n",command->arguments[0]);
            int execresult;
            do{
                execvp(command->arguments[0], command->arguments); //last line in the child
            } while (execresult==-1);
            exit(-1);
        } else {
            fprintf(stderr,"Error: fork()\n");
            exit(-1);
            //Fork Error
            }
        }


     

    while(head != NULL) { //there should always be one process in the foreground on the list; this allows us to remove zombies periodically
                        // because wait returns processes in any order, and sometimes we catch the zombies first before getting to the foreground
                        // processes
        process_id = wait3(&status, WNOHANG, NULL);
        if(process_id > 0){
            remove_node(process_id);
        } else if (process_id == -1) {
            head = NULL;
            break;
        } 
        usleep(10000); //lower the CPU usage for checking for process termination
    }




    free(linebuf);
    free_tokens(breakup);

    for (int i=0; commandSet->commands[i]; i++){
        command = commandSet->commands[i];
        if(command->arguments) free(command->arguments);
        free(command);
    }
    free(commandSet);

    }

}
