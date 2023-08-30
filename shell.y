
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> file's name]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires
{
#include <string>
#include "string.h"
#include "unistd.h"

// 导入 std::remove()
#include <algorithm>

// 导入 cout print
#include <iostream>

// 导入 vector
#include <vector>

// 导入 regcomp()
#include <regex.h>

// 导入 opendir()
#include <dirent.h>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

%token <cpp_string> WORD
%token NOTOKEN NEWLINE GREAT GREATGREAT GREATAMPERSAND GREATGREATAMPERSAND TWOGREAT LESS AMPERSAND PIPE

%{
//#define yylex yylex
#include <cstdio>
#include "shell.hh"

void yyerror(const char * s);
int yylex();

// 所有文件的array
std::vector<std::string> wildcard_array = std::vector<std::string>();

// 声明 expandWildcard
void expandWildcard(std::string prefix, std::string suffix);
%}

%%

goal:
  command_list
  ;

command_list:
  command_line
  | command_list command_line
  ;

command_line:
  pipe_list io_modifier_list background_optional NEWLINE {
    //printf("   Yacc: Execute command\n");
    Shell::_currentCommand.execute();
  }
  | NEWLINE {
    Shell::prompt();
  }
  | pipe_list io_modifier_list background_optional {
    // 文件最后一行, 无回车情况
    //printf("   Yacc: Execute command\n");
    Shell::_currentCommand.execute();
  }
  | error NEWLINE { yyerrok; }
  ;

pipe_list:
  pipe_list PIPE command_and_args
  | command_and_args
  ;

command_and_args:
  command_word argument_list {
    Shell::_currentCommand.
    insertSimpleCommand( Command::_currentSimpleCommand ); // 加simple cmd 入cmd table
  }
  ;

command_word:
  WORD {
    //printf("   Yacc: insert command \"%s\"\n", $1->c_str());
    Command::_currentSimpleCommand = new SimpleCommand(); // 创建simple cmd
    Command::_currentSimpleCommand->insertArgument( $1 );
  }
  ;

argument_list:
  argument_list argument
  | /* can be empty */
  ;

argument:
  WORD {
    // wildcard
    auto arg = *($1);
    if ((arg.find('?') != std::string::npos) | (arg.find('*') != std::string::npos)) {

      // 添加合规 file path 到 array
      expandWildcard("", arg);

      // 无匹配file & path, 添加原arg
      if (wildcard_array.size() == 0) {
        Command::_currentSimpleCommand->insertArgument( $1 );
        //return;
      }

      // 排序array
      std::sort(wildcard_array.begin(), wildcard_array.end());

      // 添加file names到argument table
      for (const auto &item : wildcard_array) {
        auto new_file = new std::string(item);
        Command::_currentSimpleCommand->insertArgument(new_file);
        //std::cout << item << "\n";
      }

      // 清空此次wildcard array
      wildcard_array.clear();

    } else {
      // 插 arg 入simple cmd
      Command::_currentSimpleCommand->insertArgument( $1 ); 
    }
  }
  ;

io_modifier_list:
  io_modifier_list io_modifier
  | /* can be empty */
  ;

io_modifier:
  GREAT WORD {
    //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    //判断是否重复设置
    if (Shell::_currentCommand._outFile == 0) {
      Shell::_currentCommand._outFile = $2; // 设 file name 置 outFile
      Shell::_currentCommand._append = false;
    } else {
      printf("Ambiguous output redirect.\n");
    }
  }
  | GREATGREAT WORD {
    //printf("   Yacc: insert output (append) \"%s\"\n", $2->c_str());
    //判断是否重复设置
    if (Shell::_currentCommand._outFile == 0) {
      Shell::_currentCommand._outFile = $2;
      Shell::_currentCommand._append = true;
    } else {
      printf("Ambiguous output redirect.\n");
    }
  }
  | GREATAMPERSAND WORD {
    //printf("   Yacc: insert output and error \"%s\"\n", $2->c_str());
    //判断是否重复设置
    if ((Shell::_currentCommand._outFile == 0) & (Shell::_currentCommand._errFile == 0)) {
      Shell::_currentCommand._outFile = $2;
      Shell::_currentCommand._errFile = new std::string(*$2);
      Shell::_currentCommand._append = false;
    } else {
      printf("Ambiguous output redirect.\n");
    }
  }
  | GREATGREATAMPERSAND WORD {
    //printf("   Yacc: insert output and error \"%s\"\n", $2->c_str());
    //判断是否重复设置
    if ((Shell::_currentCommand._outFile == 0) & (Shell::_currentCommand._errFile == 0)) {
      Shell::_currentCommand._outFile = $2;
      Shell::_currentCommand._errFile = new std::string(*$2);
      Shell::_currentCommand._append = true;
    } else {
      printf("Ambiguous output redirect.\n");
    }
  }
  | TWOGREAT WORD {
    //printf("   Yacc: insert error \"%s\"\n", $2->c_str());
    //判断是否重复设置
    if (Shell::_currentCommand._errFile == 0) {
      Shell::_currentCommand._errFile = $2;
      Shell::_currentCommand._append = false;
    } else {
      printf("Ambiguous output redirect.\n");
    }
  }
  | LESS WORD {
    //printf("   Yacc: insert input \"%s\"\n", $2->c_str());
    //判断是否重复设置
    if (Shell::_currentCommand._inFile == 0) {
      Shell::_currentCommand._inFile = $2;
    } else {
      printf("Ambiguous output redirect.\n");
    }
  }
  ;

background_optional:
  AMPERSAND {
    Shell::_currentCommand._background = true;
  }
  | /* can be empty */
  ;

%%

void expandWildcard(std::string prefix, std::string suffix) {
  // 完成路径分析, 添加文件名入array
  if (suffix.length() == 0) {
    wildcard_array.push_back(std::string(prefix));
    return;
  }

  std::string wildcard;
  std::string new_prefix_part = prefix;
  std::string new_suffix;
  bool relative_path = false;

  // 获得suffix中第一段wildcard(不一定是wildcard)
  if (suffix.at(0) == '/') {
    // 绝对路径
    wildcard = suffix.substr(1, suffix.length() - 1);
    new_prefix_part += "/";
  } else {
    // 相对路径
    wildcard = suffix;
    relative_path = true;
  }
  auto slash_index = wildcard.find("/");
  if (slash_index != std::string::npos) {
    // 中段路径
    new_suffix = wildcard.substr(slash_index, wildcard.length() - slash_index);
    wildcard = wildcard.substr(0, slash_index);
  } else {
    // 尾端路径
    new_suffix = "";
  }
  
  // 此段路径是否wild card
  if ((wildcard.find('?') != std::string::npos) | (wildcard.find('*') != std::string::npos)) {
    // 是wild card

    // 转换 wild card 到 regular expression
    std::string regular_expression = "^";
    for(std::string::size_type i = 0; i < wildcard.size(); ++i) {
      if (wildcard[i] == '*') {
        regular_expression += ".*";
      } else if (wildcard[i] == '?') {
        regular_expression += ".";
      } else if (wildcard[i] == '.') {
        regular_expression += "\\.";
      } else {
        regular_expression += wildcard[i];
      }
    }
    regular_expression += "$";

    // 是否包含隐藏文件
    bool include_hidden = false;
    if (wildcard.at(0) == '.') {
      include_hidden = true;
    }

    // 编译RE为有限自动机FSA
    regex_t preg;
    int err = regcomp( &preg, regular_expression.c_str(), REG_EXTENDED|REG_NOSUB);
    // 报错
    if (err) {
      perror("FSAcompile");
      //return;
    }

    // 打开路径
    DIR * dir;
    if(prefix.length() == 0) {
      // 起始路径
      if (relative_path) {
        // 相对路径
        dir = opendir(".");
      } else {
        // 绝对路径
        dir = opendir("/");
      }  
    } else {
      // 中段路径
      dir = opendir(prefix.c_str());
    }
    //报错
    if (dir == NULL) {
      perror("open dir");
      return;
    }

    struct dirent * ent;
    while ((ent = readdir(dir)) != NULL) {
      auto entry_name = ent->d_name;
      auto entry_type = ent->d_type;

      // 跳过隐藏文件
      if (!include_hidden && entry_name[0] == '.') {
        continue;
      }

      // 跳过中段路径的非路径类型
      if (new_suffix.length() != 0 && entry_type != DT_DIR) {
        continue;
      }

      // 对比文件名与RE
      if (regexec(&preg, entry_name, 0, NULL, 0) == 0) {
        // 符合RE
        //std::cout << "old pre: " << prefix << "\n";
        //std::cout << "new pre: " << new_prefix_part << "\n";
        //std::cout << "entry_name: " << entry_name << "\n";
        expandWildcard(new_prefix_part + entry_name, new_suffix);
      }
    }

    // 关闭路径
    closedir(dir);

  } else {
    // 非 wild card
    expandWildcard(new_prefix_part + wildcard, new_suffix);
  }
}

void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

#if 0
main()
{
  yyparse();
}
#endif
