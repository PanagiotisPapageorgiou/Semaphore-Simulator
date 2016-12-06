#include "shared_structs.h"

typedef struct TimeTable{ /*---Keeps the current state of every process and when the next event will happen----*/
	int running; //number of processes running currently
	int waiting;
	int blocked;
	int finished;
	int unborn;
	int entries;
	Process* processTable; //dynamic array with length == number of processes
} TimeTable;

/*---Argument Checking---*/
int argumentHandling(int* D,int* lo,int* hi,float* t,float* T,int argc,char* argv[]);

/*---Preparing Generator---*/
int setupTimetable(TimeTable* timeTable, ProcessQueue arrivalQueue);
int updateTimetable(TimeTable* timeTable, Process process, int index);
void printTimeTable(TimeTable timeTable);
int setupGenerator(int* shmid_Ptr, int* semidQueues_Ptr, int* semidSync_Ptr, SharedMemorySegment** shmPtr, ProcessQueue* arrivalQueue, TimeTable* timeTable, float t, float T, int D, int lo, int hi);
int calculateAllArrivals(ProcessQueue* arrivalQueuePtr,float t,int D,int lo,int hi,float T);
int calculateNextArrival(float t,int previous_arrival);

/*---Main Generator Tools---*/
int catchArrivals(int currentTime, int semid_queues,  SharedMemorySegment* shmPtr, ProcessQueue* arrivalQueuePtr, TimeTable* timeTable);
int executeTimeSlot(int currentTime, int semid_queues, SharedMemorySegment* shmPtr, TimeTable* timeTable, float T);
int syncTimeSlot(char* processName, int semid_sync, int* timeSlot,int mySem, int oppositeSem);
int beginSimulation(int D, int semid_queues, int semid_sync, float T,SharedMemorySegment* shmPtr, ProcessQueue* arrivalQueuePtr, TimeTable* timeTable);
int tearDownGenerator(SharedMemorySegment* shmPtr, ProcessQueue* arrivalQueue, TimeTable* timeTable);
