#include "./jobs.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct job_element {
    int jid;
    pid_t pid;
    process_state_t state;
    char *command;
    struct job_element *next;
};
typedef struct job_element job_element_t;

// head is the head of the list
// current is the current element being iterated over
struct job_list {
    job_element_t *head;
    job_element_t *current;
    pid_t shell_pid;
};

/* initializes job list, returns pointer */
job_list_t *init_job_list() {
    job_list_t *job_list = (job_list_t *)malloc(sizeof(job_list_t));
    job_list->head = NULL;
    job_list->current = NULL;
    job_list->shell_pid = getpid();
    return job_list;
}

/*
 * cleans up jobs list
 * Note: this function will free the job_list pointer
 * DO NOT use the pointer after this function is called
 */
void cleanup_job_list(job_list_t *job_list) {
    if (job_list == NULL) {
        return;
    }

    job_element_t *cur = job_list->head;
    while (cur != NULL) {
        job_element_t *nextElement = cur->next;

        // if we are cleaning up the shell's job list and not a child's
        if (getpid() == job_list->shell_pid) {
            /* kill process */
            if (kill(-cur->pid, SIGKILL) < 0) {
                perror("kill");
            }
        }

        if (cur->command != NULL) {
            free(cur->command);
            cur->command = NULL;
        }

        free(cur);
        cur = nextElement;
    }

    job_list->head = NULL;
    job_list->current = NULL;
    job_list->shell_pid = 0;

    free(job_list);
}

/* adds new job to list, returns 0 on success, -1 on failure */
int add_job(job_list_t *job_list, int jid, pid_t pid, process_state_t state,
            char *command) {
    if (job_list == NULL || (state != RUNNING && state != STOPPED) ||
        command == NULL) {
        return -1;
    }

    job_element_t *new = (job_element_t *)malloc(sizeof(job_element_t));
    new->jid = jid;
    new->pid = pid;

    // allocate new char*'s and copy buffers in to protect our code
    new->state = state;

    size_t cmdlen = strlen(command);
    new->command = (char *)malloc(sizeof(char) * (cmdlen + 1));
    memcpy(new->command, command, cmdlen);
    new->command[cmdlen] = 0;
    new->next = NULL;

    if (job_list->head == NULL) {
        // add to head
        job_list->head = new;
        job_list->current = new;
    } else {
        // add to tail
        job_element_t *cur = job_list->head;
        while (cur->next != NULL) {
            cur = cur->next;
        }
        cur->next = new;
    }

    return 0;
}

/* removes job from list, given job's JID,
    returns 0 on success, -1 on failure */
int remove_job_jid(job_list_t *job_list, int jid) {
    if (job_list == NULL) {
        return -1;
    }

    job_element_t *prev = NULL;
    job_element_t *cur = job_list->head;
    while (cur != NULL) {
        if (cur->jid == jid) {
            if (prev != NULL) {
                prev->next = cur->next;
            }
            if (job_list->head == cur) {
                job_list->head = cur->next;
            }
            if (job_list->current == cur) {
                job_list->current = cur->next;
            }

            if (cur->command != NULL) {
                free(cur->command);
                cur->command = NULL;
            }

            free(cur);
            cur = NULL;

            return 0;
        }

        prev = cur;
        cur = cur->next;
    }

    return -1;
}

/* removes job from list, given job's PID,
    returns 0 on success, -1 on failure */
int remove_job_pid(job_list_t *job_list, pid_t pid) {
    if (job_list == NULL) {
        return -1;
    }

    job_element_t *prev = NULL;
    job_element_t *cur = job_list->head;
    while (cur != NULL) {
        if (cur->pid == pid) {
            if (prev != NULL) {
                prev->next = cur->next;
            }
            if (job_list->head == cur) {
                job_list->head = cur->next;
            }
            if (job_list->current == cur) {
                job_list->current = cur->next;
            }

            if (cur->command != NULL) {
                free(cur->command);
                cur->command = NULL;
            }
            free(cur);
            cur = NULL;

            return 0;
        }

        prev = cur;
        cur = cur->next;
    }

    return -1;
}

/* updates job's state, given job's JID, returns 0 on success, -1 on failure */
int update_job_jid(job_list_t *job_list, int jid, process_state_t state) {
    if (job_list == NULL) {
        return -1;
    }

    job_element_t *cur = job_list->head;
    while (cur != NULL) {
        if (cur->jid == jid) {
            cur->state = state;
            return 0;
        }

        cur = cur->next;
    }

    return -1;
}

/* updates job's state, given job's PID, returns 0 on success, -1 on failure */
int update_job_pid(job_list_t *job_list, pid_t pid, process_state_t state) {
    if (job_list == NULL) {
        return -1;
    }

    job_element_t *cur = job_list->head;
    while (cur != NULL) {
        if (cur->pid == pid) {
            cur->state = state;
            return 0;
        }

        cur = cur->next;
    }

    return -1;
}

/* gets PID of job, given job's JID, returns PID on success, -1 on failure */
pid_t get_job_pid(job_list_t *job_list, int jid) {
    if (job_list == NULL) {
        return -1;
    }

    job_element_t *cur = job_list->head;
    while (cur != NULL) {
        if (cur->jid == jid) {
            return cur->pid;
        }

        cur = cur->next;
    }

    return -1;
}

/* gets JID of job, given job's PID, returns JID on success, -1 on failure */
int get_job_jid(job_list_t *job_list, pid_t pid) {
    if (job_list == NULL) {
        return -1;
    }

    job_element_t *cur = job_list->head;
    while (cur != NULL) {
        if (cur->pid == pid) {
            return cur->jid;
        }

        cur = cur->next;
    }

    return -1;
}

/*
 * gets next PID in list
 * call this in a loop to get the PID of the next job in the list
 * returns the PID if there is one, -1 if the end of the list has been reached,
 * after which it will start at the head of the list again
 */
pid_t get_next_pid(job_list_t *job_list) {  // circular iterator
    if (job_list == NULL) {
        return -1;
    }

    if (job_list->current == NULL) {
        job_list->current = job_list->head;
        return -1;
    } else {
        pid_t pid = job_list->current->pid;
        job_list->current = job_list->current->next;
        return pid;
    }
}

/* jobs command, prints out the jobs list */
void jobs(job_list_t *job_list) {
    if (job_list == NULL) {
        return;
    }

    job_element_t *cur = job_list->head;
    while (cur != NULL) {
        char *state_string = cur->state == RUNNING ? "Running" : "Stopped";
        if (printf("[%d] (%d) %s %s\n", cur->jid, cur->pid, state_string,
                   cur->command) < 0) {
            fprintf(stderr, "error printing jobs list\n");
            cleanup_job_list(job_list);
            exit(1);
        }
        cur = cur->next;
    }
}
