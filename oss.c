//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 4

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "sharedMemory.h"
#include "timestamp.h"

#define DEBUG 0 						// setting to 1 greatly increases number of logging events
#define VERBOSE 0
#define TUNING 0

//int i = 0;
int totalChildProcessCount = 0; 	// number of total child processes spawned
int signalIntercepted = 0; 				// flag to keep track when sigint occurs
int lastChildProcesses = -1;
int ossSeconds;							// store seconds
int ossUSeconds;						// store nanoseconds
int quantum = 10000;					// how many nanoseconds to increment each loop

long timeStarted = 0;					// when the OSS clock started
long timeToStop = 0;					// when the OSS should exit in real time

SmTimeStruct shmMsg;
SmTimeStruct *p_shmMsg;
pid_t childpids[5000]; 				// keep track of all spawned child pids

sem_t *sem;

void signal_handler(int signalIntercepted); // handle sigint interrupt
void increment_clock(); // update oss clock in shared memory
void kill_detach_destroy_exit(int status); // kill off all child processes and shared memory

int main(int argc, char *argv[]) {
	int childProcessCount = 0;			// number of child processes spawned
	int maxChildProcessCount = 100; 	// limit of total child processes spawned
	int childProcessMessageCount = 0;
	int opt; 							// to support argument switches below
	pid_t childpid;						// store child pid
	char timeVal[30]; 					// store formatted time string for display in logging

	int maxConcSlaveProcesses = 5;		// max concurrent child processes
	int maxOssTimeLimitSeconds = 2;
	char logFileName[50]; 				// name of log file
	strncpy(logFileName, "log.out", sizeof(logFileName)); // set default log file name
	int totalRunSeconds = 20; 			// set default total run time in seconds
	int goClock = 0;


	//gather option flags
	while ((opt = getopt(argc, argv, "hl:q:s:t:")) != -1) {
		switch (opt) {
		case 'l': // set log file name
			strncpy(logFileName, optarg, sizeof(logFileName));
			if (DEBUG) printf("opt l detected: %s\n", logFileName);
			break;
		case 'q': // set quantum amount
			quantum = atoi(optarg);
			if (DEBUG) printf("opt q detected: %d\n", quantum);
			break;
		case 's': // set number of concurrent slave processes
			maxConcSlaveProcesses = atoi(optarg);
			if (DEBUG) printf("opt s detected: %d\n", maxConcSlaveProcesses);
			break;
		case 't': // set number of total run seconds
			totalRunSeconds = atoi(optarg);
			if (DEBUG) printf("opt t detected: %d\n", totalRunSeconds);
			break;
		case 'h': // print help message
			if (DEBUG) printf("opt h detected\n");
			fprintf(stderr,"Usage: ./%s <arguments>\n", argv[0]);
			break;
		default:
			break;
		}
	}

	if (argc < 1) { /* check for valid number of command-line arguments */
			fprintf(stderr, "Usage: %s command arg1 arg2 ...\n", argv[0]);
			return 1;
	}

	// open log file for writing
	FILE *logFile;
	logFile = fopen(logFileName,"w+");

	if (logFile == NULL) {
		perror("Cannot open log file");
		exit(1);
	}

	// instantiate shared memory from oss
	getTime(timeVal);
	if (DEBUG) printf("\n\nmaster %s: create shared memory\n", timeVal);

	// refactored shared memory using struct
	int shmid;
	if ((shmid = shmget(SHM_MSG_KEY, SHMSIZE, IPC_CREAT | 0660)) == -1) {
		fprintf(stderr, "sharedMemory: shmget error code: %d", errno);
		perror("sharedMemory: Creating shared memory segment failed\n");
		exit(1);
	}
	p_shmMsg = &shmMsg;
	p_shmMsg = shmat(shmid,NULL,0);

	p_shmMsg->ossSeconds = 0;
	p_shmMsg->ossUSeconds = 0;
	p_shmMsg->userSeconds = 0;
	p_shmMsg->userUSeconds = 0;
	p_shmMsg->userPid = 0;

	// open semaphore
	sem = open_semaphore(1);

	//register signal handler
	signal(SIGINT, signal_handler);

	getTime(timeVal);
	if (DEBUG && VERBOSE) printf("master %s: entering main loop\n", timeVal);

	// this is the main loop
	while (1) {

		//what to do when signal encountered
		if (signalIntercepted) { // signalIntercepted is set by signal handler
			printf("\nmaster: //////////// oss terminating children due to a signal! //////////// \n\n");
			printf("master: parent terminated due to a signal!\n\n");

			kill_detach_destroy_exit(130);
		}

		// we put limits on the number of processes and time
		// if we hit limit then we kill em all
		if (childProcessMessageCount >= maxChildProcessCount			// process count limit
				|| ossSeconds >= maxOssTimeLimitSeconds || 			// OSS time limit
				(timeToStop != 0 && timeToStop < getUnixTime())) { 	// real time limit

			char typeOfLimit[50];
			strncpy(typeOfLimit,"",50);
			if (totalChildProcessCount >= maxChildProcessCount) strncpy(typeOfLimit,"because of process limit",50);
			if (ossSeconds > maxOssTimeLimitSeconds ) strncpy(typeOfLimit,"because of OSS time limit",50);
			if (timeToStop != 0 && timeToStop < getUnixTime()) strncpy(typeOfLimit,"because of real time limit (20s)",50);

			getTime(timeVal);
//			if (TUNING)
				printf("\nmaster %s: Halting %s.\nTotal Processes Spawned: %d\nTotal Processes Reported Time: %d\nOSS Seconds: %d.%09d\nStop Time:    %ld\nCurrent Time: %ld\n",
					timeVal, typeOfLimit, totalChildProcessCount, childProcessMessageCount, ossSeconds, ossUSeconds, timeToStop, getUnixTime());

			kill_detach_destroy_exit(0);
		}

		if (childpid != 0 && goClock) {
			if (timeToStop == 0) {
				// wait for the child processes to get set up
				struct timespec timeperiod;
				timeperiod.tv_sec = 0;
				timeperiod.tv_nsec = 50 * 1000 * 1000;
				nanosleep(&timeperiod, NULL);

				timeStarted = getUnixTime();
				timeToStop = timeStarted + (1000 * totalRunSeconds);
				getTime(timeVal);
				if (TUNING)
					printf("master %s: OSS starting clock.  Real start time: %ld  Real stop time: %ld\n", timeVal, timeStarted, timeToStop);
			}

			increment_clock();
		}


		// if we have forked up to the max concurrent child processes
		// then we wait for one to exit before forking another
		if (childProcessCount >= maxConcSlaveProcesses) {
			goClock = 1; // start the clock when max concurrent child processes are spawned

			// reduce the cpu load from looping
			struct timespec timeperiod;
			timeperiod.tv_sec = 0;
			timeperiod.tv_nsec = 5 * 1000;
			nanosleep(&timeperiod, NULL);

			// wait for child to send message
			if (p_shmMsg->userPid == 0)
				continue; // jump back to the beginning of the loop if still waiting for message

			getTime(timeVal);
//			if (DEBUG)
				printf("master %s: Child %d is terminating at my time %d.%09d because it reached %d.%09d in slave\n",
						timeVal, p_shmMsg->userPid, ossSeconds, ossUSeconds, p_shmMsg->userSeconds, p_shmMsg->userUSeconds);
			fprintf(logFile,"master %s: Child %d is terminating at my time %d.%09d because it reached %d.%09d in slave\n",
					timeVal, p_shmMsg->userPid, ossSeconds, ossUSeconds, p_shmMsg->userSeconds, p_shmMsg->userUSeconds);

			p_shmMsg->userSeconds = 0;
			p_shmMsg->userUSeconds = 0;
			p_shmMsg->userPid = 0;

			childProcessMessageCount++;
			childProcessCount--; //because a child process completed
			lastChildProcesses = childProcessCount;

		}

		char iStr[1];
		sprintf(iStr, "%d", totalChildProcessCount);

		int retryCount = 10;

		while((childpid = fork()) < 0 && retryCount > 0) {
			retryCount--;
		}


		// if error creating fork
		if (childpid == -1) {
			perror("master: Failed to fork");
			kill_detach_destroy_exit(1);
			return 1;
		}

		// child will execute
		if (childpid == 0) {
			getTime(timeVal);
			if (DEBUG && VERBOSE) printf("master %s: child check pid: %d\n", timeVal, getpid());

			getTime(timeVal);
			if (DEBUG) printf("master %s: Child %d (fork #%d from parent) will attempt to execl user\n", timeVal, getpid(), totalChildProcessCount);
			int status = execl("./user", iStr, NULL);
			getTime(timeVal);
			if (status) printf("master %s: Child (fork #%d from parent) has failed to execl user error: %d\n", timeVal, totalChildProcessCount, errno);
			perror("master: Child failed to execl() the command");
			return 1;
		}


		// parent will execute
		if (childpid != 0) {

			childpids[totalChildProcessCount] = childpid; // save child pids in an array
			childProcessCount++; // because we forked above
			totalChildProcessCount++;

			getTime(timeVal);
			if (DEBUG || TUNING) printf("master %s: parent forked child %d at %d.%09d = childPid: %d\n", timeVal, totalChildProcessCount - 1, ossSeconds, ossUSeconds, (int) childpid);

		}

		getTime(timeVal);
		if (DEBUG && VERBOSE) printf("master %s: end of while loop pid: %d\n", timeVal, getpid());
	} //end while loop

	fclose(logFile);

	kill_detach_destroy_exit(0);

	return 0;
}

// remove newline characters from palinValues
void trim_newline(char *string) {
	string[strcspn(string, "\r\n")] = 0;
}

// handle the ^C interrupt
void signal_handler(int signal) {
	if (DEBUG) printf("\nmaster: //////////// Encountered signal! //////////// \n\n");
	signalIntercepted = 1;
}

void increment_clock() {
	const int oneMillion = 1000000000;

	ossUSeconds += quantum;

	if (ossUSeconds >= oneMillion) {
		ossSeconds++;
		ossUSeconds -= oneMillion;
	}

//	if (DEBUG)
//		printf("master: updating oss clock to %d.%09d\n", ossSeconds, ossUSeconds );
	p_shmMsg->ossSeconds = ossSeconds;
	p_shmMsg->ossUSeconds = ossUSeconds;

}

void kill_detach_destroy_exit(int status) {
	// kill all running child processes
	for (int p = 0; p < totalChildProcessCount; p++) {
		if (DEBUG) printf("master: //////////// oss terminating child process %d //////////// \n", (int) childpids[p]);
		kill(childpids[p], SIGTERM);
	}

	// clean up
	shmdt(p_shmMsg);
	shmctl(SHM_MSG_KEY, IPC_RMID, NULL);

	// close semaphore
	sem_unlink(SEM_NAME);
	close_semaphore(sem);
	sem_destroy(sem);

	if (status == 0) printf("master: parent terminated normally \n\n");

	exit(status);
}
