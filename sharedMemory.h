//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 4

#ifndef SHAREDMEMORY_H_
#define SHAREDMEMORY_H_

// set up shared memory keys for communication
#define SHM_MSG_KEY 98753
#define SHMSIZE sizeof(SmTimeStruct)
#define SEM_NAME "cyb01b_p3"

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
	int ossSeconds;
	int ossUSeconds;
	int userSeconds;
	int userUSeconds;
	int userPid;
} SmTimeStruct;

sem_t* open_semaphore(int createSemaphore);

void close_semaphore(sem_t *sem);

#endif /* SHAREDMEMORY_H_ */
