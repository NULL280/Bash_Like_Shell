/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT 
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE 
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM  
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <cstdio>
#include <cstdlib>

#include <iostream>

#include "command.hh"
#include "shell.hh"

// 导入fork()
#include "unistd.h"

// 导入 strcpy()
#include <string.h>

// 导入 waitpid()
#include <sys/wait.h>

// 导入 open()  close()
#include <fcntl.h>

// 导入 setenv(), unsetenv(), getenv()
#include <stdlib.h>

// 导入 chdir()
#include <unistd.h>

// 导入 home dir
#include <sys/types.h>
#include <pwd.h>

// 导入 WEXITSTATUS()
#include <sys/wait.h>

// 存储return code ${?}
extern int * return_code;

// 存储back PID ${!}
extern int * back_PID;

// 存储 pre cmd last arg ${_}
extern std::string * pre_cmd_last_arg;

Command::Command() {
    // Initialize a new vector of Simple Commands 初始化vector
    _simpleCommands = std::vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
    _append = false;
}

// add simple cmd to end of vector
void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simpleCommand);
}

// 执行完cmd后, 清零vector, 还原Shell::_currentCommand
void Command::clear() {
    // deallocate all the simple commands in the command vector
    for (auto simpleCommand : _simpleCommands) {
        // call destructor of simple command
        delete simpleCommand;
    }

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommands.clear();

    if ( _outFile ) {
        delete _outFile;
    }
    _outFile = NULL;

    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;

    if ( _errFile ) {
        delete _errFile;
    }
    _errFile = NULL;

    _background = false;

    _append = false;
}


void Command::print() {
    // print cmd table
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simpleCommand : _simpleCommands ) {
        printf("  %-3d ", i++ );
        // print simple cmd
        simpleCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    // 转换c++字符串到c (_outFile->c_str()), 来print (? + :根据真否选择output)
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFile?_outFile->c_str():"default",
            _inFile?_inFile->c_str():"default",
            _errFile?_errFile->c_str():"default",
            _background?"YES":"NO");
    printf( "\n\n" );
}



/*
    执行cmd line
*/
void Command::execute() {
    // Don't do anything if there are no simple commands 查看是否清零
    // .size: vector 元素数量
    if ( _simpleCommands.size() == 0 ) {
        // Print new prompt
        Shell::prompt();
        return;
    }

    // 存储最后的arg
    *pre_cmd_last_arg = *(_simpleCommands.back()->_arguments.back());


    // 显示cmd table
    //print();

    // Add execution here
    // For every simple command fork a new process
    // Setup i/o redirection
    // and call exec

    // 母进程 std in, out 存档
    int tmpin = dup(0);
    int tmpout = dup(1);
    int tmperr = dup(2);

    // 获得, 定向 stderror
    if (_errFile) {
        int fderror;
        // 文件output
        if (_append) {
            // append
            fderror = open(_errFile->c_str(), O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
        } else {
            // 覆写
            fderror = open(_errFile->c_str(), O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
        }

        // error 定向
        dup2(fderror, 2);
        close(fderror);
    }

    // 获得第一条 cmd fdin
    int fdin;
    if (_inFile) {
        // 文件input
        fdin = open(_inFile->c_str(), O_RDONLY);
    } else {
        // 默认input
        fdin = dup(tmpin);
    }

    int ret;
    int fdout;
    
    // 遍历简单命令
    for (long unsigned int i = 0; i < _simpleCommands.size(); i++) {
        // 转换c++ string * vector (_simpleCommands[i]->_arguments) 至 char * array (argv)
        long unsigned int argv_num = _simpleCommands[i]->_arguments.size();
        char * argv[argv_num];
        //char ** argv = new char*[argv_num];
        for (long unsigned int j = 0; j < argv_num; j++) {
            argv[j] = (char*) _simpleCommands[i]->_arguments[j]->c_str();
        }
        argv[argv_num] = NULL;


        // input 定向
        dup2(fdin, 0);
        close(fdin);

        // 获得fdout
        if (i == _simpleCommands.size() - 1) {
            // 输出文件 (最后一条指令)
            if (_outFile) {
                if (_append) {
                    // append
                    fdout = open(_outFile->c_str(), O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
                } else {
                    // 覆写
                    fdout = open(_outFile->c_str(), O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
                }
            } else {
                // 默认input
                fdout = dup(tmpout);
            }
        } else {
            // 输出pipe
            int fdpipe[2];
            pipe(fdpipe);
            // 此loop cmd fdout
            fdout = fdpipe[1];
            // 下个loop cmd fdin
            fdin = fdpipe[0];
        }

        // output 定向
        dup2(fdout, 1);
        close(fdout);

        // 环境变量
        extern char **environ;

        // 内置函数

        // 运行 setenv
        if (!strcmp(argv[0], "setenv")) {
            setenv(argv[1], argv[2], 1);
            // 不创建子进程, 进行下个简单命令
            continue;
        }

        // 运行 unsetenv
        if (!strcmp(argv[0], "unsetenv")) {
            unsetenv(argv[1]);
            // 不创建子进程, 进行下个简单命令
            continue;
        }

        // 运行 cd
        if (!strcmp(argv[0], "cd")) {
            if (argv_num == 1) {
                // 未声明路径, 默认home
                const char *homedir = getenv("HOME");
                chdir(homedir);
            } else {
                if (chdir(argv[1]) == -1) {
                    // 报错不存在路径
                    char err_mess[32];
                    sprintf(err_mess, "cd: can't cd to %s", argv[1]);
                    perror(err_mess);
                }
            }
            
            // 不创建子进程, 进行下个简单命令
            continue;
        }

        // 创建子进程
        ret = fork();


        // 判断是否为子进程
        if (ret == 0) {
            // 关闭不需要的fd
            close(tmpin);
            close(tmpout);
            close(tmperr);

            // 运行 printenv
            if (!strcmp(argv[0], "printenv")) {
                char **tmp = environ;
                // 遍历打印环境变量
                while (*tmp != NULL) {
                    printf("%s\n", *tmp);
                    tmp++;
                }
                // 注意: 必须退出子进程, 否则会出错
                //??? why _exit() not work?
                exit(1);
            }
            // 运行子进程
            execvp(argv[0], argv);

            // execvp报错
            perror("execvp");
            _exit(1);
        } else if (ret < 0) {
            // 创建进程报错
            perror("fork");
            return;
        }
    }

    // 母进程 std in, out 还原
    dup2(tmpin, 0);
    dup2(tmpout, 1);
    dup2(tmperr, 2);
    close(tmpin);
    close(tmpout);
    close(tmperr);

    // 根据 background 决定是否后台运行 (是, 下一行cmd不等待直接运行)
    if (!_background) {
        // ret: for loop 中最后的cmd
        int status = -1;
        waitpid(ret, &status, 0);
        if (WIFEXITED(status)) {
            // 正常退出, 存储子进程return code
            *return_code = (int) WEXITSTATUS(status);
        }
    } else {
        // 存储 back PID
        *back_PID = ret;
    }


    // Clear to prepare for next command 为下一行做准备
    clear();

    // Print new prompt
    Shell::prompt();
}

// 现在正在parse的 command
SimpleCommand * Command::_currentSimpleCommand;
