#ifndef command_hh
#define command_hh

#include "simpleCommand.hh"

// Command Data Structure

struct Command {
  // simple command vector
  std::vector<SimpleCommand *> _simpleCommands;
  std::string * _outFile;
  std::string * _inFile;
  std::string * _errFile;
  // true or false
  bool _background;
  bool _append;

  // constructor
  Command();
  // constructor for insert
  void insertSimpleCommand( SimpleCommand * simpleCommand );

  void clear();
  void print();
  void execute();

  // static: 属于class的变量(实例间共享)
  // 正在parse的command
  static SimpleCommand *_currentSimpleCommand;
};

#endif
