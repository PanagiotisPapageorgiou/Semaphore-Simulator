#include <math.h>
#include <time.h>

#define UNINITIALISED -1
#define VP_START 1
#define VP_SWAP 2
#define VP_STOP 3
#define BLOCKED 4
#define WAITING 5
#define RUNNING 6
#define UNBORN -2
#define FINISHED 20

#define PROCESS 16
#define PROCESS_OP 17
#define NUMBER 18
#define CONFIRM 19

#define QUEUE_ARRAY_SIZE 10
#define NSEMS_QUEUES 6
#define GENMEM_SEM_MUTEX 0
#define GENMEM_SEM_EMPTY 1
#define GENMEM_SEM_FULL 2
#define MEMGEN_SEM_MUTEX 3
#define MEMGEN_SEM_EMPTY 4
#define MEMGEN_SEM_FULL 5

#define GENSYNC_SEM 0
#define MEMSYNC_SEM 2

#define NSEMS_SYNC 4

#define SHMKEY_GEN_MEM (key_t) 34410
#define SEMKEY_QUEUES (key_t) 54410
#define SEMKEY_SYNC (key_t) 54411

#define END_EVENTS 99

#define OK 100
#define FAIL -123
#define PERMS 0600

/*----Process Data Structure----*/
typedef struct Process{
	int pid; 
	int arrival_time; //exponential (based on mean=t)
	int size;  //between [lo,hi]
	int bursts; //k
	int state;
	int currentBurstTime;
	int burstTimeLeft;
	int burstsLeft; //k (changeable)
	int burstOpsLeft; //2k+1
	int totalWaitTime;
	int currentWaitTime;
	int arrivedInManager;
} Process;

/*----Process Queue Linked List Implementation----*/
typedef struct QueueNode{
	Process* process_ptr;
	struct QueueNode* next;
	struct QueueNode* previous;
} QueueNode;

typedef struct ProcessQueue{
	QueueNode* Head;
	QueueNode* Current;
	QueueNode* Last;
	int items;
} ProcessQueue;

/*----Message Data Structure----*/
typedef struct Message{
	int type;
	int value;
	Process process;
} Message;

/*----Queue Implementation with Array for Messages (used in Shared Memory Segment)----*/
typedef struct MessageQueue{
	int head, tail, items;
	Message message_array[QUEUE_ARRAY_SIZE];
} MessageQueue;

/*----Memory Shared between Generator and Memory Manager----*/
typedef struct SharedMemorySegment{
	MessageQueue genMemQueue;
	MessageQueue memGenQueue;
} SharedMemorySegment;

/*---Distributions---*/
double Constant (double mean);
int uniform_distribution(int rangeLow, int rangeHigh);
double Exponential(double mean);

/*---Process---*/
Process* createProcess(int pid, int arrival_time, int lo, int hi, float T);
int createNextBurstTime(Process* process_ptr, float T);
int assignProcess(Process* target, Process source);
Process* createCopyProcess(Process* originalProcess);

/*---Message---*/
Message* createMessage(int type,int value,Process* process);
int assignMessage(Message* target, Message source);
Message* createCopyMessage(Message* originalMessage);

/*---ProcessQueue---*/
void createList_QueueList(ProcessQueue*);
int isEmpty_QueueList(ProcessQueue);
int addLastList_QueueList(ProcessQueue*, Process*);
void printList_QueueList(ProcessQueue,int fd,int mode);
void popList_QueueList(ProcessQueue*);
void destroyList_QueueList(ProcessQueue*);
Process* frontList_QueueList(ProcessQueue);

/*----Array Queue----*/
int init_arrayQueue(MessageQueue*);
int clear_arrayQueue(MessageQueue*);
int isEmpty_arrayQueue(MessageQueue);
int isFull_arrayQueue(MessageQueue);
int enQueue_arrayQueue(MessageQueue*, Message);
Message* deQueue_arrayQueue(MessageQueue*);
Message* rear_arrayQueue(MessageQueue);
Message* front_arrayQueue(MessageQueue);
void printQueue_arrayQueue(MessageQueue);

/*----Shared Memory Operations----*/
int attachToSharedMemoryArea(key_t shm_key, int* shmid_Ptr, SharedMemorySegment** shmPtr);
int detachSharedMemory(SharedMemorySegment* shmPtr);

/*----Semaphore Operations----*/
int getSemaphores(key_t sem_key, int* semid,int nsems);

/*----Consumer/Producer functions----*/
int addToSharedBuffer(char* processName,Message* message_ptr,MessageQueue* queueArrayPtr,int semid,int sem_numMutex,int sem_numEmpty,int sem_numFull);
Message* takeFromSharedBuffer(char* processname,MessageQueue* queueArrayPtr,int semid,int sem_numMutex,int sem_numEmpty,int sem_numFull);
