#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include "./jobs.h"

#define BUFSIZE 1024
#define FOREGROUND 0
#define BACKGROUND 1
#define STDIN 0
#define STDOU 1

// initialization of job list and counter
job_list_t *job_list;
int jid_count = 1;

/*
 * sys_error
 *
 * description: preprocesses an array of tokens, the input path, the output
 *              path, and a sign (if any) determined by ">" or ">>"
 *
 * input: message - a pointer to a char naming the system call
 */
void sys_error(char *message) {
   perror(message);
   cleanup_job_list(job_list);
   exit(1);
 }

/*
 * error_check
 *
 * description: preprocesses an array of tokens, the input path, the output
 *              path, and a sign (if any) determined by ">" or ">>"
 *
 * input: message - a pointer to a char naming the job function call
 */
void job_error(char *message) {
    fprintf(stderr, "%s failed\n", message);
    cleanup_job_list(job_list);
    exit(1);
}

/*
 * parse
 *
 * description: preprocesses an array of tokens, the input path, the output
 *              path, and a sign (if any) determined by ">" or ">>"
 *
 * input: *str - a pointer to a null-terminated character string to break up
 *        **tokens - a pointer to an array of pointers to the tokens
 *        **i_path - a pointer to a pointer to the input path
 *        **o_path - a pointer to a pointer to the output path
 *        **sign - a pointer to a pointer to a char, indicating whether
 *                 ">" or ">>" was written in the command line
 *
 * return: -1 if an error occurred, 0 otherwise
 */
int parse(char *str, char **tokens, char **i_path, char **o_path, char **sign) {
    int TOKSIZ = (int)(strlen(str) / 2);
    char *temp[TOKSIZ];
    char *tok_str = strtok(str, " \t\n");
    temp[0] = tok_str;
    int count = 1;
    while (tok_str != NULL) {
        tok_str = strtok(NULL, " \t\n");
        temp[count] = tok_str;
        count++;
    }
    // parsing the redirection tokens
    count = 0;
    int i = 0;
    while (temp[count] != NULL) {
        // input preprocessing and error checking
        if (strcmp(temp[count], "<") == 0) {
            *i_path = temp[count + 1];
            if (*i_path == NULL) {
                fprintf(stderr, "syntax error: no input file\n");
                return -1;
            }
            if (strcmp(*i_path, "<") == 0) {
                fprintf(stderr,
                        "syntax error: input file is a redirection symbol\n");
                return -1;
            }
            if (temp[count + 2] == NULL) {
                break;
            }
            if (strcmp(temp[count + 2], "<") == 0) {
                fprintf(stderr, "syntax error: multiple input files\n");
                return -1;
            }
            count += 2;
            continue;
        }
        // output preprocessing and error checking
        if (strcmp(temp[count], ">") == 0) {
            *o_path = temp[count + 1];
            *sign = ">";
            if (*o_path == NULL) {
                fprintf(stderr, "syntax error: no output file\n");
                return -1;
            }
            if (strcmp(*o_path, ">") == 0) {
                fprintf(stderr,
                        "syntax error: output file is a redirection symbol\n");
                return -1;
            }
            if (temp[count + 2] == NULL) {
                break;
            }
            if (strcmp(temp[count + 2], ">") == 0) {
                fprintf(stderr, "syntax error: multiple output files\n");
                return -1;
            }
            count += 2;
            continue;
        }
        // output (append) preprocessing and error checking
        if (strcmp(temp[count], ">>") == 0) {
            *o_path = temp[count + 1];
            *sign = ">>";
            if (*o_path == NULL) {
                fprintf(stderr, "syntax error: no output file\n");
                return -1;
            }
            if (strcmp(*o_path, ">>") == 0) {
                fprintf(stderr,
                        "syntax error: output file is a redirection symbol\n");
                return -1;
            }
            if (temp[count + 2] == NULL) {
                break;
            }
            if (strcmp(temp[count + 2], ">>") == 0) {
                fprintf(stderr, "syntax error: multiple output files\n");
                return -1;
            }
            count += 2;
            continue;
        }
        tokens[i] = temp[count];
        count++;
        i++;
    }
    return 0;
}

/*
 * reap_print
 *
 * description: cleans up jobs list and prints correct messages
 *
 * input: status - the status
 *        jid - the job ID
 *        pid - the process ID
 *        job_type - either foreground/background
 */
void reap_print(int status, int jid, pid_t pid, int job_type) {
    if (WIFEXITED(status)) {
        if (job_type == BACKGROUND) {
            fprintf(stdout, "[%d] (%d) terminated with exit status %d\n", jid,
                    pid, WEXITSTATUS(status));
        }
        if (remove_job_pid(job_list, pid) == -1) {
            job_error("remove_job");
        }
    } else if (WIFSIGNALED(status)) {
        fprintf(stdout, "[%d] (%d) terminated by signal %d\n", jid, pid,
                WTERMSIG(status));
        if (remove_job_pid(job_list, pid) == -1) {
            job_error("remove_job");
        }
    } else if (WIFSTOPPED(status)) {
        fprintf(stdout, "[%d] (%d) suspended by signal %d\n", jid, pid,
                WSTOPSIG(status));
        if (update_job_pid(job_list, pid, STOPPED) == -1) {
            job_error("update_job");
        }
    }
}

/*
 * built_in
 *
 * description: executes the shell built-in commands
 *
 * input: **tokens - a pointer to an array of pointers to the tokens
 *
 * return: -1 if a built-in command were executed, 0 otherwise
 */
int built_in(char **tokens) {
    if (tokens[0] == NULL) {
        return -1;
    }
    if (strcmp(tokens[0], "cd") == 0) {
        char *path = tokens[1];
        int c = chdir(path);
        if (c == -1) {
            fprintf(stderr, "cd: syntax error\n");
        }
        return -1;
    }
    if (strcmp(tokens[0], "ln") == 0) {
        char *existing = tokens[1];
        char *new = tokens[2];
        int l = link(existing, new);
        if (l == -1) {
            fprintf(stderr, "ln: syntax error\n");
        }
        return -1;
    }
    if (strcmp(tokens[0], "rm") == 0) {
        const char *path = tokens[1];
        int u = unlink(path);
        if (u == -1) {
            fprintf(stderr, "rm: syntax error\n");
        }
        return -1;
    }
    if (strcmp(tokens[0], "bg") == 0 || strcmp(tokens[0], "fg") == 0) {
        int job_type = BACKGROUND;
        if (strcmp(tokens[0], "fg") == 0) {
            job_type = FOREGROUND;
        }
        if (job_type == FOREGROUND) {
            if (tokens[1] == NULL) {
                fprintf(stderr, "fg: syntax error\n");
                return -1;
            }
            if (tokens[1][0] != '%') {
                fprintf(stderr, "fg: job input does not begin with %%\n");
                return -1;
            }
        } else {
            if (tokens[1] == NULL) {
                fprintf(stderr, "bg: syntax error\n");
                return -1;
            }
            if (tokens[1][0] != '%') {
                fprintf(stderr, "bg: job input does not begin with %%\n");
                return -1;
            }
        }
        int jid = atoi(tokens[1] + 1);
        pid_t pid;
        if ((pid = get_job_pid(job_list, jid)) == -1) {
            fprintf(stderr, "job not found\n");
            return -1;
        } else {
            if (job_type == FOREGROUND) {
                if (tcsetpgrp(STDIN, pid) == -1) {
                    sys_error("tcsetpgrp");
                }
                if (kill(-pid, SIGCONT) == -1) {
                    sys_error("kill error");
                }
                int status;
                int ret = waitpid(pid, &status, WUNTRACED);
                if (ret > -1) {
                    reap_print(status, jid, pid, FOREGROUND);
                } else if (ret == -1) {
                    fprintf(stderr, "waitpid error\n");
                    return -1;
                }
                if (tcsetpgrp(STDIN, getpgrp()) == -1) {
                    sys_error("tcsetgrp");
                }
            } else {
                if (kill(-pid, SIGCONT) == -1) {
                    sys_error("kill_error");
                }
            }
        }
        return -1;
    }
    if (strcmp(tokens[0], "jobs") == 0) {
        jobs(job_list);
        return -1;
    }
    if (strncmp(tokens[0], "exit", 4) == 0) {
        cleanup_job_list(job_list);
        exit(0);
    }
    return 0;
}

/*
 * redir
 *
 * description: opens the necessary files for input/output redirections
 *
 * input: *i_path - a pointer to an input path
 *        *o_path - a pointer to an output path
 *        *sign - pointer to a char, indicating whether ">" or ">>" was
 *                written in the command line
 *
 * return: -1 if a built-in command were executed, 0 otherwise
 */
int redir(char *i_path, char *o_path, char *sign) {
    // check if input path exists
    if (i_path != NULL) {
        if (close(STDIN) == -1) {
            perror("close");
            return -1;
        }
        if (open(i_path, O_RDONLY, 0400) == -1) {
            perror("open");
            return -1;
        }
    }
    // check if output path exists
    if (o_path != NULL) {
        if (close(STDOU) == -1) {
            perror("close");
            return -1;
        };
        // ">" operator present
        if (strcmp(sign, ">") == 0) {
            if (open(o_path, O_RDWR | O_CREAT | O_TRUNC, 0600) == -1) {
                perror("open");
                return -1;
            }
        }
        //  ">>" operator present
        if (strcmp(sign, ">>") == 0) {
            if (open(o_path, O_RDWR | O_CREAT | O_APPEND, 0600) == -1) {
                perror("open");
                return -1;
            }
        }
    }
    return 0;
}

/*
 * execute
 *
 * description: executes a specified program
 *
 * input: **tokens - a pointer to an array of pointers to the tokens
 *        *i_path - a pointer to the input path
 *        *o_path - a pointer to the output path
 *        *sign - a pointer to a char, indicating whether ">" or ">>"
 *                was written in the command line
 */
void execute(char **tokens, char *i_path, char *o_path, char *sign) {
    // preprocessing path
    char *program_tokens[512];
    char path[512];
    strcpy(path, tokens[0]);
    char *cur_str = strtok(tokens[0], "/");
    program_tokens[0] = cur_str;
    int count = 1;
    while (cur_str != NULL) {
        cur_str = strtok(NULL, "/");
        program_tokens[count] = cur_str;
        count++;
    }
    char *program_name = program_tokens[count - 2];
    // preprocessing the arguments array
    char *argv[512];
    argv[0] = program_name;
    int arg_count = 1;
    while (tokens[arg_count] != NULL) {
        argv[arg_count] = tokens[arg_count];
        arg_count++;
    }
    argv[arg_count] = NULL;
    // checking foreground/background
    int job_type = FOREGROUND;
    if (*argv[arg_count - 1] == '&') {
        job_type = BACKGROUND;
        argv[arg_count - 1] = NULL;
    }
    // execution of program
    int status;
    pid_t child_pid;
    if (!(child_pid = fork())) {
        // setting process group ID
        if (setpgid(child_pid, child_pid) == -1) {
            sys_error("setpgid");
        }
        // resetting the process group
        if (job_type == FOREGROUND) {
            if (tcsetpgrp(STDIN, getpid()) == -1) {
                sys_error("tcsetpgrp");
            }
        }
        // signal handling in child process
        if (signal(SIGINT, SIG_DFL) == SIG_ERR) {
            sys_error("signal");
        }
        if (signal(SIGTSTP, SIG_DFL) == SIG_ERR) {
            sys_error("signal");
        }
        if (signal(SIGQUIT, SIG_DFL) == SIG_ERR) {
            sys_error("signal");
        }
        if (signal(SIGTTOU, SIG_DFL) == SIG_ERR) {
            sys_error("signal");
        }
        // redirection call
        if (redir(i_path, o_path, sign) == -1) {
            exit(1);
        }
        execv(path, argv);
        perror("execv");
        exit(1);
    }
    if (child_pid == -1) {
        sys_error("fork");
    }
    if (job_type == BACKGROUND) {
        if (add_job(job_list, jid_count, child_pid, RUNNING, path) == -1) {
            job_error("add_job");
        } else {
            fprintf(stdout, "[%d] (%d)\n", jid_count, child_pid);
            jid_count++;
        }
    } else {
        int ret = waitpid(child_pid, &status, WUNTRACED);
        if (ret > -1) {
            if (WIFSIGNALED(status)) {
                fprintf(stdout, "[%d] (%d) terminated by signal %d\n",
                        jid_count, child_pid, WTERMSIG(status));
                jid_count++;
            } else if (WIFSTOPPED(status)) {
                fprintf(stdout, "[%d] (%d) suspended by signal %d\n", jid_count,
                        child_pid, WSTOPSIG(status));
                if (add_job(job_list, jid_count, child_pid, STOPPED, path) ==
                    -1) {
                    job_error("add_job");
                } else {
                    jid_count++;
                }
            }
        } else if (ret == -1) {
            fprintf(stderr, "waitpid error\n");
            return;
        }
        if (tcsetpgrp(STDIN, getpid()) == -1) {
            sys_error("tcsetpgrp");
        }
    }
}

int main() {
    // initialize job_list
    job_list = init_job_list();
    // signal handling in shell
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        sys_error("signal");
    }
    if (signal(SIGTSTP, SIG_IGN) == SIG_ERR) {
        sys_error("signal");
    }
    if (signal(SIGQUIT, SIG_IGN) == SIG_ERR) {
        sys_error("signal");
    }
    if (signal(SIGTTOU, SIG_IGN) == SIG_ERR) {
        sys_error("signal");
    }
    while (1) {
        // initialize buffer
        char buffer[BUFSIZE];
        memset(buffer, 0, BUFSIZE);
        // reaping jobs in background
        int status;
        pid_t pid;
        while ((pid = get_next_pid(job_list)) != -1) {
            int ret = waitpid(pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
            if (ret > 0) {
                int jid = get_job_jid(job_list, pid);
                if (jid == -1) {
                    job_error("get_job");
                }
                reap_print(status, jid, pid, BACKGROUND);
                if (WIFCONTINUED(status)) {
                    fprintf(stdout, "[%d] (%d) resumed\n", jid, pid);
                    if (update_job_pid(job_list, pid, RUNNING) == -1) {
                        job_error("update_job");
                    }
                }
            } else if (ret == -1) {
                fprintf(stderr, "waitpid error\n");
                continue;
            }
        }
#ifdef PROMPT
        if (printf("33sh> ") < 0) {
            fprintf(stderr, "error in print function");
        }
#endif
        fflush(stdout);
        // read in standard input
        ssize_t read_int = read(0, buffer, BUFSIZE);
        if (read_int == 0) {
            cleanup_job_list(job_list);
            return 0;
        }
        if (read_int == 1) {
            continue;
        }
        if (read_int > 1) {
            // parse command line
            char *i_path = NULL;
            char *o_path = NULL;
            char *sign = NULL;
            char *tokens[512] = {NULL};
            int valid = parse(buffer, tokens, &i_path, &o_path, &sign);
            if (valid == -1) {
                continue;
            }
            // redirection error checking
            if (i_path != NULL && *tokens == NULL) {
                fprintf(stderr, "error: redirects with no command\n");
                continue;
            }
            if (o_path != NULL && *tokens == NULL) {
                fprintf(stderr, "error: redirects with no command\n");
                continue;
            }
            // execute built-ins/program
            if (built_in(tokens) == 0) {
                // execute a program
                execute(tokens, i_path, o_path, sign);
            }
        }
    }
    return 0;
}
