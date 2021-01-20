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

/*
	integer-valued rectangle
	for e.g. image cropping

*/
struct Clusti_iRectangle {
	struct Clusti_ivec2 min;
	struct Clusti_ivec2 max;
};

struct Clusti_CanvasParams {

	/* e.g. 8192*4096 for full-sphere equirectangular projection */
	struct Clusti_ivec2 virtualCanvasSize;

	/*
	For reducing the full sphere to e.g. an "only northern" hemisphere
	   by cropping the lower half, resulting e.g. in a 8192*2048 image,

	   When combined with below
	   projectionYaw, projectionPitch and projectionRoll,
	   this hemisphere can be reparametrized into a 4096*4096 square image,
	   which may have more desirable properties in terms of hardware video encoding,
	   streaming, and decoding.
	*/
	struct Clusti_iRectangle cropRectangle;

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
	float projectionYaw;
	float projectionPitch;
	float projectionRoll;
};

struct Clusti_BlendMaskGenerationParams {
	bool blendMaskGenerationRequired;

	//how thick is the border area
	//where the image is slowly faded towards the outside?
	int fadingRange_pixels;

	char *blendmaskBasename;
};

struct Clusti_VideoSourceParams {
	/* which OBS input will this source be? */
	int index;

	int resolution_x;
	int resolution_y;

	char *testImagePath;

	/*
		Decklink "black bar"-bug workaround:
		https://forum.blackmagicdesign.com/viewtopic.php?f=4&t=131164&p=711173&hilit=black+bar+quad#p711173

	*/
	int verticalOffset;
};

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

/* All data parsed from XML config file */
struct Clusti_StitchingConfig {

	//number of render nodes
	int numVideoSources;
	struct Clusti_CanvasParams canvasParams;
	struct Clusti_BlendMaskGenerationParams blendMaskGenerationParams;

	//array of size numVideoSources:
	struct Clusti_VideoSourceParams *videoSources;
	//array of size numVideoSources:
	struct Clusti_FrustumParams *frustaParams;
};

struct Clusti_ParsingState {
	char *configPath;
	int currentSourceIndex;
	/* to be filled, but not owned */
	struct Clusti_StitchingConfig* config_ref;
	/* rest t.b.d.*/ 
};

/**
 * @brief Main object containing all the relevant data
*/
struct Clusti_Stitcher {
	struct Clusti_ParsingState parsingState;
	struct Clusti_StitchingConfig config;
};


#endif /* CLUSTI_TYPES_H */
