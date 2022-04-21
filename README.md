# Unix-Shell
Simplified Unix shell implemented in C that executes programs entered and offers a history, pipe, and input/output redirection.

This program implements a fork() system call to create a new process in which the new command is executed.

Inter-process communication (IPC) is demonstrated using the pipe (|) and redirect (< or >) commands,
where processes communicate between one another through the use of the open() or pipe() system calls and their associated file descriptors.
