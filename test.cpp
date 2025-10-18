#include <utmpx.h>
#include <ctime>
#include <iostream>

int main() {
  setutxent();
  utmpxname("/var/log/wtmp");  // historical logins

  struct utmpx* entry;

  while ((entry = getutxent()) != nullptr) {
    if (entry->ut_type == USER_PROCESS) {
      std::time_t t = entry->ut_tv.tv_sec;
      std::cout << "User: "
                << (entry->ut_user[0] ? entry->ut_user : "(unknown)")
                << " | Host: "
                << (entry->ut_host[0] ? entry->ut_host : "(local)")
                << " | Time: " << std::ctime(&t);
    }
  }

  endutxent();
}
