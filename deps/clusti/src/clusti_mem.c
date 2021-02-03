
// ----------------------------------------------------------------------------
// includes

#include "clusti_mem_priv.h"

#include "clusti_status_priv.h"


#include <stdlib.h> // calloc
#include <stdio.h>  // printf
#include <string.h> // strncpy_s
#include <ctype.h>  // tolower

#include <assert.h> // assert
#include <stdbool.h> // bool
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Macros

#define CLUSTI_MAX_EXPECTED_STRING_LENGTH (1000000)
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Types

// global status object, defined in clusti_status.c
extern Clusti_Status g_clustiStatus;

// forwarded in header
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
	char const *filename;
	int lineNumber;
};

struct Clusti_DoublyLinkedMemoryChunkListItem {
	Clusti_MemoryChunk memoryChunk;
	Clusti_DoublyLinkedMemoryChunkListItem *previous;
	Clusti_DoublyLinkedMemoryChunkListItem *next;
};
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// internal function  forwards


void clusti_mem_registerNewAllocation(void *ptr, size_t sizeInBytes,
				      const char *file, int line);
void clusti_mem_unregisterAllocation(void *ptr);

// Helper to maintain doubly linked list that tracks allocations;
// Can return null if not found.
Clusti_DoublyLinkedMemoryChunkListItem *clusti_findMemoryItem(void *ptr);

void clusti_mem_printSummary();
void clusti_mem_printItem(int index, int numTotalItems,
			  Clusti_DoublyLinkedMemoryChunkListItem *item);




//-----------------------------------------------------------------------------
// function implementations (string helpers & string memory management)

// must be clusti_free()'d
char *clusti_String_concat(const char *s1, const char *s2)
{
	// https://stackoverflow.com/questions/8465006/how-do-i-concatenate-two-strings-in-c

	const size_t len1 = strlen(s1);
	const size_t len2 = strlen(s2);
	char *result = clusti_calloc(1, (size_t)(len1 + len2 + 1) * sizeof(char)); // +1 for the null-terminator
	assert(result);

	memcpy(result, s1, len1);
	memcpy(result + len1, s2, len2 + 1); // +1 to copy the null-terminator

	return result;
}


// returned string must be freed
char *clusti_String_lowerCase(char const *str)
{
	char *resultString = clusti_String_callocAndCopy(str);
	for (int i = 0; resultString[i] != '\0'; i++) {
		resultString[i] = tolower(resultString[i]);
	}
	return resultString;
}

bool clusti_String_impliesTrueness(char const *str)
{
	char *tmpLower = clusti_String_lowerCase(str);

	bool ret = (strcmp("true", tmpLower) == 0) ||
		   (strcmp("yes", tmpLower) == 0) ||
		   (strcmp("y", tmpLower) == 0) || (strcmp("1", tmpLower) == 0);

	clusti_free(tmpLower);

	return ret;
}


void clusti_String_reallocAndCopy(char **dst, char const *src)
{
	assert(dst != NULL);

	size_t srcLen = strnlen_s(src, CLUSTI_MAX_EXPECTED_STRING_LENGTH) *
				sizeof(char) +
			1;
	size_t dstLen = strnlen_s(*dst, CLUSTI_MAX_EXPECTED_STRING_LENGTH) *
				sizeof(char) +
			1;
	if (*dst == NULL) {
		*dst = clusti_String_callocAndCopy(src);
	} else if (srcLen <= dstLen) {
		// dst is long enough
		errno_t err = strncpy_s(*dst, srcLen, src, dstLen);
		assert(strcmp(src, *dst) == 0);
		if (err != 0) {
			clusti_status_declareError("String copy failed");
		}
	} else {
		// realloc would be faster, but break the memory management;
		clusti_free(*dst);
		*dst = clusti_String_callocAndCopy(src);
	}
}

char *clusti_String_callocAndCopy(char const *orig)
{
	size_t siz = (1 + strnlen_s(orig, CLUSTI_MAX_EXPECTED_STRING_LENGTH)) *
		     sizeof(char);

	assert(siz < CLUSTI_MAX_EXPECTED_STRING_LENGTH);
	if (siz >= CLUSTI_MAX_EXPECTED_STRING_LENGTH) {
		clusti_status_declareError(
			"string does not seem to be null terminated");
	}

	// +1 because of null termination
	char *result = clusti_calloc(1, siz);

	errno_t err = strncpy_s(result, siz, orig, siz);
	assert(strcmp(orig, result) == 0);
	if (err != 0) {
		clusti_status_declareError("string copy failed");
	}

	return result;
}
// ----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// function implementations (pure memory management )


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

void clusti_mem_registerNewAllocation(void *ptr, size_t sizeInBytes,
				      const char *file, int line)
{
	assert(sizeInBytes > 0);
	assert("mem inited" && g_clustiStatus.memoryRegistry != NULL);
	g_clustiStatus.memoryRegistry->numAllocations += (long) 1;
	g_clustiStatus.memoryRegistry->totalUsedCPUMemory += (long) sizeInBytes;


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
	newItem->memoryChunk.sizeInBytes = (long) sizeInBytes;
	newItem->memoryChunk.filename = file;
	newItem->memoryChunk.lineNumber = line;

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
			     char const *file, int line, const char *func)
{
	void *p = calloc(numInstances, numBytesPerInstance);

	if (p == NULL) {
		clusti_status_declareError("calloc failed!");
	}

	printf("Allocated = %s, %i, %s, %p[%lli]\n", file, line, func, p,
	       (long long)numBytesPerInstance * (long long)numInstances);

	/*Link List functionality goes in here*/
	clusti_mem_registerNewAllocation(
		p, (size_t)numBytesPerInstance * (size_t)numInstances, file,
		line);

	//clusti_mem_printSummary();

	return p;
}

void clusti_free_internal(void *ptr, const char *file, int line,
			  char const *func)
{
	if (ptr == NULL) {
		return; // nothing to free
	}

	clusti_mem_unregisterAllocation(ptr);

	//clusti_mem_printSummary();

	free(ptr);
}


void clusti_mem_printItem(int index, int numTotalItems,
			  Clusti_DoublyLinkedMemoryChunkListItem *item)
{
	assert(item);

	printf("Item #%d/%d: num bytes allocated: %d; Alloc'ed in %s, line %d;\n", index, numTotalItems,
	       item->memoryChunk.sizeInBytes, item->memoryChunk.filename, item->memoryChunk.lineNumber);
}

void clusti_mem_printSummary()
{
	assert(g_clustiStatus.memoryRegistry);
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	printf("Clusti Memory summary:\n");
	printf("Total allocated items: %d;\n",
	       g_clustiStatus.memoryRegistry->numAllocations);
	printf("Total bytes allocated: %d;\n",
	       g_clustiStatus.memoryRegistry->totalUsedCPUMemory);

	Clusti_DoublyLinkedMemoryChunkListItem *first =
		g_clustiStatus.memoryRegistry->firstMemoryItem;
	Clusti_DoublyLinkedMemoryChunkListItem *last =
		g_clustiStatus.memoryRegistry->lastMemoryItem;
	Clusti_DoublyLinkedMemoryChunkListItem *current = first;
	int index = 0;
	while (current != NULL) {
		clusti_mem_printItem(
			index, g_clustiStatus.memoryRegistry->numAllocations,
			current);
		index += 1;
		current = current->next;
	}
	printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");

}
