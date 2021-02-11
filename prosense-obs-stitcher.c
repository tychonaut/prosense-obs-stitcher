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

// Plugin's constructor and destructor
static void *stitch_filter_create(obs_data_t *settings, obs_source_t *context);
static void stitch_filter_destroy(void *data);

// called by plugin upon filter creation
// and also by OBS when user changes filter settings in the OBS GUI;
static void stitch_filter_update(void *data, obs_data_t *settings);

// define some per-filter settings to be set/altered by the user
// via GUI and to be saved/restored in a OBS-global json file.
static obs_properties_t *stitch_filter_properties(void *data);
static void stitch_filter_defaults(obs_data_t *settings);

// called by OBS at the beginning of each new frame.
// used here to update resolution of input source.
static void stitch_filter_tick(void *data, float seconds);
// update the uniform variables of the shader
static void stitch_filter_render(void *data, gs_effect_t *effect);

//{ simple getters for the main app
static const char *stitch_filter_get_name(void *unused);
// know the  plugin's output resolution:
static uint32_t stitch_filter_height(void *data);
static uint32_t stitch_filter_width(void *data);
//}

//{ Helpers to feed the OBS-independent calibration-loader-lib's
//  data to OBS, especially OBS's shader uniform variables

// load config, calc math stuff as soon as a path to a calibration
// file is available:
//  - either if  saved in a global json file or
//  - sepcified by the user in the OBS GUI
static void clusti_OBS_init(void *data, obs_data_t *settings);
static void clusti_OBS_deinit(void *data);

static void clusti_OBS_initUniforms(Clusti const *clusti,
				    gs_effect_t const *obsEffect,
				    Clusti_OBS_uniforms *uniforms);

static void clusti_OBS_bindUniforms(Clusti_OBS_uniforms const *uniforms);
//}

//{ old code: Hugin etc. file parsing.
void parse_file(const char *path, int cam, struct stitch_filter_data *filter);
void parse_script_crop(char *str, struct vec2 *crop_c, struct vec2 *crop_r,
		       char crop_type);
float parse_script(char *str, char *p);
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
	Clusti_OBS_uniform sinkParams_in_backgroundTexture;
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
	Clusti_OBS_uniform sourceParams_in_currentPlanarRendering;
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
	obs_source_t *context;
	gs_effect_t *effect;

	// Data for parsing the XML config file and setting up
	// the math data (in OBS-independent Graphene types
	// that have to be converted, though)
	Clusti *clusti_instance;

	int sourceIndexToUse;
	// update and issue a warning when mismatch to calibration file
	vec2 currentSinkResolution;
	vec2 currentSourceResolution;

	Clusti_OBS_uniforms clusti_OBS_uniforms;

	// old code ------
	gs_eparam_t *param_alpha;
	//gs_eparam_t *param_resO;
	gs_eparam_t *param_resI;
	gs_eparam_t *param_yrp;
	gs_eparam_t *param_ppr;
	gs_eparam_t *param_abc;
	gs_eparam_t *param_de;
	gs_eparam_t *param_crop_c;
	gs_eparam_t *param_crop_r;

	gs_texture_t *target;
	gs_image_file_t alpha;
	struct vec2 resO;
	struct vec2 resI;
	struct vec3 yrp;
	float ppr;
	struct vec3 abc;
	struct vec2 de;
	struct vec2 crop_c;
	struct vec2 crop_r;
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

	filter->context = context;

	// ---------------------------------------------------------------------------
	filter->clusti_instance = clusti_create();
	clusti_readConfig(
		filter->clusti_instance,
		// hard code for basic functioning test
		"C:/Users/Domecaster/devel/obs-studio/github_tychonaut/plugins/prosense-obs-stitcher/testData/calibration_viewfrusta.xml");
	blog(LOG_DEBUG, "stitcher: num video sources: %d",
	     filter->clusti_instance->stitchingConfig.numVideoSources);
	// ---------------------------------------------------------------------------

	stitch_filter_update(filter, settings);

	return filter;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static void stitch_filter_destroy(void *data)
{
	struct stitch_filter_data *filter = (struct stitch_filter_data *)data;

	obs_enter_graphics();
	gs_effect_destroy(filter->effect);
	obs_leave_graphics();

	// -----------------------------------
	clusti_destroy(filter->clusti_instance);
	filter->clusti_instance = NULL;
	// -----------------------------------

	bfree(filter);
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static void clusti_OBS_init(void *data, obs_data_t *settings)
{
	//TODO
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static void clusti_OBS_deinit(void *data, obs_data_t *settings)
{
	//TODO
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static void clusti_OBS_initUniforms(Clusti const *clusti,
				    gs_effect_t const *obsEffect,
				    Clusti_OBS_uniforms *uniforms)
{
	uniforms->sinkParams_in_backgroundTexture.name =
		"sinkParams_in_backgroundTexture";
	uniforms->sinkParams_in_backgroundTexture.type =
		CLUSTI_ENUM_UNIFORM_TYPE_texture;

	//TODO

	//uniforms->sinkParams_in_backgroundTexture.handle =
	//	gs_effect_get_param_by_name(
	//		obsEffect,
	//		uniforms->sinkParams_in_backgroundTexture.name);
	//uniforms->sinkParams_in_backgroundTexture.

	//filter->param_alpha = gs_effect_get_param_by_name(filter->effect,
	//						  "oldUniforms_target");
	//gs_effect_set_texture(filter->param_alpha, filter->target);
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static void clusti_OBS_bindUniforms(Clusti_OBS_uniforms const *uniforms)
{
	//TODO
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
static void stitch_filter_update(void *data, obs_data_t *settings)
{
	struct stitch_filter_data *filter = (struct stitch_filter_data *)data;

	filter->resO.x = (float)4096;
	filter->resO.y = (float)2048;

	int cam = (int)obs_data_get_int(settings, "cam");
	const char *path = obs_data_get_string(settings, "alpha");

	//original code: C seems to be less strict with th constness
	//char* res = obs_data_get_string(settings, "res");

	//workaround:
	//const char *res_const = obs_data_get_string(settings, "res");
	char const *const res_const =
		obs_data_get_string(settings, (const char *)"res");

	//char *res = (char *)malloc(strlen(res_const) * sizeof(char));
	//res =
	//strcpy(res, res_const);
	//char *res = strdup(res_const);
	//char *const res = (char *const) res_const;
	char **res_ptr = (char **)&(res_const);

	//long r = strtol(res, &res, 10);
	long r = strtol(res_const, res_ptr, 10);
	if (r > 0) {
		filter->resO.x = (float)r;
		if (res_const != NULL) {
			r = strtol(res_const + 1, NULL, 10);
			if (r > 0) {
				filter->resO.y = (float)r;
			}
		}
	}

	gs_image_file_init(&filter->alpha, path);

	obs_enter_graphics();

	gs_image_file_init_texture(&filter->alpha);

	filter->target = filter->alpha.texture;
	obs_leave_graphics();

	path = obs_data_get_string(settings, "project");
	if (path[0] != '\0') {
		parse_file(path, cam, filter);
	}

	//free(res);
	//res = NULL;
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
	obs_properties_add_int(props, "nodeIndex", "Node index", 0, 64, 1);

	// old code
	obs_properties_add_path(props, "alpha", "Mask Image", OBS_PATH_FILE,
				"*.png", NULL);
	obs_properties_add_path(props, "project", "Project File", OBS_PATH_FILE,
				"PtGUI/Hugin project (*.pts *.pto)", NULL);
	obs_properties_add_int(props, "cam", "Input #", 0, 99, 1);
	obs_properties_add_text(props, "res", "Resolution", OBS_TEXT_DEFAULT);

	return props;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static void stitch_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "nodeIndex", 0);

	// old code
	obs_data_set_default_int(settings, "cam", 0);
	obs_data_set_default_string(settings, "res", "4096x2048");
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static void stitch_filter_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);

	struct stitch_filter_data *filter = (struct stitch_filter_data *)data;

	obs_source_t *source;
	source = obs_filter_get_target(filter->context);

	//filter->clusti_instance->stitchingConfig.

	filter->resI.x = (float)obs_source_get_base_width(source);
	filter->resI.y = (float)obs_source_get_base_height(source);
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static void stitch_filter_render(void *data, gs_effect_t *effect)
{
	struct stitch_filter_data *filter = (struct stitch_filter_data *)data;

	if (!filter->target || !filter->effect) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
					     OBS_NO_DIRECT_RENDERING))
		return;

	gs_effect_set_texture(filter->param_alpha, filter->target);
	//gs_effect_set_vec2(filter->param_resO, &filter->resO);
	gs_effect_set_vec2(filter->param_resI, &filter->resI);
	gs_effect_set_vec3(filter->param_yrp, &filter->yrp);
	gs_effect_set_float(filter->param_ppr, filter->ppr);
	gs_effect_set_vec3(filter->param_abc, &filter->abc);
	gs_effect_set_vec2(filter->param_de, &filter->de);
	gs_effect_set_vec2(filter->param_crop_c, &filter->crop_c);
	gs_effect_set_vec2(filter->param_crop_r, &filter->crop_r);

	//gs_effect_set_matrix4()

	obs_source_process_filter_end(filter->context, filter->effect,
				      (uint32_t)filter->resO.x,
				      (uint32_t)filter->resO.y);

	UNUSED_PARAMETER(effect);
}
//-----------------------------------------------------------------------------

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
		return (uint32_t)filter->resO.x;
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
		return (uint32_t)filter->resO.y;
	} else {
		return (uint32_t)2048;
	}
}
//-----------------------------------------------------------------------------

//=============================================================================
//=============================================================================
// old function impls, TODO delete when obsolete

//-----------------------------------------------------------------------------
float parse_script(char *str, char *p)
{
	char *num = strstr(str, p);
	if (num != NULL) {
		return (float)strtod(num + strlen(p), NULL);
	} else {
		return 0.0;
	}
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void parse_script_crop(char *str, struct vec2 *crop_c, struct vec2 *crop_r,
		       char crop_type)
{
	struct vec4 crop;
	char *num = strchr(str, crop_type);
	if (num != NULL) {
		crop.x = strtol(num + 1, &num, 10);
	} else
		return;
	if (num != NULL) {
		crop.y = strtol(num + 1, &num, 10);
	} else
		return;
	if (num != NULL) {
		crop.z = strtol(num + 1, &num, 10);
	} else
		return;
	if (num != NULL) {
		crop.w = strtol(num + 1, NULL, 10);
	} else
		return;
	crop_c->x = (crop.y + crop.x) / 2.0f;
	crop_c->y = (crop.w + crop.z) / 2.0f;

	crop_r->x = (crop.y - crop.x) / 2.0f;
	crop_r->y = (crop.w - crop.z) / 2.0f;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void parse_file(const char *path, int cam, struct stitch_filter_data *filter)
{
	if (cam < 0)
		return;
	FILE *pFile;
	char *str;
	str = (char *)malloc(150 * 1000 * 1000);

	pFile = fopen(path, "r");
	if (pFile != NULL) {
		const char *fext = strrchr(path, '.');
		if (strcmp(fext, ".pts") == 0) {
			char *effect_path =
				obs_module_file("pts-stitcher.effect");
			obs_enter_graphics();
			filter->effect =
				gs_effect_create_from_file(effect_path, NULL);
			obs_leave_graphics();
			bfree(effect_path);

			filter->param_alpha = gs_effect_get_param_by_name(
				filter->effect, "target");
			//filter->param_resO		= gs_effect_get_param_by_name(filter->effect, "resO");
			filter->param_resI = gs_effect_get_param_by_name(
				filter->effect, "resI");
			filter->param_yrp = gs_effect_get_param_by_name(
				filter->effect, "yrp");
			filter->param_ppr = gs_effect_get_param_by_name(
				filter->effect, "ppr");
			filter->param_abc = gs_effect_get_param_by_name(
				filter->effect, "abc");
			filter->param_de = gs_effect_get_param_by_name(
				filter->effect, "de");
			filter->param_crop_c = gs_effect_get_param_by_name(
				filter->effect, "crop_c");
			filter->param_crop_r = gs_effect_get_param_by_name(
				filter->effect, "crop_r");

			while (str[0] != 'o') {
				if (fgets(str, 150 * 1000 * 1000, pFile) ==
				    NULL)
					break;
			}

			float v =
				parse_script(str, (char *)" v") * M_PI / 180.0f;
			filter->abc.x = parse_script(str, (char *)" a");
			filter->abc.y = parse_script(str, (char *)" b");
			filter->abc.z = parse_script(str, (char *)" c");
			filter->de.x = parse_script(str, (char *)" d");
			filter->de.y = parse_script(str, (char *)" e");

			int i = -1;
			while (i < cam) {
				fgets(str, 150 * 1000 * 1000, pFile);
				while (str[0] != 'o') {
					if (fgets(str, 150 * 1000 * 1000,
						  pFile) == NULL) {
						break;
					}
				}
				i++;
			}

			filter->yrp.x =
				parse_script(str, (char *)" y") * M_PI / 180.0f;
			filter->yrp.y =
				parse_script(str, (char *)" r") * M_PI / 180.0f;
			filter->yrp.z =
				parse_script(str, (char *)" p") * M_PI / 180.0f;
			parse_script_crop(str, &filter->crop_c, &filter->crop_r,
					  'C');
			filter->ppr = (filter->crop_r.x + filter->crop_r.y) / v;
		}
		if (strcmp(fext, ".pto") == 0) {
			char *effect_path =
				obs_module_file("pto-stitcher.effect");
			obs_enter_graphics();
			filter->effect =
				gs_effect_create_from_file(effect_path, NULL);
			obs_leave_graphics();
			bfree(effect_path);

			filter->param_alpha = gs_effect_get_param_by_name(
				filter->effect, "oldUniforms_target");
			//filter->param_resO		= gs_effect_get_param_by_name(filter->effect, "resO");
			filter->param_resI = gs_effect_get_param_by_name(
				filter->effect, "oldUniforms_resI");
			filter->param_yrp = gs_effect_get_param_by_name(
				filter->effect, "oldUniforms_yrp");
			filter->param_ppr = gs_effect_get_param_by_name(
				filter->effect, "oldUniforms_ppr");
			filter->param_abc = gs_effect_get_param_by_name(
				filter->effect, "oldUniforms_abc");
			filter->param_de = gs_effect_get_param_by_name(
				filter->effect, "oldUniforms_de");
			filter->param_crop_c = gs_effect_get_param_by_name(
				filter->effect, "oldUniforms_crop_c");
			filter->param_crop_r = gs_effect_get_param_by_name(
				filter->effect, "oldUniforms_crop_r");

			int i = 0;
			while (i <= cam) {
				fgets(str, 150 * 1000 * 1000, pFile);
				while (str[0] != 'i') {
					if (fgets(str, 150 * 1000 * 1000,
						  pFile) == NULL) {
						break;
					}
				}
				i++;
			}

			float v =
				parse_script(str, (char *)" v") * M_PI / 180.0f;
			filter->abc.x = parse_script(str, (char *)" a");
			filter->abc.y = parse_script(str, (char *)" b");
			filter->abc.z = parse_script(str, (char *)" c");
			filter->de.x = parse_script(str, (char *)" d");
			filter->de.y = parse_script(str, (char *)" e");

			filter->yrp.x =
				parse_script(str, (char *)" y") * M_PI / 180.0f;
			filter->yrp.y =
				parse_script(str, (char *)" r") * M_PI / 180.0f;
			filter->yrp.z =
				parse_script(str, (char *)" p") * M_PI / 180.0f;
			filter->ppr = parse_script(str, (char *)" w") / v;
		}

		fclose(pFile);
	}
	free(str);
}
//-----------------------------------------------------------------------------

#ifdef __cplusplus
} // extern "C"
#endif
