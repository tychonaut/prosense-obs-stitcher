#ifndef CLUSTI_H
#define CLUSTI_H

// TODO sort out privacy of data structures;
// For quicker programming progress, allow direct access do library user.
// If Sticking to this: TODO create/rename/move haeders appropriately.
#include "clusti_types.h"

// ----------------------------------------------------------------------------
// Type forward decls.

struct Clusti;
typedef struct Clusti Clusti;

// ----------------------------------------------------------------------------
// Function API

/// @brief Constructor
/// @return Clusti_Stitcher with all members set to zero
Clusti *clusti_create();
/// @brief Destructor
void clusti_destroy(Clusti *instance);

void clusti_readConfig(Clusti *instance, char const *configPath);


/// @brief Read a file of arbitrary length from file into a buffer.
/// @param configPath
/// @return string with contents; must be clusti_free()'d.
char *clusti_loadFileContents(char const *configPath);


// ----------------------------------------------------------------------------
// public memory interface:
// Use clusti_free() to delete any heap memory allocated by clusti
// (including strings).
// Feel free to use clusti_calloc() and clusti_free() for your own heap stuff.
// (Just some basic leak detection here, nothing critically important.)

#define clusti_calloc(NUM_INSTANCES, NUM_BYTES_PER_INSTANCE)              \
	clusti_calloc_internal((NUM_INSTANCES), (NUM_BYTES_PER_INSTANCE), \
			       __FILE__, __LINE__, __FUNCTION__)

void *clusti_calloc_internal(size_t numInstances, size_t numBytesPerInstance,
			     char const *file, int line, const char *func);

#define clusti_free(PTR) \
	clusti_free_internal(PTR, __FILE__, __LINE__, __FUNCTION__)

void clusti_free_internal(void *ptr, const char *file, int line,
			  char const *func);
// ----------------------------------------------------------------------------

#endif //CLUSTI_H
