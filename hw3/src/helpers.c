#include "helpers.h"
#include "linkedList.h"
#include "icssh.h"
#include <sys/types.h>
#include <unistd.h>


static void sio_reverse(char s[]) {
    int c, i, j;

    for (i = 0, j = sio_strlen(s)-1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void sio_error(char s[]) /* Put error message and exit */
{
    sio_puts(s);
    _exit(1);                                      //line:csapp:sioexit
}

static void sio_ltoa(long v, char s[], int b) 
{
    int c, i = 0;
    int neg = v < 0;

    if (neg)
	v = -v;

    do {  
        s[i++] = ((c = (v % b)) < 10)  ?  c + '0' : c - 10 + 'a';
    } while ((v /= b) > 0);

    if (neg)
	s[i++] = '-';

    s[i] = '\0';
    sio_reverse(s);
}

static size_t sio_strlen(char s[])
{
    int i = 0;

    while (s[i] != '\0')
        ++i;
    return i;
}

ssize_t sio_puts(char s[]) /* Put string */
{
    return write(STDOUT_FILENO, s, sio_strlen(s)); //line:csapp:siostrlen
}

ssize_t sio_putl(long v) /* Put long */
{
    char s[128];
    
    sio_ltoa(v, s, 10); /* Based on K&R itoa() */  //line:csapp:sioltoa
    return sio_puts(s);
}
    
ssize_t Sio_putl(long v)
{
    ssize_t n;
  
    if ((n = sio_putl(v)) < 0)
	sio_error("Sio_putl error");
    return n;
}

ssize_t Sio_puts(char s[])
{
    ssize_t n;
  
    if ((n = sio_puts(s)) < 0)
	sio_error("Sio_puts error");
    return n;
}

void freeAndNull(job_info* job, char * line) {
	free_job(job);
	job = NULL;
	free(line);
	line = NULL;
}

void changeDir(job_info *job) {
    			if (job->procs->argc == 1) {
				char* home = getenv("HOME");
				int pathCheck = chdir(home);
				if (pathCheck == 0) {
					char* cPath = getcwd(NULL, 0);
					printf("%s\n", cPath);
					free(cPath);
					cPath = NULL;
				} else {
					fprintf(stderr, DIR_ERR);
				}

			} else {
				int pathCheck = chdir((job->procs->argv)[1]);
				if (pathCheck == 0) {
					char* cPath = getcwd(NULL, 0);
					printf("%s\n", cPath);
					free(cPath);
					cPath = NULL;
				} else {
					fprintf(stderr, DIR_ERR);
				}
			}
}

int redirectionCheck(job_info*job) {
    if (job->in_file != NULL) {
        if(job->out_file != NULL) {
            if(strcmp(job->in_file, job->out_file) == 0) { // in == out
                return -1;
            }
        }
        if (job->procs->err_file != NULL) {
            if (strcmp(job->in_file, job->procs->err_file) == 0) { // in == err
                return -1;
            }
        }
    }
    if(job->out_file != NULL && job->procs->err_file != NULL) {
        if(strcmp(job->out_file, job->procs->err_file) == 0) { // out == err
            return -1;
        }
    }
    return 1;
}

int openIn(job_info* job, char* line) {
    int in = 0;
    int inSaved = 0;
    in = open(job->in_file, O_RDONLY, 0777);
    if (in == -1) {
        close(in);
        fprintf(stderr, RD_ERR);
        freeAndNull(job, line);
        return -1;
    } else {
        inSaved = dup(STDIN_FILENO);
        dup2(in, STDIN_FILENO);
        close(in);
    }
    
    return inSaved;
}

int openOut(job_info* job, char* line) {
    int out = 0;
    int outSaved = 0;
    out = open(job->out_file, O_CREAT | O_WRONLY, 0777);
    outSaved = dup(STDOUT_FILENO);
    // copy fd of out to STDOUT_FILENO
    dup2(out, STDOUT_FILENO);
    // close out fd (don't need anymore as STDOUT_FILENO is pointing to where out was)
    close(out);
    return outSaved;
}

int openErr(job_info* job, char* line) {
    int errOut = 0;
    int errSaved = 0;
    errOut = open(job->procs->err_file, O_CREAT | O_WRONLY, 0777);
    errSaved = dup(STDERR_FILENO);
    dup2(errOut, STDERR_FILENO);
    close(errOut);
    return errSaved;
}

void piping(job_info* job, char* line, List_t* bgList) {
    int fd[2];
    pid_t pid;
    int exec_result;
    int exit_status;
    pid_t wait_result;
    
    proc_info* proc = job->procs;

    sigset_t mask_all, mask_child, prev_mask;
	sigfillset(&mask_all);
	sigemptyset(&mask_child);
	sigaddset(&mask_child, SIGCHLD);

    int pipeReadEnd = STDIN_FILENO;
    int firstPipe = 1;

    while (proc != NULL) {
        if (pipe(fd) == -1) {
            exit(EXIT_FAILURE);
        }

        // block sigchild
		sigprocmask(SIG_BLOCK, &mask_child, &prev_mask);

        if ((pid = fork()) < 0) {
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            // unblock sigchild
			sigprocmask(SIG_SETMASK, &prev_mask, NULL);

            // pipeReadEnd initially set to STDIN_FILENO before start of while loop
            // this dup2 allows the output of the previous pipe to be connected to
            // the current pipe
            dup2(pipeReadEnd, STDIN_FILENO);

            // if there is another process then then we need to continue writing
            // to the pipe, otherwise we can just print the final output
            if (proc->next_proc != NULL) {
                dup2(fd[1], STDOUT_FILENO);
            }

            close(fd[0]);
            int exec_result = execvp(proc->cmd, proc->argv);

            if (exec_result < 0) {  //Error checking
                printf(EXEC_ERR, proc->cmd);
                freeAndNull(job, line);
                validate_input(NULL);
                exit(EXIT_FAILURE);
            }
            

        } else {
            if (firstPipe && job->bg) { // if job is a background process
                sigprocmask(SIG_BLOCK, &mask_all, NULL); // block all
                time_t receivedTime;
                bgentry_t* bgEnt = createBGEntry(job, pid, time(&receivedTime));
                insertInOrder(bgList, bgEnt);
				sigprocmask(SIG_SETMASK, &prev_mask, NULL); // set mask to prev (before block all)
                firstPipe = 0;

            } else {

                wait_result = waitpid(pid, &exit_status, 0);
                if (wait_result < 0) {
                    printf(WAIT_ERR);
                    exit(EXIT_FAILURE);
                }
            }
            // set mask to before blocking child
            sigprocmask(SIG_SETMASK, &prev_mask, NULL);

            // fd of read end of pipe is saved so that the next child in the loop
            // can use this for stdin
            pipeReadEnd = fd[0];
            close(fd[1]);
            proc = proc->next_proc;
        }

    }
}