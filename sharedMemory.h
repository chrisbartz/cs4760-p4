//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 4

#ifndef SHAREDMEMORY_H_
#define SHAREDMEMORY_H_

// set up shared memory keys for communication
#define SHM_MSG_KEY 98753
#define SHMSIZE sizeof(SmStruct)
#define SEM_NAME "cyb01b_p4"
#define MAX_PROCESS_CONTROL_BLOCKS 18

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

typedef struct {
	int startUserSeconds;
	int startUserUSeconds;
	int endUserSeconds;
	int endUserUSeconds;
	int totalCpuTime;
	int totalTimeInSystem;
	int lastBurstLength;
	int processPriority;
	int pid;
} SmProcessControlBlock;

typedef struct {
	int ossSeconds;
	int ossUSeconds;
	int dispatchedPid;
	int dispatchedTime;
	int userPid;
	int userHaltSignal; // 0 terminated 1 halted
	int userHaltTime;
	SmProcessControlBlock pcb[MAX_PROCESS_CONTROL_BLOCKS];
} SmStruct;

sem_t* open_semaphore(int createSemaphore);

void close_semaphore(sem_t *sem);

#endif /* SHAREDMEMORY_H_ */
