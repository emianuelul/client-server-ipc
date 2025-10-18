#pragma once
#include "headers/include_libs.h"

class Command {
 public:
  virtual void execute() = 0;
  virtual ~Command() {};
};

class LogInCommand : public Command {
 private:
  std::string username;

 public:
  LogInCommand(std::string username) { this->username = username; }

  void execute() override { std::cout << "Logging in...\n"; }
};

class GetLoggedUsersCommand : public Command {
 public:
  void execute() override { std::cout << "Logged Users:\n"; }
};

class LogOutCommand : public Command {
 public:
  void execute() override { std::cout << "Loggin out...\n"; }
};

class QuitCommand : public Command {
 public:
  void execute() override { std::cout << "Quitting...\n"; }
};

class GetProcInfoCommand : public Command {
 private:
  std::string process;

 public:
  GetProcInfoCommand(std::string process) { this->process = process; }
  void execute() override { std::cout << "Process info...\n"; }
};