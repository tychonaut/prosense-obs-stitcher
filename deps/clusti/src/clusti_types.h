#ifndef CLUSTI_TYPES_H
#define CLUSTI_TYPES_H

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

struct Clusti_CanvasParams {

	/* Opening angle in degrees (default 180) */
	float domeOpeningAngle_degrees;
	/* (default 21 degrees) */
	float domeTilt_degrees;

	/* not sure if required... */
	float domeRadius_meters;

	/*
	projection rotations to e.g. fit a 8k*2k projection into 4k*4k;
	 has to be reversed at the video stream receiver;
	 */
	float projectionYaw_degrees;
	float projectionPitch_degrees;
	float projectionRoll_degrees;

	/* Because preview might be too big to literally see the whole
	   picture without zooming out
	*/
	float debug_canvasRenderScaleFactor;

	char *debug_backgroundImagePath;

	/* e.g. 8192*4096 for full-sphere equirectangular projection */
	Clusti_ivec2 virtualCanvasResolution;

	/*
	For reducing the full sphere to e.g. an "only northern" hemisphere
	   by cropping the lower half, resulting e.g. in a 8192*2048 image,

	   When combined with above
	   projectionYaw_degrees etc.,
	   this hemisphere can be reparametrized into a 4096*4096 square image,
	   which may have more desirable properties in terms of hardware video encoding,
	   streaming, and decoding.
	*/
	Clusti_iRectangle canvasCropRectangle;
};
typedef struct Clusti_CanvasParams Clusti_CanvasParams;

struct Clusti_BlendMaskParams {

	/**
	 * @brief Flag that determines if blend masks shoul be generated
	          on the fly or read from disk.
	 * @details If true, blend masks will be calculated, written to disk
	 *        and subsequently used. If false, the images are assumed to exist
	 *        and found by given dir, basename and image type, see below. 
	*/
	bool blendMaskGenerationRequested;

	// How thick is the border area
	// where the image is slowly faded towards the outside?
	int fadingRange_pixels;

	/**
	 * @brief Base name of image to be read or written
	 * @details If e.g. blendmaskBasename is "C:/Calibrations/BlendMask.png",
	 *          for image source 0, the library will either look for or write to
	 *           a file "C:/Calibrations/BlendMask001.png"
	 *           (index + 1)
	*/
	// e.g. "C:/Calibrations"
	char *blendMaskDirectory;
	// e.g. "BlendMask",
	char *blendmaskBasename;
	// e.g. "png"
	char *blendMaskImageType;
};
typedef struct Clusti_BlendMaskParams Clusti_BlendMaskParams;

struct Clusti_FrustumParams {

	float fovUp_degrees;
	float fovDown_degrees;
	float fovLeft_degrees;
	float fovRight_degrees;

	float yaw_degrees;
	float pitch_degrees;
	float roll_degrees;

	// calculated from the above data;
	// passed to and used in shader in shadowmapping-style
	graphene_matrix_t *viewProjectionMatrix;
};
typedef struct Clusti_FrustumParams Clusti_FrustumParams;

struct Clusti_VideoSourceParams {

	/* which OBS input will this source be? */
	int index;
	char *name;
	char *testImagePath;

	/*
		Decklink "black bar"-bug workaround:
		https://forum.blackmagicdesign.com/viewtopic.php?f=4&t=131164&p=711173&hilit=black+bar+quad#p711173

	*/
	int decklinkWorkaround_verticalOffset_pixels;

	Clusti_ivec2 resolution;

	Clusti_FrustumParams frustum;
};
typedef struct Clusti_VideoSourceParams Clusti_VideoSourceParams;

/* All data parsed from XML config file */
struct Clusti_StitchingConfig {

	//number of render nodes
	int numVideoSources;
	Clusti_CanvasParams canvasParams;
	Clusti_BlendMaskParams blendMaskGenerationParams;

	//array of size numVideoSources:
	Clusti_VideoSourceParams *videoSourceParamArray;
};
typedef struct Clusti_StitchingConfig Clusti_StitchingConfig;

struct Clusti_ParsingState {
	// to keep track during XLM sub-elements of VideoSource elements.
	int currentVideoSourceIndex;

	//maybe handy for later writing of updated config...
	char *configPath;

	///* to be filled, but not owned */
	//Clusti_StitchingConfig* config_ref;
	///* rest t.b.d.*/
};
typedef struct Clusti_ParsingState Clusti_ParsingState;

/**
 * @brief Main object containing all the relevant data
*/
struct Clusti_Stitcher {
	struct Clusti_ParsingState parsingState;
	struct Clusti_StitchingConfig config;
};
typedef struct Clusti_Stitcher Clusti_Stitcher;

#endif /* CLUSTI_TYPES_H */
