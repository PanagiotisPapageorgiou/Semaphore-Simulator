#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include "memory.h"

/*-----Block functions-----*/

Block* createBlock(int state, int size,int beginAddress,Process* process_ptr){

	Block* block_ptr = (Block*) NULL;
	int k;

	block_ptr = malloc(sizeof(Block));
	if(block_ptr == NULL){
		printf("createBlock: Error! Failed to allocate memory for block!\n");
		return (Block*) NULL;
	}

	block_ptr->state = state;
	block_ptr->size = size;
	block_ptr->beginAddress = beginAddress;
	block_ptr->process_ptr = (Process*) NULL;
	block_ptr->buddyLevel = 0;

	if(process_ptr != NULL){
		block_ptr->process_ptr = createCopyProcess(process_ptr);
	}

	return block_ptr;

}

Block* createCopyBlock(Block* originalBlock){

	Block* newBlock = (Block*) NULL;

	if(originalBlock == NULL){
		printf("createCopyBlock: originalBlock is NULL!\n");
		return (Block*) NULL;
	}

	newBlock = malloc(sizeof(Block));
	if(newBlock == NULL){
		printf("createCopyBlock: newBlock is NULL!\n");
		return (Block*) NULL;		
	}

	newBlock->state = originalBlock->state;
	newBlock->size = originalBlock->size;
	newBlock->buddyLevel = originalBlock->buddyLevel;
	newBlock->beginAddress = originalBlock->beginAddress;
	newBlock->process_ptr = (Process*) NULL;
	if(originalBlock->process_ptr != NULL){
		newBlock->process_ptr = malloc(sizeof(Process));
		if(newBlock->process_ptr == NULL){
			printf("createCopyBlock: failed to allocate memory for Process in Block!\n");
			return (Block*) NULL;
		}
		assignProcess(newBlock->process_ptr, *(originalBlock->process_ptr));
	}

	return newBlock;
	
}

/*----List of Blocks functions----*/

void createList_BlockList(BlockList* blockListPtr,int S){ //Initialises BlockList with 1 Free Block of Size == S

	Block* block_ptr = (Block*) NULL;

	blockListPtr->items = 0; //Number of Blocks formed in Memory
	blockListPtr->Head = (BlockNode*) NULL;
	blockListPtr->Current = (BlockNode*) NULL;
	blockListPtr->Last = (BlockNode*) NULL;
	blockListPtr->free_blocks = 0; //Number of free Blocks
	blockListPtr->taken_blocks = 0; //Number of taken Blocks
	blockListPtr->totalMemUsage = 0;
	blockListPtr->totalFreeMem = S;

	block_ptr = createBlock(FREE, S, 0, NULL);
	if(block_ptr == NULL){
		printf("createList_BlockList: block_ptr is NULL!\n");
		exit(1);
	}

	addLastList_BlockList(blockListPtr, block_ptr);

	free(block_ptr);

	(blockListPtr->free_blocks)++;

	return;
}

int isEmpty_BlockList(BlockList blockList){ //Checks if any blocks exists

	return (blockList.items == 0);

}

Block* frontList_BlockList(BlockList blockList){ /* Returns a copy of the Data in the first Node */

	if(blockList.items > 0)
		return createCopyBlock(blockList.Head->block_ptr);
	else
		return (Block*) NULL;

}

int addLastList_BlockList(BlockList* blockListPtr, Block* block_ptr){ /* Adds a Node to the end of the List */
																	  /* WARNING: This function does not deal with whether the Block is FREE or TAKEN! */

	int i=0;

	if(blockListPtr->items == 0){ /* No items yet */

		blockListPtr->Head = malloc(sizeof(BlockNode));
		if(blockListPtr->Head == NULL){
			printf("addLastList_BlockList: Failed to allocate memory for Head BlockNode!\n");
			return -1;
		}

		blockListPtr->Head->block_ptr = createCopyBlock(block_ptr);
		if(blockListPtr->Head->block_ptr == NULL){
			printf("addLastList_BlockList: Failed to allocate memory for Block!\n");
			return -1;
		}

		blockListPtr->Head->next = (BlockNode*) NULL;
		blockListPtr->Head->previous = (BlockNode*) NULL;

		blockListPtr->Current = blockListPtr->Head;
		blockListPtr->Last = blockListPtr->Head;

		(blockListPtr->items)++;

		return 0;
	}
	else if(blockListPtr->items > 0){

		blockListPtr->Last->next = malloc(sizeof(BlockNode));
		if(blockListPtr->Last->next == NULL){
			printf("addLastList_BlockList: Failed to allocate memory for Last BlockNode!\n");
			return -1;
		}

		blockListPtr->Last->next->previous = blockListPtr->Last;
		blockListPtr->Last = blockListPtr->Last->next;
		blockListPtr->Last->next = (BlockNode*) NULL;

		blockListPtr->Last->block_ptr = createCopyBlock(block_ptr);
		if(blockListPtr->Last->block_ptr == NULL){
			printf("addLastList_BlockList: Failed to allocate memory for Block!\n");
			return -1;
		}	

		blockListPtr->Current = blockListPtr->Last;

		(blockListPtr->items)++;

		return 0;
	}
	else{
		printf("addLastList_BlockList: List is not properly initialised...\n");
		return -1;
	}

}

void printList_BlockList(BlockList blockList,int fd,int mode){ /* Prints List */

	int i=0;
	char buffer[256];

	printf("printList_BlockList: Printing BlockList...\n");

	if(blockList.items > 0){
		blockList.Current = blockList.Head;
		while(blockList.Current->next != NULL){
			if(blockList.Current->block_ptr == NULL){
				printf("|%d| - BLOCK IS NULL!\n",i+1); 
				exit(1);
			}
			else{
				if(blockList.Current->block_ptr->process_ptr == NULL){
					printf("|%d| - |beginAddress: %d| - |Size: %d| - |State: FREE| - |BuddyLevel: %d| - |Process: NULL|\n",i+1, blockList.Current->block_ptr->beginAddress, blockList.Current->block_ptr->size, blockList.Current->block_ptr->buddyLevel);
					if(mode == 1){
						sprintf(buffer, "|%d| - |beginAddress: %d| - |Size: %d| - |State: FREE| - |BuddyLevel: %d| - |Process: NULL|\n",i+1, blockList.Current->block_ptr->beginAddress, blockList.Current->block_ptr->size, blockList.Current->block_ptr->buddyLevel);
						write(fd,buffer,sizeof(char)*(strlen(buffer)));			
					}
				}
				else{
					printf("|%d| - |beginAddress: %d| - |Size: %d| - |State: TAKEN| - |BuddyLevel: %d| - |Process: %d|\n",i+1, blockList.Current->block_ptr->beginAddress, blockList.Current->block_ptr->size, blockList.Current->block_ptr->buddyLevel,  blockList.Current->block_ptr->process_ptr->pid);
					if(mode == 1){
						sprintf(buffer, "|%d| - |beginAddress: %d| - |Size: %d| - |State: TAKEN| - |BuddyLevel: %d| - |Process: %d|\n",i+1, blockList.Current->block_ptr->beginAddress, blockList.Current->block_ptr->size, blockList.Current->block_ptr->buddyLevel, blockList.Current->block_ptr->process_ptr->pid); 
						write(fd,buffer,sizeof(char)*(strlen(buffer)));			
					}
				}			
			}
			blockList.Current = blockList.Current->next;
			i++;
		}

		if(blockList.Current->block_ptr == NULL){
			printf("|%d| - BLOCK IS NULL!\n",i+1); 
			exit(1);
		}
		else{
			if(blockList.Current->block_ptr->process_ptr == NULL){
					printf("|%d| - |beginAddress: %d| - |Size: %d| - |State: FREE| - |BuddyLevel: %d| - |Process: NULL|\n",i+1, blockList.Current->block_ptr->beginAddress, blockList.Current->block_ptr->size, blockList.Current->block_ptr->buddyLevel);
					if(mode == 1){
						sprintf(buffer, "|%d| - |beginAddress: %d| - |Size: %d| - |State: FREE| - |BuddyLevel: %d| - |Process: NULL|\n",i+1, blockList.Current->block_ptr->beginAddress, blockList.Current->block_ptr->size, blockList.Current->block_ptr->buddyLevel);
						write(fd,buffer,sizeof(char)*(strlen(buffer)));			
					}
			}
			else{
					printf("|%d| - |beginAddress: %d| - |Size: %d| - |State: TAKEN| - |BuddyLevel: %d| - |Process: %d|\n",i+1, blockList.Current->block_ptr->beginAddress, blockList.Current->block_ptr->size, blockList.Current->block_ptr->buddyLevel, blockList.Current->block_ptr->process_ptr->pid);
					if(mode == 1){
						sprintf(buffer, "|%d| - |beginAddress: %d| - |Size: %d| - |State: TAKEN| - |BuddyLevel: %d| - |Process: %d|\n",i+1, blockList.Current->block_ptr->beginAddress, blockList.Current->block_ptr->size, blockList.Current->block_ptr->buddyLevel, blockList.Current->block_ptr->process_ptr->pid); 
						write(fd,buffer,sizeof(char)*(strlen(buffer)));			
					}
			}			
		}
	}
	else{
		printf("printList_BlockList: Nothing to print in List...\n");
		if(mode == 1){
			sprintf(buffer, "BlockList is empty?! No Blocks in memory!"); 
			write(fd,buffer,sizeof(char)*(strlen(buffer)));			
		}
	}

}


void popList_BlockList(BlockList* blockListPtr){ /* Removes 1 Node from the Start of the List */
												 /* WARNING: This function does not deal with whether the Block is FREE or TAKEN! */

	if(blockListPtr->items > 0){

		blockListPtr->Current = blockListPtr->Head;

		blockListPtr->Head = blockListPtr->Head->next; /* Make Head point to its next and make the previous of new Head be NULL */
		if(blockListPtr->Head != NULL)
			blockListPtr->Head->previous = (BlockNode*) NULL;
		
		if(blockListPtr->Current != NULL){
			if(blockListPtr->Current->block_ptr != NULL){
				if(blockListPtr->Current->block_ptr->process_ptr != NULL){
					free(blockListPtr->Current->block_ptr->process_ptr);
				}
				free(blockListPtr->Current->block_ptr);
			}
			free(blockListPtr->Current);
			(blockListPtr->items)--;
		}

		blockListPtr->Current = blockListPtr->Head;

	}

}

void destroyList_BlockList(BlockList* blockListPtr){ /* Destroys List */

	if(blockListPtr == NULL) return;

	while(blockListPtr->items > 0) popList_BlockList(blockListPtr);

	return;
}

/*----Memory functions----*/

int createMemory(Memory* memoryPtr, int S){

	Block* initialBlockPtr = (Block*) NULL;

	if(memoryPtr == NULL){
		printf("Manager - createMemory: memoryPtr is NULL!\n");
		return -1;
	}

	createList_BlockList(&(memoryPtr->blockList), S);
	memoryPtr->size = S;

	return 0;

}

int insertProcessAtBlock(BlockList* blockListPtr, BlockNode* targetBlockNode, Process process){ //Returns 1 if Process inserted successfully, 0 is not, -1 if memory error
														
	Block* targetBlock = (Block*) NULL;
	BlockNode* newBlockNode = (BlockNode*) NULL;

	if(targetBlockNode == NULL){
		printf("insertProcessAtBlock: targetBlockNode is NULL!\n");
		return -1;
	}

	targetBlock = targetBlockNode->block_ptr;
	
	if(targetBlock->state == FREE){
		if(targetBlock->size == process.size){
			//printf("insertProcessAtBlock: Process fits exactly in Block!\n");
			targetBlock->state = TAKEN;
			targetBlock->process_ptr = createCopyProcess(&process);
			(blockListPtr->free_blocks)--;
			(blockListPtr->taken_blocks)++;
			blockListPtr->totalFreeMem = blockListPtr->totalFreeMem - process.size;
			blockListPtr->totalMemUsage = blockListPtr->totalMemUsage + process.size;
			//printf("insertProcessAtBlock: Process inserted successfully in Block!\n");

			return 1;
		}
		else if(targetBlock->size > process.size){
			//printf("insertProcessAtBlock: Process fits in Block! Block must split in two!\n");
			newBlockNode = malloc(sizeof(BlockNode));
			if(newBlockNode == NULL){
				printf("insertProcessAtBlock: Failed to allocate Memory for new Block Node!\n");
				return -1;				
			}
			newBlockNode->next = targetBlockNode->next;
			newBlockNode->previous = targetBlockNode;
			if(newBlockNode->next != NULL) newBlockNode->next->previous = newBlockNode;
			targetBlockNode->next = newBlockNode;
			newBlockNode->block_ptr = createBlock(FREE, targetBlock->size - process.size, targetBlock->beginAddress + process.size, NULL);
			
			targetBlock->size = process.size;
			targetBlock->state = TAKEN;
			targetBlock->process_ptr = createCopyProcess(&process);

			(blockListPtr->taken_blocks)++;
			blockListPtr->totalFreeMem = blockListPtr->totalFreeMem - process.size;
			blockListPtr->totalMemUsage = blockListPtr->totalMemUsage + process.size;

			(blockListPtr->items)++;
			//printf("insertProcessAtBlock: Process inserted successfully in Block!\n");

			if(targetBlockNode == blockListPtr->Last) blockListPtr->Last = newBlockNode; //New Last
			return 1;
		}
		else{
			printf("insertProcessAtBlock: Block is too small!\n");
			return 0;
		}
	}
	else{
		printf("insertProcessAtBlock: Can't insert in non FREE Block!\n");
		return 0;
	}

	return 0;
}

/*----Memory Management Algorithms----*/

int nextFit_algorithm(Memory* memoryPtr, Process process){

	static int previousAddress = 0;
	int located = 0;
	int rv = 0;

	BlockNode* tempNode = (BlockNode*) NULL;
	BlockNode* tempNode2 = (BlockNode*) NULL;
	BlockNode* targetBlockNode = (BlockNode*) NULL;
	Block* tempBlock = (Block*) NULL;
	Block* tempBlock2 = (Block*) NULL;
	BlockNode* previousBlockNode = (BlockNode*) NULL;

	printf("Manager - nextFit_algorithm: Attempting to place PID: %d using NEXT-FIT!\n", process.pid);

	if(memoryPtr == NULL){
		printf("Manager - nextFit_algorithm: memoryPtr is NULL!\n");
		return -1;
	}

	//printf("\n-----------------------------NEXT FIT-----------------------------------\n");
	//printList_BlockList(memoryPtr->blockList, -1, -1);
	if(memoryPtr->blockList.totalFreeMem < process.size){
		printf("Manager - nextFit_algorithm: Total Free Memory: %d is less than required Memory: %d\n", memoryPtr->blockList.totalFreeMem, process.size);
		return 0;
	}

	if(memoryPtr->blockList.items == 1){ //List Contains 1 (FREE) Block
		tempNode = memoryPtr->blockList.Head;
		if(tempNode->block_ptr == NULL){
			printf("Manager - nextFit_algorithm: tempNode->block_ptr is NULL!\n");
			exit(1);
		}
		if(tempNode->block_ptr->state != FREE){
			printf("Manager - nextFit_algorithm: List contains 1 non Free block! Something went wrong!\n");
			exit(1);
		}
		previousAddress = 0;
		targetBlockNode = tempNode;
	}
	else{ //List contains more Blocks
		//Attempt to locate where we previously inserted a process
		tempNode = memoryPtr->blockList.Head;
		while(tempNode != NULL){
			tempBlock = (Block*) tempNode->block_ptr;
			if(tempBlock == NULL){
				printf("Manager - nextFit_algorithm: tempBlock is NULL!\n");
				exit(1);		
			}
			if(tempBlock->beginAddress < previousAddress){
				if(tempBlock->beginAddress + tempBlock->size > previousAddress){ //Current Block contains the address we previously inserted
					//printf("Manager - nextFit_algorithm: Located BLOCK which contains the previousAddress = (%d)! \n", previousAddress);
					//Beginning now a circular search to locate a FREE Block with enough space
					located = 0;
					tempNode2 = tempNode->next;
					if(tempNode2 == NULL) tempNode2 = memoryPtr->blockList.Head; //Switch back to start of Queue
					while(tempNode2 != tempNode){
						if(tempNode2->block_ptr->state == FREE){
							if(tempNode2->block_ptr->size >= process.size){
								located = 1;
								targetBlockNode = tempNode2;
								previousAddress = targetBlockNode->block_ptr->beginAddress; //Set to remember where we insert now for next time
								break;
							}
						}
						tempNode2 = tempNode2->next;
						if(tempNode2 == NULL) tempNode2 = memoryPtr->blockList.Head;
					}
					if(located == 1) break;
					else{
						if(tempNode->block_ptr == FREE){ //Attempt as a last resort to fit Process in the Block we began the search from
							if(tempNode->block_ptr->size >= process.size){
								located = 1;
								targetBlockNode = tempNode;
								previousAddress = tempNode->block_ptr->beginAddress;
								//printf("Manager - nextFit_algorithm: Process will be inserted in Block with beginAddress: %d\n", tempNode->block_ptr->beginAddress);
								break;
							}
						}
						else{ //No block big enough for this process exists
							//printf("Manager - nextFit_algorithm: Process with PID: %d can't fit in memory!\n", process.pid);
							return 0;
						}
					}
				}
			}
			else if(tempBlock->beginAddress == previousAddress){
				//printf("Manager - nextFit_algorithm: Located BLOCK we previously inserted a Process! previousAddress = (%d)!\n", previousAddress);
				located = 0;
				if((tempBlock->state == FREE) && (tempBlock->size >= process.size)){
					located = 1;
					targetBlockNode = tempNode;
					previousAddress = targetBlockNode->block_ptr->beginAddress; //Set to remember where we insert now for next time
					//printf("Manager - nextFit_algorithm: Process will be inserted in Block with beginAddress: %d\n", tempNode->block_ptr->beginAddress);
					break;						
				}
				else{ //Begin search for next fitting block
					tempNode2 = tempNode->next;
					if(tempNode2 == NULL) tempNode2 = memoryPtr->blockList.Head; //Switch back to start of Queue
					while(tempNode2 != tempNode){
						if(tempNode2->block_ptr->state == FREE){
							if(tempNode2->block_ptr->size >= process.size){
								located = 1;
								targetBlockNode = tempNode2;
								previousAddress = targetBlockNode->block_ptr->beginAddress; //Set to remember where we insert now for next time
								break;
							}
						}
						tempNode2 = tempNode2->next;
						if(tempNode2 == NULL) tempNode2 = memoryPtr->blockList.Head;
					}
					if(located == 1) break;
					else{		
						//printf("Manager - nextFit_algorithm: Process with PID: %d can't fit in memory!\n", process.pid);
						return 0;		
					}
				}
			}
			else{
				printf("Manager - nextFit_algorithm: Passed Address! Error!\n");
				exit(1);
			}
			tempNode = tempNode->next;		
		}
	}

	//Insert at minimum fitting block
	rv = insertProcessAtBlock(&(memoryPtr->blockList), targetBlockNode, process);
	if(rv == 1){
		printf("Manager - nextFit_algorithm: Process placed in Memory successfully!\n");
		//printList_BlockList(memoryPtr->blockList, -1, -1);
		return 1;
	}
	else if(rv == -1){
		printf("Manager - nextFit_algorithm: Memory error in process placement!\n");
		exit(1);
	}
	else{
		printf("Manager - nextFit_algorithm: Process didn't fit in block for some reason?!\n");
		exit(1);
	}

	return 0;

}

int bestFit_algorithm(Memory* memoryPtr, Process process){

	int minAddress = 0;
	int minSize = -1;
	int rv = 0;
	BlockNode* tempNode = (BlockNode*) NULL;
	Block* tempBlock = (Block*) NULL;
	BlockNode* minBlockNode = (BlockNode*) NULL;

	//printf("Manager - bestFit_algorithm: Attempting to place PID: %d using BEST-FIT!\n", process.pid);

	if(memoryPtr == NULL){
		printf("Manager - bestFit_algorithm: memoryPtr is NULL!\n");
		return -1;
	}

	//printf("\n-----------------------------BEST FIT-----------------------------------\n");
	//printList_BlockList(memoryPtr->blockList, -1, -1);
	if(memoryPtr->blockList.totalFreeMem < process.size){
		printf("Manager - bestFist_algorithm: Total Free Memory: %d is less than required Memory: %d\n", memoryPtr->blockList.totalFreeMem, process.size);
		return 0;
	}


	tempNode = memoryPtr->blockList.Head;
	minAddress = 0;
	minSize = -1;
	while(tempNode != NULL){ //Search for the minimum fitting block
		tempBlock = (Block*) tempNode->block_ptr;
		if(tempBlock == NULL){
			printf("Manager - bestFit_algorithm: tempBlock is NULL!\n");
			return -1;			
		}
		if(tempBlock->state == FREE){
			if(tempBlock->size >= process.size){
				if(minSize == -1){	
					minAddress = tempBlock->beginAddress;
					minSize = tempBlock->size;
					minBlockNode = tempNode;
				}
				else if(minSize > tempBlock->size){
					minAddress = tempBlock->beginAddress;
					minSize = tempBlock->size;
					minBlockNode = tempNode;
				}
			}
		}
		tempNode = tempNode->next;
	}

	if(minSize == -1){
		printf("Manager - bestFit_algorithm: No available Free Block big enough for Process!\n");
		return 0;
	}

	//Insert at minimum fitting block
	rv = insertProcessAtBlock(&(memoryPtr->blockList), minBlockNode, process);
	if(rv == 1){
		//printf("Manager - bestFit_algorithm: Located Minimum Block Address: %d\n", minAddress);
		printf("Manager - bestFit_algorithm: Process placed in Memory successfully!\n");
		//printList_BlockList(memoryPtr->blockList, -1, -1);
		//printf("Manager - bestFit_algorithm: RETUNING TO SHORE!!\n");
		return 1;
	}
	else if(rv == -1){
		printf("Manager - bestFit_algorithm: Memory error in process placement!\n");
		exit(1);
	}
	else{
		printf("Manager - bestFit_algorithm: Process didn't fit in block for some reason?!\n");
		exit(1);
	}

	return 0;

}

/*int buddy_algorithm(Memory* memoryPtr, Process process){

	//printf("Manager - bestFit_algorithm: Attempting to place PID: %d using BEST-FIT!\n", process.pid);

	if(memoryPtr == NULL){
		printf("Manager - bestFit_algorithm: memoryPtr is NULL!\n");
		return -1;
	}

	//printf("\n-----------------------------BUDDY-----------------------------------\n");
	//printList_BlockList(memoryPtr->blockList, -1, -1);
	if(memoryPtr->blockList.totalFreeMem < process.size){
		printf("Manager - buddy_algorithm: Total Free Memory: %d is less than required Memory: %d\n", memoryPtr->blockList.totalFreeMem, process.size);
		return 0;
	}

	if(memoryPtr->blockList.items == 1){
		if(memoryPtr->blockList.Head->block_ptr->state != FREE){
			printf("Manager - bestFit_algorithm: BlockList consists only of 1 non Free Block!\n");
			exit(1);	
		}

		tempBlockNode = memoryPtr->blockList.Head;
		tempBlock = tempBlockNode->block_ptr;	
		if(tempBlock->size == process.size){
			tempBlock->state = TAKEN;
			tempBlock->process_ptr = createCopyProcess(process);
			tempBlock->buddyLevel = 0;
			return 1;
		}
		else{
			tempSize = tempBlock->size;
			tempLevel = 0;
			do{
				tempSize = tempSize/2;
				if(tempSize >= process.size){
					tempLevel++;
				}
				else{
					break;
				}
			} while(1);
		}
	}


	//Insert at minimum fitting block
	rv = insertProcessAtBlock(&(memoryPtr->blockList), targetBlockNode, process);
	if(rv == 1){
		printf("Manager - buddy_algorithm: Process placed in Memory successfully!\n");
		printList_BlockList(memoryPtr->blockList, -1, -1);
		return 1;
	}
	else if(rv == -1){
		printf("Manager - buddy_algorithm:: Memory error in process placement!\n");
		exit(1);
	}
	else{
		printf("Manager - buddy_algorithm:: Process didn't fit in block for some reason?!\n");
		exit(1);
	}

	return 0;

}*/

int removeProcessFromMemory(Memory* memoryPtr, int pid, int* waitTime){

	int located = 0;
	BlockNode* tempNode = (BlockNode*) NULL;
	BlockNode* tempNode2 = (BlockNode*) NULL;
	Block* tempBlock = (Block*) NULL;

	if(memoryPtr == NULL){
		printf("Manager - removeProcessFromMemory: memoryPtr is NULL!\n");
		return -1;
	}

	//printf("----------------------REMOVE PROCESS (PID: %d)------------------------\n", pid);
	//printList_BlockList(memoryPtr->blockList, -1, -1);
	tempNode = memoryPtr->blockList.Head;
	while(tempNode != NULL){ //Search and locate pid
		tempBlock = (Block*) tempNode->block_ptr;
		if(tempBlock == NULL){
			printf("Manager -removeProcessFromMemory: tempBlock is NULL!\n");
			exit(1);			
		}
		if(tempBlock->state == TAKEN){
			if(tempBlock->process_ptr != NULL){
				if(tempBlock->process_ptr->pid == pid){
					located = 1;
					tempBlock->state = FREE;
					*waitTime = tempBlock->process_ptr->totalWaitTime; //Return its waitTime before removing from Memory
					//printf("LALALALA: WAITTIME: %d\n", *waitTime);
					free(tempBlock->process_ptr);
					tempBlock->process_ptr = (Process*) NULL;
					memoryPtr->blockList.totalFreeMem = memoryPtr->blockList.totalFreeMem + tempBlock->size;
					(memoryPtr->blockList.taken_blocks)--;
					(memoryPtr->blockList.free_blocks)++;
					memoryPtr->blockList.totalMemUsage = memoryPtr->blockList.totalMemUsage - tempBlock->size;

					printf("Manager - removeProcessFromMemory: Process with |PID: %d| located and removed!\n", pid);

					//Now attempt to merge with neighbour free blocks
					if(tempNode->previous != NULL){
						if(tempNode->previous->block_ptr->state == FREE){ //Merge with previous
							tempNode->block_ptr->size = tempNode->block_ptr->size + tempNode->previous->block_ptr->size; //Add previous' block size to mine
							tempNode->block_ptr->beginAddress = tempNode->previous->block_ptr->beginAddress; //Take previous' beginAddress
							if(tempNode->previous->previous != NULL){
								tempNode->previous->previous->next = tempNode;
							}
							else{ //Previous is Head
								memoryPtr->blockList.Head = tempNode; //Make Current new HEAD
							}
							tempNode2 = tempNode->previous;
							tempNode->previous = tempNode->previous->previous;
							free(tempNode2->block_ptr);
							free(tempNode2);

							(memoryPtr->blockList.free_blocks)--;
							(memoryPtr->blockList.items)--;

							//printf("Manager - removeProcessFromMemory: Merged with PREVIOUS Block!\n");
							//printList_BlockList(memoryPtr->blockList);	
						}
					}
					if(tempNode->next != NULL){
						if(tempNode->next->block_ptr->state == FREE){ //Merge with next
							tempNode->block_ptr->size = tempNode->block_ptr->size + tempNode->next->block_ptr->size; //Add next' block size to mine
							if(tempNode->next->next != NULL){
								tempNode->next->next->previous = tempNode;
							}
							else{ //Next is Last
								memoryPtr->blockList.Last = tempNode; //Make Current new HEAD
							}
							tempNode2 = tempNode->next;
							tempNode->next = tempNode->next->next;
							free(tempNode2->block_ptr);
							free(tempNode2);

							(memoryPtr->blockList.free_blocks)--;
							(memoryPtr->blockList.items)--;

							//printf("Manager - removeProcessFromMemory: Merged with NEXT Block!\n");
							//printList_BlockList(memoryPtr->blockList);	
						}
					}
					//printList_BlockList(memoryPtr->blockList, -1, -1);				
					return 1;					
				}
			}
			else{
				printf("Manager -removeProcessFromMemory: Block is NULL but TAKEN ?!\n");
				exit(1);
			}
		}
		tempNode = tempNode->next;
	}

	if(located == 0){
		printf("Manager - removeProcessFromMemory: PID was not found in BlockList!\n");
		return 0;
	}

	return 0;

}
