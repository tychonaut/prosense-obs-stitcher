#ifndef CLUSTI_MEM_PRIV_H
#define CLUSTI_MEM_PRIV_H


/*
   Two technical purposes of clusti_mem :
   1. Memory allocation, tracking and leak detection
   2. String helpers for allocation and manipulation.

   Q: Why did I write stuff like this instead of using valgrind and
   an existing String library?

   A: I haven't program in pure C for a long time. I know that memory management
   is one of the biggest pitfalls to newbies to introduce subtle bugs that are hard
   to find and eradicate. As this project is non-trivial, I decided to practice and
   monitor memory management at the same time.
   Furthermore, C libraries tend to have older build systems than CMake
   that often target Unix-like environments. Windows ports are half-hearted,
   e.g. some old Visual studio Solution file instead of a CMake script.

   Keeping in mind that I am developing a piece of software that is to be
   easily distributed to both users and other developers,
   portable to various OSes and both Gstreamer and OBS Studio,
   and does't have too many licencing issues,
   I try to
	1. Stick to pure C, although C++ would make the whole development much easier,
	   but at the price of future portability issues.
	2. keep the number of dependencies low
	3. Keep the dependencies themselves small, simple
	   and as easy to build as possible without coding too much CMake scripts
	   (This project's complexity is already exploding)

   The consequence is, unfortunately, that I spend a lot of time with the basics:
   - Looking a small and easy-to-build XML library, I found libexpat after libXML2 and miniXML
     seemed too big and complicated to build to match above criteria.
   - libexpat is a stream-based parsing library, that doesn't build a tree of C-structs
     matching the XML structure. So, "building the tree" has to be done by hand.
   - The tree-building involves string copy and manipulation, requiring understanding both
     the details of the C string library functions and some book-keeping to prevent memory leaks.
   And here we are, writing low level code when just wanting to read an XML file in C under
   Windows (because OBS Studio has a bug with Decklink Capture cards in Linux).

 */

#include <stdbool.h> // bool


// ----------------------------------------------------------------------------
// pure memory management forwards

void clusti_mem_init();
void clusti_mem_deinit();



#define clusti_calloc(NUM_INSTANCES, NUM_BYTES_PER_INSTANCE)              \
	clusti_calloc_internal((NUM_INSTANCES), (NUM_BYTES_PER_INSTANCE), \
			       __FILE__, __LINE__, __FUNCTION__)

void *clusti_calloc_internal(size_t numInstances, size_t numBytesPerInstance,
			     const char *file, int line, const char *func);

#define clusti_free(PTR) \
	clusti_free_internal(PTR, __FILE__, __LINE__, __FUNCTION__)

void clusti_free_internal(void *ptr, const char *file, int line,
			  const char *func);
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// String related forwards

//
char *clusti_String_callocAndCopy(char const *str);
//
void clusti_String_reallocAndCopy(char **dst, char const *src);


// returned string must be freed
char *clusti_String_lowerCase(char const *str);
// accepts e.g. "true","1","yes","Y", no matter the casing
bool clusti_String_impliesTrueness(char const *str);
// ----------------------------------------------------------------------------

#endif // CLUSTI_MEM_PRIV_H
