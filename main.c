#include <stdio.h>
#include <stdlib.h>
#include "parse.h"
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <glob.h>
#include <unistd.h>
#include <fcntl.h>
//#include "myFunctions.h"
#include "parse.h"
#include <sys/resource.h>

extern char **environ;

static void prCmd(Cmd c)
{
  int i;

  if ( c ) {
    //printf("%s%s ", c->exec == Tamp ? "BG " : "", c->args[0]);
    //if ( c->in == Tin )
     // printf("<(%s) ", c->infile);
    if ( c->out != Tnil )
      switch ( c->out ) {
      case Tout:
	//printf(">(%s) ", c->outfile);
	break;
      case Tapp:
	//printf(">>(%s) ", c->outfile);
	break;
      case ToutErr:
	//printf(">&(%s) ", c->outfile);
	break;
      case TappErr:
	//printf(">>&(%s) ", c->outfile);
	break;
      case Tpipe:
	//printf("| ");
	break;
      case TpipeErr:
	//printf("|& ");
	break;
      default:
	fprintf(stderr, "Shouldn't get here\n");
	exit(-1);
      }

   // if ( c->nargs > 1 ) {
      //printf("[");
    //  for ( i = 1; c->args[i] != NULL; i++ )
	//printf("%d:%s,", i, c->args[i]);
      //printf("\b]");
   // }
   // putchar('\n');
    // this driver understands one command
   // if ( !strcmp(c->args[0], "end") )
   //   exit(0);
  }
}

static void prPipe(Pipe p)
{
  int i = 0;
  Cmd c;

  if ( p == NULL )
    return;

  //printf("Begin pipe%s\n", p->type == Pout ? "" : " Error");
  for ( c = p->head; c != NULL; c = c->next ) {
    //printf("  Cmd #%d: ", ++i);
    prCmd(c);
  }
  //printf("End pipe\n");
  prPipe(p->next);
}

//=====================================================================================


void signal_disabler(){

  signal(SIGINT,SIG_IGN);
  signal(SIGSTOP,SIG_IGN);
  signal(SIGTERM,SIG_IGN);
  signal(SIGKILL,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGTTOU,SIG_IGN);

}

void signal_enabler(){

  signal(SIGINT,SIG_DFL);
  signal(SIGSTOP,SIG_DFL);
  signal(SIGTERM,SIG_DFL);
  signal(SIGKILL,SIG_DFL);
  signal(SIGTTIN,SIG_DFL);
  signal(SIGTTOU,SIG_DFL);

}

int built_in(Cmd c){
    char com[1000];
    strcpy(com,c->args[0]);
    
    if(strcmp(com,"bg")==0 || strcmp(com,"cd")==0 || strcmp(com,"fg")==0 || strcmp(com,"echo")==0 ||
    strcmp(com,"jobs")==0 || strcmp(com,"kill")==0 || strcmp(com,"logout")==0||strcmp(com,"nice")==0||
    strcmp(com,"pwd")==0|| strcmp(com,"setenv")==0||strcmp(com,"unsetenv")==0 || strcmp(com,"where")==0 || strstr(com,"nice")!=NULL)
    {
        //printf("%s is a shell built-in\n",com);
        return 1;
    }
  return 0;       
}

void print_env_var(){
  int i = 1;
  char *s = *environ;
  for (; s; i++) {
    printf("%s\n", s);
    s = *(environ+i);
  }
  return 0;
}

void handle_pwd(){
  char *pwd=malloc(1000*sizeof(char));
  getcwd(pwd,1000);
  printf("%s\n",pwd);
  return;
}


void handle_setenv(Cmd c){
    if(c->nargs==1){
        print_env_var();
        return;
    }
    char *val = malloc(1000*sizeof(char));
    char *name = malloc(1000*sizeof(char));
    strcpy(name,c->args[1]);    
    if(c->nargs==2){
        setenv(name,NULL,1);
        printf("%s : %s\n",name,getenv(name));
    }
    else{
        strcpy(val,c->args[2]);
        setenv(name,val,1);
        printf("%s : %s\n",name,getenv(name));
    }    
    return;
    free(name);
    free(val);
}

void handle_unsetenv(Cmd c){
    char *name = malloc(1000*sizeof(char));
    strcpy(name,c->args[1]);
    unsetenv(name);
    free(name);
    return; 
}

void handle_kill(Cmd c){
    kill(atoi(c->args[1]),SIGKILL);    
}

void handle_cd(Cmd c){
    char *target=malloc(1000*sizeof(char));
    if(c->args[1]!=NULL)
    strcpy(target,c->args[1]);
    else{
     strcpy(target,getenv("HOME"));
    }
    char directory[1024];
    if(chdir(target) == 0) {
     getcwd(directory, sizeof(directory));
  }
}

void handle_whereis(Cmd c){
    char *path=malloc(1000*sizeof(char));
    char s[]=":";
    strcpy(path,getenv("PATH"));
    char *token=strtok(path,s);

    if(built_in(c))
	printf("%s is a shell built in \n",c->args[1]);    

    while(token){
     char full_path[1000];   
     strcpy(full_path,token);
     strcat(full_path,"/");
     //if(!(strcmp(c->args[1],"bin")==0 && strstr(token,"/bin")!=NULL)) //argument is not bin
     strcat(full_path,c->args[1]);
     
     struct stat st;
     int found = stat(full_path,&st);
     if(!found){
     printf("%s\n",full_path);
     return;
     }    
     token=strtok(NULL,s);
    }
}

void handle_echo(Cmd c){
    char *val = malloc(1000*sizeof(char));

    int i=1;
    while(i < c->nargs){
    char *var = malloc(10000*sizeof(char));    
    var = c->args[i];
    
    if(var[0]=='$'){ // It means it's an environment variable
        var = var+1; 
        strcpy(var,getenv(var));
        printf("%s ",var);
    }
    else{
        printf("%s ",var);
    }
    i++;
 }
    
    printf("\n");
    //free(var);
    free(val);
    return;    
}

void handle_nice(Cmd c){
  int which = PRIO_PROCESS;
  id_t pid;
  int ret;
  int pd_c = fork();

  if(pd_c==0){
  execvp(c->args[2],c->args+2);
  perror("execvp() failed\n");
  int new_p = atoi(c->args[1]);
  ret = getpriority(which, 0); // 0 means current process
  printf("current priority is %d",ret);
  ret = setpriority(which, 0, new_p); 
  }
  wait(NULL);
  return;
}

void handle_logout(Cmd c){
   //printf("inside handle logout\n");
    exit(0);
}

//=======================================================================================
void set_filepointers(Cmd c, int *in, int *out, int *err){
//void set_filepointers(Cmd c){
    int fd_in,fd_out,fd_err;    
    if(c->in==Tin){
    fd_in = open(c->infile,O_RDONLY|O_CREAT,0666);
    in = 10;
    dup2(fd_in,0);
    }
    
    if(c->out==Tout){
    fd_out = open(c->outfile,O_CREAT|O_RDWR|S_IRWXU,0666);
    out=11;
    dup2(fd_out,1);
    }

    else if(c->out==ToutErr){
    fd_err = open(c->outfile,O_CREAT|O_RDWR|S_IRWXU,0666);
    err=12;
    dup2(fd_err,1);
    dup2(fd_err,2);
    }    
    
    else if(c->out==Tapp){
    FILE *f = fopen(c->outfile, "a");
    fseek(f,0,SEEK_END);
    int fapp = fileno(f);
    dup2(fapp,1);   
    }
  
    else if(c->out==TappErr){
    FILE *f = fopen(c->outfile, "a");
    fseek(f,0,SEEK_END);
    int fapp = fileno(f);
    dup2(fapp,1);
    dup2(fapp,2);
    } 
         
return;
}

void unset_filepointers(int stdin_fd,int in, int stdout_fd,int out, int stderr_fd,int err){
    //if(in!=0)
    dup2(stdin_fd,0);
    //if(out!=1)
    dup2(stdout_fd,1);
    //if(err!=2)
    dup2(stderr_fd,2);
    return;    
}

//========================================================================================
void execute_command(Cmd c){
    char com[1000];
    int in=0,out=1,err=2; //assign default values
    int stdin_fd=dup(0);
    int stdout_fd=dup(1);
    int stderr_fd=dup(2);
    strcpy(com,c->args[0]);
    
    if(c->in==Tin){
        in = open(c->infile,O_RDONLY);
        FILE *f = fopen(c->infile, "r");
        
        if(f){
                int f2 = fileno(f);
                dup2(f2,0);
        }
    }
    int fdin,fdout,fderr;
    
    set_filepointers(c,&in,&out,&err);
    //set_filepointers(c); // takes care if redirection

    if(strcmp(com,"pwd")==0)
        handle_pwd();
        
    if(strcmp(com,"setenv")==0)
        handle_setenv(c);
        
    if(strcmp(com,"kill")==0)
        handle_kill(c);    
        
    if(strcmp(com,"cd")==0)
        handle_cd(c);
        
    if(strcmp(com,"where")==0)
        handle_whereis(c); 
      
    if(strcmp(com,"echo")==0)
        handle_echo(c); 

    if(strcmp(com,"unsetenv")==0)
        handle_unsetenv(c);
        
    if(strcmp(com,"logout")==0)
        handle_logout(c);
                
    if(strcmp(com,"nice")==0 || strstr(com,"nice")!=NULL){
	handle_nice(c);
    }            
    unset_filepointers(stdin_fd,in,stdout_fd,out,stderr_fd,err); 
     dup2(stdin_fd,0);
     dup2(stdout_fd,1);
     dup2(stderr_fd,3);
     close(stdin_fd);
     close(stdout_fd);
     close(stderr_fd);
return;   
}


//==================================================================================

void execute_last_command(Cmd c){
int stdin_fd=dup(0);
int stdout_fd=dup(1);
int stderr_fd=dup(2); 
int in=0,out=1,err=2;
   set_filepointers(c,&in,&out,&err);
if(built_in(c)){
    //set_filepointers(c,&in,&out,&err);
    execute_command(c);
}
      
else{
   int pd = fork();
   //set_filepointers(c,&in,&out,&err);
   if(pd==0){   
    //  dup2(stdout_fd,1);
      execvp(c->args[0],c->args);
      dup2(stdout_fd,1);
      perror("execvp() failed"); 
    }
    else{
      int status;
      wait(&status);
    }
}
unset_filepointers(stdin_fd,in,stdout_fd,out,stderr_fd,err);
return;
}

//====================================================================================

void execute_all_commands(Pipe p, int fstdin,int fstdout,int fstderr){
Cmd c = p->head;
int pipe_fd[2];
int pid;
int fd_in=0,in=0,out=1,err=2;
int in1=in,out1=out,err1=err;
int ostdin = dup(0);
int ostdout = dup(1);
int ostderr = dup(2);
if(c->next==NULL){
    execute_last_command(c);
    return;
    }
else{

    while(c->next){
    set_filepointers(c,&in,&out,&err);
	pipe(pipe_fd);
	pid=fork();
	if(pid==0){
            
	    signal_enabler();
	    //child process ;take care of pipe fd 
            close(pipe_fd[0]);
	    //dup2(fd_in,0);
	    dup2(pipe_fd[1],1);
            dup2(pipe_fd[1],2);
	    close(pipe_fd[1]);
		execvp(c->args[0],c->args);
	}
	else if(pid>0){
            wait(pid);
	    close(pipe_fd[1]);
	    //take care of pipe fd
            //fd_in = pipe_fd[0];
	    dup2(pipe_fd[0],0);
	    close(pipe_fd[0]);

	}
	    c = c->next;
    //unset_filepointers(ostdin,in,ostdout,out,ostderr,err); 
    }
  }

int last_pid = fork();
if(last_pid==0){
        set_filepointers(c,&in1,&out1,&err1);
	signal_enabler();
	execvp(c->args[0],c->args);
	return;
       }
   wait(NULL);
dup2(ostdout,1);
dup2(ostdin,0);
dup2(ostderr,3);
close(ostdin);
close(ostdout);
close(ostderr);

}


//====================================================================================
int main(int argc, char *argv[])
{
  Pipe p;
  int run=0; //only for testing ; change it to 0 later on
  char host[100];// = "armadillo";
  int z = gethostname(host, 100);
  int t = 2;
  signal_disabler();
  int in=dup(STDIN_FILENO);
  int out=dup(stdout);
  int err = dup(stderr);
  char *home=malloc(1000*sizeof(char));    
  strcpy(home,getenv("HOME"));
  strcat(home,"/.ushrc");

 if(access(home,F_OK)==0 && t==3){
    int in = dup(0);
    t = 1; int fd=0;
    fd = open(home,O_RDONLY);
    dup2(fd,0);
//   freopen(home,"r",stdin); //read from file as if reading arg from input
    if(fd==-1)
     exit(0);
    Pipe p = parse();
    while(p){
    Cmd c = p->head;
    if(strcmp(c->args[0],"logout")==0)
        exit(0);
    //execute_all_commandc(c,in,out,err);
    execute_last_command(c);
    p = p->next;
    }
   freePipe(p);
   close(fd);
   dup2(in,0);
   //return 0;
}


  while ( 1 ) {
//    printf("%s%% ", host);
     if(t==1){
      //printf("flush everytihng");
      //fflush(stdin);
      fflush(stdout);
      fflush(stderr);
      t=0;
      }
    printf("%s%% ", host);

    p = parse();
    prPipe(p);
   while(p){ 
               
   	execute_all_commands(p,in,out,err);
	p=p->next;
   }
    freePipe(p);
  dup2(in,0);
 }
}
/*........................ end of main.c ....................................*/
