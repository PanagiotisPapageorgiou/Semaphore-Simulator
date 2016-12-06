#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include "simulator_tools.h"

int main(int argc,char* argv[]){

	int shmid_genMem=UNINITIALISED;
	int semid_queues=UNINITIALISED, semid_sync=UNINITIALISED;
	int algorithmChoice = 0;
	int fd;
	char algorithmName[10];

	int D,lo,hi,S;
	float t,T;

	argumentHandling(&D,&lo,&hi,&t,&T,&S, algorithmName,argc,argv); //Check proper arguments

	setupSemaphores(&semid_queues,&semid_sync);

	setupSharedMemory(semid_queues,semid_sync,&shmid_genMem);

	deployAndWait(&fd,D,lo,hi,t,T,S,algorithmName); //Fork/Exec for Generator and Memory Manager and wait for them to finish

	ShmAndSemDestruction(shmid_genMem,semid_queues,semid_sync); //Free all Resources

	printf("\nSimulator: Bye-bye\n");

	return 0;

}
