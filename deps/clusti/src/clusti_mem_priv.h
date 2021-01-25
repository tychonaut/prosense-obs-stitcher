#ifndef CLUSTI_MEM_PRIV_H
#define CLUSTI_MEM_PRIV_H

// Memory allocation and leak detection

// forward decl, as both mem and status implementations need pointers to it
struct Clusti_DoublyLinkedList;
typedef struct Clusti_DoublyLinkedList Clusti_DoublyLinkedList;

//#define malloc(X) perror("ERROR_USE_CALLOC_INSTEAD")

#define clusti_calloc(NUM_INSTANCES, NUM_BYTES_PER_INSTANCE)              \
	clusti_calloc_internal((NUM_INSTANCES), (NUM_BYTES_PER_INSTANCE), \
			       __FILE__, __LINE__, __FUNCTION__)

void *clusti_calloc_internal(size_t numInstances, size_t numBytesPerInstance,
			     const char *file, int line, const char *func);

#define clusti_free(PTR) \
	clusti_free_internal(PTR, __FILE__, __LINE__, __FUNCTION__)

void clusti_free_internal(void *ptr, const char *file, int line,
			  const char *func);

char const *clusti_String_callocAndCopy(char const *message);

#endif // CLUSTI_MEM_PRIV_H
