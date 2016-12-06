#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include "generator_tools.h"

/*======================Argument Checking=============================*/

int argumentHandling(int* D,int* lo,int* hi,float* t,float* T,int argc,char* argv[]){

	int i = 0;
	char c;

	srand(time(NULL));

	//for(i=0; i < argc; i++){
		//printf("|i: %d| - |%s|\n", i, argv[i]);
	//}

	if(argc == 1){ //No Arguments provided
		printf("Generator: Please run the program through the Simulator process!\n");
	}
	else if(argc != 6){
		printf("Generator: Argument Error - Incorrect number of arguments!\n");
		printf("Generator: If you wish to run the program with arguments run it like this: \n");
		printf("Generator: ./Generator <Simulation_Duration> <VP_MemLo> <VP_MemHi> <avg_time_arrivals> <avg_time_mem_usage>\n\n");
		printf("Generator: TIP: You can also run the program without any arguments!\n\n");
		exit(1);
	}
	else{
		(*D) = atoi(argv[1]);
		(*lo) = atoi(argv[2]);
		(*hi) = atoi(argv[3]);
		(*t) = atof(argv[4]);
		(*T) = atof(argv[5]);
	}
	
	if((*D) <= 0){
		printf("Generator: Argument Error - Simulation Duration Time has to be a positive number!\n");
	}
	else{
		if((*lo) <= 0){
			printf("Generator: Argument Error - Low VP (KB) Memory Boundary has to be a positive number!\n");
		}
		else{
			if((*hi) <= 0){
				printf("Generator: Argument Error - High VP Memory Boundary (KB) has to be a positive number!\n");
			}
			else if((*hi) < (*lo)){
				printf("Generator: Argument Error - High VP Memory Boundary (KB) has to be greater or equal to Low Memory Boundary!\n");
			}
			else{
				if((*t) <= 0){
					printf("Generator: Argument Error - Average Time Between Arrivals has to be a positive number!\n");
				}
				else if((*t) >= (*D)){
					printf("Generator: Argument Error - Please provide an Average Time Between Arrivals that is less than the SimDuration time!\n");
				}
				else{
					if((*T) <= 0){
						printf("Generator: Argument Error - Average MemUsage Time has to be a positive number!\n");
					}
					else if((*T) >= (*D)){
						printf("Generator: Argument Error - Please provide a MemUsage Time that is less than the SimDuration time!\n");
					}
					else{
						printf("\nGenerator: Received Arguments: |D: %d| - |lo: %d| - |hi: %d| - |t: %f| - |T: %f|\n", *D, *lo, *hi , *t, *T);

						return 0;
					}
				}
			}
		}
	}

	printf("Generator: If you wish to run the program with arguments run it like this: \n");
	printf("Generator: ./Generator <Simulation_Duration> <VP_MemLo> <VP_MemHi> <avg_time_arrivals> <avg_time_mem_usage>\n\n");
	printf("Generator: TIP: You can also run the program without any arguments!\n\n");
	exit(1);

}

/*========================Preparing Generator Functions====================*/

int setupTimetable(TimeTable* timeTable, ProcessQueue arrivalQueue){ /*Initialise the timeTable struct*/

	if(timeTable == NULL){
		printf("Generator - setupTimetable: timeTable pointer is NULL!\n");
	}

	timeTable->entries = arrivalQueue.items; //Timetable has one entry for each process that will arrive
	timeTable->processTable = malloc(sizeof(Process) * timeTable->entries);
	if(timeTable->processTable == NULL){
		printf("Generator - setupTimetable: processTable ptr is NULL!\n");
		return -1;
	}

	int i=0;

	arrivalQueue.Current = arrivalQueue.Head; //Fill timetable with UNBORN Processes
	while(arrivalQueue.Current != NULL){
		assignProcess(&(timeTable->processTable[i]) , *(arrivalQueue.Current->process_ptr));
		arrivalQueue.Current = arrivalQueue.Current->next;
		i++;
	}

	timeTable->running = 0;
	timeTable->waiting = 0;
	timeTable->finished = 0;
	timeTable->blocked = 0;
	timeTable->unborn = timeTable->entries;
	

	return 0;

}

int updateTimetable(TimeTable* timeTable, Process process, int index){ //Used to update a process in the timeTable (given an index or searching for its pid)

	int i=0;
	int located = 0;

	if(index == -1){
		//printf("updateTimetable: Searching timeTable for Process with PID: %d\n", process.pid);
		for(i=0; i < timeTable->entries; i++){
			if(timeTable->processTable[i].pid == process.pid){
				located = 1;
				assignProcess(&(timeTable->processTable[i]), process);
				break;
			}
		}

		if(located == 0){
			printf("Generator - updateTimetable: ERROR! Process does not exist in timeTable!\n");
			exit(1);
		}
	}
	else{
		//printf("updateTimetable: Will update timeTable in index: %d\n", index);
		if(index >= timeTable->entries){
			printf("Generator - updateTimetable: ERROR! Invalid timeTable index!\n");
			exit(1);
		}
		else{
			//printf("updateTimetable: Updating...\n");
			assignProcess(&(timeTable->processTable[index]), process);
			//printf("updateTimetable: Updated!\n");
		}
	}

	return 0;

}

void printTimeTable(TimeTable timeTable){

	int i=0;

	printf("======================================TIMETABLE==============================================\n");
	for(i=0; i < timeTable.entries; i++){
		if(timeTable.processTable[i].state == RUNNING){
			printf("|%d| - |PID: %d| - |Arrival: %d| - |RUNNING| - |BurstsLeft: %d| - |TimeLeft: %d| - |totalWaitTime: %d|\n", i+1, timeTable.processTable[i].pid, timeTable.processTable[i].arrival_time, timeTable.processTable[i].burstsLeft, timeTable.processTable[i].burstTimeLeft, timeTable.processTable[i].totalWaitTime);
		}
		else if(timeTable.processTable[i].state == WAITING){
			printf("|%d| - |PID: %d| - |Arrival: %d| - |WAITING| - |BurstsLeft: %d| - |TimeLeft: %d| - |totalWaitTime: %d|\n", i+1, timeTable.processTable[i].pid, timeTable.processTable[i].arrival_time, timeTable.processTable[i].burstsLeft, timeTable.processTable[i].burstTimeLeft, timeTable.processTable[i].totalWaitTime);
		}
		else if(timeTable.processTable[i].state == BLOCKED){
			printf("|%d| - |PID: %d| - |Arrival: %d| - |BLOCKED| - |BurstsLeft: %d| - |TimeLeft: %d| - |totalWaitTime: %d|\n", i+1, timeTable.processTable[i].pid, timeTable.processTable[i].arrival_time, timeTable.processTable[i].burstsLeft, timeTable.processTable[i].burstTimeLeft, timeTable.processTable[i].totalWaitTime);
		}
		else if(timeTable.processTable[i].state == UNBORN){
			printf("|%d| - |PID: %d| - |Arrival: %d| - |UNBORN| - |BurstsLeft: %d| - |TimeLeft: %d| - |totalWaitTime: %d|\n", i+1, timeTable.processTable[i].pid, timeTable.processTable[i].arrival_time, timeTable.processTable[i].burstsLeft, timeTable.processTable[i].burstTimeLeft, timeTable.processTable[i].totalWaitTime);			
		}
		else if(timeTable.processTable[i].state == FINISHED){
			printf("|%d| - |PID: %d| - |Arrival: %d| - |FINISHED| - |BurstsLeft: %d| - |TimeLeft: %d| - |totalWaitTime: %d|\n", i+1, timeTable.processTable[i].pid, timeTable.processTable[i].arrival_time, timeTable.processTable[i].burstsLeft, timeTable.processTable[i].burstTimeLeft, timeTable.processTable[i].totalWaitTime);
		}
		else{
			printf("|%d| - Unknown process state!\n", i+1);
		}
	}
	
	printf("Generator - printTimeTable: |Entries: %d| - |R: %d| - |W: %d| - |U: %d| - |F: %d|\n", timeTable.entries, timeTable.running, timeTable.waiting, timeTable.unborn, timeTable.finished);

	return;

}

int setupGenerator(int* shmid_Ptr, int* semidQueues_Ptr, int* semidSync_Ptr, SharedMemorySegment** shmPtr, ProcessQueue* arrivalQueue, TimeTable* timeTable, float t, float T, int D, int lo, int hi){

	if(arrivalQueue == NULL){
		printf("Generator - setupGenerator: Queue Pointer is NULL!\n");
		exit(1);
	}

	printf("Generator: Attaching to Shared Memory...\n");

	attachToSharedMemoryArea(SHMKEY_GEN_MEM, shmid_Ptr, shmPtr); //Attach to Shared Memory

	printf("Generator: Getting Queue Semaphores...\n");

	getSemaphores(SEMKEY_QUEUES, semidQueues_Ptr , NSEMS_QUEUES); //Get all required Semaphores

	printf("Generator: Getting Sync Semaphores...\n");

	getSemaphores(SEMKEY_SYNC, semidSync_Ptr , NSEMS_SYNC);

	printf("Generator: Creating Lists...\n");

	createList_QueueList(arrivalQueue); //Create list of arrivals

	printf("Generator: Calculating all Arrivals for current Simulation...\n");

	calculateAllArrivals(arrivalQueue,t,D,lo,hi,T); //Calculate all arrivals

	setupTimetable(timeTable, *arrivalQueue); //FIll timeTable with processes

	printTimeTable(*timeTable);

	return 0;

}

int calculateAllArrivals(ProcessQueue* arrivalQueuePtr,float t,int D,int lo,int hi,float T){ /*Given a Duration and an average interval time (t)*/
																							 /*This function will calculate when every process will arrive*/
	int previous_arrival = 0, next_arrival = 0, pid = 0;
	Process* process_ptr = (Process*) NULL;
	
	if(arrivalQueuePtr == NULL){
		printf("Generator - calculateAllArrivals: arrivalQueuePtr is NULL!\n");
		return -1;
	}

	while(1){
		previous_arrival = next_arrival;
		next_arrival = calculateNextArrival(t, previous_arrival); //Get the time till next arrival from exponential
		if(next_arrival >= D) break;
		process_ptr = createProcess(pid, next_arrival, lo, hi, T);
		if(process_ptr == NULL){
			printf("Generator - calculateAllArrivals: process_ptr is NULL!\n");
			return -1;
		}
		pid++;

		addLastList_QueueList(arrivalQueuePtr, process_ptr);
		printf("Generator - calculateAllArrivals: New Process Born! - |PID: %d| - |Arrival: %d|\n", process_ptr->pid, process_ptr->arrival_time);

		if(process_ptr != NULL){
			free(process_ptr);
			process_ptr = (Process*) NULL;
		}
	}

	printf("Generator - calculateAllArrivals: Now printing List of Processes...\n\n");
	
	printList_QueueList(*arrivalQueuePtr,-1,-1);
	
	return 0;

}

int calculateNextArrival(float t,int previous_arrival){ //Given an average interval time (t) it returns the time until the next arrival

	//printf("calculateNextArrival: With previous: %d and mean: %f next will be...\n", previous_arrival
	return (int) Exponential((double) t) + previous_arrival;

}

/*========================Main Generator Functions=========================*/

int catchArrivals(int currentTime, int semid_queues, SharedMemorySegment* shmPtr, ProcessQueue* arrivalQueuePtr, TimeTable* timeTable){

/*---Wait for new Processes that haven't been born yet to arrive in the current timeSlot---*/
/*---If a new Process arrives it will be send with VP_START to Manager and a confirmation will be received from Manager----*/
/*---About whether the Process is in the waitList or is placed in the Memory---*/

	Process* process_ptr = (Process*) NULL;
	Message* confirmMessagePtr = (Message*) NULL;
	Message* processMessagePtr = (Message*) NULL;
	int arrivals = 0;

	if(arrivalQueuePtr == NULL){
		printf("Generator - catchArrivals - |Time: %d|: arrivalQueuePtr is NULL!\n", currentTime);
		return -1;
	}

	if(timeTable == NULL){
		printf("Generator - catchArrivals - |Time: %d|: timeTable is NULL!\n", currentTime);
		return -1;
	}

	printf("Generator - catchArrivals - |Time: %d|: Catching arrivals in this timeSlot...\n", currentTime);

	while(arrivalQueuePtr->items > 0){ //Still arrivals left
		//printf("Generator - catchArrivals - |Time: %d|: Extracting Process from ArrivalQueue...\n", currentTime);
		process_ptr = frontList_QueueList(*arrivalQueuePtr); /*----Extract Process from Queue----*/
		//printf("Generator - catchArrivals - |Time: %d|: Now let's check if this process arrives in this timeSlot...\n", currentTime);
		if(process_ptr->arrival_time == currentTime){
			printf("Generator - catchArrivals - |Time: %d|: New Process with |PID: %d| has just arrived!\n", currentTime, process_ptr->pid);
				
			/*----Pop Process from Queue----*/
			popList_QueueList(arrivalQueuePtr);
			arrivals++;

			/*----Send to Manager----*/
			processMessagePtr = createMessage(PROCESS_OP, VP_START, process_ptr);
			if(processMessagePtr == NULL){
				printf("Generator - catchArrivals - |Time: %d|: ProcessMessage_Ptr is NULL!\n", currentTime);
				exit(1);		
			}
			addToSharedBuffer("Generator", processMessagePtr, &(shmPtr->genMemQueue), semid_queues, GENMEM_SEM_MUTEX, GENMEM_SEM_EMPTY, GENMEM_SEM_FULL);
			free(processMessagePtr);
			processMessagePtr = (Message*) NULL;

			/*----Receive confirmation about what happened----*/
			confirmMessagePtr = (Message*) NULL;
			confirmMessagePtr = takeFromSharedBuffer("Generator",&(shmPtr->memGenQueue),semid_queues, MEMGEN_SEM_MUTEX, MEMGEN_SEM_EMPTY, MEMGEN_SEM_FULL);
			if(confirmMessagePtr == NULL){
				printf("Generator - catchArrivals - |Time: %d|: Received NULL Message from Manager!\n", currentTime);
				exit(1);		
			}

			if(confirmMessagePtr->type == CONFIRM){
				printf("Generator - catchArrivals - |Time: %d|: Received Confirmation Message!\n", currentTime);
				if(confirmMessagePtr->value == WAITING){ //Process has been placed in waitList
					process_ptr->state = WAITING;
					printf("Generator - catchArrivals - |Time: %d|: Waiting Message!\n", currentTime);
					timeTable->processTable[process_ptr->pid].state = WAITING;
					//updateTimetable(timeTable, *process_ptr, process_ptr->pid);
					timeTable->processTable[process_ptr->pid].burstOpsLeft--; //Performed VP_START - lost 1 burst Operation
					timeTable->unborn--;
					timeTable->waiting++;
				}
				else if(confirmMessagePtr->value == RUNNING){ //Process is running
					process_ptr->state = RUNNING;
					printf("Generator - catchArrivals - |Time: %d|: Running Message!\n", currentTime);
					timeTable->processTable[process_ptr->pid].state = RUNNING;
					timeTable->processTable[process_ptr->pid].burstOpsLeft--; //Performed VP_START - lost 1 burst Operation
					//updateTimetable(timeTable, *process_ptr, process_ptr->pid);
					timeTable->unborn--;
					timeTable->running++;
				}
				else{
					printf("Generator - catchArrivals - |Time: %d|: Expected RUNNING or WAITING value Message but received: %d!\n", currentTime, confirmMessagePtr->value);
					exit(1);	
				}
			}
			else{
				printf("Generator - catchArrivals - |Time: %d|: Expected CONFIRM type of Message but received: %d!\n", currentTime, confirmMessagePtr->type);
				exit(1);	
			}

			if(process_ptr != NULL){
				free(process_ptr);
				process_ptr = (Process*) NULL;
			}

			free(confirmMessagePtr);
			confirmMessagePtr = (Message*) NULL;

		}
		else if(process_ptr->arrival_time < currentTime){
			printf("Generator - catchArrivals - |Time: %d|: ERROR! Arrival Time in Head Process is LESS than currentTime!\n",currentTime);
			if(process_ptr != NULL){
				free(process_ptr);
				process_ptr = (Process*) NULL;
			}
			exit(1);
		}
		else{
			//printf("Generator - catchArrivals - |Time: %d|: Extracted Process will arrive at a later time!\n",currentTime);
			if(process_ptr != NULL){
				free(process_ptr);
				process_ptr = (Process*) NULL;
			}
			break;
		}
	}

	printf("Generator - catchArrivals - |Time: %d|: Not expecting any other process to arrive in this timeSlot!\n",currentTime);	

	/*----Notify Manager for End of Arrivals----*/
	confirmMessagePtr = createMessage(CONFIRM, END_EVENTS, NULL); //Notify Manager for End of Arrivals
	if(confirmMessagePtr == NULL){
		printf("Generator - catchArrivals - |Time: %d|: END_EVENTS Message is NULL!\n",currentTime);	
	}
	addToSharedBuffer("Generator", confirmMessagePtr, &(shmPtr->genMemQueue), semid_queues, GENMEM_SEM_MUTEX, GENMEM_SEM_EMPTY, GENMEM_SEM_FULL);
	free(confirmMessagePtr);	

	return arrivals;

}

int executeTimeSlot(int currentTime, int semid_queues, SharedMemorySegment* shmPtr, TimeTable* timeTable, float T){

/*----For every timeSlot this function moves each RUNNING Process 1 step forward in time----*/
/*----After doing that it checks if each running Process has just finished a Burst or its Life completely----*/
/*----And sends VP_SWAP/VP_STOP accordingly.----*/
/*----When a Process returns it is sent immediately back to the Manager through VP_SWAP if Bursts still exist----*/
/*----The Manager will place it in the waitList or will place it in memory to run in the next timeSlot if possible----*/
/*----After doing that it allows the Manager to place any Process already in its waitList----*/
/*----Finally any process that has returned from the Manager (BLOCKED) and has bursts still left---*/
/*----Will be resent back to the Manager with a VP_SWAP and the Manager will attempt to place them----*/
/*----Sending back a Confirmation about whether the process is in the waitList or Running----*/

	Message* processMessage = (Message*) NULL;
	Message* confirmMessage = (Message*) NULL;

	if(timeTable == NULL){
		printf("Generator - executeTimeSlot - |Time: %d|: timeTable is NULL!\n", currentTime);
		return -1;
	}

	if(shmPtr == NULL){
		printf("Generator - executeTimeSlot - |Time: %d|: shmPtr is NULL!\n", currentTime);
		return -1;
	}

	int i = 0;

	if(timeTable->running > 0){ //If Processes actually run in this timeslot
		printf("Generator - executeTimeSlot - |Time: %d|: Moving all running processes 1 timeSlot forward...\n", currentTime);
		for(i=0; i < timeTable->entries; i++){
			//if(i == 0) printf("STATE FRIEND: %d\n", timeTable->processTable[0].state);
			if(timeTable->processTable[i].state == RUNNING){
				printf("Generator - executeTimeSlot - |Time: %d|: Reducing burstTimeLeft of PID: %d...\n", currentTime, timeTable->processTable[i].pid);
				timeTable->processTable[i].burstTimeLeft--;
				if(timeTable->processTable[i].burstTimeLeft == 0){ //Current burst ends
					printf("Generator - executeTimeSlot - |Time: %d|: End of Burst for PID: %d...\n", currentTime, timeTable->processTable[i].pid);
					timeTable->processTable[i].burstsLeft--;
					timeTable->processTable[i].burstOpsLeft--; //VP_SWAP/VP_STOP aka Minus 1 burst Operation
					if(timeTable->processTable[i].burstsLeft == 0){ //No more bursts left - end of life
						printf("Generator - executeTimeSlot - |Time: %d|: End of Life for PID: %d...\n", currentTime, timeTable->processTable[i].pid);
						printf("Generator - executeTimeSlot - |Time: %d|: Sending VP_STOP to Manager for PID: %d...\n", currentTime, timeTable->processTable[i].pid);
						timeTable->processTable[i].state = FINISHED;
						timeTable->finished++;
						timeTable->running--;
						processMessage = createMessage(PROCESS_OP, VP_STOP, &(timeTable->processTable[i]));
						if(processMessage == NULL){
							printf("Generator - executeTimeSlot - |Time: %d|: Failed to allocate memory for processMessage\n", currentTime);
							return -1;
						}
					}
					else{ //Still bursts left
						printf("Generator - executeTimeSlot- |Time: %d|: Sending VP_SWAP to Manager for PID: %d...\n", currentTime, timeTable->processTable[i].pid);
						timeTable->processTable[i].state = BLOCKED;
						timeTable->processTable[i].burstTimeLeft = createNextBurstTime(&(timeTable->processTable[i]), T); //Generate next burst time
						timeTable->blocked++;
						timeTable->running--;
						processMessage = createMessage(PROCESS_OP, VP_SWAP, &(timeTable->processTable[i]));
						if(processMessage == NULL){
							printf("Generator - executeTimeSlot - |Time: %d|: Failed to allocate memory for processMessage\n", currentTime);
							return -1;
						}				
					}
   
    				/*----SEND VP_SWAP/STOP for Removal----*/
					addToSharedBuffer("Generator", processMessage, &(shmPtr->genMemQueue), semid_queues, GENMEM_SEM_MUTEX, GENMEM_SEM_EMPTY, GENMEM_SEM_FULL);
					free(processMessage);
					processMessage = (Message*) NULL;

     				/*----Receive Confirmation----*/
					confirmMessage = takeFromSharedBuffer("Generator",&(shmPtr->memGenQueue),semid_queues, MEMGEN_SEM_MUTEX, MEMGEN_SEM_EMPTY, MEMGEN_SEM_FULL);
					if(confirmMessage->type != CONFIRM){
						printf("Generator - executeTimeSlot- |Time: %d|: Received non CONFIRM Message...\n", currentTime);
						exit(1);
					}
					if(confirmMessage->value == OK){
						printf("Generator - executeTimeSlot - |Time: %d|: Received OK for VP_SWAP/STOP from Manager...\n", currentTime);
						timeTable->processTable[i].totalWaitTime = confirmMessage->process.totalWaitTime;
					}
					else{
						printf("Generator - executeTimeSlot - |Time: %d|: Received non-OK Message from Manager...;\n", currentTime);
						exit(1);
					}

					free(confirmMessage);
					confirmMessage = (Message*) NULL;

					if(timeTable->processTable[i].burstsLeft > 0){ //Resend to Manager as BLOCKED to attempt to place later
						processMessage = createMessage(PROCESS_OP, VP_SWAP, &(timeTable->processTable[i]));
						if(processMessage == NULL){
							printf("Generator - executeTimeSlot - |Time: %d|: Failed to allocate memory for processMessage\n", currentTime);
							return -1;
						}	

						timeTable->processTable[i].burstOpsLeft--; //VP_SWAP - reduce burst Operations by 1
						addToSharedBuffer("Generator", processMessage, &(shmPtr->genMemQueue), semid_queues, GENMEM_SEM_MUTEX, GENMEM_SEM_EMPTY, GENMEM_SEM_FULL);
						free(processMessage);
						processMessage = (Message*) NULL;	

			 			/*----See if Process have been placed again or waits----*/
						confirmMessage = takeFromSharedBuffer("Generator",&(shmPtr->memGenQueue),semid_queues, MEMGEN_SEM_MUTEX, MEMGEN_SEM_EMPTY, MEMGEN_SEM_FULL);
						if(confirmMessage->type != CONFIRM){
							printf("Generator - executeTimeSlot- |Time: %d|: Received non CONFIRM Message...\n", currentTime);
							exit(1);
						}
						if(confirmMessage->value == RUNNING){
							printf("Generator - executeTimeSlot - |Time: %d|: Manager has set |PID: %d| in memory!\n", currentTime, timeTable->processTable[i].pid);
							timeTable->blocked--;
							timeTable->running++;
							timeTable->processTable[i].state = RUNNING;
						}
						else if(confirmMessage->value == WAITING){
							printf("Generator - executeTimeSlot - |Time: %d|: Manager has set |PID: %d| in waitList!\n", currentTime, timeTable->processTable[i].pid);
							timeTable->blocked--;
							timeTable->waiting++;
							timeTable->processTable[i].state = WAITING;
						}
						else{
							printf("Generator - executeTimeSlot - |Time: %d|: Received non-OK Message from Manager...\n", currentTime);
							exit(1);
						}

						free(confirmMessage);
						confirmMessage = (Message*) NULL;	
		
					}
				}
			}
		}
		printf("Generator - executeTimeSlot - |Time: %d|: All running processes have moved 1 timeSlot forward!\n", currentTime);
	}
	else{
		printf("Generator - executeTimeSlot - |Time: %d|: No processes are running to move forward!\n", currentTime);
	}

	/*----Notify Manager that all Running processes have moved one timeslot----*/
	confirmMessage = createMessage(CONFIRM, END_EVENTS, NULL);
	if(confirmMessage == NULL){
		printf("Generator - executeTimeSlot - |Time: %d|: Failed to allocate memory for END_EVENTS Message!\n", currentTime);
		exit(1);		
	}
	addToSharedBuffer("Generator", confirmMessage, &(shmPtr->genMemQueue), semid_queues, GENMEM_SEM_MUTEX, GENMEM_SEM_EMPTY, GENMEM_SEM_FULL);
	free(confirmMessage);
	
	/*----Receive Updates from Manager for any Processes in its Waiting List----*/
	while(1){
		processMessage = takeFromSharedBuffer("Generator",&(shmPtr->memGenQueue),semid_queues, MEMGEN_SEM_MUTEX, MEMGEN_SEM_EMPTY, MEMGEN_SEM_FULL);
		if(processMessage->type == CONFIRM){
			if(processMessage->value == END_EVENTS){
				printf("Generator - executeTimeSlot - |Time: %d|: Received all updates from Manager for this timeslot!\n", currentTime);
				free(processMessage);
				processMessage = (Message*) NULL;
				break;		
			}
			else{
				printf("Generator - executeTimeSlot - |Time: %d|: Error! Expected END_EVENTS Confirm Message but received different value!\n", currentTime);
				free(processMessage);
				processMessage = (Message*) NULL;
				exit(1);
			}
		}
		else if(processMessage->type == PROCESS){
			if(processMessage->value == RUNNING){
				timeTable->processTable[processMessage->process.pid].state = RUNNING;
				//updateTimetable(timeTable, processMessage->process, processMessage->process.pid);
				//printf("afteeeeeeeee a billion: %d\n\n\n\n\n\n\n\n\n\n\n\n", processMessage->process.totalWaitTime);
				//exit(1);
				timeTable->processTable[processMessage->process.pid].totalWaitTime = processMessage->process.totalWaitTime;
				timeTable->waiting--;
				timeTable->running++;
				printf("Generator - executeTimeSlot - |Time: %d|: Manager has set |PID: %d| in memory!\n", currentTime, processMessage->process.pid);
			}
			else{
				printf("Generator - executeTimeSlot - |Time: %d|: Error! Expected RUNNING Message but received different value!\n", currentTime);
				free(processMessage);
				processMessage = (Message*) NULL;
				exit(1);
			}
		}
		else{
			printf("Generator - executeTimeSlot - |Time: %d|: Error! Expected CONFIRM or PROCESS Message but received different type!\n", currentTime);
			free(processMessage);
			processMessage = (Message*) NULL;
			exit(1);
		}
	}

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

int beginSimulation(int D, int semid_queues, int semid_sync, float T,SharedMemorySegment* shmPtr, ProcessQueue* arrivalQueuePtr, TimeTable* timeTable){

	int timeSlot = 0;

	while(timeSlot < D){ //Time D is divided in equal timeslots
		//printf("=======================================Generator TIMESLOT %d==============================================\n", timeSlot);
		catchArrivals(timeSlot, semid_queues,  shmPtr, arrivalQueuePtr, timeTable); //Catch any new Processes and VP_START pass them to Manager
		executeTimeSlot(timeSlot, semid_queues, shmPtr, timeTable, T);
		syncTimeSlot("Generator", semid_sync, &timeSlot , GENSYNC_SEM, MEMSYNC_SEM); //Ensures that the two processes move to next timeslot at the same time
	}

	printf("\nGenerator: Simulation has finished!!\n");

	return 0;

}

int tearDownGenerator(SharedMemorySegment* shmPtr, ProcessQueue* arrivalQueue, TimeTable* timeTable){

	if(arrivalQueue == NULL){
		printf("Generator - tearDownGenerator: Queue Pointer is NULL!\n");
		exit(1);
	}

	printf("Generator: Detaching from Shared Memory...\n");

	detachSharedMemory(shmPtr);

	printf("Generator: Destroying List...\n");

	destroyList_QueueList(arrivalQueue);

	printf("Generator: Freeing timeTable...\n");

	if(timeTable->processTable != NULL)
		free(timeTable->processTable);
	
	return 0;

}
