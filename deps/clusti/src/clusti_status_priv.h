#ifndef CLUSTI_STATUS_PRIV_H
#define CLUSTI_STATUS_PRIV_H

// Error Handling


//#include "clusti_mem_priv.h"

enum Clusti_StatusType;
typedef enum Clusti_StatusType Clusti_StatusType;
enum Clusti_StatusType {
	CLUSTI_STATUS_not_initialized = 0,
	CLUSTI_STATUS_initialized = 1,
	CLUSTI_STATUS_error = 2
};
#define CLUSTI_STATUSTYPE_NUM 3



// forward decl, as both mem and status implementations need pointers to it
struct Clusti_MemoryRegistry;
typedef struct Clusti_MemoryRegistry Clusti_MemoryRegistry;

struct Clusti_Status;
typedef struct Clusti_Status Clusti_Status;
// definition needed by clusti_mem.c
struct Clusti_Status {
	// error/status stuff:
	Clusti_StatusType currentStatusType;
	char const * statusMessage;

	// memory management stuff
	Clusti_MemoryRegistry *memoryRegistry;
};


// global status object
//extern Clusti_Status g_clustiStatus;


void clusti_status_declareInitialized();
// moved to public interface (clusti.h)
void clusti_status_declareError(char const *message);
void clusti_status_declareDeinitialized();

Clusti_StatusType clusti_status_getCurrentStatusType();
char const *clusti_status_getStatusMessage();

#endif // CLUSTI_STATUS_PRIV_H
