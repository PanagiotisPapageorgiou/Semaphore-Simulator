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
#include "shared_structs.h"

/*==================================Distribution functions==============================*/

// CREDIT: Code for Distribution functions was freely given on pamvotis.org
double Constant (double mean)
{
	return mean;
}

int uniform_distribution(int rangeLow, int rangeHigh){
	double myRand = rand()/(1.0 + RAND_MAX);
	int range = rangeHigh - rangeLow;
	double myRand_scaled = (myRand * range) + rangeLow;
	return round(myRand_scaled);
}

double Exponential(double mean)
{
	//Generate a random number between 0 and 1.
	//REMEMBER: Always cast the oparands of a division to double, or truncation will be performed.
	double R;
	R = (double)rand()/(double)(RAND_MAX+1.0);
	
	//Put it as a parameter to the inverse distribution function.
	//if(-mean*log(R) < 0){
		//return round((-1)*(-mean*log(R)));
	//}
	return round(-mean*log(R));
}

/*==================Creating and initialising a Process=====================*/

Process* createProcess(int pid, int arrival_time, int lo, int hi, float T){

	Process* process_ptr = (Process*) NULL;
	int k;

	process_ptr = malloc(sizeof(Process));
	if(process_ptr == NULL){
		printf("createProcess: Error! Failed to allocate memory for process!\n");
		return (Process*) NULL;
	}

	process_ptr->pid = pid;
	process_ptr->arrival_time = arrival_time;
	process_ptr->size = uniform_distribution(lo, hi);
	k = rand() % 18 + 2; //k is between [2,20]
	process_ptr->bursts = k; //How many times it needs to use the memory
	process_ptr->state = UNBORN;
	process_ptr->burstTimeLeft = (int) Exponential(T);
	if(process_ptr->burstTimeLeft == 0) process_ptr->burstTimeLeft++;
	process_ptr->burstsLeft = process_ptr->bursts;
	process_ptr->burstOpsLeft = 2*k + 1;
	process_ptr->currentWaitTime = 0;
	process_ptr->totalWaitTime = 0;
	process_ptr->arrivedInManager = 0;

	return process_ptr;

}

int createNextBurstTime(Process* process_ptr, float T){

	if(process_ptr == NULL){
		printf("createNextBurstTime: process_ptr is NULL!\n");
		return -1;
	}

	process_ptr->burstTimeLeft = (int) Exponential(T);
	if(process_ptr->burstTimeLeft == 0) process_ptr->burstTimeLeft++;

}

int assignProcess(Process* target, Process source){ //Handles assignment

	if(target == NULL){
		printf("assignProcess: target is NULL!\n");
		return -1;
	}

	target->pid = source.pid;
	target->arrival_time = source.arrival_time;
	target->size = source.size;
	target->bursts = source.bursts;
	target->state = source.state;
	target->burstTimeLeft = source.burstTimeLeft;
	target->burstsLeft = source.burstsLeft;
	target->currentWaitTime = source.currentWaitTime;
	target->totalWaitTime = source.totalWaitTime;
	target->burstOpsLeft = source.burstOpsLeft;
	target->arrivedInManager = source.arrivedInManager;

	return 0;
	
}

Process* createCopyProcess(Process* originalProcess){

	Process* newProcess = (Process*) NULL;

	if(originalProcess == NULL){
		printf("createCopyProcess: originalProcess is NULL!\n");
		return (Process*) NULL;
	}

	newProcess = malloc(sizeof(Process));
	if(newProcess == NULL){
		printf("createCopyProcess: newProcess is NULL!\n");
		return (Process*) NULL;		
	}

	assignProcess(newProcess, *originalProcess);

	return newProcess;
	
}

/*================================ Create and initialise a Message =========================================*/
Message* createMessage(int type,int value,Process* process){

	Message* message_ptr = (Message*) NULL;
	message_ptr = malloc(sizeof(Message));
	
	if(message_ptr == NULL){
		printf("createMessage: message_ptr is NULL!\n");
		return (Message*) NULL;
	}

	message_ptr->type = type;
	message_ptr->value = value;
	
	if(process != NULL){
		assignProcess(&(message_ptr->process), *process);
	}

	return message_ptr;

}

int assignMessage(Message* target, Message source){ //Handles assignment

	if(target == NULL){
		printf("assignMessage: target is NULL!\n");
		return -1;
	}

	target->type = source.type;
	target->value = source.value;
	assignProcess(&(target->process), source.process);

	return 0;
	
}

Message* createCopyMessage(Message* originalMessage){

	Message* newMessage = (Message*) NULL;

	if(originalMessage == NULL){
		printf("createCopyMessage: originalMessage is NULL!\n");
		return (Message*) NULL;
	}

	newMessage = malloc(sizeof(Message));
	if(newMessage == NULL){
		printf("createCopyMessage: newProcess is NULL!\n");
		return (Message*) NULL;	
	}

	assignMessage(newMessage, *originalMessage);

	return newMessage;
	
}

/*================================ Process Queue Linked List Implementation ================================*/

void createList_QueueList(ProcessQueue* queueListPtr){

	queueListPtr->items = 0;
	queueListPtr->Head = (QueueNode*) NULL;
	queueListPtr->Current = (QueueNode*) NULL;
	queueListPtr->Last = (QueueNode*) NULL;

	return;
}

int isEmpty_QueueList(ProcessQueue queueList){

	return (queueList.items == 0);

}

Process* frontList_QueueList(ProcessQueue queueList){ /* Returns a pointer to the Data of the first Node */

	if(queueList.items > 0)
		return createCopyProcess(queueList.Head->process_ptr);
	else
		return (Process*) NULL;

}

int addLastList_QueueList(ProcessQueue* queueListPtr, Process* process_ptr){ /* Adds a Node to the end of the List */

	int i=0;

	if(queueListPtr->items == 0){ /* No items yet */

		queueListPtr->Head = malloc(sizeof(QueueNode));
		if(queueListPtr->Head == NULL){
			printf("addLastList_QueueList: Failed to allocate memory for Head QueueNode!\n");
			return -1;
		}

		queueListPtr->Head->process_ptr = createCopyProcess(process_ptr);
		if(queueListPtr->Head->process_ptr == NULL){
			printf("addLastList_QueueList: Failed to allocate memory for Process!\n");
			return -1;
		}

		queueListPtr->Head->next = (QueueNode*) NULL;
		queueListPtr->Head->previous = (QueueNode*) NULL;

		queueListPtr->Current = queueListPtr->Head;
		queueListPtr->Last = queueListPtr->Head;

		(queueListPtr->items)++;

		return 0;
	}
	else if(queueListPtr->items > 0){

		queueListPtr->Last->next = malloc(sizeof(QueueNode));
		if(queueListPtr->Last->next == NULL){
			printf("addLastList_QueueList: Failed to allocate memory for Last QueueNode!\n");
			return -1;
		}

		queueListPtr->Last->next->previous = queueListPtr->Last;
		queueListPtr->Last = queueListPtr->Last->next;
		queueListPtr->Last->next = (QueueNode*) NULL;

		queueListPtr->Last->process_ptr = createCopyProcess(process_ptr);
		if(queueListPtr->Last->process_ptr == NULL){
			printf("addLastList_QueueList: Failed to allocate memory for Process!\n");
			return -1;
		}	

		(queueListPtr->items)++;

		queueListPtr->Current = queueListPtr->Last;


		return 0;
	}
	else{
		printf("addLastList_QueueList: List is not properly initialised...\n");
		return -1;
	}

}

void printList_QueueList(ProcessQueue queueList,int fd,int mode){ /* Prints List */

	int i=0;
	char buffer[256];

	if(queueList.items > 0){
		queueList.Current = queueList.Head;
		while(queueList.Current->next != NULL){
			printf("|%d| - |PID: %d| - |Arrival: %d| - |Size: %d| - |BurstsLeft: %d| - |totalWaitTime: %d|\n",i+1,queueList.Current->process_ptr->pid, queueList.Current->process_ptr->arrival_time, queueList.Current->process_ptr->size, queueList.Current->process_ptr->burstsLeft,queueList.Current->process_ptr->totalWaitTime); 
			if(mode == 1){
				sprintf(buffer, "|%d| - |PID: %d| - |Arrival: %d| - |Size: %d| - |BurstsLeft: %d| - |totalWaitTime: %d|\n",i+1,queueList.Current->process_ptr->pid, queueList.Current->process_ptr->arrival_time, queueList.Current->process_ptr->size, queueList.Current->process_ptr->burstsLeft, queueList.Current->process_ptr->totalWaitTime); 
				write(fd,buffer,sizeof(char)*(strlen(buffer)));			
			}
			queueList.Current = queueList.Current->next;
			i++;
		}

		printf("|%d| - |PID: %d| - |Arrival: %d| - |Size: %d| - |BurstsLeft: %d| - |totalWaitTime: %d|\n",i+1,queueList.Current->process_ptr->pid, queueList.Current->process_ptr->arrival_time, queueList.Current->process_ptr->size, queueList.Current->process_ptr->burstsLeft, queueList.Current->process_ptr->totalWaitTime);
		if(mode == 1){
			sprintf(buffer, "|%d| - |PID: %d| - |Arrival: %d| - |Size: %d| - |BurstsLeft: %d| - |totalWaitTime: %d|\n",i+1,queueList.Current->process_ptr->pid, queueList.Current->process_ptr->arrival_time, queueList.Current->process_ptr->size, queueList.Current->process_ptr->burstsLeft, queueList.Current->process_ptr->totalWaitTime); 
			write(fd,buffer,sizeof(char)*(strlen(buffer)));			
		}

	}
	else{
		printf("printList_QueueList: Nothing to print in List...\n");
		if(mode == 1){
			sprintf(buffer, "waitList is empty!"); 
			write(fd,buffer,sizeof(char)*(strlen(buffer)));			
		}
	}

}


void popList_QueueList(ProcessQueue* queueListPtr){ /* Removes 1 Node from the Start of the List */

	if(queueListPtr->items > 0){

		queueListPtr->Current = queueListPtr->Head;

		queueListPtr->Head = queueListPtr->Head->next; /* Make Head point to its next and make the previous of new Head be NULL */
		if(queueListPtr->Head != NULL)
			queueListPtr->Head->previous = (QueueNode*) NULL;
		
		if(queueListPtr->Current != NULL){
			if(queueListPtr->Current->process_ptr != NULL)
				free(queueListPtr->Current->process_ptr);
			free(queueListPtr->Current);
			(queueListPtr->items)--;
		}

		queueListPtr->Current = queueListPtr->Head;

	}

}

void destroyList_QueueList(ProcessQueue* queueListPtr){ /* Destroys List */

	if(queueListPtr == NULL) return;

	while(queueListPtr->items > 0) popList_QueueList(queueListPtr);

	return;
}

/*=============================================Message Array Queue Implementation=============================================*/

/*-----I have created a circular array implementation for the MessageQueue buffers of the Shared Memories----*/
/*----Credits for help: geekzquiz.com/queue-set-1introduction-and-array-implementation----*/

int init_arrayQueue(MessageQueue* queue_arrayPtr){

	int i=0;

	//printf("init_arrayQueue: Initialising New MessageQueueArray!\n");
	//fflush(stdout);

	if(queue_arrayPtr != NULL){
		return clear_arrayQueue(queue_arrayPtr);
	}
	else{
		printf("init_arrayQueue: Error! MessageQueueArray Pointer is NULL!\n");
		fflush(stdout);
		return -1;
	}

}

int clear_arrayQueue(MessageQueue* queue_arrayPtr){

	int i=0;

	queue_arrayPtr->head = 0;
	queue_arrayPtr->tail = QUEUE_ARRAY_SIZE - 1;
	queue_arrayPtr->items = 0;

	return 0;

}

int isEmpty_arrayQueue(MessageQueue queue_array){

	//printf("isEmpty_arrayQueue: Checking if queue is empty...\n");
	//fflush(stdout);
	return (queue_array.items == 0);

}

int isFull_arrayQueue(MessageQueue queue_array){

	//printf("isFull_arrayQueue: Checking if full...\n");
	//fflush(stdout);
	return (queue_array.items == QUEUE_ARRAY_SIZE);

}

int enQueue_arrayQueue(MessageQueue* queue_arrayPtr, Message message){

	//printf("enQueue_arrayQueue: Attempting to add new Message...\n");
	//fflush(stdout);

	if(queue_arrayPtr != NULL){
		if(isFull_arrayQueue(*queue_arrayPtr)){
			printf("enQueue_arrayQueue: Queue is FULL!!! SEMAPHORE ERROR!\n");
			exit(1);
		}
		else{

			//printQueue_arrayQueue(*queue_arrayPtr);

			queue_arrayPtr->tail = (queue_arrayPtr->tail + 1) % QUEUE_ARRAY_SIZE; /*----Tail must not exceed Queue boundaries (circular)----*/

			assignMessage(&(queue_arrayPtr->message_array[queue_arrayPtr->tail]), message);

			(queue_arrayPtr->items)++;

			//printf("enQueue_arrayQueue: Item added successfully!\n");
			//fflush(stdout);

			//printQueue_arrayQueue(*queue_arrayPtr);

			return 1;
		}
	}
	else{
		printf("enQueue_arrayQueue: queue_arrayPtr is NULL!\n");
		fflush(stdout);
		return -1;
	}

}

Message* deQueue_arrayQueue(MessageQueue* queue_arrayPtr){ /*---Remember to free the memory this function allocates---*/

	Message* message_ptr = (Message*) NULL;

	//printf("deQueue_arrayQueue: Attempting to remove MessagePacket from Queue...\n");
	//fflush(stdout);

	if(queue_arrayPtr == NULL){
		printf("deQueue_arrayQueue: queue_arrayPtr is NULL! NOOOOOOOOOOOOOOOOOOOOooo!\n");
		fflush(stdout);
		exit(1);
	}
	else{
		if(isEmpty_arrayQueue(*queue_arrayPtr) == 1){
			printf("deQueue_arrayQueue: Queue is Empty! SEMAPHORE ERROR\n");
			fflush(stdout);
			exit(1);
		}
		else{
			message_ptr = (Message*) malloc(sizeof(Message));
			if(message_ptr == NULL){
				printf("deQueue_arrayQueue: Failed to allocate memory for Message copy!\n");
				fflush(stdout);
				exit(1);
			}

			assignMessage(message_ptr, queue_arrayPtr->message_array[queue_arrayPtr->head]);
			queue_arrayPtr->head = (queue_arrayPtr->head + 1) % QUEUE_ARRAY_SIZE;
			(queue_arrayPtr->items)--;

			//printf("deQueue_arrayQueue: Item removed successfully!\n");
			//fflush(stdout);

			//printQueue_arrayQueue(*queue_arrayPtr);

			return message_ptr;
		}
	}

}

Message* rear_arrayQueue(MessageQueue queue_array){ /*----Returns the Last Packet of the Queue----*/

	Message* message_ptr = (Message*) NULL;

	if(isEmpty_arrayQueue(queue_array)) return (Message*) NULL;
	message_ptr = (Message*) malloc(sizeof(Message));
	if(message_ptr == NULL){
		printf("rear_arrayQueue: Failed to allocate memory for Message copy!\n");
		return (Message*) NULL;
	}

	assignMessage(message_ptr, queue_array.message_array[queue_array.tail]);

	return message_ptr;
	
}

Message* front_arrayQueue(MessageQueue queue_array){ /*----Returns the first Packet of the Queue----*/

	Message* message_ptr = (Message*) NULL;

	if(isEmpty_arrayQueue(queue_array)) return (Message*) NULL;
	message_ptr = (Message*) malloc(sizeof(Message));
	if(message_ptr == NULL){
		printf("front_arrayQueue: Failed to allocate memory for Message copy!\n");
		return (Message*) NULL;
	}

	assignMessage(message_ptr, queue_array.message_array[queue_array.head]);

	return message_ptr;
	
}

void printQueue_arrayQueue(MessageQueue queue_array){

	int i=0;
	int value = 0;
	int state = 0;
	int items_printed = 0;

	if(isEmpty_arrayQueue(queue_array) == 1){
		printf("printQueue_arrayQueue: Queue is empty!\n");
		fflush(stdout);
		return;
	}

	printf("=============================printQueue_arrayQueue(H: %d - T: %d)=====================\n", queue_array.head, queue_array.tail);
	fflush(stdout);

	while(items_printed < queue_array.items){
		if(queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].type == NUMBER){
			printf("|%d| - NUMBER Sync Value: |%d|\n", i+1, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].value);
		}
		else if(queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].type == PROCESS_OP){
			value = queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].value;
			if(value == VP_START){
				printf("|%d| - |PROCESS OP: VP_START| - |PID: %d|\n", i+1, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.pid);
			}
			else if(value == VP_SWAP){
				printf("|%d| - |PROCESS OP: VP_SWAP| - |PID: %d|\n", i+1, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.pid);
			}
			else if(value == VP_STOP){
				printf("|%d| - |PROCESS OP: VP_STOP| - |PID: %d|\n", i+1, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.pid);
			}
			else{
				printf("|%d| - Unknown PROCESS OP!\n", i+1);
			}
		}
		else if(queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].type == PROCESS){
			state = queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.state;
			if(state == RUNNING){
				printf("|%d| - |PROCESS| - |PID: %d| - |Arrival: %d| - |RUNNING| - |Size: %d| - |Bursts: %d|\n", i+1, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.pid, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.arrival_time, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.size, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.bursts);
			}
			else if(state == WAITING){
				printf("|%d| - |PROCESS| - |PID: %d| - |Arrival: %d| - |WAITING| - |Size: %d| - |Bursts: %d|\n", i+1, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.pid, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.arrival_time, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.size, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.bursts);
			}
			else if(state == BLOCKED){
				printf("|%d| - |PROCESS| - |PID: %d| - |Arrival: %d| - |BLOCKED| - |Size: %d| - |Bursts: %d|\n", i+1, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.pid, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.arrival_time, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.size, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.bursts);
			}
			else if(state == UNBORN){
				printf("|%d| - |PROCESS| - |PID: %d| - |Arrival: %d| - |UNBORN| - |Size: %d| - |Bursts: %d|\n", i+1, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.pid, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.arrival_time, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.size, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.bursts);
			}
			else if(state == FINISHED){
				printf("|%d| - |PROCESS| - |PID: %d| - |Arrival: %d| - |FINISHED| - |Size: %d| - |Bursts: %d|\n", i+1, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.pid, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.arrival_time, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.size, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.bursts);
			}
			else{
				printf("|%d| - Unknown Process state!\n", i+1);
			}
		}
		else if(queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].type == CONFIRM){
			value = queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].value;
			if(value == RUNNING){
				printf("|%d| - |CONFIRM: RUNNING| - |PID: %d|\n", i+1, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.pid);
			}
			else if(value == WAITING){
				printf("|%d| - |CONFIRM: WAITING| - |PID: %d|\n", i+1, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.pid);
			}
			else if(value == BLOCKED){
				printf("|%d| - |CONFIRM: BLOCKED| - |PID: %d|\n", i+1, queue_array.message_array[queue_array.head+i % QUEUE_ARRAY_SIZE].process.pid);
			}
			else if(value == OK){
				printf("|%d| - |CONFIRM: OK|\n", i+1);
			}
			else if(value == END_EVENTS){
				printf("|%d| - |CONFIRM: END_EVENTS|\n", i+1);
			}
			else{
				printf("|%d| - Unknown Confirm Type!\n", i+1);
			}
		}
		else{
			printf("|%d| - Unknown Message Type!\n", i+1);
		}
		if(queue_array.head+i % QUEUE_ARRAY_SIZE == queue_array.tail){
			//printf("\nReached tail: %d breaking!\n", queue_array.tail);
			break;
		}
		i++;
		items_printed++;
	}

	printf("===========================================================================================\n");
	fflush(stdout);

	return;

}

/*===============================SHARED MEMORY OPERATIONS==================================*/

int attachToSharedMemoryArea(key_t shm_key, int* shmid_Ptr, SharedMemorySegment** shmPtr){
	
	void* tmp_shm_ptr = (void*) NULL;

	/*----Shmget()----*/
	*shmid_Ptr = shmget(shm_key, sizeof(SharedMemorySegment), PERMS | IPC_CREAT);
	if(*shmid_Ptr == -1){
		printf("Shmget failed! Error Message: |%s|\n", strerror(errno));
		exit(1);
	}

	/*----Shmattach()----*/
	tmp_shm_ptr = shmat(*shmid_Ptr, (void*) 0, 0);
	if(tmp_shm_ptr == (char*) -1){
		printf("Shmat failed! Error Message: |%s|\n", strerror(errno));
		return -1;
	}

	*shmPtr = (SharedMemorySegment*) tmp_shm_ptr;
		
	return 0;
	
}

int detachSharedMemory(SharedMemorySegment* shmPtr){

	/*----ShmDetach----*/
	if(shmdt((void*) shmPtr) == -1){
		printf("Simulator: Shmdt failed! Error Message: |%s|\n", strerror(errno));
		return -1;
	}

	return 0;

}

/*==============================SEMAPHORE OPERATIONS=======================================*/

int getSemaphores(key_t sem_key, int* semid,int nsems){

	*semid = semget(sem_key, nsems, PERMS | IPC_CREAT);
	if(*semid == -1){
		printf("Semget failed! Error Message: |%s|\n", strerror(errno));
		return -1;
	}

	return 0;

}

/*==================================CONSUMER AND PRODUCER BUFFER FUNCTIONS===================================*/

/*----Basic Producer Function that uses an Empty, a Full and a Mutex Semaphore----*/
/*----The Producer waits until an Empty slot has appeared then it locks the Queue Mutex and----*/
/*----Adds its item to the buffer. Then it unlocks the Queue Mutex and Ups the Full Semaphore----*/
/*----To report a new item has arrived to the Consumer----*/

int addToSharedBuffer(char* processName,Message* message_ptr,MessageQueue* queueArrayPtr,int semid,int sem_numMutex,int sem_numEmpty,int sem_numFull){

	struct sembuf semaphoreOperation;
	Message* tmp_message = (Message*) NULL;

	if(message_ptr == NULL){
		printf("%s - addToSharedBuffer: message_ptr is NULL!\n",processName);
		fflush(stdout);
		return -1;
	}

	//printf("%s - addToSharedBuffer: Attempting to ADD a Message to the Shared Memory Buffer...\n", processName);
	//fflush(stdout);

	/*---------------Wait for an empty space----------------*/
	semaphoreOperation.sem_num = sem_numEmpty;
	semaphoreOperation.sem_op = -1;
	semaphoreOperation.sem_flg = 0;

	//printf("%s - addToSharedBuffer: Will DOWN Empty Semaphore...\n",processName);
	//fflush(stdout);

	if(semop(semid, &semaphoreOperation, 1) == -1){
		printf("%s - addToSharedBuffer: Failed to DOWN EMPTY Semaphore!\n",processName);
		fflush(stdout);
		return -1;
	}	

	//printf("%s - addToSharedBuffer: DOWNed Empty Semaphore!\n",processName);
	//fflush(stdout);

	/*----Down Queue Mutex----*/
	semaphoreOperation.sem_num = sem_numMutex;
	semaphoreOperation.sem_op = -1;
	semaphoreOperation.sem_flg = 0;

	//printf("%s - addToSharedBuffer: DOWNing Queue Mutex...\n",processName);
	//fflush(stdout);

	if(semop(semid, &semaphoreOperation, 1) == -1){
		printf("%s - addToSharedBuffer: Failed to DOWN MUTEX Semaphore!\n",processName);
		fflush(stdout);

		semaphoreOperation.sem_num = sem_numEmpty; //Revert Semaphore Operations
		semaphoreOperation.sem_op = 1;
		semaphoreOperation.sem_flg = 0;	

		if(semop(semid, &semaphoreOperation, 1) == -1){
			printf("%s - : takeFromSharedBuffer: Failed to UP EMPTY Semaphore!\n", processName);
			fflush(stdout);
		}	

		return -1;
	}	

	//printf("%s - addToSharedBuffer: DOWNed Queue Mutex!\n", processName);
	//fflush(stdout);

	//printf("%s - addToSharedBuffer: Calling enQueue_arrayQueue...!\n", processName);
	//fflush(stdout);

	/*----Store the item----*/
	if(enQueue_arrayQueue(queueArrayPtr, *message_ptr) == -1){
		printf("%s - addToSharedBuffer: enQueue_arrayQueue Failed!\n", processName);
		fflush(stdout);

		semaphoreOperation.sem_num = sem_numMutex; //Revert Semaphore Operations
		semaphoreOperation.sem_op = 1;
		semaphoreOperation.sem_flg = 0;	

		if(semop(semid, &semaphoreOperation, 1) == -1){
			printf("%s - addToSharedBuffer: Failed to UP MUTEX Semaphore!\n", processName);
			fflush(stdout);
		}

		semaphoreOperation.sem_num = sem_numEmpty;
		semaphoreOperation.sem_op = 1;
		semaphoreOperation.sem_flg = 0;	

		if(semop(semid, &semaphoreOperation, 1) == -1){
			printf("%s - takeFromSharedBuffer: Failed to UP EMPTY Semaphore!\n", processName);
			fflush(stdout);
		}	

		return -1;

	}

	//printf("%s - addToSharedBuffer: UPing Queue Mutex...\n", processName);
	//fflush(stdout);

	/*----Up Queue Mutex----*/
	semaphoreOperation.sem_num = sem_numMutex;
	semaphoreOperation.sem_op = 1;
	semaphoreOperation.sem_flg = 0;	

	if(semop(semid, &semaphoreOperation, 1) == -1){
		printf("%s - addToSharedBuffer: Failed to UP MUTEX Semaphore!\n", processName);
		fflush(stdout);
		return -1;
	}	

	//printf("%s - addToSharedBuffer: UP Queue Mutex!\n", processName);
	//fflush(stdout);

	/*----Report full new slot----*/
	semaphoreOperation.sem_num = sem_numFull;
	semaphoreOperation.sem_op = 1;
	semaphoreOperation.sem_flg = 0;	

	//printf("%s - addToSharedBuffer: UPed Queue Mutex!\n", processName);
	//fflush(stdout);

	//printf("%s - addToSharedBuffer: UPing FULL semaphore...\n", processName);
	//fflush(stdout);

	if(semop(semid, &semaphoreOperation, 1) == -1){
		printf("%s - addToSharedBuffer: Failed to UP FULL Semaphore!\n", processName);
		fflush(stdout);
		return -1;
	}	

	//printf("%s - addToSharedBuffer: UPed FULL semaphore!\n", processName);
	//fflush(stdout);

	return 0;

}

/*----Basic Consumer Function that uses a Full, an Empty and a Mutex Semaphore----*/
/*----The Consumer waits until an item has appeared then it locks the Queue Mutex and----*/
/*----Removes the item from the buffer. Then it unlocks the Queue Mutex and Ups the Empty Semaphore----*/
/*----To report a new empty slot has been created to the Producer----*/

Message* takeFromSharedBuffer(char* processname,MessageQueue* queueArrayPtr,int semid,int sem_numMutex,int sem_numEmpty,int sem_numFull){

	struct sembuf semaphoreOperation;
	Message* message_ptr = (Message*) NULL;

	//printf("%s - takeFromSharedBuffer: Attempting to CONSUME a Message from the Shared Memory Buffer...\n", processname);
	//fflush(stdout);

	/*----Wait for a stored MessagePacket----*/

	semaphoreOperation.sem_num = sem_numFull;
	semaphoreOperation.sem_op = -1;
	semaphoreOperation.sem_flg = 0;

	//printf("%s - takeFromSharedBuffer: DOWNing FULL Semaphore...\n", processname);
	//fflush(stdout);

	if(semop(semid, &semaphoreOperation, 1) == -1){
		printf("%s - takeFromSharedBuffer: takeFromSharedBuffer: Failed to DOWN FULL Semaphore!\n", processname);
		fflush(stdout);
		return (Message*) NULL;
	}	

	//printf("%s - takeFromSharedBuffer: DOWNed FULL Semaphore!\n", processname);
	//fflush(stdout);

	//printf("%s - takeFromSharedBuffer: DOWNing Queue MUTEX...\n", processname);
	//fflush(stdout);

	/*----Down Queue Mutex----*/
	semaphoreOperation.sem_num = sem_numMutex;
	semaphoreOperation.sem_op = -1;
	semaphoreOperation.sem_flg = 0;

	if(semop(semid, &semaphoreOperation, 1) == -1){
		printf("%s - takeFromSharedBuffer: Failed to DOWN MUTEX Semaphore!\n", processname);
		fflush(stdout);
		return (Message*) NULL;
	}	

	//printf("%s - takeFromSharedBuffer: DOWNed MUTEX Semaphore!\n", processname);
	//fflush(stdout);

	//printf("%s - takeFromSharedBuffer: Calling deQueue_arrayQueue...\n", processname);
	//fflush(stdout);

	/*----Remove the item----*/
	message_ptr = deQueue_arrayQueue(queueArrayPtr);

	if(message_ptr == NULL){
		printf("%s - takeFromSharedBuffer: deQueue_arrayQueue Failed!\n", processname);
		fflush(stdout);

		semaphoreOperation.sem_num = sem_numMutex;
		semaphoreOperation.sem_op = 1;
		semaphoreOperation.sem_flg = 0;	

		if(semop(semid, &semaphoreOperation, 1) == -1){
			printf("%s - takeFromSharedBuffer: Failed to UP MUTEX Semaphore!\n", processname);
			fflush(stdout);
		}

		semaphoreOperation.sem_num = sem_numFull;
		semaphoreOperation.sem_op = 1;
		semaphoreOperation.sem_flg = 0;	

		if(semop(semid, &semaphoreOperation, 1) == -1){
			printf("%s - takeFromSharedBuffer: Failed to UP FULL Semaphore!\n", processname);
			fflush(stdout);
		}	

		exit(1);
	}

	/*----Up Queue Mutex----*/
	semaphoreOperation.sem_num = sem_numMutex;
	semaphoreOperation.sem_op = 1;
	semaphoreOperation.sem_flg = 0;	

	//printf("%s - takeFromSharedBuffer: UPing MUTEX semaphore...\n", processname);
	//fflush(stdout);

	if(semop(semid, &semaphoreOperation, 1) == -1){
		printf("%s - takeFromSharedBuffer: Failed to UP MUTEX Semaphore!\n", processname);
		fflush(stdout);
		free(message_ptr);
		return (Message*) NULL;
	}	

	/*----Report new empty slot----*/
	semaphoreOperation.sem_num = sem_numEmpty;
	semaphoreOperation.sem_op = 1;
	semaphoreOperation.sem_flg = 0;	

	//printf("%s - takeFromSharedBuffer: UPed EMPTY semaphore!\n", processname);
	//fflush(stdout);

	if(semop(semid, &semaphoreOperation, 1) == -1){
		printf("%s - takeFromSharedBuffer: Failed to UP EMPTY Semaphore!\n", processname);
		fflush(stdout);
		free(message_ptr);
		return (Message*) NULL;
	}	

	return message_ptr;

}
