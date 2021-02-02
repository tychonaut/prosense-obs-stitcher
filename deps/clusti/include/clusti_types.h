#ifndef CLUSTI_TYPES_H
#define CLUSTI_TYPES_H

// for some OpenGL types
// have to  link against OpenGL
//#if defined(__APPLE__)
//#include <OpenGL/gl.h>
//#else
//#include <GL/gl.h>
//#endif


#include <graphene.h>

//#ifdef HAVE_EXPAT_CONFIG_H
//#include <expat_config.h>
//#endif
//#include <expat.h>

struct Clusti_ivec2;
typedef struct Clusti_ivec2 Clusti_ivec2;
struct Clusti_ivec2 {

	int x;
	int y;
};

// integer-valued rectangle
// for e.g. image cropping
struct Clusti_iRectangle;
typedef struct Clusti_iRectangle Clusti_iRectangle;
struct Clusti_iRectangle {

	Clusti_ivec2 min;
	Clusti_ivec2 max;
};

//   Maybe needed later...
//struct Clusti_Params_ProjectionGeometry {
//	/*
//	Must be type="equirectangular" for now.
//	Alternatives (not supported yet):
//        "fisheye", "equi-angular-cubemap"
//        (https://blog.google/products/google-ar-vr/bringing-pixels-front-and-center-vr-video/)
//	*/
//	char *type;
//	/* Opening angle in degrees (default 180) */
//	float openingAngle_degrees;
//	/* (default 21 degrees) */
//	float tilt_degrees;
//	/* not sure if required... */
//	float radius_meters;
//};
//typedef struct Clusti_Params_ProjectionGeometry Clusti_Params_ProjectionGeometry;

// -----------------------------------------------------------------------------
// Forward declarations

struct Clusti;
typedef struct Clusti Clusti;

struct Clusti_State_Parsing;
typedef struct Clusti_State_Parsing Clusti_State_Parsing;

/* All data parsed from XML config file */
struct Clusti_Params_Stitching;
typedef struct Clusti_Params_Stitching Clusti_Params_Stitching;

struct Clusti_Params_VideoSink;
typedef struct Clusti_Params_VideoSink Clusti_Params_VideoSink;
struct Clusti_Params_VideoSource;
typedef struct Clusti_Params_VideoSource Clusti_Params_VideoSource;

struct Clusti_Params_Warping;
typedef struct Clusti_Params_Warping Clusti_Params_Warping;
enum Clusti_Enum_Warping_Type;
typedef enum Clusti_Enum_Warping_Type Clusti_Enum_Warping_Type;
struct Clusti_Params_Blending;
typedef struct Clusti_Params_Blending Clusti_Params_Blending;

enum Clusti_Enum_Projection_Type;
typedef enum Clusti_Enum_Projection_Type Clusti_Enum_Projection_Type;

struct Clusti_Params_Projection;
typedef struct Clusti_Params_Projection Clusti_Params_Projection;

//struct Clusti_Params_Projection_Equirect;
//typedef struct Clusti_Params_Projection_Equirect
//	Clusti_Params_Projection_Equirect;
//
//struct Clusti_Params_Projection_Planar;
//typedef struct Clusti_Params_Projection_Planar Clusti_Params_Projection_Planar;
//
//struct Clusti_Params_Projection_Fisheye;
//typedef struct Clusti_Params_Projection_Fisheye Clusti_Params_Projection_Fisheye;
//
//struct Clusti_Params_Projection_EquiAngularCubeMap;
//typedef struct Clusti_Params_Projection_EquiAngularCubeMap
//	Clusti_Params_Projection_EquiAngularCubeMap;
//
//struct Clusti_Params_Frustum;
//typedef struct Clusti_Params_Frustum Clusti_Params_Frustum;

struct Clusti_Params_FrustumFOV;
typedef struct Clusti_Params_FrustumFOV Clusti_Params_FrustumFOV;

typedef graphene_euler_t Clusti_Params_Orientation;

// -----------------------------------------------------------------------------
// Projection Parameter Definitions

enum Clusti_Enum_Projection_Type {
	CLUSTI_ENUM_PROJECTION_TYPE_INVALID = 0,
	CLUSTI_ENUM_PROJECTION_TYPE_PLANAR = 1,
	CLUSTI_ENUM_PROJECTION_TYPE_EQUIRECT = 2,
	CLUSTI_ENUM_PROJECTION_TYPE_FISHEYE = 3,
	CLUSTI_ENUM_PROJECTION_TYPE_EQUI_ANGULAR_CUBEMAP = 4,
};
#define CLUSTI_ENUM_NUM_PROJECTION_TYPES (5)

struct Clusti_Params_FrustumFOV {
	float up_degrees;
	float down_degrees;
	float left_degrees;
	float right_degrees;
};

//struct Clusti_Params_Frustum {
//
//	Clusti_Params_FrustumFOV fov;
//	// Redundant: calculated from the above data;
//	// passed to and used in shader in shadowmapping-style
//	graphene_matrix_t viewProjectionMatrix;
//
//
//
//	//Clusti_Params_Orientation orientation;
//
//};

//struct Clusti_Params_Projection_Planar {
//	Clusti_Params_Frustum frustum;
//};

//struct Clusti_Params_Projection_Equirect {
//	//Clusti_Params_Orientation orientation;
//};

//struct Clusti_Params_Projection_Fisheye {
//	float openingAngle_degrees;
//	//Clusti_Params_Orientation orientation;
//};

//// https : //blog.google/products/google-ar-vr/bringing-pixels-front-and-center-vr-video/
//struct Clusti_Params_Projection_EquiAngularCubeMap {
//
//};

struct Clusti_Params_Projection {
	// e.g. type="planar"
	Clusti_Enum_Projection_Type type;

	// GRAPHENE_EULER_ORDER_SZXY,
	// as VIOSO has the euler angle convention
	// "roll pitch yaw" ("zxy", "rotationMat = yawMat * pitchMat *  rollMat")
	Clusti_Params_Orientation orientation;

	//{ Very poor man's polymorphism: Some members only used for some proj. types.
	// (Don't wanna hassle with too much pointer de/alloc and keep XML parsing simple).

	Clusti_Params_FrustumFOV planar_FrustumFOV;
	// Redundant: calculated from the above data;
	// passed to and used in shader in shadowmapping-style
	graphene_matrix_t planar_viewProjectionMatrix;

	float fisheye_openingAngle_degrees;
	//}
};
//-----------------------------------------------------------------------------

struct Clusti_Params_VideoSink {

	int index;
	char *name;

	char *debug_backgroundImageName;
	// Because preview might be too big to literally see the whole
	//   picture without zooming out
	float debug_renderScale;

	/* e.g. 8192*4096 for full-sphere equirectangular projection */
	Clusti_ivec2 virtualResolution;

	/*
	  For reducing the full sphere to e.g. an "only northern" hemisphere
	   by cropping the lower half, resulting e.g. in a 8192*2048 image,

	   When combined with above
	   projectionYaw_degrees etc.,
	   this hemisphere can be reparametrized into a 4096*4096 square image,
	   which may have more desirable properties in terms of hardware video encoding,
	   streaming, and decoding.
	*/
	Clusti_iRectangle cropRectangle;

	Clusti_Params_Projection projection;
};

enum Clusti_Enum_Warping_Type {
	CLUSTI_ENUM_WARPING_TYPE_none = 0,
	CLUSTI_ENUM_WARPING_TYPE_imageLUT2D = 1,
	CLUSTI_ENUM_WARPING_TYPE_imageLUT3D = 2,
	CLUSTI_ENUM_WARPING_TYPE_mesh2D = 3,
	CLUSTI_ENUM_WARPING_TYPE_mesh3D = 4,
};
#define CLUSTI_ENUM_NUM_WARPING_TYPES (5)

struct Clusti_Params_Warping {

	/**
	 * @brief   Use warping in stitching or not.
	 * @details E.g. for "Desktop Warping", the captured image is NOT warped,
	 *          hence its warping must not be inverted for a correct mapping.
	 *          Then, it must be set to false.
	*/
	bool use;
	/**
	 * @brief "imageLUT2D" "imageLUT3D" or "mesh2D" or "mesh3D";
	 *        Warping is not implemented yet!
	 * @todo turn string into enum
	*/
	Clusti_Enum_Warping_Type type;
	/**
	 * @brief invert the provided mapping or not?
	*/
	bool invert;

	// put into Clusti_Params_VideoSource
	//char *warpfileBaseName;
};

struct Clusti_Params_Blending {

	/**
	 * @brief   Use blend masks or just draw stuff over each other, possibly resulting in
	            artifacts.
	*/
	bool use;

	/**
	 * @brief Flag that determines if blend masks should be generated
	          on the fly or read from disk.
	 * @details If true, blend masks will be calculated, written to disk
	 *        and subsequently used. If false, the images are assumed to exist
	 *        and found by given dir, file name (and image type extension). 
	*/
	bool autoGenerate;

	// How thick is the border area
	// where the image is slowly faded towards the outside?
	int fadingRange_pixels;
};

struct Clusti_Params_VideoSource {

	// which OBS input will this source be?
	int index;
	char *name;
	char *testImageName;
	char *warpfileName;
	char *blendImageName;
	//  Decklink "black bar"-bug workaround:
	//  https://forum.blackmagicdesign.com/viewtopic.php?f=4&t=131164&p=711173&hilit=black+bar+quad#p711173
	int decklinkWorkaround_verticalOffset_pixels;

	Clusti_ivec2 resolution;

	Clusti_Params_Projection projection;
};

/* All data parsed from XML config file */
struct Clusti_Params_Stitching {

	Clusti_Params_Warping generalWarpParams;
	Clusti_Params_Blending generalBlendParams;

	//number of canvases
	int numVideoSinks;
	Clusti_Params_VideoSink *videoSinks;

	//number of render nodes
	int numVideoSources;
	Clusti_Params_VideoSource *videoSources;
};

enum Clusti_Enum_ParsingModes;
typedef enum Clusti_Enum_ParsingModes Clusti_Enum_ParsingModes;
enum Clusti_Enum_ParsingModes {
	CLUSTI_ENUM_PARSING_MODES_general = 0,
	CLUSTI_ENUM_PARSING_MODES_videoSinks = 1,
	CLUSTI_ENUM_PARSING_MODES_videoSources = 2,
};
#define CLUSTI_ENUM_NUM_PARSING_MODES (3)

struct Clusti_State_Parsing {

	//maybe handy for later writing of updated config...
	char const *configPath;

	Clusti_Enum_ParsingModes currentParsingMode;

	// for basic consistency checks:
	//char *currentElementName;
	//char *currentParentElementName;
	//#define CLUSTI_MAX_NAME_LENGTH (1024)
	//char currentElementName[CLUSTI_MAX_NAME_LENGTH];

	// to keep track during XLM sub-elements of VideoSource elements.
	int currentElementIndex;

	///* to be filled, but not owned */
	//Clusti_StitchingConfig* config_ref;
	///* rest t.b.d.*/
};

/**
 * @brief Main object containing all the relevant data
*/
struct Clusti {
	Clusti_State_Parsing parsingState;

	Clusti_Params_Stitching stitchingConfig;

	//  t.b.d.
};
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// RenderState


//{ HACK: forwards to omit dependency to SDL
//  TODO remove this stuff from library when giong to OBS
struct SDL_Window;
typedef struct SDL_Window SDL_Window;
typedef void *SDL_GLContext;
//}


//{ HACK: forwards to omit dependency to OpenGL
//  TODO link clusti against OpenGL and GLEW
//typedef unsigned int GLenum;
//typedef unsigned int GLbitfield;
typedef unsigned int GLuint;
//typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLboolean;
//typedef signed char GLbyte;
//typedef short GLshort;
//typedef unsigned char GLubyte;
//typedef unsigned short GLushort;
//typedef unsigned long GLulong;
typedef float GLfloat;
typedef double GLdouble;
//}


struct Clusti_State_Render;
typedef struct Clusti_State_Render Clusti_State_Render;

struct Clusti_State_Render_VideoSink;
typedef struct Clusti_State_Render_VideoSink Clusti_State_Render_VideoSink;
struct Clusti_State_Render_VideoSource;
typedef struct Clusti_State_Render_VideoSource Clusti_State_Render_VideoSource;

struct Clusti_State_Render_VideoSink {

	GLuint backgroundTexture;


	// For later use:
	// for blend mask generation:
	// Integer Texture encoding canvas coverage as bit mask;
	// Implemented as compute shader, t.b.d;
	GLuint videoSourceCoverageTexture;
};

struct Clusti_State_Render_VideoSource {

	GLuint sourceTexture;

	GLuint warpTexture;


	GLuint blendTexture;
};


struct Clusti_State_Render {

	SDL_Window *wnd;
	SDL_GLContext glContext;

	Clusti_ivec2 renderTargetRes; 
	graphene_vec2_t windowRes_f;
	//graphene_vec2_t viewportRes_f;
	//graphene_vec2_t mousePos_f;


	GLuint stitchShaderProgram;
	// For later use:
	GLuint blendMaskGenShaderProgram;

	// GL geometry for drawing a "full screen quad"
	GLuint viewPortQuadNDC_vao;
	GLuint viewPortQuadNCD_vbo;

	// For later use:
	// offscreen rendertarget for blend mask generation:
	GLuint fbo;
	//probably unused:
	GLuint renderTargetTexture_Color;
	GLuint renderTargetTexture_Graymap;
	GLuint computeIntBuffer;
	// not sure if needed for this purpose
	GLuint renderTargetTexture_Depth;

	// render target(s)
	Clusti_State_Render_VideoSink *videoSinks;

	// video source(s)
	Clusti_State_Render_VideoSource *videoSources;
};








#endif /* CLUSTI_TYPES_H */
