#include "shared_structs.h"

#define FREE 0
#define TAKEN 1

#define NEXT_FIT 1
#define BEST_FIT 2
#define BUDDY 3

/*----Memory Blocks Linked List Implementation----*/

typedef struct Block{
	int state;	
	int size;
	int beginAddress;
	int buddyLevel;
	Process* process_ptr;
} Block;

typedef struct BlockNode{
	Block* block_ptr;
	struct BlockNode* next;
	struct BlockNode* previous;
} BlockNode;

typedef struct BlockList{
	BlockNode* Head;
	BlockNode* Current;
	BlockNode* Last;
	int items;
	int free_blocks;
	int taken_blocks;
	int totalMemUsage;
	int totalFreeMem;
} BlockList;

/*----Memory----*/
typedef struct Memory{
	BlockList blockList;
	int size;
	long statistic_totalMemUsage;
	long product_MemTime;
} Memory;

/*----Block functions----*/
Block* createBlock(int state, int size,int beginAddress,Process* process_ptr);
Block* createCopyBlock(Block* originalBlock);

/*----BlockList functions----*/
void createList_BlockList(BlockList* blockListPtr,int S);
int isEmpty_BlockList(BlockList blockList);
Block* frontList_BlockList(BlockList blockList);
int addLastList_BlockList(BlockList* blockListPtr, Block* block_ptr);
void printList_BlockList(BlockList blockList,int fd, int mode);
void popList_BlockList(BlockList* blockListPtr);
void destroyList_BlockList(BlockList* blockListPtr);

/*----Memory functions----*/
int createMemory(Memory* memoryPtr, int S);
int insertProcessAtBlock(BlockList* blockListPtr, BlockNode* targetBlockNode, Process process);
int removeProcessFromMemory(Memory* memoryPtr, int pid, int* waitTime);
int bestFit_algorithm(Memory* memoryPtr, Process process);
int nextFit_algorithm(Memory* memoryPtr, Process process);
