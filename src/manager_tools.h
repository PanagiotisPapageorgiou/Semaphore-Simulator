#include "memory.h"

int setupManager(int* fd,int* shmid_Ptr, int* semidQueues_Ptr, int* semidSync_Ptr, SharedMemorySegment** shmPtr, ProcessQueue* waitQueue, ProcessQueue* statsQueue,Memory* memoryPtr, int D, int S);
int attemptToPlace(int currentTime, int algorithmChoice, Message** processMessagePtr, Message** confirmMessagePtr, Memory* memoryPtr, ProcessQueue* waitQueuePtr,int op, int semid_queues, SharedMemorySegment* shmPtr,int fd);
int waitForArrivals(int fd,int currentTime, int semid_queues, SharedMemorySegment* shmPtr, ProcessQueue* waitQueuePtr, ProcessQueue* statsQueue,Memory* memoryPtr,int algorithmChoice);
int executeTimeSlot(int fd, int currentTime, int semid_queues, SharedMemorySegment* shmPtr, ProcessQueue* waitQueuePtr, ProcessQueue* statsQueue,Memory* memoryPtr,int algorithmChoice);
int syncTimeSlot(char* processName, int semid_sync, int* timeSlot,int mySem, int oppositeSem);
int beginSimulation(int fd,int D, int semid_queues, int semid_sync, SharedMemorySegment* shmPtr, ProcessQueue* waitQueuePtr, ProcessQueue* statsQueue, Memory* memoryPtr,int algorithmChoice);
int tearDownManager(SharedMemorySegment* shmPtr, ProcessQueue* waitQueue, ProcessQueue* statsQueue,Memory* memoryPtr);
