/*
 * Name: River Veek
 * Date: 12 November 2020
 * Part: 2 
 * 
 * Notes:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>


void send_sig(pid_t *pool, int size, int sig) {
	// loop through each child in pool
	for(int i = 0; i < size; i++) {
		// 	current pid
		// 	current signal
		// 	child from pool
		// sleep(2);  // for dramatic effect
		printf("Parent process: %d - Sending signal: %d to child process: %d\n",
					getpid(), sig, pool[i]);
		kill(pool[i], sig);  // send sig to each child in pool
	}
}

int main(int argc, char *argv[]) {
	
	// locals
	FILE *in = fopen(argv[1], "r");  // open file
	char *line_buff = NULL;  // will be alloc by getline()
	size_t len = 0;  // num bytes to alloc by getline()
	ssize_t num_read;  // num bytes read by getline()
	char *tok;  // for use by strtok_r()
	char *save1;  // for use by strtok_r()
	int num_lines = 0;  // for use in getting num lines in file
	char c;  // for use in getting num lines in file
	int i = 0, k = 0;  // for use in main loop, commands array
	int status;  // for use in waiting for threads to end
	int num_args;  // for use in commands arr of str

	// add SIGUSR1 to process mask
	int sig;
	sigset_t sigset, oldset;  // set to add signals to and old set (for sigprocmask)
	sigemptyset(&sigset);  // initializes set
	sigaddset(&sigset, SIGUSR1);  // add SIGUSR1 to set
	sigprocmask(SIG_BLOCK, &sigset, &oldset);  // block mask (block means opposite here)	
		
	// get len of file
	while((c = getc(in)) != EOF) {
		if(c == '\n') {
			num_lines++;
		}
	}
	// reset fp
	fseek(in, 0, SEEK_SET);
	
	// alloc mem for pool of processes (one per line in file)
	pid_t *pids = malloc(sizeof(pid_t) * num_lines);  // pool of child processes
	
	// get line from in file
	// fork new child process
	// exec command from line
	while((num_read = getline(&line_buff, &len, in)) != -1) {
		// printf("Line=%s, Length=%zd\n", line_buff, num_read);  //TEST	
		
		// get number of args
		num_args = 0;
		for(int j = 0; j < strlen(line_buff); j++) {
			if(line_buff[j] == ' ') {
				num_args++;
			}
		}
		num_args++;  // num args == num spaces + 1

		// alloc mem to hold commands (to be passed to execvp)	
		char *commands[num_args + 1];  // uninit data, freed automatically, add one for NULL arg
		
		// tokenize line
		k = 0;
		// i was stuck on this fucking bug for 2 hours straight
		// it is currently 5:09am and i am tired and sad
		char *cpy = line_buff;  // need to make a COPY for some god damned reason
		while((tok = strtok_r(cpy, " \n", &save1)) != NULL) {
			// printf("token: %s\n", tok);  //TEST
			commands[k] = tok;
			// printf("%s\n", commands[k]);  //TEST
			cpy = NULL;  // reset cpy (else error)	
			k++;
		}
		commands[num_args] = NULL;  // set last arg to NULL (for use in execvp)	
		
		// exec processes
		pids[i] = fork();
		// handle errors with forking	
		if(pids[i] < 0) {
			perror("ERROR: ");
			exit(EXIT_FAILURE);
		}
		// if process is chlid, exec process
		if(pids[i] == 0) {
			// printf("EXEC...\n");  //TEST
			// wait for SIGUSR1 before calling execvp()
			sigwait(&sigset, &sig);  // wait for signals in sigset
			int info = execvp(commands[0], commands);
			if(info == -1) {
				free(pids);
				free(line_buff);
				fclose(in);
			}
			perror("ERROR: ");
			exit(0);
		}
		
		i++;
	}
	
	// now that all children waiting, send SIGUSR1 to all 
	send_sig(pids, num_lines, SIGUSR1);

	// now that all children exec, send SIGSTOP to all
	send_sig(pids, num_lines, SIGSTOP);

	// now that all children sus, send SIGCONT to all
	send_sig(pids, num_lines, SIGCONT);

	// wait for threads to close
	for(int j = 0; j < num_lines; j++) {
		waitpid(pids[j], &status, WUNTRACED | WCONTINUED); 
	}

	// closing remarks
	free(pids);
	free(line_buff);
	fclose(in);
	
	// NO MEM LEAKAGE THUS FAR *********************
}
