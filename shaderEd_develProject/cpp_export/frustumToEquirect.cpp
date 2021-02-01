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


void CreateVAO(GLuint &geoVAO, GLuint geoVBO);
GLuint CreatePlane(GLuint &vbo, float sx, float sy);
GLuint CreateScreenQuadNDC(GLuint &vbo);
GLuint CreateCube(GLuint &vbo, float sx, float sy, float sz);
std::string LoadFile(const std::string &filename);
GLuint CreateShader(const char *vsCode, const char *psCode);
GLuint LoadTexture(const std::string &filename);


int test_expat(void);
static bool graphene_test_matrix_near();
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

	std::chrono::time_point<std::chrono::system_clock> timerStart;
	timerStart = std::chrono::system_clock::now();
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

	//clusti_getResolutionOfVideoSink();

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


	renderState.viewportRes_f = renderState.windowRes_f;
	graphene_vec2_init(&renderState.mousePos_f, 0.0f, 0.0f);




	// -----------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------
	// start of to-refactor section


		// required:
	//renderState.renderTargetResolution.x = 800, renderState.renderTargetResolution.y = 600;
	

	// init

	// shaders
	std::string ShaderSource_frustumToEquirect_vert =
		LoadFile("../../shaders/frustumToEquirect.vert");
	std::string ShaderSource_frustumToEquirect_frag =
		LoadFile("../../shaders/frustumToEquirect.glsl");

	// system variables
	float sysTime = 0.0f, sysTimeDelta = 0.0f;
	unsigned int sysFrameIndex = 0;


	//float sedMouseX = 0, sedMouseY = 0;
	//glm::vec2 sysMousePosition(sedMouseX, sedMouseY);
	glm::vec2 sysMousePosition(
		graphene_vec2_get_x(&(renderState.mousePos_f)),
		graphene_vec2_get_y(&(renderState.mousePos_f)));


	//glm::vec2 sysViewportSize(renderState.renderTargetResolution.x,
	//			  renderState.renderTargetResolution.y);
	glm::vec2 sysViewportSize(
		graphene_vec2_get_x(&(renderState.windowRes_f)),
		graphene_vec2_get_y(&(renderState.windowRes_f)));


	glm::mat4 sysView(0.998291f, -0.000051f, 0.058435f, 0.000000f,
			  0.000000f, 1.000000f, 0.000873f, 0.000000f,
			  -0.058435f, -0.000871f, 0.998291f, 0.000000f,
			  -0.038967f, 0.006246f, -7.157361f, 1.000000f);
	glm::mat4 sysProjection =
		glm::perspective(glm::radians(45.0f),
				graphene_vec2_get_x(&(renderState.windowRes_f)) /
				graphene_vec2_get_y(&(renderState.windowRes_f)),
				 0.1f, 1000.0f);
	glm::mat4 sysOrthographic = glm::ortho(
		0.0f, graphene_vec2_get_x(&(renderState.windowRes_f)),
		graphene_vec2_get_y(&(renderState.windowRes_f)), 0.0f, 0.1f,
		1000.0f);


	glm::mat4 sysGeometryTransform = glm::mat4(1.0f);
	glm::mat4 sysViewProjection = sysProjection * sysView;
	glm::mat4 sysViewOrthographic = sysOrthographic * sysView;

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

	GLuint frustumToEquirect_pass_SP =
		CreateShader(ShaderSource_frustumToEquirect_vert.c_str(),
			     ShaderSource_frustumToEquirect_frag.c_str());

	GLuint fullscreenQuad_VAO, fullscreenQuad_VBO;
	fullscreenQuad_VAO = CreateScreenQuadNDC(fullscreenQuad_VBO);

	SDL_Event event;
	bool run = true;
	while (run) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				run = false;
			} else if (event.type == SDL_MOUSEMOTION) {
				//sedMouseX = (float)event.motion.x;
				//sedMouseY = (float)event.motion.y;
				graphene_vec2_init(&(renderState.mousePos_f),
					(float)event.motion.x,
					(float)event.motion.y);
				sysMousePosition = glm::vec2(
					(graphene_vec2_get_x(
						 &(renderState.mousePos_f)) /
					 graphene_vec2_get_x(
						 &(renderState.windowRes_f))),
					1.0f - (graphene_vec2_get_y(&(
							renderState.mousePos_f)) /
						graphene_vec2_get_y(&(
							renderState
								.windowRes_f))));
					//1.0f - (sedMouseY /
					//       renderState
					//	       .renderTargetResolution
					//	       .y));
			} else if (event.type == SDL_WINDOWEVENT &&
				   event.window.event ==
					   SDL_WINDOWEVENT_RESIZED) {
				graphene_vec2_init(&(renderState.windowRes_f),
						   (float)event.window.data1,
						   (float)event.window.data2);

				sysViewportSize = glm::vec2(
					graphene_vec2_get_x(
						&(renderState.windowRes_f)),
					graphene_vec2_get_y(
						&(renderState.windowRes_f)));

				sysProjection = glm::perspective(
					glm::radians(45.0f),
					(graphene_vec2_get_x(
						 &(renderState.windowRes_f)) /
					 graphene_vec2_get_y(
						 &(renderState.windowRes_f))),
					0.1f, 1000.0f);


				sysOrthographic = glm::ortho(
					0.0f,
					graphene_vec2_get_x(
						&(renderState.windowRes_f)),
					graphene_vec2_get_y(
						&(renderState.windowRes_f)),
					0.0f, 0.1f, 1000.0f);


				sysGeometryTransform = glm::mat4(1.0f);
				sysViewProjection = sysProjection * sysView;
				sysViewOrthographic = sysOrthographic * sysView;

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

		//wftz auto generation ...?
		//glUniform1f(glGetUniformLocation(frustumToEquirect_pass_SP,
		//				 "oldcode_in_params.backgroundTexture"),
		//	    0.000000f);


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
			    0.460000f);
		glUniform2i(
			glGetUniformLocation(
				frustumToEquirect_pass_SP,
				"oldcode_in_params.renderTargetResolution_uncropped"),
			8192, 4096);
		glUniformMatrix4fv(
			glGetUniformLocation(
				frustumToEquirect_pass_SP,
				"oldcode_in_params.frustum_viewProjectionMatrix"),
			1, GL_FALSE, glm::value_ptr(sysViewProjection));

		sysGeometryTransform =
			glm::translate(glm::mat4(1),
				       glm::vec3(0.000000f, 0.000000f,
						 0.000000f)) *
			glm::yawPitchRoll(0.000000f, 0.000000f, 0.000000f) *
			glm::scale(glm::mat4(1.0f),
				   glm::vec3(1.000000f, 1.000000f, 1.000000f));
		glUniformMatrix4fv(
			glGetUniformLocation(frustumToEquirect_pass_SP,
					     "modelMat"),
			1, GL_FALSE, glm::value_ptr(sysGeometryTransform));
		glBindVertexArray(fullscreenQuad_VAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		float curTime =
			std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::system_clock::now() - timerStart)
				.count() /
			1000000.0f;
		sysTimeDelta = curTime - sysTime;
		sysTime = curTime;
		sysFrameIndex++;

		SDL_GL_SwapWindow(renderState.wnd);
	}

	// sdl2
	SDL_GL_DeleteContext(renderState.glContext);
	SDL_DestroyWindow(renderState.wnd);
	SDL_Quit();

	// end of to-refactor section
	// -----------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------




	return renderState;
}











/* Custom matcher for near matrices */
static bool graphene_test_matrix_near()
{
	graphene_matrix_t *m = graphene_matrix_alloc();
	m = graphene_matrix_init_perspective(m, 45.0f, 1.6f, 0.1f, 1000.0f);
	graphene_matrix_free(m);

	const graphene_matrix_t *check = m;

	for (unsigned i = 0; i < 4; i++) {
		graphene_vec4_t m_row, check_row;

		graphene_matrix_get_row(m, i, &m_row);
		graphene_matrix_get_row(check, i, &check_row);

		if (!graphene_vec4_near(&m_row, &check_row, 0.1f))
			return false;
	}

	return true;
}






/*---------------------------------------------------------------------------*/
/* test linking against expat */

static const char *xml =
	"<data>\n"
	"    <header length=\"4\">\n"
	"            <item name=\"time\" type=\"time\">16</item>\n"
	"            <item name=\"ref\" type=\"string\">3843747</item>\n"
	"            <item name=\"port\" type=\"int16\">0</item>\n"
	"            <item name=\"frame\" type=\"int16\">20</item>\n" 
	"            <item name=\"deioemer\" type=\"string\"> wadde hadde DUDDe da <embeddedTag> umbedded char data </embeddedTag> blubberdi </item>\n"
	"    </header>\n"
	"</data>\n";

void reset_char_data_buffer();
void process_char_data_buffer();
static bool grab_next_value;

void start_element(void *data, const char *element, const char **attribute)
{
	process_char_data_buffer();
	reset_char_data_buffer();

	if (strcmp("item", element) == 0) {
		size_t matched = 0;

		for (size_t i = 0; attribute[i]; i += 2) {
			if ((strcmp("name", attribute[i]) == 0) &&
			    (strcmp("frame", attribute[i + 1]) == 0))
				++matched;

			if ((strcmp("type", attribute[i]) == 0) &&
			    (strcmp("int16", attribute[i + 1]) == 0))
				++matched;
		}

		if (matched == 2) {
			printf("this is the element you are looking for\n");
			grab_next_value = true;
		}
	}
}

void end_element(void *data, const char *el)
{
	process_char_data_buffer();
	reset_char_data_buffer();
}

static char char_data_buffer[1024];
static size_t offs;
static bool overflow;

void reset_char_data_buffer(void)
{
	offs = 0;
	overflow = false;
	grab_next_value = false;
}

// pastes parts of the node together
void char_data(void *userData, const XML_Char *s, int len)
{
	if (!overflow) {
		if (len + offs >= sizeof(char_data_buffer)) {
			overflow = true;
		} else {
			memcpy(char_data_buffer + offs, s, len);
			offs += len;
		}
	}
}

// if the element is the one we're after, convert the character data to
// an integer value
void process_char_data_buffer(void)
{
	if (offs > 0) {
		char_data_buffer[offs] = '\0';

		printf("character data: %s\n", char_data_buffer);

		if (grab_next_value) {
			int value = atoi(char_data_buffer);

			printf("the value is %d\n", value);
		}
	}
}

int test_expat(void)
{
	XML_Parser parser = XML_ParserCreate(NULL);

	XML_SetElementHandler(parser, start_element, end_element);
	XML_SetCharacterDataHandler(parser, char_data);

	reset_char_data_buffer();

	FILE *fp = NULL;
	errno_t err = fopen_s(&fp, "../../../testdata/calibration_viewfrusta.xml", "rb");
	if (!fp || (err != 0)) {
		perror("../../../testdata/calibration_viewfrusta.xml");
		exit(1);
	}



	if (XML_Parse(parser, xml, (int) strlen(xml), XML_TRUE) == XML_STATUS_ERROR)
		printf("Error: %s\n",
		       XML_ErrorString(XML_GetErrorCode(parser)));

	XML_ParserFree(parser);

	return 0;
}


//-----------------------------------------------------------------------------------










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
GLuint CreatePlane(GLuint& vbo, float sx, float sy)
{
	float halfX = sx / 2;
	float halfY = sy / 2;

	GLfloat planeData[] = {
		halfX,
		halfY,
		0,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		halfX,
		-halfY,
		0,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		-halfX,
		-halfY,
		0,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		-halfX,
		halfY,
		0,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		halfX,
		halfY,
		0,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		-halfX,
		-halfY,
		0,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
	};

	// create vbo
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 6 * 18 * sizeof(GLfloat), planeData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLuint vao = 0;
	CreateVAO(vao, vbo);

	return vao;
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
GLuint CreateCube(GLuint& vbo, float sx, float sy, float sz)
{
	float halfX = sx / 2.0f;
	float halfY = sy / 2.0f;
	float halfZ = sz / 2.0f;

	// vec3, vec3, vec2, vec3, vec3, vec4
	GLfloat cubeData[] = {
		// front face
		-halfX,
		-halfY,
		halfZ,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		halfX,
		-halfY,
		halfZ,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		halfX,
		halfY,
		halfZ,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		-halfX,
		-halfY,
		halfZ,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		halfX,
		halfY,
		halfZ,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		-halfX,
		halfY,
		halfZ,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,

		// back face
		halfX,
		halfY,
		-halfZ,
		0.0f,
		0.0f,
		-1.0f,
		1.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		halfX,
		-halfY,
		-halfZ,
		0.0f,
		0.0f,
		-1.0f,
		1.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		-halfX,
		-halfY,
		-halfZ,
		0.0f,
		0.0f,
		-1.0f,
		0.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		-halfX,
		halfY,
		-halfZ,
		0.0f,
		0.0f,
		-1.0f,
		0.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		halfX,
		halfY,
		-halfZ,
		0.0f,
		0.0f,
		-1.0f,
		1.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		-halfX,
		-halfY,
		-halfZ,
		0.0f,
		0.0f,
		-1.0f,
		0.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,

		// right face
		halfX,
		-halfY,
		halfZ,
		1.0f,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		halfX,
		-halfY,
		-halfZ,
		1.0f,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		halfX,
		halfY,
		-halfZ,
		1.0f,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		halfX,
		-halfY,
		halfZ,
		1.0f,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		halfX,
		halfY,
		-halfZ,
		1.0f,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		halfX,
		halfY,
		halfZ,
		1.0f,
		0.0f,
		0.0f,
		0.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,

		// left face
		-halfX,
		halfY,
		-halfZ,
		-1.0f,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		-halfX,
		-halfY,
		-halfZ,
		-1.0f,
		0.0f,
		0.0f,
		1.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		-halfX,
		-halfY,
		halfZ,
		-1.0f,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		-halfX,
		halfY,
		halfZ,
		-1.0f,
		0.0f,
		0.0f,
		0.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		-halfX,
		halfY,
		-halfZ,
		-1.0f,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		-halfX,
		-halfY,
		halfZ,
		-1.0f,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,

		// top face
		-halfX,
		halfY,
		halfZ,
		0.0f,
		1.0f,
		0.0f,
		0.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		halfX,
		halfY,
		halfZ,
		0.0f,
		1.0f,
		0.0f,
		1.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		halfX,
		halfY,
		-halfZ,
		0.0f,
		1.0f,
		0.0f,
		1.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		-halfX,
		halfY,
		halfZ,
		0.0f,
		1.0f,
		0.0f,
		0.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		halfX,
		halfY,
		-halfZ,
		0.0f,
		1.0f,
		0.0f,
		1.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		-halfX,
		halfY,
		-halfZ,
		0.0f,
		1.0f,
		0.0f,
		0.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,

		// bottom face
		halfX,
		-halfY,
		-halfZ,
		0.0f,
		-1.0f,
		0.0f,
		1.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		halfX,
		-halfY,
		halfZ,
		0.0f,
		-1.0f,
		0.0f,
		1.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		-halfX,
		-halfY,
		halfZ,
		0.0f,
		-1.0f,
		0.0f,
		0.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		-halfX,
		-halfY,
		-halfZ,
		0.0f,
		-1.0f,
		0.0f,
		0.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		halfX,
		-halfY,
		-halfZ,
		0.0f,
		-1.0f,
		0.0f,
		1.0f,
		1.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		-halfX,
		-halfY,
		halfZ,
		0.0f,
		-1.0f,
		0.0f,
		0.0f,
		0.0f,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
	};

	// create vbo
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 36 * 18 * sizeof(GLfloat), cubeData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLuint vao = 0;
	CreateVAO(vao, vbo);

	return vao;
}
std::string LoadFile(const std::string& filename)
{
	std::ifstream file(filename);
	std::string src((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());
	file.close();
	return src;
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
