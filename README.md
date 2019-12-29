Instructions for use:
-
Once the user is in the directory containing the program, simply type into the
command line "make all" to compile sh.c. Afterwards, if a user wants to run the
shell with the prompt "33sh> ", simply type into the command line "./33sh".
Otherwise, type into the command line "./33noprompt". Regardless of the target,
the shell will initiate waiting for a user's input to be executed.

Overview of design:
-
parse:
parse is responsible for preprocessing all of the necessary components from the
command line: tokens, input path, and output path. The standard input is
tokenized using strtok with delimiters only pertaining to white space, tabs, and
new lines, in which each token is stored in a temporary array. Then, in a while
loop, each token is checked if any redirection operators are present. If they
are, then the function will store either the correct input or output path
(depending on the redirection operator). A variable called 'sign' is also set to
">" if the ">" operator was present or ">>" if the ">>" operator was present.
If an error is thrown during these conditional processes, parse returns -1. Note
that if any redirection operators are present, they along with their corresponding
paths will be skipped over when setting the final array of tokens. All of the
remaining tokens are then stored in an array called 'tokens'. If parse runs
without throwing any errors, it returns 0.

built_in:
built_in is responsible for handling any shell built-in commands. In a shell,
a built-in command is only executed if it is at the beginning of the command
line. Thus, the function checks if any of the commands (cd, ln, rm, or exit) is
present in the first element of the array of tokens (tokens[0]). If it exits,
then the program checks if the corresponding path(s) are valid. If the path(s)
are not valid, an error is thrown and built_in returns -1. Otherwise, the
built-in is executed correctly and built_in returns 0. This function is also
responsible for handling the fg, bg, and jobs commands. It checks if any of
the commands is present in the first element of the array of tokens (tokens[0]).
If either fg or bg is present, then the job ID is parsed and the process ID is
found for both cases to be used to send the signal SIGCONT. If the function is
moving a process into the foreground, built_in will ensure that terminal sends
signals to those processes, reap any terminated processes with 'waitpid', and
call reap_print. If jobs is present, then the function simply calls 'jobs' from
jobs.c.

redir:
redir is responsible for opening the necessary files. If neither the input path
or output path is present, then redir returns 0. If the input path is present,
then the function closes the stdin, and opens the file corresponding to the
input path. If an error occurs during this process, redir returns -1. If the
output path is present, then the function closes stdout. 'sign' is then checked
whether ">" or ">>" is present, which corresponds to different executions for
those redirections. If ">" is present, then the file corresponding to the output
path is opened given that the contents can be truncated. If ">>" is present,
then the file corresponding to the output path is opened given that the contents
can be appended. If an error occurs during either process, redir returns -1.

execute:
execute is responsible for executing any programs outlined in the command line.
In order to provide the necessary arguments to execv, execute preprocesses the
file name and the array of arguments. The file name is parsed from the first
element in the array of tokens using a "/" delimiter. These tokens are stored
in an array called 'program_tokens' and the last element in that array is stored
in a variable called 'program_name' (which is the file name). The function then
stores program_name in the first element of an array called 'argv'. It then
preprocesses rest of the tokens by storing them consecutively in 'argv'. The last
element in 'argv' is set to NULL. Then, execute creates a new process with fork,
calls on redir to ensure that the input/output redirecting has been executed
correctly, and then calls on execv with the arguments being 'program_name' and
'argv'. If any error occurs during the execv process, the function exits.
The function is also responsible for executing foreground and background
processes differently. Only if the child process is in the foreground, then
execute will ensures that terminal sends signals to that process. If the child
process is in the background (denoted by '&' in the command line) the function
will execute the program and add it to the job list. Otherwise, the child process
is in the foreground, so the function will reap any terminated process with
'waitpid' and will evaluate the macros for signal termination and signal stoppage.
If a process was suspended by a signal, then it will be added to the job list.

reap_print:
reap_print is responsible cleaning up the process table and printing the correct
messages for the macros pertaining to normal termination, signal termination,
and signal stoppage. Corresponding job functions are called to clean and maintain
the job list. An argument called 'job_type' is also used conditionally to print
certain error messages depending on foreground or background processes.

main:
main is responsible for initializing the buffer, reading in the standard input
from terminal, and calling on the necessary functions. A while loop is set to
run the shell indefinitely until the user exits or unless an error is thrown
while reading in the standard input. The array of tokens, input path, output
path, and sign (for indicating the presence of ">" or ">>") are all initialized
in main. Also there is redirection error checking prior to any execution of
the built-ins or programs.

Other Functionality:
-
Signals are handled in main and execute. In main, the response to the signals
outlined in the PDF are set to be ignored in the shell to avoid having the shell
exit. In execute, the response to the same set of signals are reset back to their
default behavior so that child processes can accept and react on signals.b
