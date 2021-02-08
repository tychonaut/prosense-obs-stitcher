
#include "clusti.h"

// clusti_math_ViewProjectionMatrixFromFrustumAndOrientation
#include "clusti_math.h" 

#include "clusti_types_priv.h"
// clusti_status_declareError
#include "clusti_status_priv.h" 
// clusti_calloc, clusti_free,
// clusti_String_callocAndCopy, clusti_String_impliesTrueness
#include "clusti_mem_priv.h" 


#ifdef HAVE_EXPAT_CONFIG_H
#include <expat_config.h>
#endif
#include <expat.h>


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> // tolower

#include <assert.h> // assert

// ----------------------------------------------------------------------------
// Internal function  forward decls. 





void clusti_Parser_startElement_callback(void *userdata,
	const char *elementName,
	const char **attributeNamesAndValues);

void clusti_Parser_endElement_callback(void *userdata,
					 const char *elementName);


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
void clusti_parseFrustumFOV(Clusti *instance, Clusti_State_Parsing *parser,
		     const char **attributeNamesAndValues);
void clusti_parseOrientation(Clusti *instance, Clusti_State_Parsing *parser,
			     const char **attributeNamesAndValues);
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
// function impls.




//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// TODO outsource to clusti math files





//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------






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

	//------------------------------------
	// create ViewProjectionMatrices
	for (int i = 0; i < instance->stitchingConfig.numVideoSources; i++) {
		clusti_math_ViewProjectionMatrixFromFrustumAndOrientation(&(
			instance->stitchingConfig.videoSources[i].projection));
	}
	//------------------------------------



	clusti_free(fileContents);

	XML_ParserFree(parser);
}








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
		} else if ((strcmp("warpFileName",
				   attributeNamesAndValues[i]) == 0)) {
			assert((source->warpfileName) == NULL);
			source->warpfileName = clusti_String_callocAndCopy(
				attributeNamesAndValues[i + 1]);
		} else if ((strcmp("blendImageName",
				   attributeNamesAndValues[i]) == 0)) {
			assert((source->blendImageName) == NULL);
			source->blendImageName = clusti_String_callocAndCopy(
				attributeNamesAndValues[i + 1]);
		} else if ((strcmp("testImageName",
				   attributeNamesAndValues[i]) == 0)) {
			assert((source->testImageName) == NULL);
			source->testImageName =
				clusti_String_callocAndCopy(attributeNamesAndValues[i + 1]);
		} else if ((strcmp("decklinkWorkaround_verticalOffset_pixels",
				   attributeNamesAndValues[i]) == 0)) {
			source->decklinkWorkaround_verticalOffset_pixels =
				atoi(attributeNamesAndValues[i + 1]);
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
				      .type = CLUSTI_ENUM_WARPING_TYPE_none,
				      .invert = true};

	for (size_t i = 0; attributeNamesAndValues[i]; i += 2) {
		if ((strcmp("use", attributeNamesAndValues[i]) == 0)) {
			warp.use = clusti_String_impliesTrueness(
				attributeNamesAndValues[i + 1]);
		} else if ((strcmp("type", attributeNamesAndValues[i]) == 0)) {
			if (strcmp("none", attributeNamesAndValues[i + 1]) ==
			    0) {
				warp.type = CLUSTI_ENUM_WARPING_TYPE_none;
			} else if (strcmp("imageLUT2D",
					  attributeNamesAndValues[i + 1]) ==
				   0) {
				warp.type = CLUSTI_ENUM_WARPING_TYPE_imageLUT2D;
			} else if (strcmp("imageLUT3D",
					  attributeNamesAndValues[i + 1]) ==
				   0) {
				warp.type = CLUSTI_ENUM_WARPING_TYPE_imageLUT3D;
			} else if (strcmp("mesh2D",
					  attributeNamesAndValues[i + 1]) ==
				   0) {
				warp.type = CLUSTI_ENUM_WARPING_TYPE_mesh2D;
			} else if (strcmp("mesh3D",
					  attributeNamesAndValues[i + 1]) ==
				   0) {
				warp.type = CLUSTI_ENUM_WARPING_TYPE_mesh3D;
			} else  {
				warp.type = CLUSTI_ENUM_WARPING_TYPE_none;

				printf("unexpected warp type attribute: %s\n",
				       attributeNamesAndValues[i +1]);
				clusti_status_declareError(
					"unexpected warp type attribute");
			}
		} else if ((strcmp("invert",
				   attributeNamesAndValues[i]) == 0)) {
			warp.invert = clusti_String_impliesTrueness(
				attributeNamesAndValues[i + 1]);
		} else {
			printf("unexpected xml attribute: %s\n",
			       attributeNamesAndValues[i]);
			clusti_status_declareError("unexpected xml attribute");
		}
	}

	instance->stitchingConfig.generalWarpParams = warp;


	//TODO test


	// TODO general, if string hassle becomes too cumbersome:
	// MAYBE USE SDS lib for strings! https://github.com/antirez/sds
	// or strstr()
}

void clusti_parseBlending(Clusti *instance, Clusti_State_Parsing *parser,
			  const char **attributeNamesAndValues)
{
	Clusti_Params_Blending blend = {
		.use = false, .autoGenerate = false, .fadingRange_pixels = 128};

	for (size_t i = 0; attributeNamesAndValues[i]; i += 2) {
		if ((strcmp("use", attributeNamesAndValues[i]) == 0)) {
			blend.use = clusti_String_impliesTrueness(
				attributeNamesAndValues[i + 1]);
		}  else if ((strcmp("autoGenerate", attributeNamesAndValues[i]) ==
			    0)) {
			blend.autoGenerate = clusti_String_impliesTrueness(
				attributeNamesAndValues[i + 1]);
		} else if ((strcmp("fadingRange_pixels",
				   attributeNamesAndValues[i]) == 0)) {
			blend.fadingRange_pixels =
				atoi(attributeNamesAndValues[i + 1]);
		} else {
			printf("unexpected xml attribute: %s\n",
			       attributeNamesAndValues[i]);
			clusti_status_declareError("unexpected xml attribute");
		}
	}

	instance->stitchingConfig.generalBlendParams = blend;

	//TODO test
}




void clusti_parseRectangle(Clusti *instance, Clusti_State_Parsing *parser,
			   const char **attributeNamesAndValues)
{
	Clusti_iRectangle rect = {.lowerLeft = {0, 0}, .extents = {0, 0}};

	for (size_t i = 0; attributeNamesAndValues[i]; i += 2) {
		if ((strcmp("lowerLeft_x", attributeNamesAndValues[i]) == 0)) {
			rect.lowerLeft.x = atoi(attributeNamesAndValues[i + 1]);
		} else if((strcmp("lowerLeft_y", attributeNamesAndValues[i]) == 0)){
			rect.lowerLeft.y = atoi(attributeNamesAndValues[i + 1]);
		} else if ((strcmp("extents_x", attributeNamesAndValues[i]) == 0)) {
			rect.extents.x = atoi(attributeNamesAndValues[i + 1]);
		} else if ((strcmp("extents_y", attributeNamesAndValues[i]) == 0)) {
			rect.extents.y = atoi(attributeNamesAndValues[i + 1]);
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


// get projection from current parsing state and index
Clusti_Params_Projection *
clusti_Parser_getCurrentProjection(Clusti const *instance, Clusti_State_Parsing const *parser)
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

	return proj;
}


void clusti_parseProjection(Clusti *instance, Clusti_State_Parsing *parser,
			    const char **attributeNamesAndValues)
{
	Clusti_Params_Projection *proj =
		clusti_Parser_getCurrentProjection(instance, parser);

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
				proj->type = CLUSTI_ENUM_PROJECTION_TYPE_PLANAR;
			} else if ((strcmp("fishEye",
					   attributeNamesAndValues[i + 1]) ==
				    0)) {
				proj->type =
					CLUSTI_ENUM_PROJECTION_TYPE_FISHEYE;
			} else {
				proj->type =
					CLUSTI_ENUM_PROJECTION_TYPE_INVALID;
				printf("(yet) unsupported projection type: %s\n",
				       attributeNamesAndValues[i + 1]);
				clusti_status_declareError(
					"(yet) unsupported projection type in xml attribute");
			}
			//proj->type =
		} else if ((strcmp("fishEye_aperture_angle", attributeNamesAndValues[i]) == 0)) {
			proj->fisheye_aperture_degrees =
				(float)atof(attributeNamesAndValues[i + 1]);
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
	Clusti_Params_Projection *proj =
		clusti_Parser_getCurrentProjection(instance, parser);

	// nothing to parse here, just consistency check
	assert(proj->type == CLUSTI_ENUM_PROJECTION_TYPE_PLANAR);

	//TODO test
}

void clusti_parseFrustumFOV(Clusti *instance, Clusti_State_Parsing *parser,
		     const char **attributeNamesAndValues)
{
	Clusti_Params_Projection *proj =
		clusti_Parser_getCurrentProjection(instance, parser);

	//consistency check
	assert(proj->type == CLUSTI_ENUM_PROJECTION_TYPE_PLANAR);


	for (size_t i = 0; attributeNamesAndValues[i]; i += 2) {
		if ((strcmp("up", attributeNamesAndValues[i]) == 0)) {
			proj->planar_FrustumFOV.up_degrees =
				(float)atof(attributeNamesAndValues[i + 1]);
		} else if ((strcmp("down", attributeNamesAndValues[i]) == 0)) {
			proj->planar_FrustumFOV.down_degrees =
				(float)atof(attributeNamesAndValues[i + 1]);
		} else if ((strcmp("left", attributeNamesAndValues[i]) == 0)) {
			proj->planar_FrustumFOV.left_degrees =
				(float)atof(attributeNamesAndValues[i + 1]);
		} else if ((strcmp("right", attributeNamesAndValues[i]) == 0)) {
			proj->planar_FrustumFOV.right_degrees =
				(float)atof(attributeNamesAndValues[i + 1]);
		} else if ((strcmp("unit", attributeNamesAndValues[i]) == 0)) {
			if (strcmp(attributeNamesAndValues[i + 1], "degree") != 0) {
				printf("(yet) unsupported angle unit: %s\n",
				       attributeNamesAndValues[i+1]);
				clusti_status_declareError(
					"(yet) unsupported angle unit");
			}
		} else {
			printf("unexpected xml attribute: %s\n",
			       attributeNamesAndValues[i]);
			clusti_status_declareError("unexpected xml attribute");
		}
	}


	//TODO test 
}

void clusti_parseOrientation(Clusti *instance, Clusti_State_Parsing *parser,
			     const char **attributeNamesAndValues)
{
	Clusti_Params_Projection *proj =
		clusti_Parser_getCurrentProjection(instance, parser);

	float yaw = 0.0f;
	float pitch = 0.0f;
	float roll = 0.0f;
	//proj->orientation

	for (size_t i = 0; attributeNamesAndValues[i]; i += 2) {
		if ((strcmp("type", attributeNamesAndValues[i]) == 0)) {
			if (strcmp(attributeNamesAndValues[i + 1],
				   "euler_static_zxy") != 0) {
				printf("(yet) unsupported euler angle type: %s\n",
				       attributeNamesAndValues[i + 1]);
				clusti_status_declareError(
					"(yet) unsupported euler angle type");
			}
		} else if ((strcmp("axisConvention", attributeNamesAndValues[i]) == 0)) {
			if (strcmp(attributeNamesAndValues[i + 1],
				   "DIN9300") != 0) {
				printf("(yet) unsupported euler axis convention type: %s\n",
				       attributeNamesAndValues[i + 1]);
				printf("In this prototype implementation, only DIN9300 is supported");
				clusti_status_declareError(
					"(yet) unsupported euler angle type");
			}
		} else if ((strcmp("unit", attributeNamesAndValues[i]) == 0)) {
			if (strcmp(attributeNamesAndValues[i + 1], "degree") !=
			    0) {
				printf("(yet) unsupported angle unit: %s\n",
				       attributeNamesAndValues[i + 1]);
				clusti_status_declareError(
					"(yet) unsupported angle unit");
			}
		} else if ((strcmp("yaw", attributeNamesAndValues[i]) == 0)) {
			yaw = (float)atof(attributeNamesAndValues[i + 1]);

			// suspecting ISO 9300 axis conventions
			// (pitch axis = right, yaw axis = down, roll axis = front)
			// from VIOSO, , effectively flipping
			// signs of yaw and roll in our right handed,
			// (+x=right +y=up, -z=front) coodinate system
			yaw *= -1.0f;

		} else if ((strcmp("pitch", attributeNamesAndValues[i]) == 0)) {
			pitch = (float)atof(attributeNamesAndValues[i + 1]);
		} else if ((strcmp("roll", attributeNamesAndValues[i]) == 0)) {

			// see above comment
			roll = (float)atof(attributeNamesAndValues[i + 1]);
			roll *= -1.0f;

		} else {
			printf("unexpected xml attribute: %s\n",
			       attributeNamesAndValues[i]);
			clusti_status_declareError("unexpected xml attribute");
		}
	}

	// Create graphene structure from the parsed information
	// GRAPHENE_EULER_ORDER_SZXY,
	// as VIOSO and OpenSpace and ParaView have the euler angle convention
	// "roll pitch yaw" ("zxy", "rotationMat = yawMat * pitchMat *  rollMat")
	// angles are in degrees: https://ebassi.github.io/graphene/docs/graphene-Euler.html
	graphene_euler_init_with_order(&(proj->orientation), pitch, yaw, roll,
				       //orig:
				       GRAPHENE_EULER_ORDER_SZXY);
				       //GRAPHENE_EULER_ORDER_RZXY);
				       //GRAPHENE_EULER_ORDER_ZYX);
				       //test:
				       //GRAPHENE_EULER_ORDER_RZXY);
				       //GRAPHENE_EULER_ORDER_RYXZ);
				       //GRAPHENE_EULER_ORDER_SYXZ);
					//test2:
				       //GRAPHENE_EULER_ORDER_SZXY);

	//TODO test
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
		clusti_parseFrustumFOV(instance, parser, attributeNamesAndValues);
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
