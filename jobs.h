#ifndef JOBS_H_
#define JOBS_H_

#include <sys/types.h>
#include <unistd.h>

typedef enum { RUNNING, STOPPED } process_state_t;

typedef struct job_list job_list_t;

/* initializes job list, returns pointer */
job_list_t *init_job_list();
/*
 * cleans up jobs list
 * Note: this function will free the job_list pointer
 * DO NOT use the pointer after this function is called
 */
void cleanup_job_list(job_list_t *job_list);

/* adds new job to list, returns 0 on success, -1 on failure */
int add_job(job_list_t *job_list, int jid, pid_t pid, process_state_t state,
            char *command);

/* removes job from list, given job's JID,
        returns 0 on success, -1 on failure */
int remove_job_jid(job_list_t *job_list, int jid);
/* removes job from list, given job's PID,
        returns 0 on success, -1 on failure */
int remove_job_pid(job_list_t *job_list, pid_t pid);

/* updates job's state, given job's JID, returns 0 on success, -1 on failure */
int update_job_jid(job_list_t *job_list, int jid, process_state_t state);
/* updates job's state, given job's PID, returns 0 on success, -1 on failure */
int update_job_pid(job_list_t *job_list, pid_t pid, process_state_t state);

/* gets PID of job, given job's JID, returns PID on success, -1 on failure */
pid_t get_job_pid(job_list_t *job_list, int jid);
/* gets JID of job, given job's PID, returns JID on success, -1 on failure */
int get_job_jid(job_list_t *job_list, pid_t pid);

/*
 * gets next PID in list
 * call this in a loop to get the PID of the next job in the list
 * returns the PID if there is one, -1 if the end of the list has been reached,
 * after which it will start at the head of the list again
 */
pid_t get_next_pid(job_list_t *job_list);

/* jobs command, prints out the jobs list */
void jobs(job_list_t *job_list);

#endif  // JOBS_H_
