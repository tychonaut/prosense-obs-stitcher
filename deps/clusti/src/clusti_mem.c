

#include "clusti_mem_priv.h"

#include "clusti_status_priv.h"

//#ifdef malloc
//#undef malloc
//#endif
//
//#ifdef calloc
//#undef calloc
//#endif

#include <stdlib.h> // calloc
#include <stdio.h>  // printf
#include <string.h> // strncpy_s
#include <assert.h> // assert

// global status object, defined in clusti_status.c
extern Clusti_Status g_clustiStatus;

//struct Clusti_MemoryRegistry;
//typedef struct Clusti_MemoryRegistry Clusti_MemoryRegistry;

struct Clusti_MemoryChunk;
typedef struct Clusti_MemoryChunk Clusti_MemoryChunk;

struct Clusti_DoublyLinkedMemoryChunkListItem;
typedef struct Clusti_DoublyLinkedMemoryChunkListItem
	Clusti_DoublyLinkedMemoryChunkListItem;

struct Clusti_MemoryRegistry {
	long numAllocations;
	long totalUsedCPUMemory;
	Clusti_DoublyLinkedMemoryChunkListItem *firstMemoryItem;
	Clusti_DoublyLinkedMemoryChunkListItem *lastMemoryItem;
};

struct Clusti_MemoryChunk {
	void *data;
	long sizeInBytes;
	//char *info;
};

struct Clusti_DoublyLinkedMemoryChunkListItem {
	Clusti_MemoryChunk memoryChunk;
	Clusti_DoublyLinkedMemoryChunkListItem *previous;
	Clusti_DoublyLinkedMemoryChunkListItem *next;
};

// internal forwards
void clusti_mem_registerNewAllocation(void *ptr, size_t sizeInBytes);
void clusti_mem_unregisterAllocation(void *ptr);

void clusti_mem_printSummary();

// can return null if not found
Clusti_DoublyLinkedMemoryChunkListItem *clusti_findMemoryItem(void *ptr);

//-----------------------------------------------------------------------------

void clusti_mem_init()
{
	assert(g_clustiStatus.memoryRegistry == NULL);

	g_clustiStatus.memoryRegistry =
		calloc(1, sizeof(Clusti_MemoryRegistry));

	assert(g_clustiStatus.memoryRegistry != NULL);

	assert(g_clustiStatus.memoryRegistry->numAllocations == 0);
	assert(g_clustiStatus.memoryRegistry->totalUsedCPUMemory == 0);
	// fails for some reason:
	//assert(g_clustiStatus.memoryRegistry->firstMemoryItem ==
	//       g_clustiStatus.memoryRegistry->lastMemoryItem == NULL);
	assert(g_clustiStatus.memoryRegistry->firstMemoryItem == NULL);
	assert(g_clustiStatus.memoryRegistry->lastMemoryItem == NULL);
}

void clusti_mem_deinit()
{
	assert(g_clustiStatus.memoryRegistry != NULL);

	assert(g_clustiStatus.memoryRegistry->numAllocations == 0);
	assert(g_clustiStatus.memoryRegistry->totalUsedCPUMemory == 0);
	assert(g_clustiStatus.memoryRegistry->firstMemoryItem == NULL);
	assert(g_clustiStatus.memoryRegistry->lastMemoryItem == NULL);

	free(g_clustiStatus.memoryRegistry);
	g_clustiStatus.memoryRegistry = NULL;

	assert(g_clustiStatus.memoryRegistry == NULL);
}

void clusti_mem_registerNewAllocation(void *ptr, size_t sizeInBytes)
{
	assert(sizeInBytes > 0);
	assert("mem inited" && g_clustiStatus.memoryRegistry != NULL);
	g_clustiStatus.memoryRegistry->numAllocations += 1;
	g_clustiStatus.memoryRegistry->totalUsedCPUMemory += sizeInBytes;

	Clusti_DoublyLinkedMemoryChunkListItem *newItem =
		calloc(1, sizeof(Clusti_DoublyLinkedMemoryChunkListItem));

	assert(newItem != NULL);
	if (newItem == NULL) {
		clusti_status_declareError("calloc failed!");
	}
	//assert(newItem->previous == newItem->next == NULL);
	assert(newItem->previous == NULL);
	assert(newItem->next == NULL);

	newItem->memoryChunk.data = ptr;
	newItem->memoryChunk.sizeInBytes = sizeInBytes;

	Clusti_DoublyLinkedMemoryChunkListItem *first =
		g_clustiStatus.memoryRegistry->firstMemoryItem;
	Clusti_DoublyLinkedMemoryChunkListItem *last =
		g_clustiStatus.memoryRegistry->lastMemoryItem;

	if (last == NULL) {
		// we register the first of the library's payload allocation here:
		assert(first == NULL);
		// inite main structure's pointer to first item to new item
		g_clustiStatus.memoryRegistry->firstMemoryItem = newItem;
		first = last = newItem;
	} else {
		//interlink succeding items
		last->next = newItem;
		newItem->previous = last;
	}
	// update main structure's pointer to last item
	g_clustiStatus.memoryRegistry->lastMemoryItem = newItem;
}

void clusti_mem_unregisterAllocation(void *ptr)
{
	Clusti_DoublyLinkedMemoryChunkListItem *toDelete =
		clusti_findMemoryItem(ptr);

	assert(toDelete != NULL);
	if (toDelete == NULL) {
		clusti_status_declareError(
			"memory item to be deleted cannot be not found im memory registry");
	}

	// remove item from doubly linked list, then free
	Clusti_DoublyLinkedMemoryChunkListItem *pred = toDelete->previous;
	Clusti_DoublyLinkedMemoryChunkListItem *succ = toDelete->next;
	if (pred != NULL) {
		assert(pred->next == toDelete);
		pred->next = succ;
	} else {
		// predecessor is null means we are deleting the first item.
		// so adapt the first-pointer to next item (can also vbe null)
		g_clustiStatus.memoryRegistry->firstMemoryItem = succ;
	}

	if (succ != NULL) {
		assert(succ->previous == toDelete);
		succ->previous = pred;
	} else {
		// successor is null means we are deleting the last item.
		// so adapt the last-pointer to previous item (can also vbe null)
		g_clustiStatus.memoryRegistry->lastMemoryItem = pred;
	}

	assert(g_clustiStatus.memoryRegistry);
	g_clustiStatus.memoryRegistry->numAllocations -= 1;
	assert(g_clustiStatus.memoryRegistry->numAllocations >= 0);

	g_clustiStatus.memoryRegistry->totalUsedCPUMemory -=
		toDelete->memoryChunk.sizeInBytes;
	assert(g_clustiStatus.memoryRegistry->totalUsedCPUMemory >= 0);

	free(toDelete);
	toDelete = NULL;
}

Clusti_DoublyLinkedMemoryChunkListItem *clusti_findMemoryItem(void *ptr)
{
	assert("mem inited" && g_clustiStatus.memoryRegistry != NULL);
	assert(ptr != NULL);

	Clusti_DoublyLinkedMemoryChunkListItem *first =
		g_clustiStatus.memoryRegistry->firstMemoryItem;
	Clusti_DoublyLinkedMemoryChunkListItem *last =
		g_clustiStatus.memoryRegistry->lastMemoryItem;

	Clusti_DoublyLinkedMemoryChunkListItem *currentItem = first;
	while (currentItem != NULL && currentItem->memoryChunk.data != ptr) {
		currentItem = currentItem->next;
	}

	if (currentItem == NULL) {
		clusti_status_declareError(
			"memory item not found im memory registry");
	}

	return currentItem;
}

void *clusti_calloc_internal(size_t numInstances, size_t numBytesPerInstance,
			     const char *file, int line, const char *func)
{

	void *p = calloc(numInstances, numBytesPerInstance);

	if (p == NULL) {

		//g_clustiStatus.currentStatusType = CLUSTI_STATUS_error;
		//perror("calloc failed!");
	}

	printf("Allocated = %s, %i, %s, %p[%lli]\n", file, line, func, p,
	       (long long)numBytesPerInstance * (long long)numInstances);

	/*Link List functionality goes in here*/
	clusti_mem_registerNewAllocation(p, (long)numBytesPerInstance *
						    (long)numInstances);

	clusti_mem_printSummary();

	return p;
}

void clusti_free_internal(void *ptr, const char *file, int line,
			  const char *func)
{
	clusti_mem_unregisterAllocation(ptr);

	clusti_mem_printSummary();

	free(ptr);
}

char const *clusti_String_callocAndCopy(char const *orig)
{
#define CLUSTI_MAX_EXPECTED_STRING_LENGTH (1000000)

	size_t siz = strnlen_s(orig, CLUSTI_MAX_EXPECTED_STRING_LENGTH) *
		     sizeof(char);

	assert(siz < CLUSTI_MAX_EXPECTED_STRING_LENGTH);
	if (siz >= CLUSTI_MAX_EXPECTED_STRING_LENGTH) {
		clusti_status_declareError(
			"string does not seem to be null terminated");
	}

	// +1 because of null termination
	char *result = clusti_calloc(1, siz + 1);

	errno_t err = strncpy_s(result, siz, orig, siz);
	assert(strcmp(orig, result) == 0);

	if (err != 0) {
		clusti_status_declareError("string copy failed");
	}

	return result;
}

void clusti_mem_printItem(int index, int numTotalItems, Clusti_DoublyLinkedMemoryChunkListItem* item)
{
	assert(item);

	printf("Item #%d/%d: num bytes allocated: %d;\n", index,
	       numTotalItems,
	       item->memoryChunk.sizeInBytes);
}

void clusti_mem_printSummary()
{
	assert(g_clustiStatus.memoryRegistry);
	printf("Clusti Memory summary:\n");
	printf("Total allocated items: %d;\n", g_clustiStatus.memoryRegistry->numAllocations);
	printf("Total bytes allocated: %d;\n",
	       g_clustiStatus.memoryRegistry->totalUsedCPUMemory);

	Clusti_DoublyLinkedMemoryChunkListItem *first =
		g_clustiStatus.memoryRegistry->firstMemoryItem;
	Clusti_DoublyLinkedMemoryChunkListItem *last =
		g_clustiStatus.memoryRegistry->lastMemoryItem;
	Clusti_DoublyLinkedMemoryChunkListItem *current = first;
	int index = 0;
	while (current != NULL ) {
		clusti_mem_printItem(
			index, g_clustiStatus.memoryRegistry->numAllocations,
			current);
		index += 1;
		current = current->next;
	}

}
