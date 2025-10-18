#pragma once
#include "include_libs.h"
#include "session_manager.h"

class Command {
 public:
  virtual std::string execute() = 0;
  virtual ~Command() {};
};

class LogInCommand : public Command {
 private:
  std::string user;

 public:
  LogInCommand(std::string user) : user(user) {}

  std::string execute() override {
    if (SessionManager::getInstance().login(this->user)) {
      return std::string("Logged In Successfully");
    } else {
      return std::string("Wrong Name");
    }
  }
};

class GetLoggedUsersCommand : public Command {
 private:
 public:
  std::string execute() override {
    if (!SessionManager::getInstance().isLoggedIn()) {
      return std::string(
          "You must be logged in in order to use this command\n");
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
      return std::string("<ERROR> Error creating pipe\n");
    }

    pid_t pid = fork();
    if (pid == -1) {
      return std::string("<ERROR> Error forking \n");
    }

    if (pid == 0) {
      close(pipefd[0]);

      utmpname("/var/log/wtmp");

      setutent();
      struct utmp* entry;
      std::string output = "Logged in users: \n";

      std::map<std::string, utmp> active_sessions;

      while ((entry = getutent()) != NULL) {
        std::string line = std::string(entry->ut_line);

        if (entry->ut_type == USER_PROCESS) {
          active_sessions[line] = *entry;
        } else if (entry->ut_type == DEAD_PROCESS) {
          active_sessions.erase(line);
        }
      }

      endutent();

      if (active_sessions.empty()) {
        output += "No active user sessions!\n";
      } else {
        for (const auto& [line, session] : active_sessions) {
          time_t login_time = session.ut_tv.tv_sec;
          output += "User: " + std::string(session.ut_user) +
                    " | Host: " + std::string(session.ut_host) +
                    " | Time of entry: " + std::string(ctime(&login_time));
        }
      }

      endutent();
      write(pipefd[1], output.c_str(), output.length());
      close(pipefd[1]);
      exit(0);
    } else {
      close(pipefd[1]);

      char buffer[1024];
      int bytes = read(pipefd[0], buffer, sizeof(buffer));
      if (bytes > 0) {
        buffer[bytes] = '\0';
        return std::string(buffer);
      }

      return std::string("Can't read utmp");

      close(pipefd[0]);
      wait(NULL);
    }
  }
};

class LogOutCommand : public Command {
 public:
  std::string execute() override { return std::string("Loggin out...\n"); }
};

class QuitCommand : public Command {
 public:
  std::string execute() override { return std::string("Quitting...\n"); }
};

class GetProcInfoCommand : public Command {
 private:
  std::string process;

 public:
  GetProcInfoCommand(std::string process) { this->process = process; }
  std::string execute() override { return std::string("Process info...\n"); }
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