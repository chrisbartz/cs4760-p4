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

#define DEBUG 1 			// setting to 1 greatly increases number of logging events
#define TUNING 1
#define WAIT_INTERVAL 150 * 1000 * 1000 // max time to wait

SmStruct shmMsg;
SmStruct *p_shmMsg;

int childId; 				// store child id number assigned from parent
int startSeconds;			// store oss seconds when initializing shared memory
int startUSeconds;			// store oss nanoseconds when initializing shared memory
int endSeconds;				// store oss seconds to exit
int endUSeconds;			// store oss nanoseconds to exit
int exitSeconds;			// store oss seconds when exiting
int exitUSeconds;			// store oss nanoseconds when exiting


void critical_section();

int main(int argc, char *argv[]) {
childId = atoi(argv[0]); // saves the child id passed from the parent process
char timeVal[30]; // formatted time values for logging
time_t t;
srand((unsigned)time(&t)); // random generator
int interval = (rand() % WAIT_INTERVAL) + 1;
const int oneMillion = 1000000000;

// a quick check to make sure user received a child id
getTime(timeVal);
if (childId < 0) {
	if (DEBUG) printf("user   %s: Something wrong with child id: %d\n", timeVal, getpid());
	exit(1);
} else {
	if (DEBUG)
		printf("user   %s: child %d (#%d) started normally after execl\n", timeVal, (int) getpid(), childId);

	// instantiate shared memory from oss
	getTime(timeVal);
	if (DEBUG) printf("user   %s: child %d (#%d) create shared memory\n", timeVal, (int) getpid(), childId);

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

	endSeconds = startSeconds;
	endUSeconds = startUSeconds + interval;

	if (endUSeconds > oneMillion) {
		endSeconds++;
		endUSeconds -= oneMillion;
	}

	getTime(timeVal);
	if (TUNING)
		printf("user   %s: child %d (#%d) read start time in shared memory:        %d.%09d\n"
			"                                child %d (#%d) interval .%09d calculates end time: %d.%09d\n",
			timeVal, (int) getpid(), childId, startSeconds, startUSeconds, (int) getpid(), childId, interval, endSeconds, endUSeconds);

	// open semaphore
	sem_t *sem = open_semaphore(0);

	struct timespec timeperiod;
	timeperiod.tv_sec = 0;
	timeperiod.tv_nsec = 5 * 1000;

	while (!(p_shmMsg->ossSeconds >= endSeconds && p_shmMsg->ossUSeconds >= endUSeconds)) {
		// reduce the cpu load from looping
		nanosleep(&timeperiod, NULL);
	} // wait for the end

	// critical section
	// implemented with semaphores

	// wait for our turn
	sem_wait(sem);

	getTime(timeVal);
	if (TUNING) printf("user   %s: child %d (#%d) entering CRITICAL SECTION\n", timeVal, (int) getpid(), childId);

	// when it is our turn
	critical_section();

	getTime(timeVal);
	if (TUNING) printf("user   %s: child %d (#%d) exiting CRITICAL SECTION\n", timeVal, (int) getpid(), childId);

	// give up the turn
	sem_post(sem);

	// clean up shared memory
	shmdt(p_shmMsg);

	// close semaphore
	close_semaphore(sem);

	getTime(timeVal);
	if (DEBUG) printf("user   %s: child %d (#%d) exiting normally\n", timeVal, (int) getpid(), childId);
}
exit(0);
}


// this part should occur within the critical section if
// implemented correctly since it accesses shared resources
void critical_section() {

//	while (p_shmMsg->userPid != 0); // wait until shmMsg is clear
////	if (DEBUG) fprintf(stdout, "user  %s: child %d updating shared memory\n", timeVal, (int) getpid());
//
//	// lets capture this moment in time
//	exitSeconds = p_shmMsg->ossSeconds;
//	exitUSeconds = p_shmMsg->ossUSeconds;
//
//	p_shmMsg->userPid = (int) getpid();
//	p_shmMsg->userSeconds = exitSeconds;
//	p_shmMsg->userUSeconds = exitUSeconds;

}
