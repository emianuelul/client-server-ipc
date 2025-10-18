#pragma once
#include "include_libs.h"

class SessionManager {
 private:
  std::string currUser;
  bool isLogged = false;

  SessionManager() {}

  bool validate(std::string user) {
    std::ifstream file("users.conf");

    if (!file.is_open()) {
      return false;
    }

    std::string line;
    while (std::getline(file, line)) {
      int colonPos = line.find(':');

      std::string fileUser = line.substr(0, colonPos);

      if (fileUser == user) {
        file.close();
        return true;
      }
    }

    file.close();
    return false;
  }

 public:
  static SessionManager& getInstance() {
    static SessionManager instance;
    return instance;
  }

  bool login(std::string user) {
    if (validate(user)) {
      isLogged = true;
      currUser = user;
      return true;
    }
    return false;
  }

  bool logout() {
    if (isLogged) {
      currUser.clear();
      isLogged = false;
      return true;
    }

    return true;
  }

  bool isLoggedIn() { return isLogged; }

  std::string getCurrentUser() { return currUser; }
};