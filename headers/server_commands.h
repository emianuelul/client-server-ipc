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

  // COMUNICARE PRIN FIFO-URI
  std::string execute() override {
    const char* fifo_path = "./temp/login_fifo";

    unlink(fifo_path);
    if (mkfifo(fifo_path, 0777) == -1) {
      throw PipeException("Error creating FIFO");
    }

    int pid = fork();
    if (pid == -1) {
      unlink(fifo_path);
      throw ForkException("Error Forking");
    }

    if (pid == 0) {
      int fifo_fd = open(fifo_path, O_WRONLY);
      if (fifo_fd == -1) {
        _exit(1);
      }

      std::string output;
      if (SessionManager::getInstance().login(this->user)) {
        output = "1";
      } else {
        output = "0";
      }

      write(fifo_fd, output.c_str(), output.length());
      close(fifo_fd);
      _exit(0);
    } else {
      int fifo_fd = open(fifo_path, O_RDONLY);
      if (fifo_fd == -1) {
        unlink(fifo_path);
        throw PipeException("Error opening FIFO for reading");
      }

      char buffer[256];
      int bytes = read(fifo_fd, buffer, sizeof(buffer) - 1);

      close(fifo_fd);
      wait(NULL);
      unlink(fifo_path);

      if (bytes > 0) {
        buffer[bytes] = '\0';
        if (buffer[0] == '1') {
          SessionManager::getInstance().login(this->user);
          return std::string("Logged In Successfully!");
        }
      }

      throw AuthException("Failed Logging In: Wrong Username!");
    }
  }
};

class GetLoggedUsersCommand : public Command {
 private:
 public:
   // COMUNICARE PRIN PIPE ANONIM
  std::string execute() override {
    if (!SessionManager::getInstance().isLoggedIn()) {
      throw AuthException("You must be logged in to use this command!");
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
      throw PipeException("Error creating pipe");
    }

    pid_t pid = fork();
    if (pid == -1) {
      close(pipefd[0]);
      close(pipefd[1]);
      throw ForkException("Error Forking");
    }

    if (pid == 0) {
      close(pipefd[0]);

      utmpname("/var/log/wtmp");

      setutent();
      struct utmp* entry;
      std::string output = "Logged in users: \n\n";

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
      _exit(0);
    } else {
      close(pipefd[1]);

      char buffer[1024];
      std::string output;
      int bytes = read(pipefd[0], buffer, sizeof(buffer));
      if (bytes > 0) {
        buffer[bytes] = '\0';
        output = buffer;
      }

      close(pipefd[0]);
      wait(NULL);

      if (bytes > 0) {
        return output;
      }

      throw FileException("Can't read utmp!");
    }
  }
};

class LogOutCommand : public Command {
 public:
  std::string execute() override {
    if (SessionManager::getInstance().logout()) {
      return std::string("Logged Out Successfully");
    } else {
      throw AuthException("Can't log out if you're not logged in!");
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

  // COMUNICARE PRIN SOCKETPAIR
  std::string execute() override {
    if (!SessionManager::getInstance().isLoggedIn()) {
      throw AuthException("You must be logged in to use this command!");
    }

    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == -1) {  // Check FIRST!
      throw PipeException("Error creating socketpair");
    }

    int pid = fork();
    if (pid == -1) {
      close(sockets[0]);
      close(sockets[1]);
      throw ForkException("Error Forking");
    }

    // 0 - parinte
    // 1 - fiu
    if (pid == 0) {
      close(sockets[0]);

      std::string path = "/proc/" + process + "/status";
      std::ifstream file(path);
      if (!file.is_open()) {
        std::string err = "Cannot open " + path;
        write(sockets[1], err.c_str(), err.length());
        close(sockets[1]);
        _exit(1);
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

      vmsize = vmsize.empty() ? std::string("none") : vmsize;

      std::string output = "Name: " + name + " | State: " + state +
                           " | UID: " + uid + " | PPID: " + ppid +
                           " | VmSize: " + vmsize;

      write(sockets[1], output.c_str(), output.length());
      close(sockets[1]);
      _exit(0);
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
      close(sockets[0]);

      if (output.find("Cannot open ") != std::string::npos) {
        throw FileException(output.c_str());
      }

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
      throw InvalidCommandException("Invalid Command!");
    }
  }
};