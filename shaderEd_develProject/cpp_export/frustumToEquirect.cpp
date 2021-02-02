#include <chrono>
#include <fstream>
#include <string>

#include <GL/glew.h>

#include <SDL.h>
#undef main

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
//#define GLM_ENABLE_EXPERIMENTAL
//#include <glm/gtx/euler_angles.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"




#ifdef __cplusplus
extern "C" {
#endif

#include <clusti.h>

#ifdef __cplusplus
}  /* end extern "C" */
#endif


#ifdef HAVE_EXPAT_CONFIG_H
#include <expat_config.h>
#endif
#include <expat.h>

#include <graphene.h>


// -----------------------------------------------------------------------------------
// function forwards


void createRenderState(Clusti *clusti);

void createFBO(Clusti_State_Render *renderState);

//generate VAO and VBO
void CreateScreenQuadNDC(Clusti_State_Render *renderState);

GLuint LoadTexture(const std::string &filename);

GLuint CreateShader(const char *vsCode, const char *psCode);

void handleUserInput(Clusti_State_Render *renderState);

void setupStichingShaderUniforms(Clusti const *clusti, int videoSinkIndex,
				 int videoSourceIndex);


// -----------------------------------------------------------------------------------




//const GLenum FBO_Buffers[] = {GL_COLOR_ATTACHMENT0,  GL_COLOR_ATTACHMENT1,
//			      GL_COLOR_ATTACHMENT2,  GL_COLOR_ATTACHMENT3,
//			      GL_COLOR_ATTACHMENT4,  GL_COLOR_ATTACHMENT5,
//			      GL_COLOR_ATTACHMENT6,  GL_COLOR_ATTACHMENT7,
//			      GL_COLOR_ATTACHMENT8,  GL_COLOR_ATTACHMENT9,
//			      GL_COLOR_ATTACHMENT10, GL_COLOR_ATTACHMENT11,
//			      GL_COLOR_ATTACHMENT12, GL_COLOR_ATTACHMENT13,
//			      GL_COLOR_ATTACHMENT14, GL_COLOR_ATTACHMENT15};





// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	// ---------------------------------------------------------------------------
	Clusti *stitcher = clusti_create();

	clusti_readConfig(stitcher,
			  "../../../testdata/calibration_viewfrusta.xml");

	createRenderState(stitcher);

	//TODO extract render lopp that is currently within createRenderState()...

	clusti_destroy(stitcher);
	// ---------------------------------------------------------------------------

	return 0;
}



void createFBO(Clusti_State_Render *renderState)
{
	//generate offscreen render target (FBO)
	glGenFramebuffers(1, &(renderState->fbo));

	// render to offscreen render target
	glBindFramebuffer(GL_FRAMEBUFFER, renderState->fbo);

	//if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
	//    GL_FRAMEBUFFER_COMPLETE) {
	//	printf("framebuffer incomplete");
	//	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//	glDeleteFramebuffers(1, &(renderState->fbo));
	//	exit(-1);
	//}


	// create, bind, alloc mem for and unbind color texture
	glGenTextures(1, &(renderState->renderTargetTexture_Color));

	glBindTexture(GL_TEXTURE_2D, renderState->renderTargetTexture_Color);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderState->renderTargetRes.x,
		     renderState->renderTargetRes.y, 0, GL_RGBA,
		     GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	// bind color texture to FBO
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			       GL_TEXTURE_2D,
			       renderState->renderTargetTexture_Color, 0);  


	// create, bind, alloc mem for and unbind depth render buffer (RBO)
	glGenRenderbuffers(1, &(renderState->renderTargetRBO_Depth));

	glBindRenderbuffer(GL_RENDERBUFFER, renderState->renderTargetRBO_Depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
			      renderState->renderTargetRes.x,
			      renderState->renderTargetRes.y);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// bind depth RBO to FBO
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
				  GL_RENDERBUFFER,
				  renderState->renderTargetRBO_Depth);


	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		printf("framebuffer incomplete");
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &(renderState->fbo));
		exit(-1);
	}


	//render to window again
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



void setupStichingShaderUniforms(Clusti const *clusti, int videoSinkIndex,
				 int videoSourceIndex)
{
	Clusti_State_Render const *renderState = &(clusti->renderState);
	Clusti_Params_Stitching const *config = &(clusti->stitchingConfig);

	Clusti_Params_VideoSink const *sink_config =
		&config->videoSinks[videoSinkIndex];
	Clusti_Params_VideoSource const *source_config =
		&config->videoSources[videoSourceIndex];

	Clusti_State_Render_VideoSink const *sink_render =
		&renderState->videoSinks[videoSinkIndex]; 
	Clusti_State_Render_VideoSource const *source_render =
		&renderState->videoSources[videoSourceIndex];

	GLuint currProg = renderState->stitchShaderProgram;

	GLint currULoc = 0;

	// -----------------------------------
	//int
	currULoc = glGetUniformLocation(currProg, "sinkParams_in.index");
	glUniform1i(currULoc, videoSinkIndex);

	//sampler2D
	currULoc = glGetUniformLocation(currProg,
					"sinkParams_in.backgroundTexture");
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, sink_render->backgroundTexture);
	glUniform1i(currULoc, 0);

	//ivec2
	// TODO adapt
	currULoc = glGetUniformLocation(currProg, "sinkParams_in.resolution");
	glUniform2i(currULoc, sink_config->virtualResolution.x,
		    sink_config->virtualResolution.y);


	//int 
	currULoc = glGetUniformLocation(currProg, "sourceParams_in.index");
	glUniform1i(currULoc, videoSourceIndex);

	//sampler2D 
	currULoc = glGetUniformLocation(
		currProg, "sourceParams_in.currentPlanarRendering");
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, source_render->sourceTexture);
	glUniform1i(currULoc, 1);

	//mat4
	currULoc = glGetUniformLocation(
		currProg, "sourceParams_in.frustum_viewProjectionMatrix");
	GLfloat matBuff[16];
	graphene_matrix_to_float(
		&source_config->projection.planar_viewProjectionMatrix,
		matBuff);
	graphene_matrix_print(
		&source_config->projection.planar_viewProjectionMatrix);
	glUniformMatrix4fv(currULoc, 1, GL_TRUE, matBuff);
	//int 
	currULoc = glGetUniformLocation(
		currProg,
		"sourceParams_in.decklinkWorkaround_verticalOffset_pixels");
	glUniform1i(currULoc, source_config->decklinkWorkaround_verticalOffset_pixels);

	









	// old uniforms: -----------------------------------
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, source_render->sourceTexture);
	glUniform1i(glGetUniformLocation(
			    renderState->stitchShaderProgram,
			    "oldParams_in_planarRend"),
			    //"oldParams_in.currentPlanarRendering"),
		    0);

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, sink_render->backgroundTexture);
	glUniform1i(
		glGetUniformLocation(renderState->stitchShaderProgram,
				     //"oldParams_in.backgroundTexture"),
				     "oldParams_in_bgt"),
		    1);

	glUniform1f(glGetUniformLocation(renderState->stitchShaderProgram,
					 "oldParams_in.domeRadius"),
		    300.000000f);
	glUniform1f(
		glGetUniformLocation(renderState->stitchShaderProgram,
				     "oldParams_in.virtualScreenWidth"),
		600.000000f);
	glUniform1f(
		glGetUniformLocation(renderState->stitchShaderProgram,
				     "oldParams_in.virtualScreenHeight"),
		199.199997f);
	glUniform1f(
		glGetUniformLocation(renderState->stitchShaderProgram,
				     "oldParams_in.virtualScreenAzimuth"),
		-37.689999f);
	glUniform1f(glGetUniformLocation(
			    renderState->stitchShaderProgram,
			    "oldParams_in.virtualScreenElevation"),
		    3.600000f);
	glUniform1f(glGetUniformLocation(renderState->stitchShaderProgram,
					 "oldParams_in.domeAperture"),
		    180.000000f);
	glUniform1f(glGetUniformLocation(renderState->stitchShaderProgram,
					 "oldParams_in.domeTilt"),
		    21.610001f);
	glUniform2i(
		glGetUniformLocation(
			renderState->stitchShaderProgram,
			"oldParams_in.renderTargetResolution_uncropped"),
		8192, 4096);
}


void createRenderState(Clusti *clusti)
{
	Clusti_State_Render* renderState = &(clusti->renderState);


	// -----------------------------------------------------------------------------------
	// Create SDL window and OpenGL context

	stbi_set_flip_vertically_on_load(1);

	// init sdl2
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
		printf("Failed to initialize SDL2\n");
		exit(-1);
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
			    SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // double buffering

	if (clusti->stitchingConfig.numVideoSinks != 1) {
		printf("Only one video sink currently supported.\n");
		exit(-1);

	}

	renderState->quit = false;

	renderState->leftMouseButtonDown = false;
	

	renderState->renderTargetRes = {
		.x = (clusti->stitchingConfig.videoSinks[0]
			      .cropRectangle.max.x -
		      clusti->stitchingConfig.videoSinks[0]
			      .cropRectangle.min.x),
		.y = (clusti->stitchingConfig.videoSinks[0]
			      .cropRectangle.max.y -
		      clusti->stitchingConfig.videoSinks[0]
			      .cropRectangle.min.y)
	};

	renderState->currentRenderScale =
		(clusti->stitchingConfig.videoSinks[0]
			 .debug_renderScale);
	renderState->canvasViewPosition_pixels_lowerLeft = {0, 0};

	// window can be smaller than render target
	graphene_vec2_init(
		&(renderState->windowRes_f),
			   (float)(renderState->renderTargetRes.x) *
				   renderState->currentRenderScale,
			   (float)(renderState->renderTargetRes.y) *
				   renderState->currentRenderScale);

	// open window
	renderState->wnd = SDL_CreateWindow(
		"Clusti_Standalone", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		(int)graphene_vec2_get_x(&(renderState->windowRes_f)),
		(int)graphene_vec2_get_y(&(renderState->windowRes_f)),
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
			SDL_WINDOW_ALLOW_HIGHDPI);

	SDL_SetWindowMinimumSize(renderState->wnd, 200, 200);

	// get GL context
	renderState->glContext = SDL_GL_CreateContext(renderState->wnd);
	SDL_GL_MakeCurrent(renderState->wnd, renderState->glContext);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);

	// init glew
	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		printf("Failed to initialize GLEW\n");
		exit(-1);
	}


	// -----------------------------------------------------------------------------------
	// Framebuffer stuff:
	createFBO(renderState);
	

	// -----------------------------------------------------------------------------------
	// shaders
	char *ShaderSource_frustumToEquirect_vert = clusti_loadFileContents(
		"../../shaders/frustumToEquirect.vert");
	char *ShaderSource_frustumToEquirect_frag =
		clusti_loadFileContents("../../shaders/frustumToEquirect.glsl");
	//GLuint frustumToEquirect_pass_SP =
	renderState->stitchShaderProgram =
		CreateShader(ShaderSource_frustumToEquirect_vert,
			     ShaderSource_frustumToEquirect_frag);


	char *ShaderSource_textureViewer_vert =
		clusti_loadFileContents("../../shaders/viewTexture.vert");
	char *ShaderSource_textureViewer_frag =
		clusti_loadFileContents("../../shaders/viewTexture.frag");
	renderState->textureViewerShaderProgram =
		CreateShader(ShaderSource_textureViewer_vert,
			     ShaderSource_textureViewer_frag);

	// -----------------------------------------------------------------------------------
	// Full screen quad geometry
	CreateScreenQuadNDC(renderState);

	// -----------------------------------------------------------------------------------
	// load video source and sink textures

	char const *dataDirectory = "../../../testData/";
	char *finalFilePath = NULL;

	renderState->numVideoSinks = clusti->stitchingConfig.numVideoSinks;
	renderState->videoSinks = (Clusti_State_Render_VideoSink *)
		clusti_calloc(renderState->numVideoSinks,
			      sizeof(Clusti_State_Render_VideoSink));
	for (int i = 0; i < (int) renderState->numVideoSinks; i++) {

		finalFilePath = clusti_String_concat(
			dataDirectory, clusti->stitchingConfig.videoSinks[i].debug_backgroundImageName);

		Clusti_State_Render_VideoSink *currentSink =
			&(renderState->videoSinks[i]);

		//load and assign background texture to render state
		currentSink->backgroundTexture = LoadTexture(finalFilePath);
		assert(currentSink->backgroundTexture);

		clusti_free(finalFilePath);
		finalFilePath = NULL;
	}


	renderState->numVideoSources = clusti->stitchingConfig.numVideoSources;
	renderState->videoSources =
		(Clusti_State_Render_VideoSource *)clusti_calloc(
			renderState->numVideoSources,
			sizeof(Clusti_State_Render_VideoSource));
	for (int i = 0; i < (int) renderState->numVideoSources; i++) {
		finalFilePath = clusti_String_concat(
			dataDirectory, clusti->stitchingConfig.videoSources[i]
					       .testImageName);

		Clusti_State_Render_VideoSource *currentSource =
			&(renderState->videoSources[i]);

		//load and assign background texture to render state
		currentSource->sourceTexture = LoadTexture(finalFilePath);
		assert(currentSource->sourceTexture);

		clusti_free(finalFilePath);
		finalFilePath = NULL;

		// TODO warp and blend
	}

	//GLuint backgroundTexture = LoadTexture(
	//	"../../../testData/fisheye_as_equirect_180_bourke.jpg");

	//GLuint currentVideoSourceTexture =
	//	LoadTexture("../../../testData/RT001.png");



	// -----------------------------------------------------------------------------------
	// render loop
	// TODO outsource to own function renderLoop() to be called by main()
	while (!renderState->quit) {

		handleUserInput(renderState);

		if (renderState->quit) {
			break;
		}


		// RENDER -----------------------------------------------------

		// render stitching to offscreen texture:
		glBindFramebuffer(GL_FRAMEBUFFER, renderState->fbo);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
			GL_STENCIL_BUFFER_BIT);
		glViewport(0, 0,
			   renderState->renderTargetRes.x,
			   renderState->renderTargetRes.y);

		// use stitching shader
		glUseProgram(renderState->stitchShaderProgram);

		for (int i = 0; i < (int)renderState->numVideoSources; i++) {
			// hardcode to sink 0 for now
			setupStichingShaderUniforms(clusti, 0, i);
			// draw viewport-sized quad
			glBindVertexArray(renderState->viewPortQuadNDC_vao);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		// make sure the texture is written before showing
		glFlush();



		// -----------------------------------------------------------------------------------
		// render offscreen texture to window, scale and pan enabled:
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
			GL_STENCIL_BUFFER_BIT);

		glViewport(0, 0,
			   (GLsizei)graphene_vec2_get_x(
				   &(renderState->windowRes_f)),
			   (GLsizei)graphene_vec2_get_y(
				   &(renderState->windowRes_f)));

		glUseProgram(renderState->textureViewerShaderProgram);

		glActiveTexture(GL_TEXTURE0 + 0);
		//glBindTexture(GL_TEXTURE_2D, currentVideoSourceTexture);
		glBindTexture(GL_TEXTURE_2D, renderState->renderTargetTexture_Color);
		glUniform1i(glGetUniformLocation(
				    renderState->textureViewerShaderProgram,
				    "textureToShow"),
			    0);
		glUniform1f(glGetUniformLocation(
				    renderState->textureViewerShaderProgram,
				    "renderScale"),
			    renderState->currentRenderScale);

		glUniform2i(glGetUniformLocation(
				    renderState->textureViewerShaderProgram,
				    "offset_pixels"),
			    renderState->canvasViewPosition_pixels_lowerLeft.x,
			    renderState->canvasViewPosition_pixels_lowerLeft.y);
		
		// draw viewport-sized quad
		glBindVertexArray(renderState->viewPortQuadNDC_vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glUseProgram(0);
		glFlush();



		// show image on screen
		SDL_GL_SwapWindow(renderState->wnd);
	}

	//clusti
	clusti_free(ShaderSource_frustumToEquirect_vert);
	clusti_free(ShaderSource_frustumToEquirect_frag);

	clusti_free(ShaderSource_textureViewer_vert);
	clusti_free(ShaderSource_textureViewer_frag);


	// sdl2
	SDL_GL_DeleteContext(renderState->glContext);
	SDL_DestroyWindow(renderState->wnd);
	SDL_Quit();

	// end of to-refactor section
	// -----------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------
}




















void CreateScreenQuadNDC(Clusti_State_Render *renderState)
{
	GLfloat sqData[] = {
		-1, -1, 0.0f, 0.0f, 1, -1, 1.0f, 0.0f, 1,  1, 1.0f, 1.0f,
		-1, -1, 0.0f, 0.0f, 1, 1,  1.0f, 1.0f, -1, 1, 0.0f, 1.0f,
	};

	//GLuint vao;

	// create vao
	glGenVertexArrays(1, &(renderState->viewPortQuadNDC_vao));
	glBindVertexArray(renderState->viewPortQuadNDC_vao);

	// create vbo
	glGenBuffers(1, &(renderState->viewPortQuadNDC_vbo));
	glBindBuffer(GL_ARRAY_BUFFER, renderState->viewPortQuadNDC_vbo);

	// vbo data
	glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(GLfloat), sqData, GL_STATIC_DRAW);

	// vertex positions
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
	glEnableVertexAttribArray(0);

	// vertex texture coords
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}




GLuint CreateShader(const char* vsCode, const char* psCode)
{
	GLint success = 0;
	char infoLog[512];

	// create vertex shader
	unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vsCode, nullptr);
	glCompileShader(vs);
	glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vs, 512, NULL, infoLog);
		printf("Failed to compile the shader.\n");
		return 0;
	}

	// create pixel shader
	unsigned int ps = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(ps, 1, &psCode, nullptr);
	glCompileShader(ps);
	glGetShaderiv(ps, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(ps, 512, NULL, infoLog);
		printf("Failed to compile the shader:\n %s\n", infoLog);
		return 0;
	}

	// create a shader program for gizmo
	GLuint retShader = glCreateProgram();
	glAttachShader(retShader, vs);
	glAttachShader(retShader, ps);
	glLinkProgram(retShader);
	glGetProgramiv(retShader, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(retShader, 512, NULL, infoLog);
		printf("Failed to create the shader:\n %s\n", infoLog);
		return 0;
	}

	glDeleteShader(vs);
	glDeleteShader(ps);

	return retShader;
}



GLuint LoadTexture(const std::string& file)
{
	int width, height, nrChannels;
	unsigned char* data = stbi_load(file.c_str(), &width, &height, &nrChannels, 0);
	unsigned char* paddedData = nullptr;

	if (data == nullptr)
		return 0;

	GLenum fmt = GL_RGB;
	if (nrChannels == 4)
		fmt = GL_RGBA;
	else if (nrChannels == 1)
		fmt = GL_LUMINANCE;

	if (nrChannels != 4) {
		size_t padSize = (size_t)width * (size_t)height * (size_t)4;
		paddedData = (unsigned char *)malloc(padSize);
		if (paddedData != NULL) {
			for (int x = 0; x < width; x++) {
				for (int y = 0; y < height; y++) {
					if (nrChannels == 3) {
						paddedData[(y * width + x) * 4 + 0] = data[(y * width + x) * 3 + 0];
						paddedData[(y * width + x) * 4 + 1] = data[(y * width + x) * 3 + 1];
						paddedData[(y * width + x) * 4 + 2] = data[(y * width + x) * 3 + 2];
					} else if (nrChannels == 1) {
						unsigned char val = data[(y * width + x) * 1 + 0];
						paddedData[(y * width + x) * 4 + 0] = val;
						paddedData[(y * width + x) * 4 + 1] = val;
						paddedData[(y * width + x) * 4 + 2] = val;
					}
					paddedData[(y * width + x) * 4 + 3] = (unsigned char)(255);
				}
			}
		} else {
			perror("malloc failed!");
		}
	}

	// normal texture
	GLuint ret = 0;
	glGenTextures(1, &ret);
	glBindTexture(GL_TEXTURE_2D, ret);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, paddedData == nullptr ? data : paddedData);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (paddedData != nullptr)
		free(paddedData);
	stbi_image_free(data);

	return ret;
}







void handleUserInput(Clusti_State_Render *renderState)
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			renderState->quit = true;
		} else if (event.type == SDL_MOUSEWHEEL) {
			const float zoomSpeed = 1.1f;
			if (event.wheel.y > 0) // scroll up
			{
				// zoom in: enlarge scale
				renderState->currentRenderScale *= zoomSpeed;

			} else if (event.wheel.y < 0) // scroll down
			{
				// Put code for handling "scroll down" here!
				// zoom out: reduce scale
				renderState->currentRenderScale /= zoomSpeed;
			}

			printf("renderScale: %f\n",
			       renderState->currentRenderScale);

			if (event.wheel.x > 0) // scroll right
			{
				// ...
			} else if (event.wheel.x < 0) // scroll left
			{
				// ...
			}


			//
		} else if (event.type == SDL_MOUSEBUTTONDOWN){
			renderState->leftMouseButtonDown = true;
			renderState->lastButtonDownMousePos = {
				event.button.x,
				// invert y axis
				(int)graphene_vec2_get_y(
					&renderState->windowRes_f) -
					event.button.y};

			printf("last mouse position: (%d, %d)\n",
			       renderState->lastButtonDownMousePos.x,
			       renderState->lastButtonDownMousePos.y);

		} else if (event.type == SDL_MOUSEBUTTONUP) {
			renderState->leftMouseButtonDown = false;
		} else if (event.type == SDL_MOUSEMOTION) {

			if (renderState->leftMouseButtonDown) {

				int diff_x =
					event.motion.x -
					renderState->lastButtonDownMousePos.x;
				int diff_y =
					// invert y axis
					((int)graphene_vec2_get_y(
						 &renderState->windowRes_f) -
					 event.motion.y) -
					renderState->lastButtonDownMousePos.y;


				renderState->canvasViewPosition_pixels_lowerLeft
					.x += diff_x;
				renderState->canvasViewPosition_pixels_lowerLeft
					.y += diff_y;

				printf("view texture lower left corner position: (%d, %d)\n",
				       renderState
					       ->canvasViewPosition_pixels_lowerLeft
					       .x,
				       renderState
					       ->canvasViewPosition_pixels_lowerLeft
					       .y);

				//update last mous pos to current
				renderState->lastButtonDownMousePos = {
					event.button.x,
					// invert y axis
					(int)graphene_vec2_get_y(
						&renderState->windowRes_f) -
						event.button.y};
			}

			////sedMouseX = (float)event.motion.x;
			////sedMouseY = (float)event.motion.y;
			//graphene_vec2_init(&(renderState.mousePos_f),
			//	(float)event.motion.x,
			//	(float)event.motion.y);
			//sysMousePosition = glm::vec2(
			//	(graphene_vec2_get_x(
			//		 &(renderState.mousePos_f)) /
			//	 graphene_vec2_get_x(
			//		 &(renderState.windowRes_f))),
			//	1.0f - (graphene_vec2_get_y(&(
			//			renderState.mousePos_f)) /
			//		graphene_vec2_get_y(&(
			//			renderState
			//				.windowRes_f))));

		} else if (event.type == SDL_WINDOWEVENT &&
			   event.window.event == SDL_WINDOWEVENT_RESIZED) {

			graphene_vec2_init(&(renderState->windowRes_f),
					   (float)event.window.data1,
					   (float)event.window.data2);

			//probably obsolete
			//glBindTexture(GL_TEXTURE_2D,
			//	      renderState->renderTargetTexture_Color);
			//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8192, 4096, 0,
			//	     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			//glBindTexture(GL_TEXTURE_2D,
			//	      renderState->renderTargetTexture_Depth);
			//glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8,
			//	     8192, 4096, 0, GL_DEPTH_STENCIL,
			//	     GL_UNSIGNED_INT_24_8, NULL);
			//glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
}
