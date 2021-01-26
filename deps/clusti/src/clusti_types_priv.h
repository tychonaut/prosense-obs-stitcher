#ifndef CLUSTI_TYPES_PRIV_H
#define CLUSTI_TYPES_PRIV_H

#ifdef HAVE_EXPAT_CONFIG_H
#include <expat_config.h>
#endif
#include <expat.h>

#include <graphene.h>

struct Clusti_ivec2 {

	int x;
	int y;
};
typedef struct Clusti_ivec2 Clusti_ivec2;

/*
	integer-valued rectangle
	for e.g. image cropping

*/
struct Clusti_iRectangle {

	Clusti_ivec2 min;
	Clusti_ivec2 max;
};
typedef struct Clusti_iRectangle Clusti_iRectangle;




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
struct Clusti_Params_Blending;
typedef struct Clusti_Params_Blending Clusti_Params_Blending;


enum Clusti_Enum_Projection_Type;
typedef enum Clusti_Enum_Projection_Type Clusti_Enum_Projection_Type;

struct Clusti_Params_Projection;
typedef struct Clusti_Params_Projection Clusti_Params_Projection;

struct Clusti_Params_Projection_Equirect;
typedef struct Clusti_Params_Projection_Equirect
	Clusti_Params_Projection_Equirect;

struct Clusti_Params_Projection_Planar;
typedef struct Clusti_Params_Projection_Planar Clusti_Params_Projection_Planar;

struct Clusti_Params_Projection_Fisheye;
typedef struct Clusti_Params_Projection_Fisheye Clusti_Params_Projection_Fisheye;

struct Clusti_Params_Projection_EquiAngularCubeMap;
typedef struct Clusti_Params_Projection_EquiAngularCubeMap
	Clusti_Params_Projection_EquiAngularCubeMap;


struct Clusti_Params_Frustum;
typedef struct Clusti_Params_Frustum Clusti_Params_Frustum;

struct Clusti_Params_FOV;
typedef struct Clusti_Params_FOV Clusti_Params_FOV;

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





struct Clusti_Params_FOV {
	float up_degrees;
	float down_degrees;
	float left_degrees;
	float right_degrees;
};

struct Clusti_Params_Frustum {

	Clusti_Params_FOV fov;

        /*
	   GRAPHENE_EULER_ORDER_SZXY,
           as VIOSO has the euler angle convention 
           "roll pitch yaw" ("zxy", "rotationMat = yawMat * pitchMat *  rollMat")
	*/
	Clusti_Params_Orientation orientation;

	// Redundant: calculated from the above data;
	// passed to and used in shader in shadowmapping-style
	graphene_matrix_t viewProjectionMatrix;
};



struct Clusti_Params_Projection_Planar {
	Clusti_Params_Frustum frustum;
};

struct Clusti_Params_Projection_Equirect {
	Clusti_Params_Orientation orientation;
};

struct Clusti_Params_Projection_Fisheye {
	float openingAngle_degrees;
	Clusti_Params_Orientation orientation;
};

struct Clusti_Params_Projection_EquiAngularCubeMap {
	// https : //blog.google/products/google-ar-vr/bringing-pixels-front-and-center-vr-video/
	Clusti_Params_Orientation orientation;
};

struct Clusti_Params_Projection {
	// e.g. type="dome"
	Clusti_Enum_Projection_Type type;

	// Poor man's polymorphism; Don't wanna hassle with too much pointer de/alloc
	union {
		Clusti_Params_Projection_Planar planar;
		Clusti_Params_Projection_Equirect equirect;
		Clusti_Params_Projection_Fisheye fisheye;
		Clusti_Params_Projection_EquiAngularCubeMap equiAngCM;
	};

};
//-----------------------------------------------------------------------------

struct Clusti_Params_VideoSink {

	int index;
	char *name;

	/* Because preview might be too big to literally see the whole
	   picture without zooming out
	*/
	float debug_renderScale;

	char *debug_backgroundImageName;

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
	*/
	char *type;
	/**
	 * @brief invert the provided mapping or not?
	*/
	bool invert;
	char *warpfileBaseName;

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
	 *        and found by given dir, basename and image type, see below. 
	*/
	bool autoGenerate;

	// How thick is the border area
	// where the image is slowly faded towards the outside?
	int fadingRange_pixels;

	/**
	 * @brief Base name of image to be read or written
	 * @details If e.g. the calibration xml file is "C:/Calibrations/myCalib.xml",
	 *          for image source 0, the library will in this directory
	 *          either look for or write to
	 *          a file "C:/Calibrations/BlendMask001.png"
	 *          (index + 1)
	*/
	char *imageBaseName;
};






struct Clusti_Params_VideoSource {

	/* which OBS input will this source be? */
	int index;
	char *name;
	char *testImageName;

	/*
	  Decklink "black bar"-bug workaround:
	  https://forum.blackmagicdesign.com/viewtopic.php?f=4&t=131164&p=711173&hilit=black+bar+quad#p711173

	*/
	int decklinkWorkaround_verticalOffset_pixels;

	Clusti_ivec2 resolution;

	Clusti_Params_Projection projection;
};



/* All data parsed from XML config file */
struct Clusti_Params_Stitching {

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
	struct Clusti_State_Parsing parsingState;

	struct Clusti_Params_Stitching stitchingConfig;

	//  t.b.d.
};


#endif /* CLUSTI_TYPES_PRIV_H */
