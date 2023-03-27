#include "icssh.h"
#include "linkedList.h"
#include "helpers.h"
#include <readline/readline.h>
#include <signal.h>
#include <stdio.h>

int killChildFlag = 0;

void sigchild_handler(int status) {
    killChildFlag = 1;
}

void sigusr2_handler(int status) {
	Sio_puts("Hi User! I am process ");
	Sio_putl((long)getpid());
	Sio_puts("\n");
}

int main(int argc, char* argv[]) {
	char* line;
	int exec_result;
	int exit_status;
	pid_t pid;
	pid_t wait_result;
	time_t receivedTime;
	sigset_t mask_all, mask_child, prev_mask;

	int inSaved = STDIN_FILENO;
	int outSaved = STDOUT_FILENO;
	int errSaved = STDERR_FILENO;

	sigfillset(&mask_all);
	sigemptyset(&mask_child);
	sigaddset(&mask_child, SIGCHLD);
#ifdef GS
    rl_outstream = fopen("/dev/null", "w");
#endif

	// Setup segmentation fault handler
	if (signal(SIGSEGV, sigsegv_handler) == SIG_ERR) {
		perror("Failed to set signal handler");
		exit(EXIT_FAILURE);
	}

	if (signal(SIGCHLD, sigchild_handler) == SIG_ERR) {
		perror("Failed to install sigchild handler");
		exit(EXIT_FAILURE);
	}

	if (signal(SIGUSR2, sigusr2_handler) == SIG_ERR) {
		perror("Failed to install sigusr2 handler");
		exit(EXIT_FAILURE);
	}

	//create list for background processes
	List_t* bgList = createList(&bgentryComparator);


    // print the prompt & wait for the user to enter commands string
	while ((line = readline(SHELL_PROMPT)) != NULL) {

		// remove terminated processes from list if flag is set
		if (killChildFlag) {
			// kill only terminated bg processes
			while((pid = waitpid(-1, &exit_status, WNOHANG)) > 0) {
				removeByPID(bgList, pid);
			}
			killChildFlag = 0;
		}

		time(&receivedTime);

        // MAGIC HAPPENS! Command string is parsed into a job struct
        // Will print out error message if command string is invalid
		job_info* job = validate_input(line);
        if (job == NULL) { // Command was empty string or invalid
			free(line);
			line = NULL;
			continue;
		}

        //Prints out the job linked list struture for debugging
        #ifdef DEBUG   // If DEBUG flag removed in makefile, this will not longer print
            debug_print_job(job);
        #endif

		
		// exit shell
		if (strcmp(job->procs->cmd, "exit") == 0) {
			deleteList(&bgList);
			free(bgList);
			bgList = NULL;
			//Terminating the shell
			freeAndNull(job, line);
            validate_input(NULL);   // calling validate_input with NULL will free the memory it has allocated
            return 0;
		}
		
		// Change working directory
		if (strcmp(job->procs->cmd, "cd") == 0) {
			// change directorory
			changeDir(job);
			freeAndNull(job, line);
			continue;
		}

		// print last childs exit status
		if (strcmp(job->procs->cmd, "estatus") == 0) {
			printf("%i\n", WEXITSTATUS(exit_status));
			freeAndNull(job, line);
			continue;
		}

		// print last childs exit status
		if (strcmp(job->procs->cmd, "ascii53") == 0) {
			printAscii();
			freeAndNull(job, line);
			continue;
		}

		// print list of background processes
		if (strcmp(job->procs->cmd, "bglist") == 0) {
			printList(bgList, STR_MODE);
			freeAndNull(job, line);
			continue;
		}

		// Execute piping
		if (job->nproc > 1) {
			piping(job, line, bgList);
			if(!job->bg){
				free_job(job);
				job = NULL;
			}
			free(line);
			line = NULL;
			continue;
		}

		// block sigchild
		sigprocmask(SIG_BLOCK, &mask_child, &prev_mask);

		if ((pid = fork()) < 0) {
			exit(EXIT_FAILURE);
		}
		if (pid == 0) {
			// unblock sigchild
			sigprocmask(SIG_SETMASK, &prev_mask, NULL);

			// error checking for file redirection
			if (redirectionCheck(job) == -1) {
				fprintf(stderr, RD_ERR);
                freeAndNull(job, line);
				exit(EXIT_FAILURE);
			}

			// perform file redirection
			if(job->in_file != NULL) {
				if((inSaved = openIn(job, line)) == -1) {
					validate_input(NULL);
					exit(EXIT_FAILURE);
				}
			}

			if (job->out_file != NULL) {
				outSaved = openOut(job, line);
			}

			if (job->procs->err_file != NULL) {
				errSaved = openErr(job, line);
			}

            // get the first command in the job list
		    proc_info* proc = job->procs;
			exec_result = execvp(proc->cmd, proc->argv);

			if (exec_result < 0) {  //Error checking
				printf(EXEC_ERR, proc->cmd);
				
				// Cleaning up to make Valgrind happy 
				// (not necessary because child will exit. Resources will be reaped by parent)
				freeAndNull(job, line);
				validate_input(NULL);  // calling validate_input with NULL will free the memory it has allocated
				exit(EXIT_FAILURE);
			}
		} else {
			
			if (job->bg) { // if job is a background process
				sigprocmask(SIG_BLOCK, &mask_all, NULL);
				bgentry_t* bgEnt = createBGEntry(job, pid, receivedTime);
				insertInOrder(bgList, bgEnt);
				sigprocmask(SIG_SETMASK, &prev_mask, NULL);
				
			} else {

				// As the parent, wait for the foreground job to finish
				wait_result = waitpid(pid, &exit_status, 0);

				if (wait_result < 0) {
					printf(WAIT_ERR);
					exit(EXIT_FAILURE);
				}
			}
			sigprocmask(SIG_SETMASK, &prev_mask, NULL);
		}
		
		// reset STDOUT_FILENO to what is was pointing at before
		if (job->out_file != NULL) {
			fflush(stdout);
			dup2(outSaved, STDOUT_FILENO);
		}
		
		if(job->procs->err_file != NULL) {
			fflush(stderr);
			dup2(errSaved, STDERR_FILENO);
		}

		if (job->in_file != NULL) {
			fflush(stdin);
			dup2(inSaved, STDIN_FILENO);
		}
		
		// if a foreground job, we no longer need the data
		if(!job->bg){
			free_job(job);
			job = NULL;
		}
		free(line);
		line = NULL;
	}

    // calling validate_input with NULL will free the memory it has allocated
    validate_input(NULL);

#ifndef GS
	fclose(rl_outstream);
#endif
	return 0;
}




