/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <cstring>
#include "y.tab.hh"

// 导入wirte(), read()
#include <unistd.h>

// 导入fork()
#include "unistd.h"

// 导入c_str()
#include <cstring>

// 导入cpp print
#include <iostream>

// 导入waitpid()
#include <sys/wait.h>

// 导入 open()  close()
#include <fcntl.h>

// 导入 setenv(), unsetenv(), getenv(), realpath()
#include <stdlib.h>

// 导入 realpath()
#include <limits.h>

// 存储return code ${?}
extern int * return_code;

// 存储back PID ${!}
extern int * back_PID;

// 存储 pre cmd last arg ${_}
extern std::string * pre_cmd_last_arg;

// 存储 shell path ${SHELL}
extern char * shell_path;

// 导入shell exit
extern void shell_exit();

// 导入编辑器
extern "C" char * read_line();

int mygetc(FILE * f) {
  static char *p;
  char ch;

     if (!isatty(0)) {
  // stdin is not a tty. Call real getc
    return getc(f);
  }

  // stdin is a tty. Call our read_line.
  if (p==NULL || *p == 0) {
    char * s = read_line();
    p = s;
  }

  ch = *p;
  p++;

  return ch;
}

#undef getc
#define getc(f) mygetc(f)

// 导入编辑器结束

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

%}

%option noyywrap

%%
\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

">>" {
  return GREATGREAT;
}

">&" {
  return GREATAMPERSAND;
}

">>&" {
  return GREATGREATAMPERSAND;
}

"2>" {
  return TWOGREAT;
}

"<" {
  return LESS;
}

"&" {
  return AMPERSAND;
}

"|" {
	return PIPE;
}

exit {
	// 退出 (exit)

	//printf("Good bye!!\n");
	//_exit(0);
	
	shell_exit();
}

source[ ][^ \t\n<>|&][^ \t\n<>|&]* {
	// source 函数

	// 文件input
	char * file_dir = yytext + 7;

	int file_input = open(file_dir, O_RDONLY);

	
	// !!! 长度受限
	int length = 1024;
	// 创建buffer
	char* output = new char[length];
	// 读取文件
	int ture_length = read(file_input, output, length);
	// 关闭文件接口
	close(file_input);

	//printf("%s", output);

	output[ture_length] = '\0';

	// 从后向前loop buffer
	for(int i = ture_length - 1; i >= 0; i--) {
		/*
		// 替换'\n' 为 ' '
		if(output[i] == '\n')
		{
			output[i] = ' ';
		}
		*/
		// 丢回lex
		myunputc(output[i]);
	}
}


$\([^\t\n][^\t\n]*\)|\`[^\n\`]*\` {
	// Subshell (`` $())

	// 得到带$()string的ptr
	std::string command = std::string(yytext);
	//std::string command = std::string(yytext);

	// 去除$
	if (command.at(0) == '$') {
    	command = command.substr(1,command.size() - 1);
  	}
	
	// 去除() (或者是'')
	command = command.substr(1, command.size() - 2);
	

	// 存档母进程stdin stdout用于恢复
	int tmpin = dup(0);
	int tmpout = dup(1);

	// 创建pipe
	int pipein[2];
	int pipeout[2];
	pipe(pipein);
	pipe(pipeout);

	// 子进程stdin重定向
	dup2(pipein[0], 0);
	close(pipein[0]);

	// 子进程stdout重定向
	dup2(pipeout[1], 1);
	close(pipeout[1]);

	// 创建子进程
	int pid = fork();

	// 运行子进程(subshell)
	if(pid == 0) {

		// 关闭pipeout[0]接口 (母读取)
		close(pipeout[0]);
		// 关闭pipein[1]接口 (母写入)
		close(pipein[1]);

		// 运行子进程
		char ** argv = new char*[1];
		argv[0] = (char*)"/proc/self/exe";
		execvp(argv[0], argv);
		perror("subshell execlp");
		_exit(1);

	} else if(pid < 0) {	
		// 子进程报错
		perror("fork");
		_exit(1);
	} else {
		// 运行母进程

		// 关闭pipein[0]接口 (子读取)
		close(pipein[0]);
		// 关闭pipeout[1]接口 (子写入)
		close(pipeout[1]);

		// 向子进程输入命令
		write(pipein[1], command.c_str(), command.size());
		// 向子进程输入命令结束
		write(pipein[1], "\nexit\n", 6);
		// 关闭pipein[1]接口 (母写入)
		close(pipein[1]);

		// 等待子进程结束
		waitpid(pid, NULL, 0);

		// 母进程stdin stdout重定向(还原)
		dup2(tmpin, 0);
		close(tmpin);
		dup2(tmpout, 1);
		close(tmpout);

		// 初始化buffer长度
		// !!! 长度受限
		int length = 1024;

		// 创建buffer
		char* output = new char[length];

		// 读取子进程输出
		int ture_length = read(pipeout[0], output, length);

    	// 关闭pipeout[0]接口 (母读取)
		close(pipeout[0]);

		// 从后向前loop buffer
		for(int i = ture_length - 1; i >= 0; i--) {
			// 替换'\n' 为 ' '
			if(output[i] == '\n')
			{
				output[i] = ' ';
			}
			// 丢回lex
			myunputc(output[i]);
		}
	}
}

([^ \\\t\n<>|&]*\\.[^ \\\t\n<>|&]*)* {
	// Escaping (\)

	// 原始string
	auto word = new std::string(yytext);

	// 处理
	for (int i = 0; i < (word->length()); i++) {
		if (word->at(i) == '\\') {
			word->erase(i, 1);
		}
    }

	yylval.cpp_string = word;
	return WORD;
}

\"[^\t\n\"]*\"  {
	// Quotes ("")

	// 原始string
	auto word = new std::string(yytext);

	// 去除""
	*word = word->substr(1, word->length() - 2);

	yylval.cpp_string = word;
	return WORD;
}

~[^ \t\n]* {
	// 3.2: Tilde expansion (~)

	// 原始string
	auto word = std::string(yytext);

	auto current_user = std::string(getenv("USER"));
	auto path = std::string(getenv("HOME"));
	path = path.substr(0, path.length() - current_user.length());

	if (word.length() == 1 || word.at(1) == '/') {
		// current user
		word = path + current_user + word.substr(1, word.length() - 1);
	} else {
		// other user
		word = path + word.substr(1, word.length() - 1);
	}

	yylval.cpp_string = new std::string(word);
	return WORD;
}

([^ \t\n<>|&]*$\{[^ \t\n]*\}[^ \t\n<>|&]*)* {
	// Environment variable (${})

	// 原始string
	auto word = std::string(yytext);

	// 处理
	for (int i = 1; i < (word.length()); i++) {
		// 发现env
		if (word.at(i) == '{' & word.at(i - 1) == '$') {
			// 找env结束
			for (int j = i + 1; j < (word.length()); j++) {
				if (word.at(j) == '}') {
					// 获得env_name
					auto env_name = word.substr(i + 1, j - i - 1);

					// 获取env
					std::string env = "";
					if (env_name == "$") {
						// ${$} shell PID
						// to_string: int 到 string
						env = std::to_string(getpid());
					} else if (env_name == "?") {
						// ${?} return code for last command
						env = std::to_string(*return_code);				
					} else if (env_name == "!") {
						// ${!} last background cmd PID
						env = std::to_string(*back_PID);
					} else if (env_name == "_") {
						// ${_} pre cmd last arg
						env = *pre_cmd_last_arg;
					} else if (env_name == "SHELL") {
						// ${SHELL} path of shell
						char absolute_path[1024];
						realpath(shell_path, absolute_path);
						env = std::string(absolute_path);
					} else {
						// 通用情况
						env = std::string(getenv(env_name.c_str()));
					}

					// 修改原word
					word.replace(word.find(word.substr(i - 1, j - i + 2)), j - i + 2, env);

					// end this env
					break;
				}
			}
		}
    }

	yylval.cpp_string = new std::string(word);
	return WORD;
}

[^ \t\n<>|&][^ \t\n<>|&]*  {
	// WORD 需要放最后!

	auto word = new std::string(yytext);

	/*
	// Environment variable (${})
	if (command.at(0) == '$' ) {
    	command = command.substr(1,command.size() - 1);
  	}
	*/

	yylval.cpp_string = word;
	return WORD;
}


%%