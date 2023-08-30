#ifndef shell_hh
#define shell_hh

#include "command.hh"

struct Shell {
  // static 将被parse
  static void prompt();

  static Command _currentCommand;
};

#endif
