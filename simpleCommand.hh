#ifndef simplcommand_hh
#define simplecommand_hh

#include <string>
#include <vector>

struct SimpleCommand {

  // Simple command is simply a vector of strings
  // string vector
  std::vector<std::string *> _arguments;

  SimpleCommand();
  // 移除obj时候call. deconstructor
  ~SimpleCommand();
  void insertArgument( std::string * argument );
  void print();
};

#endif
