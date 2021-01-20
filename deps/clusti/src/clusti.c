


#include "clusti.h"
#include "clusti_types.h"

#include <assert.h>


/* private forwards*/




Clusti_Stitcher* clusti_Stitcher_create() {

	Clusti_Stitcher *stitcher =
		(Clusti_Stitcher *)malloc(sizeof(Clusti_Stitcher));

	// init all to zero
	if (stitcher != NULL) {
		memset(stitcher, 0, sizeof(Clusti_Stitcher));
	} else {
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




