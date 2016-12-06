#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "generator_tools.h"

int main(int argc,char* argv[]){

	int D,lo,hi; //Basic Parameters
	float t,T;
	int timeSlot = 0;
	int shmid = UNINITIALISED, semid_queues = UNINITIALISED, semid_sync = UNINITIALISED;
	ProcessQueue arrivalQueue;
	SharedMemorySegment* shmPtr = (SharedMemorySegment*) NULL;
	TimeTable timeTable;

	argumentHandling(&D,&lo,&hi,&t,&T,argc,argv); //Check Basic Parameters are set properly

	/*----Setup Generator----*/
	setupGenerator(&shmid, &semid_queues, &semid_sync, &shmPtr, &arrivalQueue, &timeTable, t, T, D, lo, hi);

	/*----Run Simulation----*/
	beginSimulation(D, semid_queues, semid_sync, T, shmPtr, &arrivalQueue, &timeTable);

	/*----TearDown Generator----*/
	tearDownGenerator(shmPtr, &arrivalQueue, &timeTable);

	return 0;
}
