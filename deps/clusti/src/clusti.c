

#include "clusti.h"

#include "clusti_mem_priv.h"
#include "clusti_status_priv.h"
#include "clusti_types_priv.h"

#include <assert.h>

/* private forwards*/

Clusti_Stitcher *clusti_Stitcher_create()
{

	// alloc and init all to zero
	Clusti_Stitcher *stitcher =
		(Clusti_Stitcher *)clusti_calloc(1, sizeof(Clusti_Stitcher));

	assert(stitcher != NULL);

	if (stitcher == NULL) {
		perror("Stitcher alloc failed!");
		exit(-1);
	}

	clusti_status_declareInitialized();

	return stitcher;
}

/**
 * @brief Destroy Stitcher object
*/
void clusti_Stitcher_destroy(Clusti_Stitcher *stitcher)
{
	//TODO free stichter's pointer members


	clusti_free(stitcher);


	clusti_status_declareDeinitialized();
}
