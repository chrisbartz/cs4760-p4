//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 4

#include "queue.h"

int pop(int queue[]) {
	int returnValue = -1;
	if (queue[0] == -1) {
		return -1;
	} else {
		returnValue = queue[0];
		for (int i = 0; i < MAX_PROCESS_CONTROL_BLOCKS - 1; i++) {
			queue[i] = queue[i + 1]; // shift all values left
		}
		queue[MAX_PROCESS_CONTROL_BLOCKS - 1] = -1; // last one always becomes empty
	}
	return returnValue;
}

int peek(int queue[]) {
	return queue[0];
}

void push_back(int queue[], int pushValue) {
	for (int i = 0; i < MAX_PROCESS_CONTROL_BLOCKS; i++) {
		if (queue[i] < 0) {
			queue[i] = pushValue;
			break;
		}
	}
}

void initialize(int queue[]) {
	for (int i = 0; i < MAX_PROCESS_CONTROL_BLOCKS; i++)
		queue[i] = -1;
}

void printQueue(int queue[]) {
	for (int i = 0; i < MAX_PROCESS_CONTROL_BLOCKS; i++) {
		printf("%d,", queue[i]);
	}

	printf("\n");
}
