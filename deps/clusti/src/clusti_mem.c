


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



struct Clusti_DoublyLinkedList {
	void *data;
	char *info;
	Clusti_DoublyLinkedList *previous;
	Clusti_DoublyLinkedList *next;
};


void clusti_mem_registerNewAllocation(void* ptr, char* info) {
	//TODO
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

	return p;
}


void clusti_free_internal(void *ptr, const char *file, int line,
			  const char *func)
{

	//TODO

	free(ptr);
}




char const *clusti_String_callocAndCopy(char const *orig)
{
#define CLUSTI_MAX_EXPECTED_STRING_LENGTH (1000000)

	int siz = strnlen_s(orig, CLUSTI_MAX_EXPECTED_STRING_LENGTH) *
		  sizeof(char);

	assert(siz < CLUSTI_MAX_EXPECTED_STRING_LENGTH);
	if (siz >= CLUSTI_MAX_EXPECTED_STRING_LENGTH) {
		clusti_status_declareError("string does not seem to be null terminated");
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
