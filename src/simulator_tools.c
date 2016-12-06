#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <fcntl.h>
#include "simulator_tools.h"

/*======================ARGUMENT CHECKING=============================*/

int argumentHandling(int* D,int* lo,int* hi,float* t,float* T,int* S,char* algorithmName,int argc,char* argv[]){

	int i = 0;
	char c;

	srand(time(NULL));

	for(i=0; i < argc; i++){
		printf("|i: %d| - |%s|\n", i, argv[i]);
	}

	if(argc == 1){ //No Arguments provided
		printf("Simulator: Provide Simulation Duration: ");
		scanf("%d", D);
		c = getchar();
		printf("\n");

		printf("Simulator: Provide lo [Low VP Memory Boundary (KB) ]: ");
		scanf("%d", lo);
		c = getchar();
		printf("\n");

		printf("Simulator: Provide hi [High VP Memory Boundary (KB) ]: ");
		scanf("%d", hi);
		c = getchar();
		printf("\n");

		printf("Simulator: Provide t (Average time between arrivals): ");
		scanf("%f", t);
		c = getchar();
		printf("\n");

		printf("Simulator: Provide T (Average time of Memory Usage): ");
		scanf("%f", T);
		c = getchar();
		printf("\n");

		printf("Simulator: Provide S (Memory Size in KB): ");
		scanf("%d", S);
		c = getchar();
		printf("\n");

		do{
			printf("Simulator: Provide AlgorithmChoice: \n");
			printf("Simulator: BD for Buddy\nSimulator: NF for Next-Fit\nSimulator: BF for Best-Fit\n");
			scanf("%s", algorithmName);
			c = getchar();
			printf("\n");
			if(!strcmp(algorithmName, "NF")){
				break;
			}
			else if(!strcmp(algorithmName, "BF")){
				break;
			}
			else if(!strcmp(algorithmName, "BD")){
				break;
			}
			else{
				printf("Simulator: Invalid algorithm Choice please try again!\n");
			}
		}while(1);

	}
	else if(argc != 8){
		printf("Simulator: Argument Error - Incorrect number of arguments!\n");
		printf("Simulator: If you wish to run the program with arguments run it like this: \n");
		printf("Simulator: ./Simulator <Simulation_Duration> <VP_MemLo> <VP_MemHi> <avg_time_arrivals> <avg_time_mem_usage> <S> <BD/NF/BF>\n\n");
		printf("TIP: You can also run the program without any arguments!\n\n");
		exit(1);
	}
	else{
		(*D) = atoi(argv[1]);
		(*lo) = atoi(argv[2]);
		(*hi) = atoi(argv[3]);
		(*t) = atof(argv[4]);
		(*T) = atof(argv[5]);
		(*S) = atoi(argv[6]);
		strcpy(algorithmName, argv[7]);
	}
	
	if((*D) <= 0){
		printf("Simulator: Argument Error - Simulation Duration Time has to be a positive number!\n");
	}
	else{
		if((*lo) <= 0){
			printf("Simulator: Argument Error - Low VP (KB) Memory Boundary has to be a positive number!\n");
		}
		else{
			if((*hi) <= 0){
				printf("Simulator: Argument Error - High VP Memory Boundary (KB) has to be a positive number!\n");
			}
			else if((*hi) < (*lo)){
				printf("Simulator: Argument Error - High VP Memory Boundary (KB) has to be greater or equal to Low Memory Boundary!\n");
			}
			else{
				if((*t) <= 0){
					printf("Simulator: Argument Error - Average Time Between Arrivals has to be a positive number!\n");
				}
				else if((*t) >= (*D)){
					printf("Simulator: Argument Error - Please provide an Average Time Between Arrivals that is less than the SimDuration time!\n");
				}
				else{
					if((*T) <= 0){
						printf("Simulator: Argument Error - Average MemUsage Time has to be a positive number!\n");
					}
					else if((*T) >= (*D)){
						printf("Simulator: Argument Error - Please provide a MemUsage Time that is less than the SimDuration time!\n");
					}
					else{
						if((*S) <= 0){
							printf("Simulator: Argument Error - Please provide a Memory Size (KB) that is a positive number!\n");
						}
						else if((*S) <= (*hi)){
							printf("Simulator: Argument Error - Please provide a Memory Size (KB) that is greater than the VP High Memory Boundary (KB)!\n");
						}
						else{
							printf("\nSimulator: Received Arguments: |D: %d| - |S: %d|\n", *D, *S);
							if(!strcmp(algorithmName, "NF")){
								printf("\nSimulator: Received Arguments: |D: %d| - |lo: %d| - |hi: %d| - |t: %f| - |T: %f| - |S: %d| - |NEXT_FIT|\n", *D, *lo, *hi , *t, *T, *S);
								return 0;
							}
							else if(!strcmp(algorithmName, "BF")){
								printf("\nSimulator: Received Arguments: |D: %d| - |lo: %d| - |hi: %d| - |t: %f| - |T: %f| - |S: %d| - |BEST_FIT|\n", *D, *lo, *hi , *t, *T, *S);
								return 0;
							}
							else if(!strcmp(algorithmName, "BD")){
								printf("\nSimulator: Received Arguments: |D: %d| - |lo: %d| - |hi: %d| - |t: %f| - |T: %f| - |S: %d| - |BUDDY|\n", *D, *lo, *hi , *t, *T, *S);

								return 0;
							}
							else{
								printf("Simulator: Invalid algorithm Choice please try again!\n");
							}
						}
					}
				}
			}
		}
	}

	printf("Simulator: If you wish to run the program with arguments run it like this: \n");
	printf("Simulator: ./Simulator <Simulation_Duration> <VP_MemLo> <VP_MemHi> <avg_time_arrivals> <avg_time_mem_usage> <S> <BD/NF/BF>\n\n");
	printf("Simulator: TIP: You can also run the program without any arguments!\n\n");
	exit(1);

}

/*=========================================SHARED MEMORY FUNCTIONS====================================*/

int shared_memory_destruction(int shmid_genMem){

	if((shmid_genMem != UNINITIALISED) && (shmid_genMem != -1)){ //or simply if a shmid is positive/initialised
		printf("Simulator: Now Destroying Gen-Mem Shared Memory Segment...\n");
		if(shmctl(shmid_genMem, IPC_RMID, NULL) == -1){
			printf("Simulator: Semctl failed! Error Message: |%s|\n", strerror(errno));		
			return -1;
		}		
	}

	return 0;
}

int shared_memory_creation(int* shmid_genMemPtr){

	/*----Creating Memory Segment Gen-Mem----*/
	printf("Simulator: Creating Gen-Mem Shared Memory Segment... - GEN_MEM_SHARED_MEMORY_KEY:%d\n", SHMKEY_GEN_MEM);
	*shmid_genMemPtr = shmget(SHMKEY_GEN_MEM, sizeof(SharedMemorySegment), PERMS | IPC_CREAT);
	if(*shmid_genMemPtr == -1){
		printf("Simulator: Shmget failed! Error Message: |%s|\n", strerror(errno));
		return -1;
	}

	return 0;
}

int shared_memory_initialisation(int shmid_genMem){ //Attach to Shared Memory Segment and Initialise its contents

	void* tmp_shm_ptr = (void*) NULL;
	SharedMemorySegment* shmPtr = (SharedMemorySegment*) NULL;

	tmp_shm_ptr = shmat(shmid_genMem, (void*) 0, 0);
	if(tmp_shm_ptr == (char*) -1){
		printf("Shmat failed! Error Message: |%s|\n", strerror(errno));
		return -1;
	}

	shmPtr = (SharedMemorySegment*) tmp_shm_ptr;
	if(init_arrayQueue(&(shmPtr->memGenQueue)) == -1){
		printf("Simulator: Failed to initialise inputQueue!\n");
		return -1;
	}
	if(init_arrayQueue(&(shmPtr->genMemQueue)) == -1){
		printf("Simulator: Failed to initialise outputQueue!\n");
		return -1;
	}

	if(detachSharedMemory(shmPtr) == -1){
		printf("Simulator: Failed to detach from Shared Memory after Initialisation!\n");
		return -1;
	}

	return 0;

}

int setupSharedMemory(int semid_queues,int semid_sync,int* shmid_genMem){ //Create, Attach and Initialise Shared Memory

	/*----Shared Memory Segment Creation----*/
	printf("\nSimulator: Creating Shared Memory Segment...\n");
	if(shared_memory_creation(shmid_genMem) == -1){
		printf("Simulator: Failed at shared memory creation!\n");
		printf("Simulator: Now Destroying Semaphores...\n");
		if(semaphores_destruction(semid_queues,semid_sync) == -1){
			printf("Simulator: Failed at Semaphores Destruction!\n");
		}
		printf("Exiting...");	
		exit(1);
	}

	/*----Shared Memory Initialisation----*/
	if(shared_memory_initialisation(*shmid_genMem) == -1){
		printf("Simulator: Failed at shared memory initialisation!\n");
		ShmAndSemDestruction(*shmid_genMem,semid_queues,semid_sync);
		exit(1);
	}

	return 0;

}

/*=======================================SEMAPHORE FUNCTIONS==========================================*/

int semaphores_destruction(int semid_queues,int semid_sync){

	int semnum;

	if((semid_queues != UNINITIALISED) && (semid_queues != -1)){
		printf("Simulator: Now Destroying Queues Semaphores...\n");
		if(semctl(semid_queues, semnum, IPC_RMID) == -1){
			printf("Simulator: Semctl failed! Error Message: |%s|\n", strerror(errno));		
			return -1;
		}		
	}

	if((semid_sync != UNINITIALISED) && (semid_sync != -1)){
		printf("Simulator: Now Destroying Sync Semaphores...\n");
		if(semctl(semid_sync, semnum, IPC_RMID) == -1){
			printf("Simulator: Semctl failed! Error Message: |%s|\n", strerror(errno));		
			return -1;
		}		
	}

	return 0;
}

int semaphores_creation(int* semid_queuesPtr,int* semid_syncPtr){
	
	/*----Creating Semaphore for Queues----*/
	printf("Simulator: Creating Semaphores for memGenQueue and genMemQueue access... - QUEUES_SEMAPHORE_KEY:%d\n", SEMKEY_QUEUES);
	*semid_queuesPtr = semget(SEMKEY_QUEUES, NSEMS_QUEUES, PERMS | IPC_CREAT);
	if(*semid_queuesPtr == -1){
		printf("Simulator: Semget failed! Error Message: |%s|\n", strerror(errno));
		return -1;
	}
	
	/*----Creating Semaphores for Sync----*/
	printf("Simulator: Creating Semaphores for Sync... - SYNC_SEMAPHORE_KEY:%d\n", SEMKEY_SYNC);
	*semid_syncPtr = semget(SEMKEY_SYNC, NSEMS_SYNC, PERMS | IPC_CREAT);
	if(*semid_syncPtr == -1){
		printf("Simulator: Semget failed! Error Message: |%s|\n", strerror(errno));
		return semaphores_destruction(*semid_queuesPtr,*semid_syncPtr);
	}

	return 0;

}

int semaphores_initialisation(int semid_queues,int semid_sync){

	int semnum;

	union semun{
		int val;
		struct semid_ds *buf;
		unsigned short *array;
	} argQueues, argSync;

	unsigned short queuesArray[NSEMS_QUEUES] = {1, QUEUE_ARRAY_SIZE, 0, 1, QUEUE_ARRAY_SIZE, 0}; //GenMem Buffer(mutex,empty,full) MemGen Buffer(mutex,empty,full)
	unsigned short syncArray[NSEMS_SYNC] = {0, 0, 0, 0}; //2 Semaphores for TimeSlot Synchronisation, 2 Semaphores for Execution Synchronisation

	argQueues.array = queuesArray;
	argSync.array = syncArray;

	/*----Queues Semaphores Initialisation----*/
	if(semctl(semid_queues, semnum, SETALL, argQueues) == -1){
		printf("Simulator: Semctl failed! Error Message: |%s|\n", strerror(errno));
		printf("Simulator: Destroying semaphore...\n");
		return -1;
	}

	/*----Sync Semaphores Initialisation----*/
	if(semctl(semid_sync, semnum, SETALL, argSync) == -1){
		printf("Simulator: Semctl failed! Error Message: |%s|\n", strerror(errno));
		printf("Simulator: Destroying semaphore...\n");
		return -1;
	}

	return 0;

}

int setupSemaphores(int* semid_queues,int* semid_sync){

	/*----Semaphores Creation----*/
	printf("\nSimulator: Creating all Semaphores...\n");
	if(semaphores_creation(semid_queues,semid_sync) == -1){
		printf("Simulator: Failed at Semaphores Creation!\n");	
		printf("Exiting...");
		exit(1);
	}

	/*----Semaphores Initialisation----*/
	printf("\nSimulator: Initialising Semaphores...\n");
	if(semaphores_initialisation(*semid_queues,*semid_sync) == -1){
		printf("Simulator: Failed at Semaphores Initialisation!\n");
		printf("Simulator: Now Destroying Semaphores...\n");
		if(semaphores_destruction(*semid_queues,*semid_sync) == -1){
			printf("Simulator: Failed at Semaphores Destruction!\n");
		}
		printf("Exiting...");		
		exit(1);
	}
	
	return 0;

}

/*==========================DEPLOY GENERATOR AND MEMORY MANAGER======================================*/

int deployAndWait(int* fd,int D,int lo,int hi,float t,float T,int S,char* algorithmChoice){

	int i=0;
	pid_t gen_pid;
	pid_t mem_pid;

	char Dstring[20];
	char lostring[20];
	char histring[20];
	char tstring[20];
	char Tstring[20];
	char Sstring[20];
	char fdString[20];
	char buffer[256];

	char** argvGen = (char**) NULL;
	char** argvMem = (char**) NULL;

	argvGen = malloc(sizeof(char*) * 7);
	if(argvGen == NULL){
		printf("\nSimulator: argvGen is NULL!\n");
		exit(1);
	}
	for(i=0; i < 6; i++){
		argvGen[i] = malloc(sizeof(char) * 20);
		if(argvGen[i] == NULL){
			printf("\nSimulator: argvGen[%d] is NULL!\n", i);
			exit(1);
		}
	}

	sprintf(Dstring,"%d",D);
	sprintf(lostring,"%d",lo);
	sprintf(histring,"%d",hi);
	sprintf(tstring,"%f",t);
	sprintf(Tstring,"%f",T);
	sprintf(Sstring,"%d",S);

	argvGen[6] = (char*) NULL;
	strcpy(argvGen[0], "./Generator");
	strcpy(argvGen[1], Dstring);
	strcpy(argvGen[2], lostring);
	strcpy(argvGen[3], histring);
	strcpy(argvGen[4], tstring);
	strcpy(argvGen[5], Tstring);

	printf("\nSimulator: Now Deploying Generator...\n");
	gen_pid = fork();
	if(gen_pid == -1){
		printf("Simulator: Failed to fork child for Generator Process!\n");
		return -1;
	}
	else if(gen_pid == 0){ //Child to exec Generator
		printf("Child Proccess (menerator): Hello! I am the child process with PID: %lu\n", (long) getpid());
		if(execv(argvGen[0], argvGen) == -1){
			printf("Simulator: Execv failed! Error Message: |%s|\n", strerror(errno));
			if(argvGen != NULL){
				for(i=0; i < 6; i++){
					if(argvGen[i] != NULL){
						free(argvGen[i]);
					}
				}
				free(argvGen);
			}
			exit(1);
		}
	}

	if(argvGen != NULL){
		for(i=0; i < 6; i++){
			if(argvGen[i] != NULL){
				free(argvGen[i]);
			}
		}
		free(argvGen);
	}

	/*----Preparing to deploy Manager----*/
	argvMem = malloc(sizeof(char*) * 6);
	if(argvMem == NULL){
		printf("\nSimulator: argvMem is NULL!\n");
		exit(1);
	}
	for(i=0; i < 5; i++){
		argvMem[i] = malloc(sizeof(char) * 20);
		if(argvMem[i] == NULL){
			printf("\nSimulator: argvMem[%d] is NULL!\n", i);
			exit(1);
		}
	}

	printf("\nManager: Creating LOG File...\n");

	if((*fd = open("SimulationLog.txt", O_CREAT|O_RDWR|O_TRUNC|O_APPEND,0644)) == -1){
		printf("Simulator: Failed to create LOG file!\n");
		exit(1);
	}	
	sprintf(fdString,"%d", *fd);

	argvMem[5] = (char*) NULL;
	strcpy(argvMem[0], "./Manager");
	strcpy(argvMem[1], Dstring);
	strcpy(argvMem[2], Sstring);
	strcpy(argvMem[3], algorithmChoice);
	strcpy(argvMem[4], fdString);

	/*-----Printing to log file----*/
	sprintf(buffer, "========================================SIMULATOR START========================================\n");
	write(*fd,buffer,sizeof(char)*(strlen(buffer)));

	if(!strcmp(algorithmChoice, "NF")){
		sprintf(buffer, "Simulator Parameters: D=%d , lo=%d, hi=%d, t=%f, T=%f, S=%d, NEXT-FIT\n\nBeginning Simulation!\n", D, lo, hi, t, T, S);
		write(*fd,buffer,sizeof(char)*(strlen(buffer)));
	}
	else if(!strcmp(algorithmChoice, "BF")){
		sprintf(buffer, "Simulator Parameters: D=%d , lo=%d, hi=%d, t=%f, T=%f, S=%d, BEST-FIT\n\nBeginning Simulation!\n", D, lo, hi, t, T, S);
		write(*fd,buffer,sizeof(char)*(strlen(buffer)));
	}
	else{
		sprintf(buffer, "Simulator Parameters: D=%d , lo=%d, hi=%d, t=%f, T=%f, S=%d, BUDDY-FIT\n\nBeginning Simulation!\n", D, lo, hi, t, T, S);
		write(*fd,buffer,sizeof(char)*(strlen(buffer)));
	}

	printf("\nSimulator: Now Deploying Memory Manager...\n");
	mem_pid = fork();
	if(mem_pid == -1){
		printf("Simulator: Failed to fork child for Memory Manager Process!\n");
		return -1;
	}
	else if(mem_pid == 0){ //Child to exec Memory Manager
		printf("Child Proccess (memory manager): Hello! I am the child process with PID: %lu\n", (long) getpid());
		if(execv(argvMem[0], argvMem) == -1){
			printf("Simulator: Execv failed! Error Message: |%s|\n", strerror(errno));
			if(argvMem != NULL){
				for(i=0; i < 5; i++){
					if(argvMem[i] != NULL){
						free(argvMem[i]);
					}
				}
				free(argvMem);
			}
			exit(1);
		}
	}

	if(argvMem != NULL){
		for(i=0; i < 5; i++){
			if(argvMem[i] != NULL){
				free(argvMem[i]);
			}
		}
		free(argvMem);
	}

	/*----Wait for processes----*/
	printf("\nSimulator: Deployed Processes successfully\nNow waiting from them to terminate...\n");

	waitpid(gen_pid, NULL, 0);

	printf("\nSimulator: Generator just exited!\n");

	waitpid(mem_pid, NULL, 0);

	printf("\nSimulator: Memory Manager just exited!\n");
	
	close(*fd);

	return 0;

	
}


/*==========================SEMAPHORE AND SHARED MEMORY TEAR DOWN OPERATIONS=========================*/

int ShmAndSemDestruction(int shmid_genMem,int semid_queues,int semid_sync){

	/*----Shared Memory Segments Destruction----*/
	printf("\nSimulator: Destroying Shared Memory Segment...\n");
	if(shared_memory_destruction(shmid_genMem) == -1){
		printf("Simulator: Failed at shared memory destruction!\n");
		printf("\nSimulator: Destroying all Semaphores...\n");
		if(semaphores_destruction(semid_queues,semid_sync) == -1){
			printf("Simulator: Failed at Semaphores Destruction!\n");	
		}			
		printf("Exiting...");
		exit(1);
	}	

	/*----Semaphores Destruction----*/
	printf("\nSimulator: Destroying all Semaphores...\n");
	if(semaphores_destruction(semid_queues,semid_sync) == -1){
		printf("Simulator: Failed at Semaphores Destruction!\n");
		printf("Exiting...");		
		exit(1);
	}

	return 0;
	
}
