//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 4


#include "sharedMemory.h"

#define DEBUG 0
#define SHMSIZE sizeof(SmTimeStruct)

sem_t* open_semaphore(int createSemaphore) {
	if (DEBUG) printf("sharedMemory: Creating semaphore\n");
	if (createSemaphore)
		return sem_open(SEM_NAME, O_CREAT|O_EXCL, 0660, 1);
	else
		return sem_open(SEM_NAME, 0);
}

void close_semaphore(sem_t *sem) {
	if (DEBUG) printf("sharedMemory: closing semaphore\n");
	sem_close(sem);
}
