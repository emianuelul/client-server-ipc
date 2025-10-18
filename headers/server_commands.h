#pragma once
#include "include_libs.h"
#include "session_manager.h"

class Command {
 public:
  virtual void execute() = 0;
  virtual ~Command() {};
};

class LogInCommand : public Command {
 private:
  std::string user;

 public:
  LogInCommand(std::string user) : user(user) {}

  void execute() override {
    if (SessionManager::getInstance().login(this->user)) {
      std::cout << "<SERVER> LOGGED IN SUCCESSFULLY\n";
    } else {
      std::cout << "<SERVER> WRONG USERNAME OR PASSWORD\n";
    }
  }
};

class GetLoggedUsersCommand : public Command {
 private:
 public:
  void execute() override {
    if (SessionManager::getInstance().isLoggedIn()) {
      std::cout << "<SERVER> Current User: "
                << SessionManager::getInstance().getCurrentUser() << '\n';
    } else {
      std::cout << "<SERVER> You must be logged to access this command!\n";
    }
  }
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

class CommandFactory {
 public:
  static std::unique_ptr<Command> createCommand(std::string command) {
    if (command.find("login :") == 0) {
      std::string username = command.substr(8);

      username.erase(0, username.find_first_not_of(" \t\n"));
      username.erase(username.find_last_not_of(" \t\n") + 1);

      return std::make_unique<LogInCommand>(username);
    } else if (command.find("logout") == 0) {
      return std::make_unique<LogOutCommand>();
    } else if (command.find("quit") == 0) {
      return std::make_unique<QuitCommand>();
    } else if (command.find("get-logged-users") == 0) {
      return std::make_unique<GetLoggedUsersCommand>();
    } else if (command.find("get-proc-info :") == 0) {
      std::string pid = command.substr(16);

      pid.erase(0, pid.find_first_not_of(" \t"));
      pid.erase(pid.find_last_not_of(" \t") + 1);

      return std::make_unique<GetProcInfoCommand>(pid);
    } else {
      throw std::invalid_argument("Invalid command -> " + command);
    }
  }
};