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
      return std::string("You must be logged in in order to use this command");
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
  std::string execute() override {
    if (SessionManager::getInstance().logout()) {
      return std::string("Logged Out Successfully");
    } else {
      return std::string("Can't log out if you're not logged in!");
    }
  }
};

class QuitCommand : public Command {
 public:
  std::string execute() override { return std::string("QUIT"); }
};

class GetProcInfoCommand : public Command {
 private:
  std::string process;

 public:
  GetProcInfoCommand(std::string process) { this->process = process; }
  std::string execute() override {
    if (!SessionManager::getInstance().isLoggedIn()) {
      return std::string("You must be logged in to use this command!");
    }

    int sockets[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    int pid = fork();

    // 0 - parinte
    // 1 - fiu
    if (pid == 0) {
      close(sockets[0]);

      std::string path = "/proc/" + process + "/status";
      std::ifstream file(path);
      if (!file.is_open()) {
        return std::string("Error opening " + path);
      }

      std::string name, ppid, uid, vmsize, state;
      std::string line;
      while (std::getline(file, line)) {
        if (line.find("Name:") == 0) {
          name = line.substr(line.find(":") + 1);
          name.erase(0, name.find_first_not_of(" \n\t"));
        } else if (line.find("State:") == 0) {
          state = line.substr(line.find(":") + 1);
          state.erase(0, state.find_first_not_of(" \n\t"));
        } else if (line.find("Uid:") == 0) {
          uid = line.substr(line.find(":") + 1);
          uid.erase(0, uid.find_first_not_of(" \n\t"));
        } else if (line.find("PPid:") == 0) {
          ppid = line.substr(line.find(":") + 1);
          ppid.erase(0, ppid.find_first_not_of(" \n\t"));
        } else if (line.find("VmSize:") == 0) {
          vmsize = line.substr(line.find(":") + 1);
          vmsize.erase(0, vmsize.find_first_not_of(" \n\t"));
        }
      }

      std::string output = "Name: " + name + " | State: " + state +
                           " | UID: " + uid + " | PPID: " + ppid +
                           " | VmSize: " + vmsize;

      write(sockets[1], output.c_str(), output.length());
      close(sockets[1]);
      exit(0);
    } else {
      close(sockets[1]);
      char buffer[1024];
      std::string output = "";

      int bytes = read(sockets[0], buffer, sizeof(buffer));
      if (bytes > 0) {
        buffer[bytes] = '\0';
        output = buffer;
      }

      wait(NULL);
      return output;
    }
  }
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