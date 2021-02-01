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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

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


Clusti_State_Render createRenderState(Clusti const *stitcherConfig);


GLuint LoadTexture(const std::string &filename);

void CreateVAO(GLuint &geoVAO, GLuint geoVBO);
GLuint CreateShader(const char *vsCode, const char *psCode);
GLuint CreateScreenQuadNDC(GLuint &vbo);


//GLuint CreatePlane(GLuint &vbo, float sx, float sy);
//GLuint CreateCube(GLuint &vbo, float sx, float sy, float sz);

// -----------------------------------------------------------------------------------




const GLenum FBO_Buffers[] = {GL_COLOR_ATTACHMENT0,  GL_COLOR_ATTACHMENT1,
			      GL_COLOR_ATTACHMENT2,  GL_COLOR_ATTACHMENT3,
			      GL_COLOR_ATTACHMENT4,  GL_COLOR_ATTACHMENT5,
			      GL_COLOR_ATTACHMENT6,  GL_COLOR_ATTACHMENT7,
			      GL_COLOR_ATTACHMENT8,  GL_COLOR_ATTACHMENT9,
			      GL_COLOR_ATTACHMENT10, GL_COLOR_ATTACHMENT11,
			      GL_COLOR_ATTACHMENT12, GL_COLOR_ATTACHMENT13,
			      GL_COLOR_ATTACHMENT14, GL_COLOR_ATTACHMENT15};





// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	// ---------------------------------------------------------------------------
	Clusti *stitcher = clusti_create();

	clusti_readConfig(stitcher,
			  "../../../testdata/calibration_viewfrusta.xml");

	Clusti_State_Render renderState = createRenderState(stitcher);

	clusti_destroy(stitcher);
	// ---------------------------------------------------------------------------

	return 0;
}








Clusti_State_Render createRenderState(Clusti const *stitcherConfig)
{
	//{ some non-C variables thate hopefully turn out obsolete
	//  for our purposes

	//std::chrono::time_point<std::chrono::system_clock> timerStart;
	//timerStart = std::chrono::system_clock::now();
	//}


	Clusti_State_Render renderState;


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


	renderState.renderTargetRes = {
		.x = (stitcherConfig->stitchingConfig.videoSinks[0]
			      .cropRectangle.max.x -
		      stitcherConfig->stitchingConfig.videoSinks[0]
			      .cropRectangle.min.x),
		.y = (stitcherConfig->stitchingConfig.videoSinks[0]
			      .cropRectangle.max.y -
		      stitcherConfig->stitchingConfig.videoSinks[0]
			      .cropRectangle.min.y)
	};

	float const debugRenderScale =
		(stitcherConfig->stitchingConfig.videoSinks[0]
			 .debug_renderScale);
	graphene_vec2_init(
		&(renderState.windowRes_f),
		(float)(renderState.renderTargetRes.x) * debugRenderScale,
		(float)(renderState.renderTargetRes.y) * debugRenderScale);

	// open window
	renderState.wnd = SDL_CreateWindow(
		"Clusti_Standalone", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		(int)graphene_vec2_get_x(&(renderState.windowRes_f)),
		(int)graphene_vec2_get_y(&(renderState.windowRes_f)),
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
			SDL_WINDOW_ALLOW_HIGHDPI);

	SDL_SetWindowMinimumSize(renderState.wnd, 200, 200);

	// get GL context
	renderState.glContext = SDL_GL_CreateContext(renderState.wnd);
	SDL_GL_MakeCurrent(renderState.wnd, renderState.glContext);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);

	// init glew
	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		printf("Failed to initialize GLEW\n");
		exit(-1);
	}


	//renderState.viewportRes_f = renderState.windowRes_f;
	//graphene_vec2_init(&renderState.mousePos_f, 0.0f, 0.0f);


 

	// -----------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------
	// start of to-refactor section
	

	// init

	// shaders
	//std::string ShaderSource_frustumToEquirect_vert =
	//	LoadFile("../../shaders/frustumToEquirect.vert");
	//std::string ShaderSource_frustumToEquirect_frag =
	//	LoadFile("../../shaders/frustumToEquirect.glsl");



	char *ShaderSource_frustumToEquirect_vert = clusti_loadFileContents(
		"../../shaders/frustumToEquirect.vert");
	char *ShaderSource_frustumToEquirect_frag =
		clusti_loadFileContents("../../shaders/frustumToEquirect.glsl");


	////float sedMouseX = 0, sedMouseY = 0;
	////glm::vec2 sysMousePosition(sedMouseX, sedMouseY);
	//glm::vec2 sysMousePosition(
	//	graphene_vec2_get_x(&(renderState.mousePos_f)),
	//	graphene_vec2_get_y(&(renderState.mousePos_f)));


	////glm::vec2 sysViewportSize(renderState.renderTargetResolution.x,
	////			  renderState.renderTargetResolution.y);
	//glm::vec2 sysViewportSize(
	//	graphene_vec2_get_x(&(renderState.windowRes_f)),
	//	graphene_vec2_get_y(&(renderState.windowRes_f)));


	// objects
	// renderTarget render texture
	GLuint renderTarget_Color, renderTarget_Depth;
	glGenTextures(1, &renderTarget_Color);
	glBindTexture(GL_TEXTURE_2D, renderTarget_Color);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8192, 4096, 0, GL_RGBA,
		     GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	glGenTextures(1, &renderTarget_Depth);
	glBindTexture(GL_TEXTURE_2D, renderTarget_Depth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 8192, 4096, 0,
		     GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	GLuint earth = LoadTexture("../../../testData/RT001.png");

	//GLuint domemaster_vioso_4096x4096 = LoadTexture("../../domemaster-vioso_4096x4096.jpg");

	GLuint fisheye_180_bourke =
		LoadTexture("../../../testData/fisheye_180_bourke.jpg");

	GLuint fisheye_as_equirect_180_bourke = LoadTexture(
		"../../../testData/fisheye_as_equirect_180_bourke.jpg");

	//GLuint frustumToEquirect_pass_SP =
	//	CreateShader(ShaderSource_frustumToEquirect_vert.c_str(),
	//		     ShaderSource_frustumToEquirect_frag.c_str());

	GLuint frustumToEquirect_pass_SP =
		CreateShader(ShaderSource_frustumToEquirect_vert,
			     ShaderSource_frustumToEquirect_frag);

	GLuint fullscreenQuad_VAO, fullscreenQuad_VBO;
	fullscreenQuad_VAO = CreateScreenQuadNDC(fullscreenQuad_VBO);

	SDL_Event event;
	bool run = true;
	while (run) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				run = false;
			} else if (event.type == SDL_MOUSEMOTION) {
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
				   event.window.event ==
					   SDL_WINDOWEVENT_RESIZED) {
				graphene_vec2_init(&(renderState.windowRes_f),
						   (float)event.window.data1,
						   (float)event.window.data2);

				//sysViewportSize = glm::vec2(
				//	graphene_vec2_get_x(
				//		&(renderState.windowRes_f)),
				//	graphene_vec2_get_y(
				//		&(renderState.windowRes_f)));


				glBindTexture(GL_TEXTURE_2D,
					      renderTarget_Color);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8192,
					     4096, 0, GL_RGBA, GL_UNSIGNED_BYTE,
					     NULL);
				glBindTexture(GL_TEXTURE_2D,
					      renderTarget_Depth);
				glTexImage2D(GL_TEXTURE_2D, 0,
					     GL_DEPTH24_STENCIL8, 8192, 4096, 0,
					     GL_DEPTH_STENCIL,
					     GL_UNSIGNED_INT_24_8, NULL);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}

		if (!run) {
			break;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
			GL_STENCIL_BUFFER_BIT);
		glViewport(0, 0,
			   (GLsizei)graphene_vec2_get_x(
				   &(renderState.windowRes_f)),
			   (GLsizei)graphene_vec2_get_y(
				   &(renderState.windowRes_f)));

		// RENDER

		// frustumToEquirect_pass shader pass
		glUseProgram(frustumToEquirect_pass_SP);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, earth);
		glUniform1i(glGetUniformLocation(frustumToEquirect_pass_SP,
						 "oldcode_in_params.currentPlanarRendering"),
			    0);

		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, fisheye_as_equirect_180_bourke);
		glUniform1i(glGetUniformLocation(frustumToEquirect_pass_SP,
						 "oldcode_in_params.backgroundTexture"),
			    1);




		glUniform1f(glGetUniformLocation(frustumToEquirect_pass_SP,
						 "oldcode_in_params.domeRadius"),
			    300.000000f);
		glUniform1f(
			glGetUniformLocation(frustumToEquirect_pass_SP,
					     "oldcode_in_params.virtualScreenWidth"),
			200.000000f);
		glUniform1f(
			glGetUniformLocation(frustumToEquirect_pass_SP,
					     "oldcode_in_params.virtualScreenHeight"),
			199.199997f);
		glUniform1f(
			glGetUniformLocation(frustumToEquirect_pass_SP,
					     "oldcode_in_params.virtualScreenAzimuth"),
			-37.689999f);
		glUniform1f(glGetUniformLocation(
				    frustumToEquirect_pass_SP,
				    "oldcode_in_params.virtualScreenElevation"),
			    3.600000f);
		glUniform1f(glGetUniformLocation(frustumToEquirect_pass_SP,
						 "oldcode_in_params.domeAperture"),
			    180.000000f);
		glUniform1f(glGetUniformLocation(frustumToEquirect_pass_SP,
						 "oldcode_in_params.domeTilt"),
			    21.610001f);
		glUniform1f(glGetUniformLocation(
				    frustumToEquirect_pass_SP,
				    "oldcode_in_params.debug_previewScalefactor"),
			    0.5f);
		glUniform2i(
			glGetUniformLocation(
				frustumToEquirect_pass_SP,
				"oldcode_in_params.renderTargetResolution_uncropped"),
			8192, 4096);



		glBindVertexArray(fullscreenQuad_VAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);




		SDL_GL_SwapWindow(renderState.wnd);
	}

	//clusti
	clusti_free(ShaderSource_frustumToEquirect_vert);
	clusti_free(ShaderSource_frustumToEquirect_frag);


	// sdl2
	SDL_GL_DeleteContext(renderState.glContext);
	SDL_DestroyWindow(renderState.wnd);
	SDL_Quit();

	// end of to-refactor section
	// -----------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------




	return renderState;
}



















void CreateVAO(GLuint& geoVAO, GLuint geoVBO)
{
	if (geoVAO != 0)
		glDeleteVertexArrays(1, &geoVAO);

	glGenVertexArrays(1, &geoVAO);
	glBindVertexArray(geoVAO);
	glBindBuffer(GL_ARRAY_BUFFER, geoVBO);

	// POSITION
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 18 * sizeof(float), (void*)(0 * sizeof(GLfloat)));
	glEnableVertexAttribArray(0);

	// NORMAL
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 18 * sizeof(float), (void*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// TEXCOORD
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 18 * sizeof(float), (void*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


GLuint CreateScreenQuadNDC(GLuint& vbo)
{
	GLfloat sqData[] = {
		-1,
		-1,
		0.0f,
		0.0f,
		1,
		-1,
		1.0f,
		0.0f,
		1,
		1,
		1.0f,
		1.0f,
		-1,
		-1,
		0.0f,
		0.0f,
		1,
		1,
		1.0f,
		1.0f,
		-1,
		1,
		0.0f,
		1.0f,
	};

	GLuint vao;

	// create vao
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// create vbo
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

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

	return vao;
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
		printf("Failed to create the shader.\n");
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



