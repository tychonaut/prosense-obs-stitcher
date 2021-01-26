
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



void clusti_parseVideoSinks(Clusti *instance, Clusti_State_Parsing *parser,
			    const char **attributeNamesAndValues)
{
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
	// assert(instance->stitchingConfig. == 0);
	//TODO
}
void clusti_parseVideoSource(Clusti *instance, Clusti_State_Parsing *parser,
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
		// toplevel, all shall be null
		//assert(parser->currentElementName == 0);
		assert(parser->currentParentElementName == 0);
	} else if (strcmp(elementName, "Stitching") == 0) {
		assert(strcmp(parser->currentParentElementName, 
			      "Cluster") == 0);
	} else if (strcmp(elementName, "VideoSinks") == 0) {
		clusti_parseVideoSinks(instance, parser,
				       attributeNamesAndValues);
	} else if (strcmp(elementName, "VideoSink") == 0) {
		clusti_parseVideoSink(instance, parser,
				       attributeNamesAndValues);
	} else if (strcmp(elementName, "VideoSource") == 0) {
	} else if (strcmp(elementName, "VideoSource") == 0) {
	}

	// update the backtracking info
	clusti_String_reallocAndCopy(&(parser->currentParentElementName),
				     elementName);





	//test
	if (strcmp(elementName, "VideoSource") == 0) {
		for (size_t i = 0; attributeNamesAndValues[i]; i += 2) {
			if ((strcmp("name", attributeNamesAndValues[i]) ==
			     0) &&
			    (strcmp("ArenaRT4",
				    attributeNamesAndValues[i + 1]) == 0)) {
				printf("node with name %s found.\n",
				       attributeNamesAndValues[i + 1]);
			}
			
			
		}

	}

}

void clusti_Parser_endElement_callback(void *userdata, const char *elementName)
{

}
