# client-server-ipc

Project done as homework for a Uni Subject.
Client App 'speaks' to the Server through a FIFO file, same goes the other way.

# How To Use

- ## Shell Scripts:

  - I've written 2 shell scripts:
    - `./shell_scripts/compile_apps.sh` to speed up compiling, compiles both client.cpp and server.cpp at the same time
    - `./shell_scripts/run_app.sh` -> _USE THIS TO RUN THE APP_

- ## Commands:

  - `login` - Input username and password
  - `get-logged-users` - prints out the users -_Can't be used if you're not logged in_
  - `get-proc-info : <pid>` - prints out information about the specified process id - _Can't be used if you're not logged in_
  - `logout` - Logs the user out of the current session - _Can't be used if you're not logged in_
  - `quit` - Closes the app (including the server)
