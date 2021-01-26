
#include "clusti.h"

#include "clusti_status_priv.h"
#include "clusti_types_priv.h"
#include "clusti_mem_priv.h"

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

/* -------------------------------------------------------------------------- */
/* Internal function  forward decls. */

/**
 * @brief Read a text line of arbitrary length from file.
	  Returned buffer must be freed

 * @param f file pointer
 * @return String containing the text line or NULL on error.
 * @details See  https://stackoverflow.com/questions/29576799/reading-an-unknown-length-line-from-stdin-in-c-with-fgets/29576944 
*/
//char *clusti_getline(FILE *f);

/**
 * @brief Read a file of arbitrary length from file into a buffer.
 * @param configPath 
 * @return 
*/
char *clusti_loadFileContents(const char *configPath);


void clusti_Parser_startElement_callback(void *userdata,
	const char *elementName,
	const char **attributeNamesAndValues);

void clusti_Parser_endElement_callback(void *userdata,
					 const char *elementName);


/* -------------------------------------------------------------------------- */




//char *clusti_getline(FILE *f)
//{
//	size_t size = 0;
//	size_t len = 0;
//	size_t last = 0;
//	char *buf = NULL;
//	/* to omit memory leak*/
//	char *buf_temp = NULL;
//
//	do {
//		/* BUFSIZ is defined as "the optimal read size for this platform" */
//		size += BUFSIZ;
//		/* realloc(NULL,n) is the same as malloc(n) */
//		buf_temp = realloc(
//			buf,
//			size); 
//		if (buf_temp == NULL) {
//			free(buf);
//			return NULL;
//		}
//		buf = buf_temp;
//
//		/* Actually do the read. Note that fgets puts a terminal
//		   '\0' on the end of the string, so we make sure we overwrite this */
//		fgets(buf + last, size, f);
//		len = strlen(buf);
//		last = len - 1;
//	} while (!feof(f) && buf[last] != '\n');
//
//	return buf;
//}


char* clusti_loadFileContents(const char* configPath) {

	FILE *fp = NULL;
	long lSize = 0L;
	char *buffer = NULL;

	//fp = fopen(configPath, "rb");
	errno_t err = fopen_s(&fp, configPath, "rb");
	if (!fp || (err != 0)  ) {
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


void clusti_readConfig(Clusti *instance,
				char const *configPath)
{
	assert(instance);
	instance->parsingState.configPath = configPath;
	instance->parsingState.currentElementIndex = -1;

	char *fileContents = clusti_loadFileContents(configPath);
	assert(fileContents);
	printf("[File contents:]\n %s \n[End of file contents]\n",
		fileContents);
	
	XML_Parser parser = XML_ParserCreate("utf-8");

	XML_SetElementHandler(parser,
		clusti_Parser_startElement_callback,
		clusti_Parser_endElement_callback);

	// N.b. no character data expected in config file,
	// hence no need for a callback
	//XML_SetCharacterDataHandler(parser, handle_data);

	XML_SetUserData(parser, instance);

	/* parse the xml */
	if (XML_Parse(parser, fileContents, (int) strlen(fileContents), XML_TRUE) ==
	    XML_STATUS_ERROR) {
		printf("Error: %s\n", XML_ErrorString(XML_GetErrorCode(parser)));
	}

	clusti_free(fileContents);

	XML_ParserFree(parser);
}



void clusti_parseVideoSinks(Clusti *instance, Clusti_State_Parsing *parser,
			    const char **attributeNamesAndValues);
void clusti_parseVideoSources(Clusti *instance, Clusti_State_Parsing *parser,
			      const char **attributeNamesAndValues);
void clusti_parseVideoSink(Clusti *instance, Clusti_State_Parsing *parser,
			   const char **attributeNamesAndValues);
void clusti_parseVideoSource(Clusti *instance, Clusti_State_Parsing *parser,
			   const char **attributeNamesAndValues);

void clusti_parseWarping(Clusti *instance, Clusti_State_Parsing *parser,
			 const char **attributeNamesAndValues);
void clusti_parseBlending(Clusti *instance, Clusti_State_Parsing *parser,
			  const char **attributeNamesAndValues);

void clusti_parseRectangle(Clusti *instance, Clusti_State_Parsing *parser,
			     const char **attributeNamesAndValues);
// also for "VirtualResolution" XML tag
void clusti_parseResolution(Clusti *instance, Clusti_State_Parsing *parser,
			     const char **attributeNamesAndValues);
void clusti_parseProjection(Clusti *instance, Clusti_State_Parsing *parser,
			     const char **attributeNamesAndValues);
void clusti_parseFrustum(Clusti *instance, Clusti_State_Parsing *parser,
			     const char **attributeNamesAndValues);
void clusti_parseFOV(Clusti *instance, Clusti_State_Parsing *parser,
			     const char **attributeNamesAndValues);
void clusti_parseOrientation(Clusti *instance, Clusti_State_Parsing *parser,
			     const char **attributeNamesAndValues);






void clusti_parseVideoSinks(Clusti *instance, Clusti_State_Parsing *parser,
			    const char **attributeNamesAndValues)
{
	assert(parser->currentParsingMode == CLUSTI_ENUM_PARSING_MODES_general);
	parser->currentParsingMode = CLUSTI_ENUM_PARSING_MODES_videoSinks;
	// reset index
	parser->currentElementIndex = 0;

	assert(instance->stitchingConfig.numVideoSinks == 0);

	for (size_t i = 0; attributeNamesAndValues[i]; i += 2) {
		if ((strcmp("num", attributeNamesAndValues[i]) == 0)) {
			instance->stitchingConfig.numVideoSinks =
				atoi(attributeNamesAndValues[i + 1]);
			printf("numVideoSinks: %d \n",
			       instance->stitchingConfig.numVideoSinks);	
		} else {
			printf("unexpected xml attribute: %s\n",
			       attributeNamesAndValues[i]);
			clusti_status_declareError("unexpected xml attribute");
		}
	}

	assert(instance->stitchingConfig.numVideoSinks > 0);

	// allocate array for the sinks 
	instance->stitchingConfig.videoSinks =
		clusti_calloc(instance->stitchingConfig.numVideoSinks,
			      sizeof(Clusti_Params_VideoSink));

}

void clusti_parseVideoSources(Clusti *instance, Clusti_State_Parsing *parser,
			      const char **attributeNamesAndValues)
{
	assert(parser->currentParsingMode == CLUSTI_ENUM_PARSING_MODES_general);
	parser->currentParsingMode = CLUSTI_ENUM_PARSING_MODES_videoSources;
	// reset index
	parser->currentElementIndex = 0;

	// init'ed to 0 by calloc();
	assert(instance->stitchingConfig.numVideoSources == 0);

	for (size_t i = 0; attributeNamesAndValues[i]; i += 2) {
		if ((strcmp("num", attributeNamesAndValues[i]) == 0)) {
			instance->stitchingConfig.numVideoSources =
				atoi(attributeNamesAndValues[i + 1]);
			printf("numVideoSources: %d \n",
			       instance->stitchingConfig.numVideoSources);
		} else {
			printf("unexpected xml attribute: %s\n",
			       attributeNamesAndValues[i]);
			clusti_status_declareError("unexpected xml attribute");
		}
	}

	assert(instance->stitchingConfig.numVideoSources > 0);

	// allocate array for the sinks
	instance->stitchingConfig.videoSources =
		clusti_calloc(instance->stitchingConfig.numVideoSources,
			      sizeof(Clusti_Params_VideoSource));
}


void clusti_parseVideoSink(Clusti *instance, Clusti_State_Parsing *parser,
			   const char **attributeNamesAndValues)
{
	assert(parser->currentParsingMode ==
	       CLUSTI_ENUM_PARSING_MODES_videoSinks);

	assert(parser->currentElementIndex >= 0);
	assert(parser->currentElementIndex <
	       instance->stitchingConfig.numVideoSinks);

	assert(instance->stitchingConfig.videoSinks);

	Clusti_Params_VideoSink* sink = &(instance->stitchingConfig.videoSinks[parser->currentElementIndex]);

	for (size_t i = 0; attributeNamesAndValues[i]; i += 2) {
		if ((strcmp("index", attributeNamesAndValues[i]) == 0)) {
			sink->index = atoi(attributeNamesAndValues[i + 1]);
		} else if ((strcmp("name", attributeNamesAndValues[i]) == 0)) {
			assert((sink->name) == NULL);
			sink->name = clusti_String_callocAndCopy(
				attributeNamesAndValues[i + 1]);
		} else if ((strcmp("debug_renderScale", attributeNamesAndValues[i]) == 0)) {
			sink->debug_renderScale =
				(float)atof(attributeNamesAndValues[i + 1]);
		} else if ((strcmp("debug_backgroundImageName", attributeNamesAndValues[i]) == 0)) {
			assert((sink->debug_backgroundImageName) == NULL);
			sink->debug_backgroundImageName =
				clusti_String_callocAndCopy(
				attributeNamesAndValues[i + 1]);
		}
		else {
			printf("unexpected xml attribute: %s\n",
			       attributeNamesAndValues[i]);
			clusti_status_declareError("unexpected xml attribute");
		}
	}
}

void clusti_parseVideoSource(Clusti *instance, Clusti_State_Parsing *parser,
			     const char **attributeNamesAndValues)
{
	assert(parser->currentParsingMode ==
	       CLUSTI_ENUM_PARSING_MODES_videoSources);

	assert(parser->currentElementIndex >= 0);
	assert(parser->currentElementIndex <
	       instance->stitchingConfig.numVideoSources);

	assert(instance->stitchingConfig.videoSources);

	Clusti_Params_VideoSource *source =
		&(instance->stitchingConfig
			  .videoSources[parser->currentElementIndex]);

	for (size_t i = 0; attributeNamesAndValues[i]; i += 2) {
		if ((strcmp("index", attributeNamesAndValues[i]) == 0)) {
			source->index = atoi(attributeNamesAndValues[i + 1]);
		} else if ((strcmp("name", attributeNamesAndValues[i]) == 0)) {
			assert((source->name) == NULL);
			source->name = clusti_String_callocAndCopy(
				attributeNamesAndValues[i + 1]);
		} else if ((strcmp("testImageName",
				   attributeNamesAndValues[i]) == 0)) {
			assert((source->testImageName) == NULL);
			source->testImageName =
				clusti_String_callocAndCopy(attributeNamesAndValues[i + 1]);
		} else if ((strcmp("decklinkWorkaround_verticalOffset_pixels",
				   attributeNamesAndValues[i]) == 0)) {
			source->decklinkWorkaround_verticalOffset_pixels =
				(float)atof(attributeNamesAndValues[i + 1]);
		} else {

			printf("unexpected xml attribute: %s\n",
			       attributeNamesAndValues[i]);
			clusti_status_declareError("unexpected xml attribute");
		}
	}
}





void clusti_parseWarping(Clusti *instance, Clusti_State_Parsing *parser,
			 const char **attributeNamesAndValues)
{
	Clusti_Params_Warping warp = {.use = false,
				      .type = "none", //"imageLUT2D",
				      .invert = true,
				      .warpfileBaseName = NULL};


	//TODO CONTINUE HERE; MAYBE USE SDS lib for strings! https://github.com/antirez/sds
	// or strstr()

	//instance->stitchingConfig.
	//Clusti_Params_VideoSource *source =
	//	&(instance->stitchingConfig
	//		  .videoSources[parser->currentElementIndex]);
	//source-> = vec;

	//TODO
}

void clusti_parseBlending(Clusti *instance, Clusti_State_Parsing *parser,
			  const char **attributeNamesAndValues)
{
	//TODO
}




void clusti_parseRectangle(Clusti *instance, Clusti_State_Parsing *parser,
			   const char **attributeNamesAndValues)
{
	Clusti_iRectangle rect = {.min = {0, 0}, .max = {0, 0}};

	for (size_t i = 0; attributeNamesAndValues[i]; i += 2) {
		if ((strcmp("x_min", attributeNamesAndValues[i]) == 0)) {
			rect.min.x = atoi(attributeNamesAndValues[i + 1]);
		} else if((strcmp("x_max", attributeNamesAndValues[i]) == 0)){
			rect.min.y = atoi(attributeNamesAndValues[i + 1]);
		} else if ((strcmp("y_min", attributeNamesAndValues[i]) == 0)) {
			rect.max.x = atoi(attributeNamesAndValues[i + 1]);
		} else if ((strcmp("y_max", attributeNamesAndValues[i]) == 0)) {
			rect.max.y = atoi(attributeNamesAndValues[i + 1]);
		}
		else
		{
			printf("unexpected xml attribute: %s\n",
			       attributeNamesAndValues[i]);
			clusti_status_declareError("unexpected xml attribute");
		}
	}

	Clusti_Params_VideoSink *sink =
		&(instance->stitchingConfig
			  .videoSinks[parser->currentElementIndex]);
	sink->cropRectangle = rect;

	//TODO test
}

// also for "VirtualResolution" XML tag
void clusti_parseResolution(Clusti *instance, Clusti_State_Parsing *parser,
			    const char **attributeNamesAndValues)
{
	Clusti_ivec2 vec = {0, 0};

	// do the parsing:
	for (size_t i = 0; attributeNamesAndValues[i]; i += 2) {
		if ((strcmp("x", attributeNamesAndValues[i]) == 0)) {
			vec.x = atoi(attributeNamesAndValues[i + 1]);
		} else if ((strcmp("y", attributeNamesAndValues[i]) == 0)) {
			vec.y = atoi(attributeNamesAndValues[i + 1]);
		}  else {
			printf("unexpected xml attribute: %s\n",
			       attributeNamesAndValues[i]);
			clusti_status_declareError("unexpected xml attribute");
		}
	}

	// distinguish source or sink:
	if (parser->currentParsingMode ==
	    CLUSTI_ENUM_PARSING_MODES_videoSinks) {
		Clusti_Params_VideoSink *sink =
			&(instance->stitchingConfig
				  .videoSinks[parser->currentElementIndex]);
		sink->virtualResolution = vec;
	} else if (parser->currentParsingMode ==
		   CLUSTI_ENUM_PARSING_MODES_videoSources) {
		Clusti_Params_VideoSource *source =
			&(instance->stitchingConfig
				  .videoSources[parser->currentElementIndex]);
		source->resolution = vec;
	} else {
		clusti_status_declareError("Internal error: inconistent parsing mode");	
	}
		

	//TODO test
}

void clusti_parseProjection(Clusti *instance, Clusti_State_Parsing *parser,
			    const char **attributeNamesAndValues)
{
	Clusti_Params_Projection *proj = NULL;

	// distinguish source or sink:
	if (parser->currentParsingMode ==
	    CLUSTI_ENUM_PARSING_MODES_videoSinks) {
		Clusti_Params_VideoSink *sink =
			&(instance->stitchingConfig
				  .videoSinks[parser->currentElementIndex]);
		proj = &(sink->projection);
	} else if (parser->currentParsingMode ==
		   CLUSTI_ENUM_PARSING_MODES_videoSources) {
		Clusti_Params_VideoSource *source =
			&(instance->stitchingConfig
				  .videoSources[parser->currentElementIndex]);
		proj = &(source->projection);
	} else {
		clusti_status_declareError(
			"Internal error: inconistent parsing mode");
	}

	assert(proj);

	// do the parsing:
	for (size_t i = 0; attributeNamesAndValues[i]; i += 2) {
		if ((strcmp("type", attributeNamesAndValues[i]) == 0)) {
			if ((strcmp("equirectangular",
				    attributeNamesAndValues[i + 1]) == 0)) {
				proj->type =
					CLUSTI_ENUM_PROJECTION_TYPE_EQUIRECT;
			} else if ((strcmp("planar",
					   attributeNamesAndValues[i + 1]) ==
				    0)) {
				proj->type =
					CLUSTI_ENUM_PROJECTION_TYPE_PLANAR;
			} else {
				proj->type =
					CLUSTI_ENUM_PROJECTION_TYPE_INVALID;
				printf("(yet) unsupported projection type: %s\n",
				       attributeNamesAndValues[i + 1]);
				clusti_status_declareError(
					"unsupported projection type in xml attribute");
			}
			//proj->type =
		} else {
			printf("unexpected xml attribute: %s\n",
			       attributeNamesAndValues[i]);
			clusti_status_declareError("unexpected xml attribute");
		}
	}

	//TODO test
}

void clusti_parseFrustum(Clusti *instance, Clusti_State_Parsing *parser,
			 const char **attributeNamesAndValues)
{
	//TODO
}

void clusti_parseFOV(Clusti *instance, Clusti_State_Parsing *parser,
		     const char **attributeNamesAndValues)
{
	//TODO
}

void clusti_parseOrientation(Clusti *instance, Clusti_State_Parsing *parser,
			     const char **attributeNamesAndValues)
{
	//TODO
}








void clusti_Parser_startElement_callback(void *userdata,
					 const char *elementName,
					 const char **attributeNamesAndValues)
{
	Clusti *instance = (Clusti *)userdata;
	assert(instance);
	Clusti_State_Parsing *parser = &(instance->parsingState);


	if (strcmp(elementName, "Cluster") == 0) {
		// toplevel, all shall general parsing mode
		assert(parser->currentParsingMode ==
		       CLUSTI_ENUM_PARSING_MODES_general);
	} else if (strcmp(elementName, "Stitching") == 0) {
		assert(parser->currentParsingMode ==
		       CLUSTI_ENUM_PARSING_MODES_general);

	} else if (strcmp(elementName, "VideoSinks") == 0) {
		clusti_parseVideoSinks(instance, parser,
				       attributeNamesAndValues);
	} else if (strcmp(elementName, "VideoSink") == 0) {
		clusti_parseVideoSink(instance, parser,
				       attributeNamesAndValues);



	} else if (strcmp(elementName, "VideoSources") == 0) {
		clusti_parseVideoSources(instance, parser,
				       attributeNamesAndValues);

	} else if (strcmp(elementName, "VideoSource") == 0) {
		clusti_parseVideoSource(instance, parser,
				      attributeNamesAndValues);


	} else if (strcmp(elementName, "VirtualResolution") == 0) {
		clusti_parseResolution(instance, parser,
				       attributeNamesAndValues);
	} else if (strcmp(elementName, "CropRectangle") == 0) {
		clusti_parseRectangle(instance, parser,
				       attributeNamesAndValues);
	} else if (strcmp(elementName, "Projection") == 0) {
		clusti_parseProjection(instance, parser, attributeNamesAndValues);
	} else if (strcmp(elementName, "Orientation") == 0) {
		clusti_parseOrientation(instance, parser, attributeNamesAndValues);
	} else if (strcmp(elementName, "Warping") == 0) {
		clusti_parseWarping(instance, parser, attributeNamesAndValues);
	} else if (strcmp(elementName, "Blending") == 0) {
		clusti_parseBlending(instance, parser, attributeNamesAndValues);
	} else if (strcmp(elementName, "Resolution") == 0) {
		clusti_parseResolution(instance, parser,
				       attributeNamesAndValues);
	} else if (strcmp(elementName, "Frustum") == 0) {
		clusti_parseFrustum(instance, parser, attributeNamesAndValues);
	} else if (strcmp(elementName, "FOV") == 0) {
		clusti_parseFOV(instance, parser, attributeNamesAndValues);
	} else  {
		printf("unexpected xml element: %s\n", elementName);
		clusti_status_declareError("unexpected XML element");
	}


	////test
	//if (strcmp(elementName, "VideoSource") == 0) {
	//	for (size_t i = 0; attributeNamesAndValues[i]; i += 2) {
	//		if ((strcmp("name", attributeNamesAndValues[i]) ==
	//		     0) &&
	//		    (strcmp("ArenaRT4",
	//			    attributeNamesAndValues[i + 1]) == 0)) {
	//			printf("node with name %s found.\n",
	//			       attributeNamesAndValues[i + 1]);
	//		}
	//		
	//		
	//	}
	//}

}




void clusti_Parser_endElement_callback(void *userdata, const char *elementName)
{
	Clusti *instance = (Clusti *)userdata;
	assert(instance);
	Clusti_State_Parsing *parser = &(instance->parsingState);

	if (strcmp(elementName, "VideoSinks") == 0) {
		assert(parser->currentParsingMode ==
		       CLUSTI_ENUM_PARSING_MODES_videoSinks);

		// reset parsing mode
		parser->currentParsingMode = CLUSTI_ENUM_PARSING_MODES_general;
		// reset index
		parser->currentElementIndex = 0;

	} else if (strcmp(elementName, "VideoSink") == 0) {
		assert(parser->currentParsingMode ==
		       CLUSTI_ENUM_PARSING_MODES_videoSinks);

		// advance index
		parser->currentElementIndex += 1;

	} else if (strcmp(elementName, "VideoSources") == 0) {

		assert(parser->currentParsingMode ==
		       CLUSTI_ENUM_PARSING_MODES_videoSources);

		// reset parsing mode
		parser->currentParsingMode = CLUSTI_ENUM_PARSING_MODES_general;
		// reset index
		parser->currentElementIndex = 0;

	} else if (strcmp(elementName, "VideoSource") == 0) {
		assert(parser->currentParsingMode ==
		       CLUSTI_ENUM_PARSING_MODES_videoSources);

		// advance index for next element
		parser->currentElementIndex += 1;
	}
}
