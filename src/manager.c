#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "manager_tools.h"

int main(int argc,char* argv[]){

	int D; //Basic Parameters
	int timeSlot = 0;
	int shmid = UNINITIALISED, semid_queues = UNINITIALISED, semid_sync = UNINITIALISED, S;
	int algorithmChoice;
	int fd;
	ProcessQueue waitingQueue;
	ProcessQueue statsQueue;
	SharedMemorySegment* shmPtr = (SharedMemorySegment*) NULL;
	Memory memory;

	argumentHandling(&fd, &D,&S, &algorithmChoice,argc,argv); //Check Basic Parameters are set properly

	/*----Setup Manager----*/
	setupManager(&fd, &shmid, &semid_queues, &semid_sync, &shmPtr, &waitingQueue, &statsQueue, &memory, D, S);

	/*----Run Simulation----*/
	beginSimulation(fd, D, semid_queues, semid_sync, shmPtr, &waitingQueue, &statsQueue,  &memory, algorithmChoice);

	/*----TearDown Manager----*/
	tearDownManager(shmPtr, &waitingQueue,  &statsQueue, &memory);

	return 0;
}
