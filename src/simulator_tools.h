#include "shared_structs.h"

#define NEXT_FIT 1
#define BEST_FIT 2
#define BUDDY 3

/*----Argument Checking----*/
int argumentHandling(int* D,int* lo,int* hi,float* t,float* T,int* S, char*,int argc,char* argv[]);

/*----Shared Memory----*/
int shared_memory_destruction(int shmid_genMem);
int shared_memory_creation(int* shmid_genMemPtr);
int shared_memory_initialisation(int shmid_genMem);
int setupSharedMemory(int semid_queues,int semid_sync,int* shmid_genMem);

/*----Semaphores----*/
int semaphores_destruction(int semid_queues,int semid_sync);
int semaphores_creation(int* semid_queuesPtr,int* semid_syncPtr);
int semaphores_initialisation(int semid_queues,int semid_sync);
int setupSemaphores(int* semid_queues,int* semid_sync);

/*----Free All Resources----*/
int ShmAndSemDestruction(int shmid_genMem,int semid_queues,int semid_sync);

/*----Handle Generator and Memory Manager----*/
int deployAndWait(int* fd,int D,int lo,int hi,float t,float T,int S,char* algorithmChoice);
