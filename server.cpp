#include "headers/include_libs.h"

void cleanup() {
  unlink("./temp/server2client");
  unlink("./temp/client2server");
  rmdir("./temp");
}

int main(int argc, char** argv) {
  atexit(cleanup);

  mkdir("./temp", 0777);
  if (mkfifo("./temp/server2client", 0777) == -1) {
    std::cout << errno;
    exit(1);
  }
  if (mkfifo("./temp/client2server", 0777) == -1) {
    std::cout << errno;
    exit(1);
  }

  int c2s = open("./temp/client2server", O_RDONLY);
  int s2c = open("./temp/server2client", O_WRONLY);

  if (c2s == -1) {
    std::cout << "Eroare la deschiderea lui client2server in server\n";
    exit(2);
  }

  if (s2c == -1) {
    std::cout << "Eroare la deschiderea lui server2client in server\n";
    exit(2);
  }

  while (true) {
    std::string feedback = "Serverul a primit comanda: ";
    char clientBuffer[256];
    int bytes = read(c2s, &clientBuffer, sizeof(clientBuffer));

    if (bytes <= 0) {
      std::cout << "Clientul s-a deconectat.\n Closing Server...\n";
      break;
    }
    clientBuffer[bytes] = '\0';
    std::string comanda = clientBuffer;

    if (comanda == "login\n") {
      feedback = "Logging in...";
    } else {
      feedback += comanda;
    }

    write(s2c, feedback.c_str(), feedback.length());
  }

  close(c2s);
  close(s2c);

  return 0;
}