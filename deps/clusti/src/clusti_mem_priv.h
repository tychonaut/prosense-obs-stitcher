#ifndef CLUSTI_MEM_PRIV_H
#define CLUSTI_MEM_PRIV_H

// Memory allocation and leak detection

void clusti_mem_init();
void clusti_mem_deinit();



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

void clusti_String_reallocAndCopy(char **dst, char const *src);

char *clusti_String_callocAndCopy(char const *str);

#endif // CLUSTI_MEM_PRIV_H
