/*
Copyright (C) 2017 by Artyom Sabadyr <zvenayte@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef __cplusplus
extern "C" {
#endif
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Includes:

#include <obs.h>
#include <obs-module.h>

#include <graphics/vec2.h>
#include <graphics/matrix4.h>
#include <graphics/image-file.h>

#include "clusti.h"
#include <expat.h>

#include <stdio.h>

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// OBS module definition:

OBS_DECLARE_MODULE()

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Type forwards:

#ifdef __cplusplus
//	forward decl makes trouble in C++, whyever..
#else
struct obs_source_info stitch_filter;

// get rid of the "struct" keyword:
typedef struct vec2 vec2;
typedef struct vec3 vec3;
typedef struct vec4 vec4;
typedef struct matrix4 matrix4;
#endif

struct Clusti_OBS_uniform;
typedef struct Clusti_OBS_uniform Clusti_OBS_uniform;

struct Clusti_OBS_uniforms;
typedef struct Clusti_OBS_uniforms Clusti_OBS_uniforms;

enum Clusti_Enum_Uniform_Type;
typedef enum Clusti_Enum_Uniform_Type Clusti_Enum_Uniform_Type;

typedef struct stitch_filter_data stitch_filter_data;

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Function forwards:

// called by main app:
bool obs_module_load(void);

// Plugin's constructor
// Also: load XML config and calc math stuff 
// as soon as a path to a calibrationfile is available:
//  - either if  saved in a global json file or
//  - sepcified by the user in the OBS GUI
static void *stitch_filter_create(obs_data_t *settings, obs_source_t *context);
// Plugin's cdestructor
static void stitch_filter_destroy(void *data);


// called by plugin upon filter creation
// and also by OBS when user changes filter settings in the OBS GUI;
static void stitch_filter_update(void *data, obs_data_t *settings);

static void initState(stitch_filter_data *filter, obs_data_t *settings);
// if user config changes, purge and rebuild;
// in order to reuse code with stitch_filter_destroy(),
// extract this stuff into this.
static void purgeState(stitch_filter_data *filter);

// define some per-filter settings to be set/altered by the user
// via GUI and to be saved/restored in a OBS-global json file.
static obs_properties_t *stitch_filter_properties(void *data);
static void stitch_filter_defaults(obs_data_t *settings);

// called by OBS at the beginning of each new frame.
// used here to update resolution of input source.
static void stitch_filter_tick(void *data, float seconds);
// update the uniform variables of the shader
static void stitch_filter_render(void *data, gs_effect_t *effect);

//experimental: try to get some time stamp info
static struct obs_source_frame *
stitch_filter_filterRawFrame(void *data,
					 struct obs_source_frame *frame);

//{ simple getters for the main app
static const char *stitch_filter_get_name(void *unused);
// know the  plugin's output resolution:
static uint32_t stitch_filter_height(void *data);
static uint32_t stitch_filter_width(void *data);
//}

//{ Helpers to feed the OBS-independent calibration-loader-lib's
//  data to OBS, especially OBS's shader uniform variables


//static void clusti_OBS_init(void *data, obs_data_t *settings);
//static void clusti_OBS_deinit(void *data);

static void clusti_OBS_initUniforms(Clusti const *clusti,
				    obs_data_t const *settings,
				    gs_effect_t const *obsEffect,
				    Clusti_OBS_uniforms *uniforms);

static void clusti_OBS_bindUniforms(Clusti_OBS_uniforms const *uniforms);
//}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Global statically initialized variable to hold plugin info
// relevant for main app:

// N.b. if compiling this as C++ source,
// C++20 must be the minumum standard
// to (weirdly) support the old named-member
// init. of structs.
struct obs_source_info stitch_filter = {
	.id = "prosense_obs_stitcher_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = stitch_filter_get_name,
	.create = stitch_filter_create,
	.destroy = stitch_filter_destroy,
	.update = stitch_filter_update,
	.video_tick = stitch_filter_tick,
	.video_render = stitch_filter_render,
	.filter_video = stitch_filter_filterRawFrame,
	.get_properties = stitch_filter_properties,
	.get_defaults = stitch_filter_defaults,
	.get_width = stitch_filter_width,
	.get_height = stitch_filter_height,
};



//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Type definitions:

// OBS types to fill the shader's uniform variables:
// For each shader's uniform variable ,
// it will need two host variables:
// 1. a storage of and "OBS math" type
//    (struct vecX, struct matrix4 ...)
//    to store the arithmetic data
//    in a layout compatible to OBS's graphics
//    subsystem. (There may be binary comatibility
//    between OBS and Graphene, but alignment etc.
//    may mess this up. Better save than sorry.)
// 2. A handle of type gs_eparam_t identifying the uniform
//    variable in the shader (in OpenGL, this would be a
//    GLuint acquired via
//    glGetUniformLocation(myShaderHandle, "myUniformVarName")).
enum Clusti_Enum_Uniform_Type {
	CLUSTI_ENUM_UNIFORM_TYPE_invalid = 0,
	CLUSTI_ENUM_UNIFORM_TYPE_bool = 1,
	CLUSTI_ENUM_UNIFORM_TYPE_int = 2,
	CLUSTI_ENUM_UNIFORM_TYPE_float = 3,
	CLUSTI_ENUM_UNIFORM_TYPE_vec2 = 4,
	CLUSTI_ENUM_UNIFORM_TYPE_vec3 = 5,
	CLUSTI_ENUM_UNIFORM_TYPE_vec4 = 6,
	CLUSTI_ENUM_UNIFORM_TYPE_mat4 = 7,
	CLUSTI_ENUM_UNIFORM_TYPE_texture = 8,
};
#define CLUSTI_ENUM_NUM_UNIFORM_TYPES 9
typedef enum Clusti_Enum_Uniform_Type Clusti_Enum_Uniform_Type;

struct Clusti_OBS_uniform {
	char const *name;
	Clusti_Enum_Uniform_Type type;

	gs_eparam_t *handle;
	union {
		bool _bool;
		int _int;
		float _float;

		struct {
			graphene_vec2_t native;
			vec2 obs;
		} _vec2;

		struct {
			graphene_vec3_t native;
			vec3 obs;
		} _vec3;

		struct {
			graphene_vec4_t native;
			vec4 obs;
		} _vec4;

		struct {
			graphene_matrix_t native;
			matrix4 obs;
		} _mat4;

		// get texture pointer via image->texture;
		//gs_texture_t *_texture;
		gs_image_file_t image;
	};
};
typedef struct Clusti_OBS_uniform Clusti_OBS_uniform;

struct Clusti_OBS_uniforms {
	// For the meaning of the uniform variables,
	// please have a look a the respective effect file.

	//{ Video sink params

	// another image also seems to mess with their home brew shader wrapper layer!
	Clusti_OBS_uniform sinkParams_in_backgroundTexture;

	Clusti_OBS_uniform sinkParams_in_testImageTexture;

	Clusti_OBS_uniform sinkParams_in_index;
	// resolution the whole 4pi steradian panorama image would have;
	Clusti_OBS_uniform sinkParams_in_resolution_virtual;
	Clusti_OBS_uniform sinkParams_in_cropRectangle_lowerLeft;
	// resolution of render target:
	Clusti_OBS_uniform sinkParams_in_cropRectangle_extents;
	// in case of fisheye projection (alternative to eqirect):
	// hacky flip from equirect to fisheye;
	// dependent compilation would be better, but not for this prototype...
	Clusti_OBS_uniform sinkParams_in_useFishEye;
	Clusti_OBS_uniform sinkParams_in_fishEyeFOV_angle;
	//}

	//{ Video source params
	// not passable directly, instead "uniform texture2d image"
	// is passed automatically by OBS
	//Clusti_OBS_uniform sourceParams_in_currentPlanarRendering;
	Clusti_OBS_uniform sourceParams_in_resolution;
	Clusti_OBS_uniform sourceParams_in_index;
	//  Decklink 'black bar'-bug workaround:
	//  https://forum.blackmagicdesign.com/viewtopic.php?f=4&t=131164&p=711173&hilit=black+bar+quad#p711173
	//  ('VideoSource.decklinkWorkaround_verticalOffset_pixels' in config file)
	Clusti_OBS_uniform
		sourceParams_in_decklinkWorkaround_verticalOffset_pixels;

	Clusti_OBS_uniform
		sourceParams_in_frustum_reorientedViewProjectionMatrix;
	//{ just for debugging, prefer pre-accumulated versions
	//  like frustum_viewProjectionMatrix
	//  or frustum_reorientedViewProjectionMatrix:
	//  frustum_viewMatrix = transpose(frustum_rotationMatrix)
	Clusti_OBS_uniform sourceParams_in_frustum_viewMatrix;
	Clusti_OBS_uniform sourceParams_in_frustum_projectionMatrix;
	// accumulated VP matrix: in OpenGL notation (right-to-left-multiply):
	// frustum_viewProjectionMatrix =  frustum_projectionMatrix * frustum_viewMatrix;
	Clusti_OBS_uniform sourceParams_in_frustum_viewProjectionMatrix;
};
typedef struct Clusti_OBS_uniforms Clusti_OBS_uniforms;

struct stitch_filter_data {

	obs_source_t *obsFilterObject;

	gs_effect_t *frustumStitchEffect;
	
	// Data for parsing the XML config file and setting up
	// the math data (in OBS-independent Graphene types
	// that have to be converted, though)
	Clusti *clusti_instance;

	int sinkIndexToUse;
	int sourceIndexToUse;
	// update and issue a warning when mismatch to calibration file
	vec2 currentSinkResolution;
	vec2 currentSourceResolution;

	Clusti_OBS_uniforms clusti_OBS_uniforms;
};
typedef struct stitch_filter_data stitch_filter_data;
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Function implementations

//-----------------------------------------------------------------------------
bool obs_module_load(void)
{
	obs_register_source(&stitch_filter);

	return true;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static void *stitch_filter_create(obs_data_t *settings, obs_source_t *context)
{
	struct stitch_filter_data *filter =
		(struct stitch_filter_data *)bzalloc(sizeof(*filter));

	filter->obsFilterObject = context;

	stitch_filter_update(filter, settings);

	return filter;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static void stitch_filter_destroy(void *data)
{
	struct stitch_filter_data *filter = (struct stitch_filter_data *)data;

	purgeState(filter);

	bfree(filter);
}
//-----------------------------------------------------------------------------




//-----------------------------------------------------------------------------

static void clusti_OBS_initUniformTexture(Clusti_OBS_uniform *uni_out,
					  gs_effect_t const *obsEffect,
					  obs_data_t const *settings,
					  char *const varName,
					  char *const textureFileName)
{
	uni_out->name = varName;
	uni_out->type = CLUSTI_ENUM_UNIFORM_TYPE_texture;
	uni_out->handle = gs_effect_get_param_by_name(obsEffect, uni_out->name);

	char const *calibFilePath = obs_data_get_string(
		(obs_data_t *)settings, (const char *)"calibrationFile");
	assert(strcmp(calibFilePath, "") != 0);

	if (strcmp(calibFilePath, "") != 0) {
		char *imgDir = clusti_string_getDirectoryFromPath(calibFilePath);
		char *imgPath = clusti_String_concat(imgDir, textureFileName);

		gs_image_file_init(&uni_out->image, imgPath);
		obs_enter_graphics();
		gs_image_file_init_texture(&uni_out->image);
		obs_leave_graphics();

		clusti_free(imgPath);
		clusti_free(imgDir);
	}
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static matrix4 clusti_OBS_matrix_convert(graphene_matrix_t const * graMa)
{
	float matBuffer[16];
	graphene_matrix_to_float(graMa, matBuffer);

	matrix4 ret = {
		.x =
			{
				.x = matBuffer[0],
				.y = matBuffer[1],
				.z = matBuffer[2],
				.w = matBuffer[3]
			},
		.y =
			{
				.x = matBuffer[4],
				.y = matBuffer[5],
				.z = matBuffer[6],
				.w = matBuffer[7]
			},
		.z =
			{
				.x = matBuffer[8],
				.y = matBuffer[9],
				.z = matBuffer[10],
				.w = matBuffer[11]
			},
		.t =
			{
				.x = matBuffer[12],
				.y = matBuffer[13],
				.z = matBuffer[14],
				.w = matBuffer[15]
			}
	};

	return ret;

}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
static void clusti_OBS_initUniforms(Clusti const *clusti,
				    obs_data_t const *settings,
				    gs_effect_t const *obsEffect,
				    Clusti_OBS_uniforms *uniforms)
{
	Clusti_OBS_uniform *currUni = NULL;
	// always 0 now, but maybe one wnats multiple sinks in the future
	const int sinkIndex = 0;
	// cluster node index is way more interesting
	const int nodeIndex = (int)obs_data_get_int((obs_data_t *)settings,
						    (char const *)"nodeIndex");


	// texture
	// sinkParams_in_backgroundTexture
	clusti_OBS_initUniformTexture(
		&uniforms->sinkParams_in_backgroundTexture,
		obsEffect,
		settings,
		"sinkParams_in_backgroundTexture",
		clusti->stitchingConfig.videoSinks[0].debug_backgroundImageName
	);

	// texture
	// sinkParams_in_backgroundTexture
	clusti_OBS_initUniformTexture(
		&uniforms->sinkParams_in_testImageTexture, obsEffect, settings,
		"sinkParams_in_testImageTexture",
		clusti->stitchingConfig.videoSources[nodeIndex].testImageName);

	

	// int
	// sinkParams_in_index
	currUni = &uniforms->sinkParams_in_index;
	currUni->name = "sinkParams_in_index";
	currUni->type = CLUSTI_ENUM_UNIFORM_TYPE_int;
	currUni->handle = gs_effect_get_param_by_name(obsEffect, currUni->name);
	currUni->_int = sinkIndex;

	// vec2
	// sinkParams_in_resolution_virtual
	currUni = &uniforms->sinkParams_in_resolution_virtual;
	currUni->name = "sinkParams_in_resolution_virtual";
	currUni->type = CLUSTI_ENUM_UNIFORM_TYPE_vec2;
	currUni->handle = gs_effect_get_param_by_name(obsEffect, currUni->name);
	currUni->_vec2.obs =
		(vec2){.x = (float)clusti->stitchingConfig.videoSinks[sinkIndex]
				    .virtualResolution.x,
		       .y = (float)clusti->stitchingConfig.videoSinks[sinkIndex]
				    .virtualResolution.y};
	// complication with ivec2 defeates the purpose of native container here.
	// maybe we can dith them at all, but for debugging it might be useful.
	graphene_vec2_init_from_float(&currUni->_vec2.native,
				      currUni->_vec2.obs.ptr);
		
	// vec2
	// sinkParams_in_cropRectangle_lowerLeft
	currUni = &uniforms->sinkParams_in_cropRectangle_lowerLeft;
	currUni->name = "sinkParams_in_cropRectangle_lowerLeft";
	currUni->type = CLUSTI_ENUM_UNIFORM_TYPE_vec2;
	currUni->handle = gs_effect_get_param_by_name(obsEffect, currUni->name);
	currUni->_vec2.obs =
		(vec2){.x = (float)clusti->stitchingConfig.videoSinks[sinkIndex]
				    .cropRectangle.lowerLeft.x,
		       .y = (float)clusti->stitchingConfig.videoSinks[sinkIndex]
				    .cropRectangle.lowerLeft.y};
	graphene_vec2_init_from_float(&currUni->_vec2.native,
				      currUni->_vec2.obs.ptr);

	// vec2
	// sinkParams_in_cropRectangle_extents
	currUni = &uniforms->sinkParams_in_cropRectangle_extents;
	currUni->name = "sinkParams_in_cropRectangle_extents";
	currUni->type = CLUSTI_ENUM_UNIFORM_TYPE_vec2;
	currUni->handle = gs_effect_get_param_by_name(obsEffect, currUni->name);
	currUni->_vec2.obs =
		(vec2){.x = (float)clusti->stitchingConfig.videoSinks[sinkIndex]
				    .cropRectangle.extents.x,
		       .y = (float)clusti->stitchingConfig.videoSinks[sinkIndex]
				    .cropRectangle.extents.y};
	graphene_vec2_init_from_float(&currUni->_vec2.native,
				      currUni->_vec2.obs.ptr);

	// bool
	// sinkParams_in_useFishEye
	currUni = &uniforms->sinkParams_in_useFishEye;
	currUni->name = "sinkParams_in_useFishEye";
	currUni->type = CLUSTI_ENUM_UNIFORM_TYPE_bool;
	currUni->handle = gs_effect_get_param_by_name(obsEffect, currUni->name);
	currUni->_bool = clusti->stitchingConfig.videoSinks[sinkIndex].projection.type
		== CLUSTI_ENUM_PROJECTION_TYPE_FISHEYE;

	// float
	// sinkParams_in_fishEyeFOV_angle
	currUni = &uniforms->sinkParams_in_fishEyeFOV_angle;
	currUni->name = "sinkParams_in_fishEyeFOV_angle";
	currUni->type = CLUSTI_ENUM_UNIFORM_TYPE_float;
	currUni->handle = gs_effect_get_param_by_name(obsEffect, currUni->name);
	currUni->_float = clusti->stitchingConfig.videoSinks[sinkIndex]
				  .projection.fisheye_aperture_degrees;

	// vec2
	// sourceParams_in_resolution
	currUni = &uniforms->sourceParams_in_resolution;
	currUni->name = "sourceParams_in_resolution";
	currUni->type = CLUSTI_ENUM_UNIFORM_TYPE_vec2;
	currUni->handle = gs_effect_get_param_by_name(obsEffect, currUni->name);
	currUni->_vec2.obs = (vec2){
		.x = (float)clusti->stitchingConfig.videoSources[nodeIndex]
			     .resolution.x,
		.y = (float)clusti->stitchingConfig.videoSources[nodeIndex]
			     .resolution.y};
	graphene_vec2_init_from_float(&currUni->_vec2.native,
				      currUni->_vec2.obs.ptr);

	// int
	// sourceParams_in_index
	currUni = &uniforms->sourceParams_in_index;
	currUni->name = "sourceParams_in_index";
	currUni->type = CLUSTI_ENUM_UNIFORM_TYPE_int;
	currUni->handle = gs_effect_get_param_by_name(obsEffect, currUni->name);
	currUni->_int = nodeIndex;

	// int
	// sourceParams_in_decklinkWorkaround_verticalOffset_pixels
	currUni =
		&uniforms->sourceParams_in_decklinkWorkaround_verticalOffset_pixels;
	currUni->name =
		"sourceParams_in_decklinkWorkaround_verticalOffset_pixels";
	currUni->type = CLUSTI_ENUM_UNIFORM_TYPE_int;
	currUni->handle = gs_effect_get_param_by_name(obsEffect, currUni->name);
	currUni->_int = clusti->stitchingConfig.videoSources[nodeIndex]
				.decklinkWorkaround_verticalOffset_pixels;

	// mat4
	// sourceParams_in_frustum_reorientedViewProjectionMatrix
	currUni =
		&uniforms->sourceParams_in_frustum_reorientedViewProjectionMatrix;
	currUni->name = "sourceParams_in_frustum_reorientedViewProjectionMatrix";
	currUni->type = CLUSTI_ENUM_UNIFORM_TYPE_mat4;
	currUni->handle = gs_effect_get_param_by_name(obsEffect, currUni->name);
	// calculate the matrix:
	graphene_matrix_t reorientedViewProjectionMatrix =
		clusti_math_reorientedViewProjectionMatrix(
			&clusti->stitchingConfig.videoSinks[sinkIndex]
				 .projection.orientation,
			&clusti->stitchingConfig.videoSources[nodeIndex]
				 .projection);
	currUni->_mat4.native = reorientedViewProjectionMatrix;
	currUni->_mat4.obs =
		clusti_OBS_matrix_convert(&reorientedViewProjectionMatrix);

	// mat4
	// sourceParams_in_frustum_viewMatrix
	currUni = &uniforms->sourceParams_in_frustum_viewMatrix;
	currUni->name = "sourceParams_in_frustum_viewMatrix";
	currUni->type = CLUSTI_ENUM_UNIFORM_TYPE_mat4;
	currUni->handle = gs_effect_get_param_by_name(obsEffect, currUni->name);
	graphene_matrix_init_from_matrix(
		&currUni->_mat4.native,
		&clusti->stitchingConfig.videoSources[nodeIndex]
			.projection.planar_viewMatrix
	);
	currUni->_mat4.obs = clusti_OBS_matrix_convert(&currUni->_mat4.native);

	// mat4
	// sourceParams_in_frustum_projectionMatrix
	currUni = &uniforms->sourceParams_in_frustum_projectionMatrix;
	currUni->name = "sourceParams_in_frustum_projectionMatrix";
	currUni->type = CLUSTI_ENUM_UNIFORM_TYPE_mat4;
	currUni->handle = gs_effect_get_param_by_name(obsEffect, currUni->name);
	graphene_matrix_init_from_matrix(
		&currUni->_mat4.native,
		&clusti->stitchingConfig.videoSources[nodeIndex]
			 .projection.planar_projectionMatrix);
	currUni->_mat4.obs = clusti_OBS_matrix_convert(&currUni->_mat4.native);

	// mat4
	// sourceParams_in_frustum_viewProjectionMatrix
	currUni = &uniforms->sourceParams_in_frustum_viewProjectionMatrix;
	currUni->name = "sourceParams_in_frustum_viewProjectionMatrix";
	currUni->type = CLUSTI_ENUM_UNIFORM_TYPE_mat4;
	currUni->handle = gs_effect_get_param_by_name(obsEffect, currUni->name);
	graphene_matrix_init_from_matrix(
		&currUni->_mat4.native,
		&clusti->stitchingConfig.videoSources[nodeIndex]
			 .projection.planar_viewProjectionMatrix);
	currUni->_mat4.obs = clusti_OBS_matrix_convert(&currUni->_mat4.native);

}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static void clusti_OBS_bindUniforms(Clusti_OBS_uniforms const *uniforms)
{
	Clusti_OBS_uniform const *currUni = NULL;

	// texture
	// sinkParams_in_backgroundTexture
	currUni = &uniforms->sinkParams_in_backgroundTexture;
	gs_effect_set_texture(currUni->handle, currUni->image.texture);

	
	// texture
	// sinkParams_in_backgroundTexture
	currUni = &uniforms->sinkParams_in_testImageTexture;
	gs_effect_set_texture(currUni->handle, currUni->image.texture);
	

	// int
	// sinkParams_in_index
	currUni = &uniforms->sinkParams_in_index;
	gs_effect_set_int(currUni->handle, currUni->_int);

	// vec2
	// sinkParams_in_resolution_virtual
	currUni = &uniforms->sinkParams_in_resolution_virtual;
	gs_effect_set_vec2(currUni->handle, &currUni->_vec2.obs);
		
	// vec2
	// sinkParams_in_cropRectangle_lowerLeft
	currUni = &uniforms->sinkParams_in_cropRectangle_lowerLeft;
	gs_effect_set_vec2(currUni->handle, &currUni->_vec2.obs);

	// vec2
	// sinkParams_in_cropRectangle_extents
	currUni = &uniforms->sinkParams_in_cropRectangle_extents;
	gs_effect_set_vec2(currUni->handle, &currUni->_vec2.obs);

	// bool
	// sinkParams_in_useFishEye
	currUni = &uniforms->sinkParams_in_useFishEye;
	gs_effect_set_bool(currUni->handle, currUni->_bool);

	// float
	// sinkParams_in_fishEyeFOV_angle
	currUni = &uniforms->sinkParams_in_fishEyeFOV_angle;
	gs_effect_set_float(currUni->handle, currUni->_float);

	// vec2
	// sourceParams_in_resolution
	currUni = &uniforms->sourceParams_in_resolution;
	gs_effect_set_vec2(currUni->handle, &currUni->_vec2.obs);

	// int
	// sourceParams_in_index
	currUni = &uniforms->sourceParams_in_index;
	gs_effect_set_int(currUni->handle, currUni->_int);

	// int
	// sourceParams_in_decklinkWorkaround_verticalOffset_pixels
	currUni = &uniforms->sourceParams_in_decklinkWorkaround_verticalOffset_pixels;
	gs_effect_set_int(currUni->handle, currUni->_int);

	// mat4
	// sourceParams_in_frustum_reorientedViewProjectionMatrix
	currUni = &uniforms->sourceParams_in_frustum_reorientedViewProjectionMatrix;
	gs_effect_set_matrix4(currUni->handle, &currUni->_mat4.obs);

	// mat4
	// sourceParams_in_frustum_viewMatrix
	currUni = &uniforms->sourceParams_in_frustum_viewMatrix;
	gs_effect_set_matrix4(currUni->handle, &currUni->_mat4.obs);

	// mat4
	// sourceParams_in_frustum_projectionMatrix
	currUni = &uniforms->sourceParams_in_frustum_projectionMatrix;
	gs_effect_set_matrix4(currUni->handle, &currUni->_mat4.obs);

	// mat4
	// sourceParams_in_frustum_viewProjectionMatrix
	currUni = &uniforms->sourceParams_in_frustum_viewProjectionMatrix;
	gs_effect_set_matrix4(currUni->handle, &currUni->_mat4.obs);
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static void initState(stitch_filter_data *filter, obs_data_t *settings)
{
	int nodeIndex = (int)obs_data_get_int(settings, "nodeIndex");
	filter->sourceIndexToUse = nodeIndex;
	// maybe make customizable later
	filter->sinkIndexToUse = 0;

	assert(filter->clusti_instance == NULL);

	//hack: not update, but purge and recreate
	filter->clusti_instance = clusti_create();

	const char *calibFilePath =
		obs_data_get_string(settings, "calibrationFile");
	assert(strcmp(calibFilePath, "") != 0);

	clusti_readConfig(filter->clusti_instance, calibFilePath);

	blog(LOG_DEBUG, "stitcher: num video sources: %d",
	     filter->clusti_instance->stitchingConfig.numVideoSources);


	bool overrideCalibFile =
		obs_data_get_bool(settings, "overrideCalibFile");

	if (overrideCalibFile) {

		// override output resolution:
		char const *resolutionString =
			obs_data_get_string(settings, (const char *)"resolution_out");
		if (resolutionString != NULL) {
			long r =
				strtol(resolutionString, &(char*)resolutionString, 10);
			if (r > 0) {
				filter->currentSinkResolution.x = (float)r;
				if (resolutionString != NULL) {
					r = strtol(resolutionString + 1, NULL,
						   10);
					if (r > 0) {
						filter->currentSinkResolution.y =
							(float)r;
					}
				}
			}
		}


		
		bool stitchToFishEye =
			obs_data_get_bool(settings, "stitchToFishEye");
		if (stitchToFishEye) {
			filter->clusti_instance->stitchingConfig
				.videoSinks[filter->sinkIndexToUse]
				.projection.type =
				CLUSTI_ENUM_PROJECTION_TYPE_FISHEYE;
			// TODO make customizable
			filter->clusti_instance->stitchingConfig
				.videoSinks[filter->sinkIndexToUse]
				.projection.fisheye_aperture_degrees = 180.0f;
		}


	} else {
		filter->currentSinkResolution.x =
			(float)
			filter->clusti_instance->stitchingConfig
				.videoSinks[filter->sinkIndexToUse].cropRectangle.extents.x;
		filter->currentSinkResolution.y =
			(float)filter->clusti_instance->stitchingConfig
				.videoSinks[filter->sinkIndexToUse]
				.cropRectangle.extents.y;
	}



	//create effect
	char *effect_path = obs_module_file("frustum-stitcher.effect");
	obs_enter_graphics();
	filter->frustumStitchEffect =
		gs_effect_create_from_file(effect_path, NULL);
	bfree(effect_path);
	obs_leave_graphics();


	clusti_OBS_initUniforms(filter->clusti_instance, settings,
				filter->frustumStitchEffect,
				&filter->clusti_OBS_uniforms);








	 
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static void purgeState(stitch_filter_data *filter)
{
	//hack: not update, but purge and recreate
	obs_enter_graphics();
	gs_effect_destroy(filter->frustumStitchEffect);
	filter->frustumStitchEffect = NULL;
	obs_leave_graphics();

	if (filter->clusti_instance != NULL) {
		clusti_destroy(filter->clusti_instance);
		filter->clusti_instance = NULL;
	}
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static void stitch_filter_update(void *data, obs_data_t *settings)
{
	stitch_filter_data *filter = (stitch_filter_data *)data;

	// ----------------------------
	// ideas for plugin-global settings update...
	obs_source_t *obsSource_filterTarget= obs_filter_get_target(filter->obsFilterObject);
	//obs_scene_find_source_recursive()
	obs_scene_t *obsScene = obs_scene_from_source(obsSource_filterTarget);
	//obs_scene_get_settin
	//obs_source_get_settings()

	//obs_source_get_filter_by_name
	//obs_filter
	//obs_scene_find_source()
	//obs_source_update();


	// ----------------------------
	const char *calibFilePath = obs_data_get_string(settings, "calibrationFile");
	// string not empty?
	if (strcmp(calibFilePath, "") != 0) {

		if (filter->clusti_instance != NULL) {
			//hack: not update, but purge and recreate
			purgeState(filter); 
		}

		initState(filter, settings);
	}
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static obs_properties_t *stitch_filter_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_path(props, "calibrationFile",
				"Calibration XML file", OBS_PATH_FILE, "*.xml",
				NULL);
	obs_properties_add_int(props, "nodeIndex", "Render Node index", 0, 64,
			       1);

	obs_properties_add_bool(props, "overrideCalibFile",
				"Override calib file settings");

	// The following settings will only be used if "overrideCalibFile" is true:
	obs_properties_add_bool(props, "stitchToFishEye", "Stitch to FishEye");
	obs_properties_add_text(props, "resolution_out", "Resolution", OBS_TEXT_DEFAULT);



	return props;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static void stitch_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "nodeIndex", 0);

	obs_data_set_default_bool(settings, "overrideCalibFile", false);
	
	obs_data_set_default_bool(settings, "stitchToFishEye", false);
	obs_data_set_default_string(settings, "resolution_out", "4096x2048");

}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static void stitch_filter_tick(void *data, float seconds)
{
	struct stitch_filter_data *filter = (struct stitch_filter_data *)data;

	//seconds;
	blog(LOG_DEBUG, "tick timestamp for source"
		"%d: %f seconds",
	     filter->sourceIndexToUse, 
	     seconds);

	obs_source_t *source;
	source = obs_filter_get_target(filter->obsFilterObject);
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static void stitch_filter_render(void *data, gs_effect_t *effect)
{
	struct stitch_filter_data *filter = (struct stitch_filter_data *)data;

	//!filter->target ||
	if (!filter->frustumStitchEffect) {
		obs_source_skip_video_filter(filter->obsFilterObject);
		return;
	}



	if (!obs_source_process_filter_begin(filter->obsFilterObject, GS_RGBA,
					     OBS_NO_DIRECT_RENDERING)) {
		return;
	}

	////obs_source_t* obsVideoSource =
	//struct obs_source *obsVideoSource =
	//	obs_filter_get_target(filter->obsFilterObject);


	////obsVideoSource->last_frame_ts;
	////obs_source_frame2

	//struct obs_source_frame *currentFrame =
	//	obs_source_get_frame(obsVideoSource);
	//if (currentFrame == NULL) {
	//	blog(LOG_DEBUG, "stitcher: source #%d: No Frame available",
	//	     filter->sourceIndexToUse);
	//}
	//else {
	//	blog(LOG_DEBUG, "stitcher: source #%d, timestamp: %ul",
	//		filter->sourceIndexToUse,
	//		currentFrame->timestamp);

	//	obs_source_release_frame(obsVideoSource, currentFrame);
	//}

	

	clusti_OBS_bindUniforms(&filter->clusti_OBS_uniforms);

	obs_source_process_filter_end(
		filter->obsFilterObject, filter->frustumStitchEffect,
		(uint32_t)filter->currentSinkResolution.x,
		(uint32_t)filter->currentSinkResolution.y);

	UNUSED_PARAMETER(effect);
}


//-----------------------------------------------------------------------------
//experimental: try to get some time stamp info
static struct obs_source_frame *
stitch_filter_filterRawFrame(void *data,
					 struct obs_source_frame *frame)
{
	struct stitch_filter_data *filter = (struct stitch_filter_data *)data;

	if (frame == NULL) {
		blog(LOG_DEBUG, "stitcher: source #%d: No Frame available",
		     filter->sourceIndexToUse);
	} else {
		/*blog(LOG_DEBUG, "stitcher: source #%d, timestamp: %llu",
		     filter->sourceIndexToUse, frame->timestamp);*/

		//obs_source_release_frame(obsVideoSource, currentFrame);
	}

	return frame;
}
//-----------------------------------------------------------------------------}


//-----------------------------------------------------------------------------
static const char *stitch_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Prosense stitcher";
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static uint32_t stitch_filter_width(void *data)
{
	if (data != NULL) {
		struct stitch_filter_data *filter =
			(struct stitch_filter_data *)data;
		return (uint32_t)filter->currentSinkResolution.x;
	} else {
		return (uint32_t)4096;
	}
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static uint32_t stitch_filter_height(void *data)
{
	if (data != NULL) {
		struct stitch_filter_data *filter =
			(struct stitch_filter_data *)data;
		return (uint32_t)filter->currentSinkResolution.y;
	} else {
		return (uint32_t)2048;
	}
}
//-----------------------------------------------------------------------------


#ifdef __cplusplus
} // extern "C"
#endif
