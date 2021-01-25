


#include "clusti.h"
#include "clusti_types_priv.h"

#include <assert.h>


/* private forwards*/




Clusti_Stitcher* clusti_Stitcher_create() {

	// alloc and init all to zero
	Clusti_Stitcher *stitcher =
		(Clusti_Stitcher *)calloc(1, sizeof(Clusti_Stitcher));

	if (stitcher == NULL){
		assert(0);
		exit(-1);
	}

	return stitcher;
}

/**
 * @brief Destroy Stitcher object
*/
void clusti_Stitcher_destroy(Clusti_Stitcher *stitcher) {
	free(stitcher);
}




