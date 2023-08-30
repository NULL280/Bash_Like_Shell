#include <cstdio>
#include <cstdlib>

#include <iostream>

#include "simpleCommand.hh"

// constructor
SimpleCommand::SimpleCommand() {
  _arguments = std::vector<std::string *>();
}

// deconstructor
SimpleCommand::~SimpleCommand() {
  // iterate over all the arguments and delete them
  // loop vector中的元素
  for (auto & arg : _arguments) {
    delete arg;
  }
}

// insert arg to end of vector
void SimpleCommand::insertArgument( std::string * argument ) {
  // simply add the argument to the vector
  _arguments.push_back(argument);
}

// Print out the simple command
void SimpleCommand::print() {
  for (auto & arg : _arguments) {
    // c++ print (可用printf替代)
    std::cout << "\"" << *arg << "\" \t";
  }
  // effectively the same as printf("\n\n");
  std::cout << std::endl;
}
