#include "headers/include_libs.h"

int main(int argc, char** argv) {
  int c2s = open("./temp/client2server", O_WRONLY);
  int s2c = open("./temp/server2client", O_RDONLY);

  if (c2s == -1) {
    std::cout << "Eroare la deschiderea lui client2server in client\n";
    exit(2);
  }

  if (s2c == -1) {
    std::cout << "Eroare la deschiderea lui server2client in client\n";
    exit(2);
  }

  while (true) {
    std::string input;
    std::cout << "<CLIENT> Enter a command: ";
    std::getline(std::cin, input);

    input += '\n';

    write(c2s, input.c_str(), input.length());

    char serverBuffer[256];
    int bytes = read(s2c, &serverBuffer, sizeof(serverBuffer));

    if (bytes > 0) {
      serverBuffer[bytes] = '\0';
      // std::cout << serverBuffer << '\n';
    }
  }

  close(c2s);
  close(s2c);

  return 0;
}