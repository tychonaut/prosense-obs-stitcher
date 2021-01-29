

#include "clusti.h"

#include "clusti_types_priv.h"
// clusti_status_declareInitialized, clusti_status_declareError,
// clusti_status_declareDeinitialized
#include "clusti_status_priv.h"
// clusti_mem_init, clusti_calloc, clusti_free, clusti_mem_deinit
#include "clusti_mem_priv.h"

#include <stdlib.h> // exit
#include <assert.h> // assert



Clusti *clusti_create()
{
	//setup global memory tracker
	clusti_mem_init();

	// alloc and init all to zero
	Clusti *instance =
		(Clusti *)clusti_calloc(1, sizeof(Clusti));

	assert(instance != NULL);

	if (instance == NULL) {
		clusti_status_declareError("Stitcher alloc failed!");
		exit(-1);
	}

	clusti_status_declareInitialized();

	return instance;
}

/**
 * @brief Destroy Stitcher object
*/
void clusti_destroy(Clusti *instance)
{
	assert(instance);

	//TODO free stichter's pointer members
	for (int i = 0; i < instance->stitchingConfig.numVideoSinks; i++) {
		Clusti_Params_VideoSink *sink =
			&(instance->stitchingConfig.videoSinks[i]);
		clusti_free(sink->name);
		clusti_free(sink->debug_backgroundImageName);
	}
	for (int i = 0; i < instance->stitchingConfig.numVideoSources; i++) {
		Clusti_Params_VideoSource *source =
			&(instance->stitchingConfig.videoSources[i]);
		clusti_free(source->name);
		clusti_free(source->warpfileName);
		clusti_free(source->blendImageName);
		clusti_free(source->testImageName);
	}


	clusti_free(instance->stitchingConfig.videoSinks);
	clusti_free(instance->stitchingConfig.videoSources);

	// Free parsing stuff:
	//TODO better free at end of parsing;
	//clusti_free(instance->parsingState.currentParentElementName);

	// free main object
	clusti_free(instance);



	//destroy memory tracker
	clusti_mem_deinit();

	//reset status, final conistency checks
	clusti_status_declareDeinitialized();
}
