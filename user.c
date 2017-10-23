//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 4

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include "sharedMemory.h"
#include "timestamp.h"
#include "queue.h"

#define DEBUG 1 			// setting to 1 greatly increases number of logging events
#define TUNING 1
#define MAX_WORK_INTERVAL 75 * 1000 * 1000 // max time to work
#define BINARY_CHOICE 2

SmStruct shmMsg;
SmStruct *p_shmMsg;

int childId; 				// store child id number assigned from parent
int pcbIndex;				// store index of pcb
int startSeconds;			// store oss seconds when initializing shared memory
int startUSeconds;			// store oss nanoseconds when initializing shared memory
int endSeconds;				// store oss seconds to exit
int endUSeconds;			// store oss nanoseconds to exit
int exitSeconds;			// store oss seconds when exiting
int exitUSeconds;			// store oss nanoseconds when exiting

char timeVal[30]; // formatted time values for logging

void do_work(int willRunForThisLong);

int main(int argc, char *argv[]) {
childId = atoi(argv[0]); // saves the child id passed from the parent process
pcbIndex = atoi(argv[1]); // saves the pcb index passed from the parent process

getTime(timeVal);
if (DEBUG) printf("user %s: PCBINDEX: %d\n", timeVal, pcbIndex);

srand(getpid()); // random generator
int processTimeRequired = rand() % (MAX_WORK_INTERVAL);
const int oneMillion = 1000000000;

// a quick check to make sure user received a child id
getTime(timeVal);
if (childId < 0) {
	if (DEBUG) printf("user %s: Something wrong with child id: %d\n", timeVal, getpid());
	exit(1);
} else {
	if (DEBUG) printf("user %s: child %d (#%d) simulated work load: %d started normally after execl\n", timeVal, (int) getpid(), childId, processTimeRequired);

	// instantiate shared memory from oss
	getTime(timeVal);
	if (DEBUG) printf("user %s: child %d (#%d) create shared memory\n", timeVal, (int) getpid(), childId);

	// refactored shared memory using struct
	int shmid;
	if ((shmid = shmget(SHM_MSG_KEY, SHMSIZE, 0660)) == -1) {
		printf("sharedMemory: shmget error code: %d", errno);
		perror("sharedMemory: Creating shared memory segment failed\n");
		exit(1);
	}
	p_shmMsg = &shmMsg;
	p_shmMsg = shmat(shmid, NULL, 0);

	startSeconds = p_shmMsg->ossSeconds;
	startUSeconds = p_shmMsg->ossUSeconds;

	// we're not doing this any more
	//	endSeconds = startSeconds;
//	endUSeconds = startUSeconds + interval;
//
//	if (endUSeconds > oneMillion) {
//		endSeconds++;
//		endUSeconds -= oneMillion;
//	}

	getTime(timeVal);
	if (TUNING || DEBUG)
		printf("user %s: child %d (#%d) read start time in shared memory: %d.%09d\n",
			timeVal, (int) getpid(), childId, startSeconds, startUSeconds);


	// open semaphore
	sem_t *sem = open_semaphore(0);

	struct timespec timeperiod;
	timeperiod.tv_sec = 0;
	timeperiod.tv_nsec = 5 * 1000;

	while (1) { // main while loop

		if (p_shmMsg->dispatchedPid != (int) getpid()) {
				nanosleep(&timeperiod, NULL); // reduce the cpu load from looping
				continue;
			}

			sem_wait(sem);

			int runTime = p_shmMsg->dispatchedTime; // this is our maximum running time

			// clear the message from oss
			p_shmMsg->dispatchedPid = 0;
			p_shmMsg->dispatchedTime = 0;

			sem_post(sem);

			getTime(timeVal);
			printf("user %s: Receiving that process %d can run for %d nanoseconds\n", timeVal, (int) getpid(), runTime);

			// making some decisions about how long to run
			int willRunForFullTime = (rand() % BINARY_CHOICE); // make decision whether we will use the full quantum
			int willRunForThisLong;

			if (willRunForFullTime) {
				willRunForThisLong = runTime; // we will run for the full quantum assigned
			} else {
				willRunForThisLong = (rand() % runTime); // determine how long we will run if partial
			}

			do_work(willRunForThisLong); // doing "work"

			sem_wait(sem);

			// report back to oss
			p_shmMsg->userPid = (int) getpid();
			p_shmMsg->userHaltSignal = 1;
			p_shmMsg->userHaltTime = willRunForThisLong;

			sem_post(sem);

			getTime(timeVal);
			if (DEBUG) printf("user %s: Process %d checking escape conditions\nTotalCPUTime: %d willRunForThisLong: %d processTimeRequired: %d\n", timeVal, (int) getpid(), p_shmMsg->pcb[pcbIndex].totalCpuTime, willRunForThisLong ,processTimeRequired);

			if (p_shmMsg->pcb[pcbIndex].totalCpuTime + willRunForThisLong > processTimeRequired)
				break;

	} // end main while loop

	getTime(timeVal);
	printf("user %s: Process %d escaped main while loop\n", timeVal, (int) getpid());

	sem_wait(sem);

	// report process termination to oss
	p_shmMsg->userPid = (int) getpid();
	p_shmMsg->userHaltSignal = 0;

	sem_post(sem);

	// clean up shared memory
	shmdt(p_shmMsg);

	// close semaphore
	close_semaphore(sem);

	getTime(timeVal);
	if (DEBUG) printf("user %s: child %d (#%d) exiting normally\n", timeVal, (int) getpid(), childId);
}
exit(0);
}


// this part should occur within the critical section if
// implemented correctly since it accesses shared resources
void do_work(int willRunForThisLong) {

	getTime(timeVal);
	printf("user %s: Process %d doing work for %d nanoseconds\n", timeVal, (int) getpid(), willRunForThisLong);

	struct timespec sleeptime;
	sleeptime.tv_sec = 0;
	sleeptime.tv_nsec = willRunForThisLong;
	nanosleep(&sleeptime, NULL); // we are doing "work" here

	getTime(timeVal);
	printf("user %s: Process %d done doing work\n", timeVal, (int) getpid());

}
