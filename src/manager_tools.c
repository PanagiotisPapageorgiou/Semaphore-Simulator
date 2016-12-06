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
#include "manager_tools.h"

/*======================Argument Checking=============================*/

int argumentHandling(int* fd,int* D,int* S,int* algorithmChoice,int argc,char* argv[]){

	int i = 0;
	char c;
	char algorithmName[10];

	srand(time(NULL));

	printf("Manager: Received %d arguments!\n", argc);

	for(i=0; i < argc; i++){
		printf("|i: %d| - |%s|\n", i, argv[i]);
	}

	if(argc == 1){ //No Arguments provided
		printf("Manager: Please run the program through the Simulator process!");
	}
	else if(argc != 5){
		printf("Manager: Argument Error - Incorrect number of arguments!\n");
		printf("Manager: If you wish to run the program with arguments run it like this: \n");
		printf("Manager: ./Manager <Simulation_Duration> <S> <BD/NF/BF> <file_descriptor>\n\n");
		printf("Manager: TIP: You can also run the program without any arguments!\n\n");
		exit(1);
	}
	else{
		(*D) = atoi(argv[1]);
		(*S) = atoi(argv[2]);
		strcpy(algorithmName, argv[3]);
		(*fd) = atoi(argv[4]);
	}
	
	if((*D) <= 0){
		printf("Manager: Argument Error - Simulation Duration Time has to be a positive number!\n");
	}
	else{
		if((*S) <= 0){
			printf("Manager: Argument Error - Please provide a Memory Size (KB) that is a positive number!\n");
		}
		else{
			printf("\nManager: Received Arguments: |D: %d| - |S: %d|\n", *D, *S);
			if(!strcmp(algorithmName, "NF")){
				*algorithmChoice = NEXT_FIT;
				printf("\nManager: Received Arguments: |D: %d| - |S: %d| - |NEXT_FIT|\n", *D, *S);
				return 0;
			}
			else if(!strcmp(algorithmName, "BF")){
				*algorithmChoice = BEST_FIT;
				printf("\nManager: Received Arguments: |D: %d| - |S: %d| - |BEST_FIT|\n", *D, *S);
				return 0;
			}
			else if(!strcmp(algorithmName, "BD")){
				*algorithmChoice = BUDDY;
				printf("\nManager: Received Arguments: |D: %d| - |S: %d| - |BUDDY|\n", *D, *S);
				return 0;
			}
			else{
				printf("Manager: Invalid algorithm Choice please try again!\n");
			}
		}
	}

	printf("Manager: If you wish to run the program with arguments run it like this: \n");
	printf("Manager: ./Manager <Simulation_Duration> <S> <BD/NF/BF>\n\n");
	printf("Manager: TIP: You can also run the program without any arguments!\n\n");
	exit(1);

}

/*========================Preparing Manager Functions====================*/

int setupManager(int* fd, int* shmid_Ptr, int* semidQueues_Ptr, int* semidSync_Ptr, SharedMemorySegment** shmPtr, ProcessQueue* waitQueue, ProcessQueue* statsQueue, Memory* memoryPtr, int D, int S){

	char buffer[256];
	if(waitQueue == NULL){
		printf("Manager - setupManager: Queue Pointer is NULL!\n");
		exit(1);
	}

	printf("Manager: Attaching to Shared Memory...\n");

	attachToSharedMemoryArea(SHMKEY_GEN_MEM, shmid_Ptr, shmPtr);

	printf("Manager: Getting Queue Semaphores...\n");

	getSemaphores(SEMKEY_QUEUES, semidQueues_Ptr , NSEMS_QUEUES);

	printf("Manager: Getting Sync Semaphores...\n");

	getSemaphores(SEMKEY_SYNC, semidSync_Ptr , NSEMS_SYNC);

	printf("Manager: Creating WaitList...\n");

	createList_QueueList(waitQueue);
		
	createList_QueueList(statsQueue);

	printf("Manager: Creating Memory...\n");

	createMemory(memoryPtr, S);

	return 0;

}

/*========================Main Manager Functions=========================*/

int attemptToPlace(int currentTime, int algorithmChoice, Message** processMessagePtr, Message** confirmMessagePtr, Memory* memoryPtr, ProcessQueue* waitQueuePtr,int op, int semid_queues, SharedMemorySegment* shmPtr, int fd){

	int rv = 0;
	char buffer[256];

	if(algorithmChoice == NEXT_FIT){
		rv = nextFit_algorithm(memoryPtr, (*processMessagePtr)->process);
	}
	else if(algorithmChoice == BEST_FIT){
		rv = bestFit_algorithm(memoryPtr, (*processMessagePtr)->process);
	}
	else if(algorithmChoice == BUDDY){
		printf("Manager - waitForArrivals - |Time: %d|: BUDDY is not ready!\n", currentTime);
		exit(1);
	}
	else{
		printf("Manager - waitForArrivals - |Time: %d|: Invalid Algorithm Choice!\n", currentTime);
		exit(1);
	}

	if(rv == 0){
		printf("Manager - waitForArrivals - |Time: %d|: Not enough memory for Process |PID: %d|! Placing in waitList!\n", currentTime, (*processMessagePtr)->process.pid);
		addLastList_QueueList(waitQueuePtr, &((*processMessagePtr)->process));
		(*confirmMessagePtr) = createMessage(CONFIRM, WAITING, &((*processMessagePtr)->process));
		if((*confirmMessagePtr) == NULL){
			printf("Manager - waitForArrivals - |Time: %d|: confirmMessagePtr is NULL!\n", currentTime);
			exit(1);
		}
		addToSharedBuffer("Manager", (*confirmMessagePtr), &(shmPtr->memGenQueue), semid_queues, MEMGEN_SEM_MUTEX, MEMGEN_SEM_EMPTY, MEMGEN_SEM_FULL);
		if((*confirmMessagePtr) != NULL){
			free((*confirmMessagePtr));
			(*confirmMessagePtr) = (Message*) NULL;
		}

		if(op == VP_START)
			sprintf(buffer, "Process with PID: %d just arrived (VP_START)! Placed in waitingList - not enough space!\n", (*processMessagePtr)->process.pid);
		else if(op == VP_SWAP)
			sprintf(buffer, "Process with PID: %d re-arrived (VP_SWAP)! Placed in waitingList - not enough space!\n", (*processMessagePtr)->process.pid);

		write(fd,buffer,sizeof(char)*(strlen(buffer)));
	}
	else if(rv == 1){
		printf("Manager - waitForArrivals - |Time: %d|: Placed |PID: %d| successfully!\n", currentTime, (*processMessagePtr)->process.pid);
		(*confirmMessagePtr) = createMessage(CONFIRM, RUNNING, &((*processMessagePtr)->process));
		if((*confirmMessagePtr) == NULL){
			printf("Manager - waitForArrivals - |Time: %d|: confirmMessagePtr is NULL!\n", currentTime);
			exit(1);
		}
		addToSharedBuffer("Manager", (*confirmMessagePtr), &(shmPtr->memGenQueue), semid_queues, MEMGEN_SEM_MUTEX, MEMGEN_SEM_EMPTY, MEMGEN_SEM_FULL);
		if((*confirmMessagePtr) != NULL){
			free((*confirmMessagePtr));
			(*confirmMessagePtr) = (Message*) NULL;
		}					

		if(op == VP_START)
			sprintf(buffer, "Process with PID: %d just arrived (VP_START)! Placed successfully!\n", (*processMessagePtr)->process.pid);
		else if(op == VP_SWAP)
			sprintf(buffer, "Process with PID: %d re-arrived (VP_SWAP)! Placed successfully!\n", (*processMessagePtr)->process.pid);

		write(fd,buffer,sizeof(char)*(strlen(buffer)));

	}
	else{
		printf("Manager - waitForArrivals - |Time: %d|: Algorithm encountered an error!\n", currentTime);
		exit(1);
	}

	return rv;

}

int waitForArrivals(int fd, int currentTime, int semid_queues, SharedMemorySegment* shmPtr, ProcessQueue* waitQueuePtr, ProcessQueue* statsQueue, Memory* memoryPtr,int algorithmChoice){

	int rv = 0;
	static int latestArrival = 0;
	char buffer[256];
	Process* process_ptr = (Process*) NULL;
	Message* confirmMessagePtr = (Message*) NULL;
	Message* processMessagePtr = (Message*) NULL;

	if(waitQueuePtr == NULL){
		printf("Manager - waitForArrivals - |Time: %d|: waitQueuePtr is NULL!\n", currentTime);
		exit(1);
	}

	if(memoryPtr == NULL){
		printf("Manager - waitForArrivals - |Time: %d|: memoryPtr is NULL!\n", currentTime);
		exit(1);
	}

	/*-----Wait for New Arrivals from Generator----*/
	printf("Manager - waitForArrivals - |Time: %d|: Waiting for new processes to arrive from Generator...\n", currentTime);
	do{ //Wait for new Processes until END_EVENTS flag arrives
		//Attempt to place any new process that arrives if no other Processes already wait
		processMessagePtr = takeFromSharedBuffer("Manager",&(shmPtr->genMemQueue), semid_queues, GENMEM_SEM_MUTEX, GENMEM_SEM_EMPTY, GENMEM_SEM_FULL);
		if(processMessagePtr == NULL){
			printf("Manager - waitForArrivals - |Time: %d|: waitQueuePtr is NULL!\n", currentTime);
			exit(1);		
		}
		if(processMessagePtr->type == CONFIRM){ //Generator is notifying us to stop waiting for Processes
			if(processMessagePtr->value == END_EVENTS){
				printf("Manager - waitForArrivals - |Time: %d|: Received END_EVENTS - Not waiting for anymore processes!\n", currentTime);
				free(processMessagePtr);
				processMessagePtr = (Message*) NULL;
				break;
			}
			else{
				printf("Manager - waitForArrivals - |Time: %d|: Unexpected CONFIRM Message!\n", currentTime);
				exit(1);	
			}
		}
		else if(processMessagePtr->type == PROCESS_OP){ //Message contains NEW Process from Generator
			if(processMessagePtr->value == VP_START){
				printf("Manager - waitForArrivals - |Time: %d|: Received NEW Process with |PID: %d|!\n", currentTime, processMessagePtr->process.pid);
				processMessagePtr->process.arrivedInManager = currentTime;
				latestArrival = processMessagePtr->process.pid;
				addLastList_QueueList(statsQueue, &(processMessagePtr->process));
				attemptToPlace(currentTime, algorithmChoice, &processMessagePtr, &confirmMessagePtr, memoryPtr, waitQueuePtr, VP_START, semid_queues, shmPtr, fd);
			}
			else{
				printf("Manager - waitForArrivals - |Time: %d|: Unexpected Process OPERATION - Expected VP_START!\n", currentTime);
				exit(1);	
			}
		}
		else{
			printf("Manager - waitForArrivals - |Time: %d|: Unexpected TYPE of Message!\n", currentTime);
			exit(1);	
		}

		if(processMessagePtr != NULL){
			free(processMessagePtr);
			processMessagePtr = (Message*) NULL;
		}
	}while(1);

	return 0;

}

int updateStatsQueue(ProcessQueue* statsQueue, Process process){

	QueueNode* tempNode = (QueueNode*) NULL;
	Process* process_ptr = (Process*) NULL;

	if(statsQueue == NULL){
		printf("updateStatsQueue: Queue is NULL!\n");
		exit(1);
	}
	
	tempNode = statsQueue->Head;
	while(tempNode != NULL){
		process_ptr = tempNode->process_ptr;
		if(process_ptr == NULL){
			printf("updateStatsQueue: Process_Ptr is NULL!\n");
			exit(1);
		}
		if(process_ptr->pid == process.pid){
			printf("Updating totalWaitTime of PID: %d with waitTime: %d\n", process.pid, process.totalWaitTime);
			process_ptr->totalWaitTime = process.totalWaitTime;
		}
		tempNode = tempNode->next;
	}

}


double averageWaitTime(ProcessQueue* statsQueue){

	long totalWaitTime = 0;
	QueueNode* tempNode = (QueueNode*) NULL;
	Process* process_ptr = (Process*) NULL;

	if(statsQueue == NULL){
		printf("averageWaitTime: Queue is NULL!\n");
		exit(1);
	}
	
	tempNode = statsQueue->Head;
	while(tempNode != NULL){
		process_ptr = tempNode->process_ptr;
		if(process_ptr == NULL){
			printf("averageWaitTime: Process_Ptr is NULL!\n");
			exit(1);
		}
		printf("|PID: %d| - |totalWaitTime: %d|\n", process_ptr->pid, process_ptr->totalWaitTime);
		totalWaitTime = totalWaitTime + tempNode->process_ptr->totalWaitTime;
		tempNode = tempNode->next;
	}
	
	return (double) totalWaitTime / statsQueue->items;
}

int executeTimeSlot(int fd, int currentTime, int semid_queues, SharedMemorySegment* shmPtr, ProcessQueue* waitQueuePtr, ProcessQueue* statsQueue, Memory* memoryPtr,int algorithmChoice){

	char buffer[256];
	int waitTime = 0;
	int rv = 0;
	Process* process_ptr = (Process*) NULL;
	Message* confirmMessagePtr = (Message*) NULL;
	Message* processMessagePtr = (Message*) NULL;
	QueueNode* processNode = (QueueNode*) NULL;

	if(waitQueuePtr == NULL){
		printf("Manager - executeTimeSlot - |Time: %d|: waitQueuePtr is NULL!\n", currentTime);
		exit(1);
	}

	if(memoryPtr == NULL){
		printf("Manager - executeTimeSlot - |Time: %d|: memoryPtr is NULL!\n", currentTime);
		exit(1);
	}

	/*-----Wait for New Events from Generator----*/
	printf("Manager - executeTimeSlot - |Time: %d|: Waiting for new events (VP_SWAP/VP_STOP) from Generator...\n", currentTime);
	do{ 
		processMessagePtr = takeFromSharedBuffer("Manager",&(shmPtr->genMemQueue), semid_queues, GENMEM_SEM_MUTEX, GENMEM_SEM_EMPTY, GENMEM_SEM_FULL);
		if(processMessagePtr == NULL){
			printf("Manager - executeTimeSlot - |Time: %d|: waitQueuePtr is NULL!\n", currentTime);
			exit(1);		
		}
		if(processMessagePtr->type == CONFIRM){ //Generator is notifying us to stop waiting for VP_SWAP/STOP
			if(processMessagePtr->value == END_EVENTS){
				printf("Manager - executeTimeSlot - |Time: %d|: Received END_EVENTS - Not waiting for anymore events!\n", currentTime);
				free(processMessagePtr);
				processMessagePtr = (Message*) NULL;
				break;
			}
			else{
				printf("Manager - executeTimeSlot - |Time: %d|: Unexpected CONFIRM Message!\n", currentTime);
				exit(1);	
			}
		}
		else if(processMessagePtr->type == PROCESS_OP){ //Message contains VP_SWAP/VP_STOP
			if((processMessagePtr->value == VP_SWAP) || (processMessagePtr->value == VP_STOP)){
				printf("Manager - executeTimeSlot - |Time: %d|: Received VP_SWAP/VP_STOP for |PID: %d|!\n", currentTime, processMessagePtr->process.pid);
				rv = removeProcessFromMemory(memoryPtr, processMessagePtr->process.pid, &waitTime);
				if(rv == 1){
					printf("Manager - executeTimeSlot - |Time: %d|: Process located and removed!\n", currentTime);
					processMessagePtr->process.totalWaitTime = waitTime;
					updateStatsQueue(statsQueue, processMessagePtr->process);
					confirmMessagePtr = createMessage(CONFIRM, OK, &(processMessagePtr->process));
					if(confirmMessagePtr == NULL){
						printf("Manager - executeTimeSlot - |Time: %d|: confirmMessagePtr is NULL!\n", currentTime);
						exit(1);
					}
					addToSharedBuffer("Manager", confirmMessagePtr, &(shmPtr->memGenQueue), semid_queues, MEMGEN_SEM_MUTEX, MEMGEN_SEM_EMPTY, MEMGEN_SEM_FULL);
					if(confirmMessagePtr != NULL){
						free(confirmMessagePtr);
						confirmMessagePtr = (Message*) NULL;
					}
					if(processMessagePtr->value == VP_SWAP){
						sprintf(buffer, "Process with PID: %d removed from memory (VP_SWAP)! (Remaing Bursts: %d)\n", processMessagePtr->process.pid, processMessagePtr->process.burstsLeft);
						write(fd,buffer,sizeof(char)*(strlen(buffer)));

						free(processMessagePtr);
						processMessagePtr = (Message*) NULL;

						/*----Waiting for generator to resend process in attempt to place again---*/
						processMessagePtr = takeFromSharedBuffer("Manager",&(shmPtr->genMemQueue), semid_queues, GENMEM_SEM_MUTEX, GENMEM_SEM_EMPTY, GENMEM_SEM_FULL);
						if(processMessagePtr == NULL){
							printf("Manager - executeTimeSlot - |Time: %d|: processMessagePtr is NULL!\n", currentTime);
							exit(1);		
						}
						if(processMessagePtr->type != PROCESS_OP){
							printf("Manager - executeTimeSlot - |Time: %d|: Expected VP_SWAP for re-placement!\n", currentTime);
							exit(1);	
						}
						
						processMessagePtr->process.arrivedInManager = currentTime;
						attemptToPlace(currentTime, algorithmChoice, &processMessagePtr, &confirmMessagePtr, memoryPtr, waitQueuePtr, VP_SWAP, semid_queues, shmPtr, fd);
					}
					else{
						sprintf(buffer, "Process with PID: %d removed from memory (VP_STOP)! (Remaining Bursts %d)\n", processMessagePtr->process.pid, processMessagePtr->process.burstsLeft);
						write(fd,buffer,sizeof(char)*(strlen(buffer)));	
						free(processMessagePtr);
						processMessagePtr = (Message*) NULL;			
					}
				}
				else if(rv == 0){
					printf("Manager - executeTimeSlot - |Time: %d|: Process not found in memory!\n", currentTime);
					confirmMessagePtr = createMessage(CONFIRM, FAIL, NULL);
					if(confirmMessagePtr == NULL){
						printf("Manager - executeTimeSlot - |Time: %d|: confirmMessagePtr is NULL!\n", currentTime);
						exit(1);
					}
					addToSharedBuffer("Manager", confirmMessagePtr, &(shmPtr->memGenQueue), semid_queues, MEMGEN_SEM_MUTEX, MEMGEN_SEM_EMPTY, MEMGEN_SEM_FULL);
					if(confirmMessagePtr != NULL){
						free(confirmMessagePtr);
						confirmMessagePtr = (Message*) NULL;
					}
					exit(1);
				}
				else{
					printf("Manager - executeTimeSlot - |Time: %d|: removeProcessFromMemory failed!\n", currentTime);
					exit(1);	
				}
			}
			else{
				printf("Manager - executeTimeSlot - |Time: %d|: Unexpected Process OPERATION - Expected VP_STOP/VP_SWAP!\n", currentTime);
				exit(1);	
			}
		}
		else{
			printf("Manager - executeTimeSlot - |Time: %d|: Unexpected TYPE of Message!\n", currentTime);
			exit(1);	
		}

		if(processMessagePtr != NULL){
			free(processMessagePtr);
			processMessagePtr = (Message*) NULL;
		}
	}while(1);
	
	/*----Attempt to place as much Waiting Processes as possible in Memory from Waiting Queue----*/
	if(waitQueuePtr->items > 0){
		printf("Manager - executeTimeSlot - |Time: %d|: Processes exist in waitQueue - check if we can place them!\n", currentTime);
		printf("Manager - Printing WaitingList...\n");
		printList_QueueList(*waitQueuePtr, -1, -1);
		processNode = waitQueuePtr->Head;
		while(processNode != NULL){
			process_ptr = processNode->process_ptr;
			if(process_ptr == NULL){
				printf("Manager - executeTimeSlot - |Time: %d|: process_ptr is NULL!\n", currentTime);
				exit(1);	
			}
			
			if(algorithmChoice == NEXT_FIT){
				rv = nextFit_algorithm(memoryPtr, *process_ptr);
			}
			else if(algorithmChoice == BEST_FIT){
				rv = bestFit_algorithm(memoryPtr, *process_ptr);
			}
			else if(algorithmChoice == BUDDY){
				printf("Manager - executeTimeSlot - |Time: %d|: BUDDY is not ready!\n", currentTime);
				exit(1);
			}
			else{
				printf("Manager - executeTimeSlot - |Time: %d|: Invalid Algorithm Choice!\n", currentTime);
				exit(1);
			}

			if(rv == 0){
				printf("Manager - executeTimeSlot - |Time: %d|: Not enough memory for Process |PID: %d|! Staying back in waitingList!\nNot checking rest of the List!", currentTime, process_ptr->pid);
				break;
			}
			else if(rv == 1){
				printf("Manager - executeTimeSlot - |Time: %d|: Placed |PID: %d| successfully!\n", currentTime, process_ptr->pid);
				sprintf(buffer, "Process with PID: %d was moved from waitList to Memory!\n", process_ptr->pid);
				write(fd,buffer,sizeof(char)*(strlen(buffer)));
				confirmMessagePtr = createMessage(PROCESS, RUNNING, process_ptr);
				if(confirmMessagePtr == NULL){
					printf("Manager - executeTimeSlot - |Time: %d|: confirmMessagePtr is NULL!\n", currentTime);
					exit(1);
				}
				addToSharedBuffer("Manager", confirmMessagePtr, &(shmPtr->memGenQueue), semid_queues, MEMGEN_SEM_MUTEX, MEMGEN_SEM_EMPTY, MEMGEN_SEM_FULL);
				if(confirmMessagePtr != NULL){
					free(confirmMessagePtr);
					confirmMessagePtr = (Message*) NULL;
				}
				processNode = processNode->next;
				popList_QueueList(waitQueuePtr);
				continue;					
			}
			else{
				printf("Manager - executeTimeSlot - |Time: %d|: BEST_FIT encountered an error!\n", currentTime);
				exit(1);
			}
	

			processNode = processNode->next;
		}

	}

	/*----Send End Events Message----*/
	confirmMessagePtr = (Message*) NULL;
	confirmMessagePtr = createMessage(CONFIRM, END_EVENTS, NULL);
	if(confirmMessagePtr == NULL){
		printf("Manager - executeTimeSlot - |Time: %d|: Failed to allocate memory for END_EVENTS Message!\n", currentTime);
		exit(1);		
	}
	addToSharedBuffer("Manager", confirmMessagePtr, &(shmPtr->memGenQueue), semid_queues, MEMGEN_SEM_MUTEX, MEMGEN_SEM_EMPTY, MEMGEN_SEM_FULL);
	free(confirmMessagePtr);

	return 0;
}

int syncTimeSlot(char* processName, int semid_sync, int* timeSlot,int mySem, int oppositeSem){ //Used to ensure the processes will move together to the next timeslot

	struct sembuf semaphoreOperation;

	/*---------------Notify the other Process timeSlot has finished----------------*/
	semaphoreOperation.sem_num = oppositeSem;
	semaphoreOperation.sem_op = 1;
	semaphoreOperation.sem_flg = 0;

	if(semop(semid_sync, &semaphoreOperation, 1) == -1){
		printf("%s - syncTimeSlot: Failed to UP Semaphore!\n",processName);
		fflush(stdout);
		return -1;
	}	

	/*---------------Wait for notification from the other Process that timeSlot has finished----------------*/
	semaphoreOperation.sem_num = mySem;
	semaphoreOperation.sem_op = -1;
	semaphoreOperation.sem_flg = 0;

	if(semop(semid_sync, &semaphoreOperation, 1) == -1){
		printf("%s - syncTimeSlot: Failed to DOWN Semaphore!\n", processName);
		fflush(stdout);
		return -1;
	}	

	(*timeSlot)++;

	return 0;

}

double averageFreeBlockSize(BlockList blockList){

	int count = 0;
	int sum = 0;

	if(blockList.free_blocks == 0) return 0;
	else{
		BlockNode* tempBlockNode = (BlockNode*) NULL;
		Block* tempBlock = (Block*) NULL;
		
		tempBlockNode = blockList.Head;
		while(tempBlockNode != NULL){
			tempBlock = tempBlockNode->block_ptr;
			if(tempBlock == NULL){
				printf("averageFreeBlockSize: NULL!");
				exit(1);
			}
			if(tempBlock->state == FREE){
				count++;
				sum = sum + tempBlock->size;
			}
			tempBlockNode = tempBlockNode->next;
		}
	}

	return ((double) sum/count);

}

int increaseWaitTimes(ProcessQueue* waitQueuePtr, int currentTime){

	QueueNode* tempNode = (QueueNode*) NULL;
	Process* process_ptr = (Process*) NULL;

	if(waitQueuePtr == NULL){
		printf("increaseWaitTimes: Queue is NULL!\n");
		exit(1);
	}
	
	tempNode = waitQueuePtr->Head;
	while(tempNode != NULL){
		process_ptr = tempNode->process_ptr;
		if(process_ptr == NULL){
			printf("increaseWaitTimes: Process_Ptr is NULL!\n");
			exit(1);
		}
		if(process_ptr->arrivedInManager < currentTime){
			process_ptr->totalWaitTime++;
		}
		tempNode = tempNode->next;
	}

}

int beginSimulation(int fd, int D, int semid_queues, int semid_sync, SharedMemorySegment* shmPtr, ProcessQueue* waitQueuePtr, ProcessQueue* statsQueue,Memory* memoryPtr,int algorithmChoice){

	int timeSlot = 0;
	char buffer[256];
	double averageBlockSizeTotal = 0.0;
	double averageBlockSize = 0.0;
	double waitTime = 0.0;

	memoryPtr->statistic_totalMemUsage = 0;
	memoryPtr->product_MemTime = 1;

	while(timeSlot < D){ //Time D is divided in equal timeslots
		printf("=======================================TIMESLOT %d==============================================\n", timeSlot);
		sprintf(buffer, "\n----------------------------------TIMESLOT %d-----------------------------------------\n", timeSlot);
		write(fd,buffer,sizeof(char)*(strlen(buffer)));

		waitForArrivals(fd, timeSlot, semid_queues, shmPtr, waitQueuePtr, statsQueue, memoryPtr, algorithmChoice);
		executeTimeSlot(fd, timeSlot, semid_queues, shmPtr, waitQueuePtr, statsQueue, memoryPtr, algorithmChoice);

		sprintf(buffer, "\n---------waitList (processes: %d)---------\n", waitQueuePtr->items);
		write(fd,buffer,sizeof(char)*(strlen(buffer)));
		printList_QueueList(*waitQueuePtr, fd, 1);

		sprintf(buffer, "\n---------Current Memory State (blocks: %d)---------\n", memoryPtr->blockList.items);
		write(fd,buffer,sizeof(char)*(strlen(buffer)));
		//printf("\n---------Current Memory State (blocks: %d)---------\n", memoryPtr->blockList.items);
		printList_BlockList(memoryPtr->blockList, fd, 1);

		sprintf(buffer, "\nTotal Memory: %dKB - Used: %dKB - Free: %dKB - Taken Blocks: %d - Free Blocks: %d\n", memoryPtr->size, memoryPtr->blockList.totalMemUsage, memoryPtr->blockList.totalFreeMem, memoryPtr->blockList.taken_blocks, memoryPtr->blockList.free_blocks);
		write(fd,buffer,sizeof(char)*(strlen(buffer)));

		/*---Calculate time-memory product---*/
		memoryPtr->statistic_totalMemUsage = memoryPtr->statistic_totalMemUsage + memoryPtr->blockList.totalMemUsage;

		memoryPtr->product_MemTime = (timeSlot + 1) * memoryPtr->statistic_totalMemUsage;
		sprintf(buffer, "\nCurrent Memory-Time Product: %ld\n", memoryPtr->product_MemTime);
		write(fd,buffer,sizeof(char)*(strlen(buffer)));	
	
		/*---Calculate average free block size in this timeSlot---*/
		averageBlockSize = averageFreeBlockSize(memoryPtr->blockList);
		sprintf(buffer, "Current Average FREE Block Size: %lf\n", averageBlockSize);
		write(fd,buffer,sizeof(char)*(strlen(buffer)));	
		
		averageBlockSizeTotal = averageBlockSizeTotal + averageBlockSize;

		/*----Increase waitTime for Processes waiting----*/
		increaseWaitTimes(waitQueuePtr, timeSlot);

		//printf("\nTotal Memory: %dKB - Used: %dKB - Free: %dKB - Taken Blocks: %d - Free Blocks: %d\n", memoryPtr->size, memoryPtr->blockList.totalMemUsage, memoryPtr->blockList.totalFreeMem, memoryPtr->blockList.taken_blocks, memoryPtr->blockList.free_blocks);	
		syncTimeSlot("Manager", semid_sync, &timeSlot , MEMSYNC_SEM, GENSYNC_SEM); //Ensures that the two processes move to next timeslot at the same time
	}

	sprintf(buffer, "\n\n=================================END OF SIMULATION====================================\n\n-------Statistics: -------\n");
	write(fd,buffer,sizeof(char)*(strlen(buffer)));

	if(algorithmChoice == NEXT_FIT){
		sprintf(buffer, "Algorithm Used: Next-Fit\n");
		write(fd,buffer,sizeof(char)*(strlen(buffer)));
	}
	else if(algorithmChoice == BEST_FIT){
		sprintf(buffer, "Algorithm Used: Best-Fit\n");
		write(fd,buffer,sizeof(char)*(strlen(buffer)));
	}

	/*---Calculate average free block size in this timeSlot---*/
	sprintf(buffer, "Average FREE Block Size: %lf\n", averageBlockSizeTotal / D);
	write(fd,buffer,sizeof(char)*(strlen(buffer)));	

	/*---Print-Time Memory Product---*/
	sprintf(buffer, "Memory-Time Product: %ld\n", memoryPtr->product_MemTime);
	write(fd,buffer,sizeof(char)*(strlen(buffer)));	

	/*---Print Average Wait Time for a Process---*/
	sprintf(buffer, "Total Number of Processes: %d\n", statsQueue->items);
	write(fd,buffer,sizeof(char)*(strlen(buffer)));
	waitTime = averageWaitTime(statsQueue);
	sprintf(buffer, "Average Process Wait Time: %lf\n", waitTime);
	write(fd,buffer,sizeof(char)*(strlen(buffer)));	

	printf("\nManager: Simulation has finished!!\n");

	return 0;

}

int tearDownManager(SharedMemorySegment* shmPtr, ProcessQueue* waitQueue, ProcessQueue* statsQueue, Memory* memoryPtr){

	if(waitQueue == NULL){
		printf("Manager - tearDownManager: Queue Pointer is NULL!\n");
		exit(1);
	}

	printf("Manager - tearDownManager: Detaching from Shared Memory...\n");

	detachSharedMemory(shmPtr);

	printf("Manager - tearDownManager: Destroying List...\n");

	destroyList_QueueList(waitQueue);
	
	destroyList_QueueList(statsQueue);

	if(memoryPtr != NULL){
		printf("Manager - tearDownManager: Destroying Memory...\n");
		destroyList_BlockList(&(memoryPtr->blockList));
	}

	return 0;

}
