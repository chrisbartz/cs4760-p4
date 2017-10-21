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

#define DEBUG 1								// setting to 1 greatly increases number of logging events
#define VERBOSE 1
#define TUNING 0

const int maxChildProcessCount = 100; 		// limit of total child processes spawned
const int maxWaitInterval = 3;				// limit on how many seconds to wait until we spawn the next child

int totalChildProcessCount = 0; 		// number of total child processes spawned
int signalIntercepted = 0; 				// flag to keep track when sigint occurs
//int lastChildProcesses = -1;
int ossSeconds;							// store oss seconds
int ossUSeconds;						// store oss nanoseconds
int quantum = 1000000000;				// base for how many nanoseconds to increment each loop; default is 1 sec

long timeStarted = 0;					// when the OSS clock started
long timeToStop = 0;					// when the OSS should exit in real time

SmStruct shmMsg;
SmStruct *p_shmMsg;
sem_t *sem;

pid_t childpids[5000]; 					// keep track of all spawned child pids

void signal_handler(int signalIntercepted); 	// handle sigint interrupt
void increment_clock(int offset); 				// update oss clock in shared memory
void kill_detach_destroy_exit(int status); 		// kill off all child processes and shared memory
int pcbMapNextAvailableIndex();
void pcbAssign(int pcbMap[], int index, int pid);
void pcbDelete(int pcbMap[], int index);

int main(int argc, char *argv[]) {
	int childProcessCount = 0;			// number of child processes spawned
	int opt; 							// to support argument switches below
	pid_t childpid;						// store child pid
	int pcbMap[MAX_PROCESS_CONTROL_BLOCKS];// for keeping track of used pcb blocks
	char timeVal[30]; 					// store formatted time string for display in logging

	int maxConcSlaveProcesses = 2;		// max concurrent child processes
	int maxOssTimeLimitSeconds = 2;		// max run time in oss seconds
	char logFileName[50]; 				// name of log file
	strncpy(logFileName, "log.out", sizeof(logFileName)); // set default log file name
	int totalRunSeconds = 20; 			// set default total run time in seconds
	int goClock = 0;

	time_t t;
	srand((unsigned)time(&t)); 					// random generator
	int interval = (rand() % maxWaitInterval);

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
	if (DEBUG) printf("\n\nOSS %s: create shared memory\n", timeVal);

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
	p_shmMsg->dispatchedPid = 0;
	p_shmMsg->dispatchedTime = 0;

	// initialize pcbMap and pcbs
	for (int i = 0; i < MAX_PROCESS_CONTROL_BLOCKS; i++)
		pcbDelete(pcbMap, i);

	// open semaphore
	sem = open_semaphore(1);

	//register signal handler
	signal(SIGINT, signal_handler);

	getTime(timeVal);
	if (DEBUG && VERBOSE) printf("OSS %s: entering main loop\n", timeVal);

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
//		if (childProcessMessageCount >= maxChildProcessCount			// process count limit
//				|| ossSeconds >= maxOssTimeLimitSeconds || 			// OSS time limit
//				(timeToStop != 0 && timeToStop < getUnixTime())) { 	// real time limit
//
//			char typeOfLimit[50];
//			strncpy(typeOfLimit,"",50);
//			if (totalChildProcessCount >= maxChildProcessCount) strncpy(typeOfLimit,"because of process limit",50);
//			if (ossSeconds > maxOssTimeLimitSeconds ) strncpy(typeOfLimit,"because of OSS time limit",50);
//			if (timeToStop != 0 && timeToStop < getUnixTime()) strncpy(typeOfLimit,"because of real time limit (20s)",50);
//
//			getTime(timeVal);
////			if (TUNING)
//				printf("\nOSS %s: Halting %s.\nTotal Processes Spawned: %d\nTotal Processes Reported Time: %d\nOSS Seconds: %d.%09d\nStop Time:    %ld\nCurrent Time: %ld\n",
//					timeVal, typeOfLimit, totalChildProcessCount, childProcessMessageCount, ossSeconds, ossUSeconds, timeToStop, getUnixTime());
//
//			kill_detach_destroy_exit(0);
//		}

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
					printf("OSS %s: OSS starting clock.  Real start time: %ld  Real stop time: %ld\n", timeVal, timeStarted, timeToStop);
			}

			increment_clock(quantum);
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
			if (p_shmMsg->dispatchedPid == 0)
				continue; // jump back to the beginning of the loop if still waiting for message

			getTime(timeVal);
//			if (DEBUG)
//				printf("OSS %s: Child %d is terminating at my time %d.%09d because it reached %d.%09d in slave\n",
//						timeVal, p_shmMsg->userPid, ossSeconds, ossUSeconds, p_shmMsg->userSeconds, p_shmMsg->userUSeconds);
//			fprintf(logFile,"OSS %s: Child %d is terminating at my time %d.%09d because it reached %d.%09d in slave\n",
//					timeVal, p_shmMsg->userPid, ossSeconds, ossUSeconds, p_shmMsg->userSeconds, p_shmMsg->userUSeconds);

//			p_shmMsg->userSeconds = 0;
//			p_shmMsg->userUSeconds = 0;
//			p_shmMsg->userPid = 0;

//			childProcessMessageCount++;
			childProcessCount--; //because a child process completed
//			lastChildProcesses = childProcessCount;

		}



		int assignedPcb = pcbMapNextAvailableIndex(pcbMap);
		getTime(timeVal);
		if (DEBUG && VERBOSE && assignedPcb != -1) printf("OSS %s: Child (fork #%d from parent) has been assigned pcb index: %d\n", timeVal, totalChildProcessCount, assignedPcb);
		if (assignedPcb == -1) // if no available pcbs then wait
			continue;

		char iStr[1];
		sprintf(iStr, "%d", totalChildProcessCount);

		char assignedPcbStr[2];
		sprintf(assignedPcbStr, "%d", assignedPcb);

		childpid = fork();

		// if error creating fork
		if (childpid == -1) {
			perror("master: Failed to fork");
			kill_detach_destroy_exit(1);
			return 1;
		}

		// child will execute
		if (childpid == 0) {
			getTime(timeVal);
			if (DEBUG) printf("OSS %s: Child %d (fork #%d from parent) will attempt to execl user\n", timeVal, getpid(), totalChildProcessCount);
			int status = execl("./user", iStr, assignedPcbStr, NULL);
			getTime(timeVal);
			if (status) printf("OSS %s: Child (fork #%d from parent) has failed to execl user error: %d\n", timeVal, totalChildProcessCount, errno);
			perror("OSS: Child failed to execl() the command");
			return 1;
		}


		// parent will execute
		if (childpid != 0) {

			childpids[totalChildProcessCount] = childpid; // save child pids in an array
			childProcessCount++; // because we forked above
			totalChildProcessCount++;

			getTime(timeVal);
			if (DEBUG || TUNING) printf("OSS %s: Generating process with PID %d and putting it in queue %d at time %d.%09d\n", timeVal, (int) childpid, 0, ossSeconds, ossUSeconds);
			fprintf(logFile,"OSS %s: Generating process with PID %d and putting it in queue %d at time %d.%09d\n", timeVal, (int) childpid, 0, ossSeconds, ossUSeconds);

		}
//
//		getTime(timeVal);
//		if (DEBUG && VERBOSE) printf("OSS %s: end of while loop pid: %d\n", timeVal, getpid());
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

void increment_clock(int offset) {
	const int oneBillion = 1000000000;

	ossUSeconds += offset;

	if (ossUSeconds >= oneBillion) {
		ossSeconds++;
		ossUSeconds -= oneBillion;
	}

	if (0 && DEBUG && VERBOSE)
		printf("master: updating oss clock to %d.%09d\n", ossSeconds, ossUSeconds );
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

int pcbMapNextAvailableIndex(int pcbMap[]) {
	for (int i = 0; i < MAX_PROCESS_CONTROL_BLOCKS; i++) {
		if (!pcbMap[i])
			return i;
	}
	return -1;
}
 void pcbAssign(int pcbMap[], int index, int pid) {
	 pcbMap[index] = 1;
	 p_shmMsg->pcb[index].pid = pid;
 }

 void pcbDelete(int pcbMap[], int index) {
 	 pcbMap[index] = 0;
 	 p_shmMsg->pcb[index].lastBurstLength = 0;
 	 p_shmMsg->pcb[index].pid = 0;
 	 p_shmMsg->pcb[index].processPriority = 0;
 	 p_shmMsg->pcb[index].totalCpuTime = 0;
 	 p_shmMsg->pcb[index].totalTimeInSystem = 0;
  }
