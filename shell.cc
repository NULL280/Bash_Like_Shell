#include <cstdio>

#include "shell.hh"

// 导入 isatty()
#include <unistd.h>

// 导入sigaction
#include <signal.h>

// 导入 wait()
#include <sys/wait.h>

// 初始化 return code ${?}
int * return_code = new int();

// 初始化 back PID ${!}
int * back_PID = new int();

// 初始化 pre cmd last arg ${_}
std::string * pre_cmd_last_arg = new std::string();

// 初始化 shell path ${SHELL}
char * shell_path = new char();

// yyparse来自yacc, 表示开始parse
int yyparse(void);

void shell_exit() {
  //delete return_code;
  //delete back_PID;
  //delete pre_cmd_last_arg;
  //delete shell_path;
  exit(0);
}

void Shell::prompt() {
  // 是terminal而非file作为stdin
  if (isatty(0)) {
    printf("myshell>");
    // 确保立刻print
    fflush(stdout);
  }
}

// ctrlc处理者
void disp1( int sig )
{

}

// 子进程结束处理者
void killzombie( int sig )
{
  //wait3(0, 0, NULL);
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]) {
  // 存储shell path
  shell_path = argv[0];

  // ctrl c 信号处理
  // 创建信号结构
  struct sigaction ctrlcsa;
	// 添加处理者 (function pointer)
  ctrlcsa.sa_handler = disp1;
  // 清除信号面具(多个信号重叠), 防止被别的信号打断
  sigemptyset(&ctrlcsa.sa_mask);
  // 默认flag (0), 会干扰system call, SA_RESTART不会出错
  ctrlcsa.sa_flags = SA_RESTART;

  // 子进程结束信号处理
  // 创建信号结构
  struct sigaction signalAction;
	// 添加处理者 (function pointer)
  signalAction.sa_handler = killzombie;
  // 清除信号面具(多个信号重叠), 防止被别的信号打断
  sigemptyset(&signalAction.sa_mask);
  // 默认flag (0), 会干扰system call, SA_RESTART不会出错
  signalAction.sa_flags = SA_RESTART;

  // sigation信号应答机
  // SIGINT: ctrl c 信号
  if(sigaction(SIGINT, &ctrlcsa, NULL)){
    perror("sigaction");
    exit(-1);
  }

  // 子进程结束信号应答机
  // SIGCHLD: 子进程结束信号
  int error = sigaction(SIGCHLD, &signalAction, NULL );
  if(error){
    perror( "sigaction" ); 
    exit( -1 ); 
  }

  // 在这 print "myshell>"
  Shell::prompt();
  // call shell.y
  yyparse();
}

Command Shell::_currentCommand;
