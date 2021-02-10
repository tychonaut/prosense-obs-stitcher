

#include "clusti_status_priv.h"

#include "clusti_mem_priv.h"


#include <stdlib.h> // calloc
#include <stdio.h>  // printf
#include <assert.h> // assert


// global status object
Clusti_Status g_clustiStatus = {.currentStatusType =
					CLUSTI_STATUS_not_initialized,
				.statusMessage = NULL,
				.memoryRegistry = NULL};

void clusti_status_declareInitialized()
{
	//first call?
	if (g_clustiStatus.numClustiInstances == 1) {
		assert(g_clustiStatus.currentStatusType ==
		       CLUSTI_STATUS_not_initialized);
		assert("null string" && (g_clustiStatus.statusMessage == NULL));
		assert("mem registry created" &&
		       g_clustiStatus.memoryRegistry != NULL);

		g_clustiStatus.currentStatusType = CLUSTI_STATUS_initialized;
	}

	//char const *copiedMsg = clusti_String_callocAndCopy(message);
	//g_clustiStatus.statusMessage = copiedMsg;
}

void clusti_status_declareError(char const *message)
{

	assert("not more than one error stacking please" &&
	       g_clustiStatus.currentStatusType == CLUSTI_STATUS_initialized);

	g_clustiStatus.currentStatusType = CLUSTI_STATUS_error;

	g_clustiStatus.statusMessage = message;

	//perror(message);

	// maybe later try to recover gracefully, but crash for now
	exit(-1);
}

void clusti_status_declareDeinitialized()
{
	//last call?
	if (g_clustiStatus.numClustiInstances == 0) {
		assert((g_clustiStatus.currentStatusType ==
			CLUSTI_STATUS_initialized) ||
		       (g_clustiStatus.currentStatusType ==
			CLUSTI_STATUS_error));
		assert("all mem freed " &&
		       g_clustiStatus.memoryRegistry == NULL);

		// only constant strings here, nothing to free
		g_clustiStatus.statusMessage = NULL;

		g_clustiStatus.currentStatusType =
			CLUSTI_STATUS_not_initialized;
	}
}

Clusti_StatusType clusti_status_getCurrentStatusType() {
	return g_clustiStatus.currentStatusType;
}

char const* clusti_status_getStatusMessage() {
	return g_clustiStatus.statusMessage;
}
