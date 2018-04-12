/**
* Shell Lab
* CS 241 - Spring 2018
*/

#include "format.h"
#include "shell.h"
#include "vector.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

extern char *optarg;
extern int optind, opterr, optopt;

typedef struct process {
    char *command;
    char *status;
    pid_t pid;
} process;

static process foreprocesses[80];
static process myprocess[80];
static int numpro=0;
static int forenumpro = 0;

void handler(){
  for(int i=0; i<forenumpro; i++)
    kill(foreprocesses[i].pid, SIGINT);
}


int pidStuff(char* command, int backgroud){

  pid_t pd = fork();
  int status;
  if(pd<0){
    print_fork_failed();
    exit(1);
  }
  else if(pd>0){
    if(setpgid(pd, pd) == -1){
      print_setpgid_failed();
      exit(1);
    }
    if(!backgroud){
      process new;
      new.status = STATUS_RUNNING;
      new.pid = pd;
      foreprocesses[forenumpro] = new;
      forenumpro++;
      int k = waitpid(pd, &status, 0);
      if(k == -1)
        exit(1); 
      if(WEXITSTATUS(status) != 0)
        return -1;
      else
        return 0;
    }
    else{
      process new;
      new.command = strdup(command);
      new.status = STATUS_RUNNING;
      new.pid = pd;
      myprocess[numpro] = new;
      numpro++;
      int k = waitpid(pd,&status,WNOHANG);
      if(k == -1)
        exit(1);
      return 0;
    }
  }
  else{
      char* args[1024];
      char* t2 = strtok(command, " ");
      int i = 0;
      while(t2 != NULL){
          args[i++] = t2;
          t2 = strtok(NULL," ");
      }
      args[i] = NULL;
      print_command_executed(getpid());
      int k = execvp(*args, args);
      if(k < 0){
        print_exec_failed(command);
        exit(1);
      }
    }
  return 0;
}

int parse_operator(char* command, char two_command[2][1024]){
  char *index=NULL, *iindex=NULL;
  if((index = strstr(command, "&&")) && *(index-1)!='\\'){
    iindex = index-1;
    while(*iindex == ' ')
      iindex--;
    char a = *(iindex+1);
    *(iindex+1) = 0;
    strcpy(two_command[0], command);
    *(iindex+1) = a;
    index += 2;
    while(*index == ' ')
      index++;
    strcpy(two_command[1], index);
    return 0;
  }
  else if((index = strstr(command, ";")) && *(index-1)!='\\'){
    iindex = index-1;
    while(*iindex == ' ')
      iindex--;
    char a = *(iindex+1);
    *(iindex+1) = 0;
    strcpy(two_command[0], command);
    *(iindex+1) = a;
    index += 1;
    while(*index == ' ')
      index++;
    strcpy(two_command[1], index);
    return 2;
  }
  else if((index = strstr(command, "||")) && *(index-1)!='\\'){
    iindex = index-1;
    while(*iindex == ' ')
      iindex--;
    char a = *(iindex+1);
    *(iindex+1) = 0;
    strcpy(two_command[0], command);
    *(iindex+1) = a;
    index += 2;
    while(*index == ' ')
      index++;
    strcpy(two_command[1], index);
    return 1;
  }
  else{
  strcpy(two_command[0], command);
  return 3;

  }
}

int shell(int argc, char *argv[]) {
    // TODO: This is the entry point for your shell.
    vector* history_vector = string_vector_create();
    int history=0,file = 0;
    char* history_file = NULL;
    char* use_file = NULL;
    int ch;
    opterr = 0;
    char* history_file_fullpath;
    char* file_fullpath;
    FILE* filept;
    while ((ch = getopt(argc, argv, "h:f:")) != -1){
      if(ch == 'h'){
        if(history == 1){
        print_usage();
        exit(0);          
        }
        history = 1;
        history_file = optarg;
      }
      else if(ch == 'f'){
        if(file  ==  1){
        print_usage();
        exit(0);          
        }  
        file = 1;     
        use_file = optarg;
      }
      else if (ch == '?' || ch == ':'){
      print_usage();
      exit(0);            
      }
      else{
      print_usage();
      exit(0);  
      }
    }
    if(optind != argc){
      print_usage();
      exit(0);
    }

    if(history){
      if(access(history_file, F_OK) == -1){
        print_history_file_error();
        exit(0);}
        history_file_fullpath = get_full_path(history_file);
        FILE *his = fopen(history_file_fullpath, "r");
        char *line = NULL;
        size_t n = 0;
        while(getline(&line, &n, his) != -1){
          line[strlen(line)-1] = 0;
          vector_push_back(history_vector, line);
        }
        free(line);
        fclose(his);
    }

    if(file){
      if(access(use_file, F_OK) == -1){
        print_script_file_error();
        exit(0);
      }
      file_fullpath = get_full_path(use_file);
      filept = fopen(file_fullpath, "r");
    }

    int flag = 1;
    char command[1024];
    memset(command,0,1024);
    signal(SIGINT, handler);

    while (1) {

      if(history){
        FILE *his = fopen(history_file_fullpath, "w");
        VECTOR_FOR_EACH(history_vector,elem,{
          fprintf(his, "%s\n", elem);
        });
        fclose(his);
      }

      char path[1024];
      memset(path,0,1024);
      getcwd(path, sizeof(path));
      pid_t cp = getpid();
      if(flag){
        if(file){
          size_t n = 0;
          char* t = NULL;
          if(getline(&t, &n, filept) != -1){
            print_prompt(path, cp);
            memset(command,0,1024);
            strcpy(command, t);
            command[strlen(command)-1] = 0;
            printf("%s\n", command);
            free(t);
          }
          else{
            fclose(filept);
            vector_destroy(history_vector);
            for(int i=0; i<numpro; i++){
              if(!strcmp(myprocess[i].status,STATUS_STOPPED) || !strcmp(myprocess[i].status,STATUS_RUNNING))
                kill(myprocess[i].pid, SIGTERM);
              free(myprocess[i].command);
            }
            free(t);
            free(file_fullpath);
            if(history)
              free(history_file_fullpath);
            exit(0);
          }
        }
        else{
          print_prompt(path, cp);
          memset(command,0,1024);
          fgets(command, 1024, stdin);
          command[strlen(command)-1] = 0;
        }
      }
      flag = 1;
      char* cd;
      if((cd = strstr(command, "cd ")) == command){
        char cdpath[1024];memset(cdpath,0,1024);
        strcpy(cdpath, cd+3);
        if(chdir(cdpath) == -1) {
          print_no_directory(cdpath);
          continue;
        }
        vector_push_back(history_vector, command);
      }
      else if(!strcmp(command,"!history")){
        VECTOR_FOR_EACH(history_vector,elem,{print_history_line(_it-vector_begin(history_vector), elem);});
      }
      else if(command[0] == '!'){
        char *pre = command+1;
        if(vector_size(history_vector) == 0)
          print_no_history_match();
        for(int i=vector_size(history_vector)-1; i>=0; i--){
          char* vec = vector_get(history_vector, i);
          if(strstr(vec, pre) == vec || *pre == '\0'){
            flag = 0;
            memset(command,0,1024);
            strcpy(command, vec);
            printf("%s\n", vec);
            break;
          }
          if(i==0)
            print_no_history_match();
        }
      }
      else if(command[0] == '#'){
        int num;
        sscanf(command+1, "%d", &num);
        if(num >= (int)vector_size(history_vector) || num<0)
          print_invalid_index();
        else{
          flag = 0;
          memset(command,0,1024);
          strcpy(command, (const char*)vector_get(history_vector,num));
          printf("%s\n", (const char*)vector_get(history_vector,num));
          continue;
        }
      }
      else if(!strcmp(command,"exit")){
        vector_destroy(history_vector);
        for(int i=0; i<numpro; i++){
          if(!strcmp(myprocess[i].status,STATUS_STOPPED) || !strcmp(myprocess[i].status,STATUS_RUNNING))
            kill(myprocess[i].pid, SIGTERM);
          free(myprocess[i].command);
        }
        if(file){
          fclose(filept);
          free(file_fullpath);
        }
        if(history)
          free(history_file_fullpath);
          exit(0);
      }
      else if(!strcmp(command,"ps")){
        vector_push_back(history_vector, command);
        print_process_info(STATUS_RUNNING, getpid(), argv[0]);
        for(int i=0; i<numpro; i++){
          if(!strcmp(myprocess[i].status, "KILLED")) continue;
          char tem[1024]; memset(tem,0,1024);
          strcpy(tem, myprocess[i].command);
          pid_t child_status = waitpid(myprocess[i].pid, NULL, WNOHANG);
          if(child_status == myprocess[i].pid || child_status == -1){
            myprocess[i].status = "KILLED";
            continue;
          }
          else if(!strcmp(myprocess[i].status, STATUS_STOPPED)){
            print_process_info(STATUS_STOPPED, myprocess[i].pid, tem);
            continue;
          }
          else if(child_status == 0){
            print_process_info(STATUS_RUNNING, myprocess[i].pid, tem);
          }
        }
      }
      else if((cd = strstr(command, "stop ")) == command){
        int stoppid;
        if(!*(cd+5)){
          print_invalid_command(command);
          continue;
        }
        sscanf(cd+4, "%d", &stoppid);
        pid_t child_status = waitpid((pid_t)stoppid, NULL, WNOHANG);
        if(child_status == -1|| child_status == (pid_t)stoppid){
          print_no_process_found(stoppid);
          continue;
        }
        kill(stoppid, SIGTSTP);
        for(int i=0; i<numpro; i++){
          if(myprocess[i].pid == stoppid){
            myprocess[i].status = STATUS_STOPPED;
            print_stopped_process(stoppid, myprocess[i].command);
            break;
          }
        }
      }
      else if((cd = strstr(command, "kill ")) == command){
        int killpid;
        if(!*(cd+5)){
          print_invalid_command(command);
          continue;
        }
        sscanf(cd+4, "%d", &killpid);
        pid_t child_status = waitpid((pid_t)killpid, NULL, WNOHANG);
        if(child_status == -1|| child_status == (pid_t)killpid){
          print_no_process_found(killpid);
          continue;
        }
        kill(killpid,SIGTERM);
        for(int i=0; i<numpro; i++){
          if(myprocess[i].pid == killpid){
            myprocess[i].status = "KILLED";
            print_killed_process(killpid, myprocess[i].command);
            break;
          }
        }
      }
      else if((cd = strstr(command, "cont ")) == command){
        int contpid;
        if(!*(cd+5)){
          print_invalid_command(command);
          continue;
        }
        sscanf(cd+4, "%d", &contpid);
        pid_t child_status = waitpid((pid_t)contpid, NULL, WNOHANG);
        if(child_status == -1|| child_status == (pid_t)contpid){
          print_no_process_found(contpid);
          continue;
        }
        kill(contpid, SIGCONT);
        for(int i=0; i<numpro; i++){
          if(myprocess[i].pid == contpid && !strcmp(myprocess[i].status,STATUS_STOPPED)){
            myprocess[i].status = STATUS_RUNNING;
            break;
          }
        }
      }
      else{
        char two_command[2][1024];
        memset(two_command, 0, 2*1024);
        char* a = two_command[0];
        char* b = two_command[1];
        int backgroud = 0;
        int opt = 3;
        for(int i=strlen(command)-1; i>=0; i--){
          if(command[i] == '&'){
            if(command[i-1] != '\\' && command[i-1] != '&' && command[i+1] == 0){
              backgroud = 1;
              command[i] = 0;
            }
            break;
          }
        }

        vector_push_back(history_vector, command);
        opt = parse_operator(command, two_command);


        char temp[1024];
        memset(temp,0,1024);
        char* c = a;
        while((a = strstr(a,"\\"))){
          *a = 0;
          strcat(temp,c);
          c = a+1;
          a++;
          if(*a == '\\')
            a++;
        }
        strcat(temp, c);
        if(*temp)
          strcpy(two_command[0], temp);

        memset(temp,0,1024);
        char* d = b;
        while((b = strstr(b,"\\"))){
          *b = 0;
          strcat(temp,d);
          d = b+1;
          b++;
          if(*b == '\\')
            b++;
        }
        strcat(temp, d);
        if(*temp)
        strcpy(two_command[1], temp);

        if(opt == 3){
          pidStuff(two_command[0], backgroud);
        }
        else if(opt == 0){
          if(pidStuff(two_command[0], backgroud) == 0)
            pidStuff(two_command[1], backgroud);
        }
        else if(opt == 1){
          if(pidStuff(two_command[0], backgroud) != 0)
            pidStuff(two_command[1], backgroud);
        }
        else if(opt == 2){
          pidStuff(two_command[0], backgroud);
          pidStuff(two_command[1], backgroud);
        }
        else{

        }

        }
      }

    return 0;
}