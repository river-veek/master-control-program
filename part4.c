/*
 * Name: River Veek
 * Date: 12 November 2020
 * Part: 4 
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


// globals
pid_t *pids;  // for use by various funcs
int num_lines = 0;  // equiv to num processes
int status;  // for use in waitpid()
int *states;  // holds states, corresponds to pids arr
int next = 0;  // next proc (from pids) to start, for use in sig_handler()
int last = 0;  // last proc to get time to run, for use in sig_handler()

void send_sig(pid_t *pool, int size, int sig) {
	// loop through each child in pool
	for(int i = 0; i < size; i++) {
		// 	current pid
		// 	current signal
		// 	child from pool
		// sleep(2);  // optional, for dramatic effect
		printf("Parent process: %d - Sending signal: %d to child process: %d\n",
					getpid(), sig, pool[i]);
		kill(pool[i], sig);  // send sig to each child in pool
	}
}

void sig_handler(int signum) {
	printf("Inside handler...\n");
	// all proc should be stopped already when \
	// 	this func is first called
	// run each proc for 2 seconds
	
	// for(int i = 0; i < num_lines; i++) {printf("states[%d]=%d\n", i, states[i]);} //TEST
	
	// suspend exec of calling process, until told otherwise
	waitpid(pids[last], &status, WUNTRACED | WCONTINUED | WNOHANG);
	if(WIFEXITED(status)) {
		printf("Process %d exited\n", pids[last]);
		states[last] = 1;  // change state from running to done
	}	
	
	// stop prev proc
	printf("-----------------------\n");	
	printf("Stopping proc %d...\n", pids[last]);
	// printf("Starting proc %d...\n", pids[next]);
	
	if(states[next] != 1) {
		printf("Starting proc %d...\n", pids[next]);
		// get info from /proc/<pid>/stat
		char buff[100];  // file pathway held here
		snprintf(buff, 16, "/proc/%d/stat\n", pids[next]);  // create file pathway
		// printf("%s\n", buff);  //TEST
		
		// create vars to hold data
		FILE *proc = fopen(buff, "r");  // open new file pathway
		int pid;  // cur pid
		char name[100];  // name of prof
		char state;  // state of prog
		int ppid;  // parent pid
		int pgpid;  // process group pid
		// scan the fomatted proc file
		fscanf(proc, "%d %s %c %d %d\n", &pid, name, &state, &ppid, &pgpid);
		fclose(proc);
		printf("|PID: %d | NAME: %s | STATE: %c | PPID: %d | PROC GRP PID: %d|\n",
		     	      pid, 	 name,	     state,     ppid,	           pgpid);
	}
	kill(pids[last], SIGSTOP);
	
	printf("-----------------------\n");	
	
	// choose which proc to run next (cannot be one that is already term)
	int ct = 0;
	while(states[next] == 1) {
		next++;
		next = next % num_lines;
		ct++;
		// if all proc are done, break, return
		if(ct == num_lines) {
			break;
		}
	}
	
	// start next process
	kill(pids[next], SIGCONT);
	
	// incr next to "point" to next proc
	last = next;
	next++;
	next = next % num_lines;  // next wraps around to beginning (if too large)
	// printf("new last=%d\n", last);  //TEST
	// printf("new next=%d\n", next);  //TEST

	alarm(2);  // cont until done
}

int main(int argc, char *argv[]) {
	
	// locals
	FILE *in = fopen(argv[1], "r");  // open file
	char *line_buff = NULL;  // will be alloc by getline()
	size_t len = 0;  // num bytes to alloc by getline()
	ssize_t num_read;  // num bytes read by getline()
	char *tok;  // for use by strtok_r()
	char *save1;  // for use by strtok_r()
	char c;  // for use in getting num lines in file
	int i = 0, k = 0;  // for use in main loop, commands array
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
	pids = malloc(sizeof(pid_t) * num_lines);  // pool of child processes
	
	// alloc mem for states arr
	states = malloc(sizeof(int) * num_lines);
	
	// init states of each proc
	// 0 ==> not done
	// 1 ==> done
	for(int i = 0; i < num_lines; i++) {
		states[i] = 0;
	}

	// get line from in file
	// fork new child process
	// exec command from line
	while((num_read = getline(&line_buff, &len, in)) != -1) {
		
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
			commands[k] = tok;
			cpy = NULL;  // reset cpy (else error)	
			k++;
		}
		commands[num_args] = NULL;  // set last arg to NULL (for use in execvp)	
		
		// exec processes
		pids[i] = fork();
		// handle errors with forking	
		if(pids[i] < 0) {
			perror("ERROR: ");
			free(pids);
			free(line_buff);
			fclose(in);
			free(states);
			exit(EXIT_FAILURE);
		}
		// if process is chlid, exec process
		if(pids[i] == 0) {
			// wait for SIGUSR1 before calling execvp()
			sigwait(&sigset, &sig);  // wait for signals in sigset
			int info = execvp(commands[0], commands);
			if(info == -1) {
				free(pids);
				free(line_buff);
				fclose(in);
				free(states);
			}
			perror("ERROR: ");
			exit(0);
		}
		
		i++;
	}
	
	// begin all proc, pause all proc
	send_sig(pids, num_lines, SIGUSR1);  // start all processes
	send_sig(pids, num_lines, SIGSTOP);  // stop all processes
	
	// send alarm
	signal(SIGALRM, sig_handler);  // register alarm handler
	alarm(2);  // call alarm handler
	printf("Inside main...\n");  //TEST

	
	// busy waiting loop
	// if all states == 1, break
	while(1) {
		int check = 1;
		for(int i = 0; i < num_lines; i++) {
			if(states[i] != 1) {
				check = 0;
			}
		}
		if(check == 1) {
			break;
		}
	}

	// closing remarks
	free(pids);
	free(line_buff);
	fclose(in);
	free(states);
}

