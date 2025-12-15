# myshell

Julio Roman Quian Bellos

CS 12600 Systems Programming In C

Fall 2025



Dependencies

- Linux (tested on Debian)

- gcc

- make

Features

- External commands via fork() + execvp()

- Foreground and background execution

- I/O redirection: > >> <

- Single pipe: cmd1 | cmd2

- Built-ins: cd, exit, quit

- Signal handling:

- Command logging to myshell.log using open()+snprintf()+write()

Run

- ./myshell

- ./myshell < tests/t01_basic.in

Known limitations

- Only one pipe supported

- No quotes/escaping

Sample Commands

- ls -l

- grep main src/main.c

- echo hello > out.txt

- cat < out.txt

- sleep 3 &

- ls | wc -l

- cd

- exit


## Build
```bash
make
