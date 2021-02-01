

#include "clusti.h"

#include "clusti_types_priv.h"
// clusti_status_declareInitialized, clusti_status_declareError,
// clusti_status_declareDeinitialized
#include "clusti_status_priv.h"
// clusti_mem_init, clusti_calloc, clusti_free, clusti_mem_deinit
#include "clusti_mem_priv.h"


#include <stdio.h>  // FILE etc..
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





char *clusti_loadFileContents(char const *configPath)
{

	FILE *fp = NULL;
	long lSize = 0L;
	char *buffer = NULL;

	//fp = fopen(configPath, "rb");
	errno_t err = fopen_s(&fp, configPath, "rb");
	if (!fp || (err != 0)) {
		perror(configPath);
		exit(1);
	}

	/* file length */
	fseek(fp, 0L, SEEK_END);
	lSize = ftell(fp);
	rewind(fp);

	/* allocate memory for entire content */
	// buffer = calloc(1, (size_t)lSize + (size_t)1L);
	buffer = clusti_calloc(1, (size_t)lSize + (size_t)1L);
	if (!buffer)
		fclose(fp), fputs("memory alloc fails", stderr), exit(1);

	/* copy the file into the buffer */
	if (1 != fread(buffer, lSize, 1, fp))
		fclose(fp), free(buffer), fputs("entire read fails", stderr),
			exit(1);

	fclose(fp);

	/* buffer is a string that contains the whole text */
	return buffer;
}

//// Just for fun: same in C++:
//std::string LoadFile(const std::string &filename)
//{
//	std::ifstream file(filename);
//	std::string src((std::istreambuf_iterator<char>(file)),
//			std::istreambuf_iterator<char>());
//	file.close();
//	return src;
//}
