//Name: Searidang Pa
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#define BUFFER_SIZE 4096
#define MAX_ARGS 10


int fork_exec(char ** argv, int fd, char * flag);
int open_io(char * token, char *mode, char ** saveptr);
int myshell(char * buf);
int fork_exec_with_pipe(char ** argv, int io_fd, char * flag_io);
int io_redirect(int io_fd, char * flag_io);
void clean_argv(char ** argv);


int main (int argc, char**argv){

  while(1){

    fprintf(stdout, "[%s]$ ", "command prompt: ");
    char * buf= malloc(BUFFER_SIZE);

    if (fgets(buf, BUFFER_SIZE, stdin)!=NULL){

      if (strcmp(buf, "exit\n")==0){ //if the user wants to exit
        fprintf(stdout, "%s\n", "exiting...");
        break;
      }
      /*forking here will prevent the io of the shell's mom to be
      messed with
      */
      pid_t child_pid;
      child_pid = fork(); 
      if (child_pid < 0){
        fprintf(stderr, "Error: %s\n", strerror(errno));
      }

      if (child_pid ==0){ //this is the child 
        if (myshell(buf)<0){
          fprintf(stderr, "Error: %s\n", strerror(errno));
        }
        break;
      }
      else{  //this is the mom 
        if (wait(NULL)<0){
          fprintf(stderr, "Error: %s\n", strerror(errno));
          return -1;
        }
      }
    }
    else{
      fprintf(stdout, "%s\n", "end of file: exiting...");
      break;
    }

    free(buf);
 }
}
/* Execute the shell: 
PARAMETERS: char * buf; the string input from the user with 
\n at the end
*/
int myshell(char * buf){
  //prev_token is for getting rid of \n before fork & exec
  char * prev_token;  

  //argument to be pass into the fork_exec() function 
  char * argv[MAX_ARGS+1];  //+1 account for the NULL in argv
  char * flag_io = NULL;
  int    io_fd = -1;
  
  //initializing temporary variables 
  clean_argv(argv);

  char * saveptr; //for parsing the string 
  char * token = strtok_r(buf, " ", &saveptr);
  int    i = 0;
  for (i = 0; token!=NULL; i++){

    if (strcmp(token,"<")==0){
      if ((io_fd = open_io(token, token, &saveptr))<0){
        return -1;
      }
      flag_io = "in";
      prev_token = NULL;
    }

    else if(strcmp(token,">")==0 || strcmp(token,">>")==0){
      if ((io_fd = open_io(token, token, &saveptr))<0){
        return -1;
      }
      flag_io = "out";
      prev_token = NULL;
    }
    
    else if(strcmp(token,"|")==0){
      if (fork_exec_with_pipe(argv, io_fd, flag_io)<0){
        return -1;
      }
      clean_argv(argv); //restart everything
      i = -1;
      io_fd = -1;  
      flag_io = NULL;
    }
    
    else{  //if we want the string in our single command to be exec 
      if (strcmp(token,"\n")==0){
        break;
      }
      argv[i]=malloc(strlen(token)+1);
      strcpy(argv[i], token);
      prev_token = token;
    }
    token = strtok_r(NULL, " ", &saveptr);

  }//end of for loop  

  //we don't want \n at the end
  if (prev_token !=NULL){ 
    prev_token = strtok_r(prev_token, "\n", &saveptr);
    if (prev_token != NULL){  //take care of space at the end
      strcpy(argv[i-1], prev_token);
    }
  }
  //exec after finishing reading 
  argv[i]=NULL;
  if (fork_exec(argv, io_fd, flag_io)<0){
    return -1;
  }
  //free malloc 
  for (int i=0;i<=MAX_ARGS && argv[i]!=NULL;i++){
    free(argv[i]);    
  }
  return 0;
}


/* fork and exec a single command we get from char ** argv with piping 
PARAMETERS: char ** argv; the command  
            int io_fd; the file descriptor of the input and 
                          output if there is redirection
            char * flag_io; whether it is input or output 
*/
int fork_exec_with_pipe(char ** argv, int io_fd, char * flag_io){ 
  int pipe_fd[2];
  if (pipe(pipe_fd)<0){ //error checking for pipe
    return -1;
  }
  pid_t child_pid = fork(); 

  if (child_pid == -1){ //error checking for fork 
    return -1;
  }

  if(child_pid == 0) { //this is the child
    if (flag_io !=NULL){ //if there is io_redirection
      if (io_redirect(io_fd, flag_io)<0){
        return -1;
      }
    }
    if (dup2(pipe_fd[1], 1)<0){ //close stdout and replace with pipe
      return -1;
    } 
    if (close(pipe_fd[1])<0){
      return -1;
    }
    if(close(pipe_fd[0])<0){
      return -1;
    }
    if (execvp(argv[0],argv)<0){ //error handling for exec
      return -1;
    }  
  }
  else{ //this is the parent 
    if (wait(NULL)<0){
      return -1;
    }
    if (dup2(pipe_fd[0], 0)<0){
      return -1;
    }
    if (close(pipe_fd[1])<0){
      return -1;
    }
    if (close(pipe_fd[0])<0){
      return -1;
    }
  }
  return 0;
}


/* fork and exec a single command we get from char ** argv
PARAMETERS: char ** argv; the command  
            int io_fd; the file descriptor of the input and output if there is redirection
            char * flag_io; whether it is input or output 
If the program is succesful, it returns 0. If it fails, it return -1.
*/

int fork_exec(char ** argv, int io_fd, char * flag_io){
  pid_t child_pid = fork(); 
  if (child_pid == -1){ //error checking for fork 
    return -1;
  }
  if(child_pid == 0) { //this is the child
    if (flag_io !=NULL){ //if there is io_redirection
      if(io_redirect(io_fd, flag_io)<0){
        return -1;  //error handling for io_redirect
      }
    }
    if (execvp(argv[0], argv)<0){ //error handling for exec
      return -1;
    }  
  }
  else{
    if (wait(NULL)<0){
      return -1;
    }
  }
  return 0;
}

/*return the file descriptor of the designated output or input file
PARAMETERS: char * token which we would get the file_name from. 
            char * mode; whether it is read, write or append
            char ** saveptr; keeps our token safe from being 
                                    destroyed from strtok()
*/
int open_io(char * token, char *mode, char ** saveptr){
  if (saveptr !=NULL && *saveptr !=NULL){
    token = strtok_r(NULL, " ", saveptr);
  }
  char * file_name;
  if (token != NULL){
    file_name = strtok(token, "\n"); 
  }
  int result_fd = -1;
  if (strcmp(mode, ">")==0){
    result_fd= open(file_name, O_WRONLY | O_CREAT, 0666);
  }
  if (strcmp(mode, ">>")==0){
    result_fd= open(file_name, O_WRONLY | O_CREAT|O_TRUNC, 0666);
  }
  else if ((strcmp(mode,"<")==0)){
    result_fd= open(file_name, O_RDONLY);
  } 
  if (result_fd <0){ //error checking
    return -1;
  } 
  return result_fd;
}

/*redirect the input and output of the current process 
PARAMETERS: int io_fd: the file descriptor of the file to be 
                       redirected to/from
            char * flag_io: whether it is input or output 
If the program is succesful, it returns 0. If it fails, it return -1.
*/
int io_redirect(int io_fd, char * flag_io){

  if (io_fd < 0){   //error handling for open file 
    return -1;
  }
  if (strcmp(flag_io, "out")==0){
    if (dup2(io_fd, 1) <0){  //error checking for dup2 
      return -1;
    }
  }
  else if ((strcmp(flag_io, "in")==0)){
    if (dup2(io_fd, 0) <0){ //error checking for dup2 
      return -1;
    }
  }
  if (close(io_fd)<0){ //error handling for close file 
    return -1;
  }
  return 0;
}


/*replace everything in char ** argv with NULL*/
void clean_argv(char ** argv){
  if (argv !=NULL){
    for (int i = 0; i <=MAX_ARGS; i++){
      argv[i]=NULL;
    } 
  }
}
