// ==========================================================================
// Barebones OpenGL Core Profile Boilerplate
//    using the GLFW windowing system (http://www.glfw.org)
//
// Loosely based on
//  - Chris Wellons' example (https://github.com/skeeto/opengl-demo) and
//  - Camilla Berglund's example (http://www.glfw.org/docs/latest/quick.html)
//
// Author:  Sonny Chan, University of Calgary
// I edited this. I am Cam. Cam Hardy
// Date:    2016
// ==========================================================================

#include <iostream>
#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <vector>
#include "glm/glm.hpp"
// specify that we want the OpenGL core profile before including GLFW headers
// specify that we want the OpenGL core profile before including GLFW headers
#ifdef _WIN32
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#else
#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#define PI 3.14159265359
#define WIDTH 1024.0 // wanna change the window size? change these
#define HEIGHT 1024.0 // this one too ok

using namespace std;
using namespace glm;

//Forward definitions
bool CheckGLErrors(string location);
void QueryGLVersion();
string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);

GLFWwindow* window = 0;

GLuint texID;
int tWidth;                   // image width
int tHeight;                  // image height
int cState = 1;               // color state
int eState = 0;               // edge filter state
int bState = 0;               // blur state
float zLevel = 1.0;           // zoom level
vec2 offset = vec2(0.0);      // translate offset
bool modifier = false;        // shift key modifier
bool cmodifier = false;       // control key modifier
float theta = 0.0;            // rotation angle
float edge_thres = 0.2;       // cel shader line threshold
float edge_thres2 = 5.0;      // just another cel shader line threshold
int spacebarPressed = 0;      // spacebar activatews wacky color mode

float progress = 0.f;

// --------------------------------------------------------------------------
// GLFW callback functions

// reports GLFW errors
void ErrorCallback(int error, const char* description)
{
    cout << "GLFW ERROR " << error << ":" << endl;
    cout << description << endl;
}

//==========================================================================
// TUTORIAL STUFF


//vec2 and vec3 are part of the glm math library.
//Include in your own project by putting the glm directory in your project,
//and including glm/glm.hpp as I have at the top of the file.
//"using namespace glm;" will allow you to avoid writing everyting as glm::vec2
vector<vec2> points;
vector<vec2> uvs;

//Structs are simply acting as namespaces
//Access the values like so: VAO::LINES
struct VAO{
	enum {LINES=0, COUNT};		//Enumeration assigns each name a value going up
										//LINES=0, COUNT=1
};

struct VBO{
	enum {POINTS=0, COLOR, COUNT};	//POINTS=0, COLOR=1, COUNT=2
};

struct SHADER{
	enum {LINE=0, COUNT};		//LINE=0, COUNT=1
};

GLuint vbo [VBO::COUNT];		//Array which stores OpenGL's vertex buffer object handles
GLuint vao [VAO::COUNT];		//Array which stores Vertex Array Object handles
GLuint shader [SHADER::COUNT];		//Array which stores shader program handles


//Gets handles from OpenGL
void generateIDs()
{
	glGenVertexArrays(VAO::COUNT, vao);		//Tells OpenGL to create VAO::COUNT many
														// Vertex Array Objects, and store their
														// handles in vao array
	glGenBuffers(VBO::COUNT, vbo);		//Tells OpenGL to create VBO::COUNT many
													//Vertex Buffer Objects and store their
													//handles in vbo array
}

//Clean up IDs when you're done using them
void deleteIDs()
{
	for(int i=0; i<SHADER::COUNT; i++)
	{
		glDeleteProgram(shader[i]);
	}

	glDeleteVertexArrays(VAO::COUNT, vao);
	glDeleteBuffers(VBO::COUNT, vbo);
}


//Describe the setup of the Vertex Array Object
bool initVAO()
{
	glBindVertexArray(vao[VAO::LINES]);		//Set the active Vertex Array

	glEnableVertexAttribArray(0);		//Tell opengl you're using layout attribute 0 (For shader input)
	glBindBuffer( GL_ARRAY_BUFFER, vbo[VBO::POINTS] );		//Set the active Vertex Buffer
	glVertexAttribPointer(
		0,				//Attribute
		2,				//Size # Components
		GL_FLOAT,	//Type
		GL_FALSE, 	//Normalized?
		sizeof(vec2),	//Stride
		(void*)0			//Offset
		);

	glEnableVertexAttribArray(1);		//Tell opengl you're using layout attribute 1
	glBindBuffer(GL_ARRAY_BUFFER, vbo[VBO::COLOR]);
	glVertexAttribPointer(
		1,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(vec2),
		(void*)0
		);

	return !CheckGLErrors("initVAO");		//Check for errors in initialize
}


//Loads buffers with data
bool loadBuffer(const vector<vec2>& points, const vector<vec2>& colors)
{
	glBindBuffer(GL_ARRAY_BUFFER, vbo[VBO::POINTS]);
	glBufferData(
		GL_ARRAY_BUFFER,				//Which buffer you're loading too
		sizeof(vec2)*points.size(),	//Size of data in array (in bytes)
		&points[0],							//Start of array (&points[0] will give you pointer to start of vector)
		GL_STATIC_DRAW						//GL_DYNAMIC_DRAW if you're changing the data often
												//GL_STATIC_DRAW if you're changing seldomly
		);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[VBO::COLOR]);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(vec2)*uvs.size(),
		&uvs[0],
		GL_STATIC_DRAW
		);

	return !CheckGLErrors("loadBuffer");
}

//Compile and link shaders, storing the program ID in shader array
bool initShader()
{
	string vertexSource = LoadSource("vertex.glsl");		//Put vertex file text into string
	string fragmentSource = LoadSource("fragment.glsl");		//Put fragment file text into string

	GLuint vertexID = CompileShader(GL_VERTEX_SHADER, vertexSource);
	GLuint fragmentID = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

	shader[SHADER::LINE] = LinkProgram(vertexID, fragmentID);	//Link and store program ID in shader array

	return !CheckGLErrors("initShader");
}

//For reference:
//	https://open.gl/textures
GLuint createTexture(const char* filename)
{
	int components;
	GLuint texID;

	//stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(filename, &tWidth, &tHeight, &components, 0);

	if(data != NULL)
	{
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);

		if(components==3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tWidth, tHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		else if(components==4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tWidth, tHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//Clean up
		glBindTexture(GL_TEXTURE_2D, 0);
		stbi_image_free(data);

		return texID;
	}
	else
	{
		cout << "Error loading texture" << endl;
	}

	return 0;	//Error
}

//Use program before loading texture
//	texUnit can be - GL_TEXTURE0, GL_TEXTURE1, etc...
bool loadTexture(GLuint texID, GLuint texUnit, GLuint program, const char* uniformName)
{
	glActiveTexture(texUnit);
	glBindTexture(GL_TEXTURE_2D, texID);

	glUseProgram(shader[SHADER::LINE]);
	GLint uniformLocation = glGetUniformLocation(program, "image");
	glUniform1i(uniformLocation, 0);

	return !CheckGLErrors("loadTexture");
}

// generate a rectangle to render the image onto, and scale it so the image maintains its original aspect ratio
void generateRect(float width, float height)
{
  points.clear();
  uvs.clear();

  if (width < height) {
    width = (width/height) * 2;
    height = 2;
  }
  else {
    height = (height/width) * 2;
    width = 2;
  }

	vec2 p00 = vec2(
		-width*0.5f,
		height*0.5f);
	vec2 uv00 = vec2(
		0.f,
		0.f);

	vec2 p01 = vec2(
		width*0.5f,
		height*0.5f);
	vec2 uv01 = vec2(
		1.f,
		0.f);

	vec2 p10 = vec2(
		-width*0.5f,
		-height*0.5f);
	vec2 uv10 = vec2(
		0.f,
		1.f);

	vec2 p11 = vec2(
		width*0.5f,
		-height*0.5f);
	vec2 uv11 = vec2(
		1.f,
		1.f);

	//Triangle 1
	points.push_back(p00);
	points.push_back(p10);
	points.push_back(p01);

	uvs.push_back(uv00);
	uvs.push_back(uv10);
	uvs.push_back(uv01);

	//Triangle 2
	points.push_back(p11);
	points.push_back(p01);
	points.push_back(p10);

	uvs.push_back(uv11);
	uvs.push_back(uv01);
	uvs.push_back(uv10);
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GL_TRUE);
  }

    // use number 1-5 to select an image
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
      texID = createTexture("image1-mandrill.png");
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
      texID = createTexture("image2-uclogo.png");
    }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
      texID = createTexture("image3-aerial.jpg");
    }
    if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
      texID = createTexture("image4-thirsk.jpg");
    }
    if (key == GLFW_KEY_5 && action == GLFW_PRESS) {
      texID = createTexture("image5-pattern.png");
    }
    if (key == GLFW_KEY_6 && action == GLFW_PRESS) {
      texID = createTexture("ohboy.jpg");
    }
    if (key == GLFW_KEY_7 && action == GLFW_PRESS) {
      texID = createTexture("pokemon.png");
    }
    if (key == GLFW_KEY_8 && action == GLFW_PRESS) {
      texID = createTexture("keyboard.jpg");
    }

    generateRect((float)tWidth, (float)tHeight);
    loadBuffer(points, uvs);

    // key modifiers
    if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS) {
      modifier = true;
    }
    if (key == GLFW_KEY_RIGHT_SHIFT && action == GLFW_PRESS) {
      modifier = true;
    }
    if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE) {
      modifier = false;
    }
    if (key == GLFW_KEY_RIGHT_SHIFT && action == GLFW_RELEASE) {
      modifier = false;
    }
    if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_PRESS) {
      cmodifier = true;
    }
    if (key == GLFW_KEY_RIGHT_CONTROL && action == GLFW_PRESS) {
      cmodifier = true;
    }
    if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_RELEASE) {
      cmodifier = false;
    }
    if (key == GLFW_KEY_RIGHT_CONTROL && action == GLFW_RELEASE) {
      cmodifier = false;
    }
    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
      spacebarPressed = 1;
    }
    if (key == GLFW_KEY_M && action == GLFW_RELEASE) {
      spacebarPressed = 0;
    }

    // how about some zoom, OR EVEN ROTATES
    // arrow keys and stuff
    // hold shift to access these amazeballs feature
    if (key == GLFW_KEY_UP && action == GLFW_PRESS && modifier) {
      zLevel /= 1.2;
    }
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS && modifier) {
      zLevel *= 1.2;
    }
    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS && modifier) {
      theta += PI/6;
    }
    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS && modifier) {
      theta -= PI/6;
    }

    // this bit is to control the edge thresholds for the crazy cel shader`
    if (key == GLFW_KEY_UP && action == GLFW_PRESS && cmodifier) {
      edge_thres += 0.1;
      if (edge_thres > 1.0) {
        edge_thres -= 0.1;
      }
    }
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS && cmodifier) {
      edge_thres -= 0.1;
      if (edge_thres < 0.0) {
        edge_thres += 0.1;
      }
    }
    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS && cmodifier) {
      edge_thres2 -= 1.0;
      if (edge_thres2 < 0.0) {
        edge_thres2 += 1.0;
      }
    }
    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS && cmodifier) {
      edge_thres2 += 1.0;
    }

    // muh translations
    // use the arrows, luke
    // no shift required
    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS && !modifier && !cmodifier) {
      offset.x -= 0.1;
    }
    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS && !modifier && !cmodifier) {
      offset.x += 0.1;
    }
    if (key == GLFW_KEY_UP && action == GLFW_PRESS && !modifier && !cmodifier) {
      offset.y += 0.1;
    }
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS && !modifier && !cmodifier) {
      offset.y -= 0.1;
    }

    // use keys q, w, e, r, t, y, u to change color filter
    if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
      cState = 1; // natural color
    }
    if (key == GLFW_KEY_W && action == GLFW_PRESS) {
      cState = 2; // b&w variant 1
    }
    if (key == GLFW_KEY_E && action == GLFW_PRESS) {
      cState = 3; // b&w variant 2
    }
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
      cState = 4; // b&w variant 3
    }
    if (key == GLFW_KEY_T && action == GLFW_PRESS) {
      cState = 5; // color inversion
    }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS) {
      cState = 6; // cel shading
    }
    if (key == GLFW_KEY_U && action == GLFW_PRESS) {
      cState = 7; // full saturation
    }

    // use keys a, s, d, f to change edge effects
    if (key == GLFW_KEY_A && action == GLFW_PRESS) {
      eState = 1; // no kernel
    }
    if (key == GLFW_KEY_S && action == GLFW_PRESS) {
      eState = 2; // vertical sobel kernel
    }
    if (key == GLFW_KEY_D && action == GLFW_PRESS) {
      eState = 3; // horizontal sobel kernel
    }
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
      eState = 4; // unsharp mask kernel
    }
    if (key == GLFW_KEY_G && action == GLFW_PRESS) {
      eState = 5; // mystery effect
    }

    // use keys z and x to increment or decrement the level of gaussian blur (any odd value >= 3x3)
    if (bState > 0) {
      if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
        bState-=2;
      }
    }
    if (key == GLFW_KEY_X && action == GLFW_PRESS) {
      bState+=2;
    }

    // whew, now that you've done that, i guess u render or whatever
    loadTexture(texID, GL_TEXTURE0, shader[SHADER::LINE], "image");
}

bool loadUniforms()
{
  // window width
  GLint widthUniformLocation = glGetUniformLocation(shader[SHADER::LINE], "width");
  glUniform1f(widthUniformLocation, WIDTH);

  // window height
  GLint heightUniformLocation = glGetUniformLocation(shader[SHADER::LINE], "height");
  glUniform1f(heightUniformLocation, HEIGHT);

  // color state
  GLint cStateUniformLocation = glGetUniformLocation(shader[SHADER::LINE], "cState");
  glUniform1i(cStateUniformLocation, cState);

  // edge filter state
  GLint eStateUniformLocation = glGetUniformLocation(shader[SHADER::LINE], "eState");
  glUniform1i(eStateUniformLocation, eState);

  // blur state
  GLint bStateUniformLocation = glGetUniformLocation(shader[SHADER::LINE], "bState");
  glUniform1i(bStateUniformLocation, bState);

  // zoom level
  GLint zLevelUniformLocation = glGetUniformLocation(shader[SHADER::LINE], "zLevel");
  glUniform1f(zLevelUniformLocation, zLevel);

  // translation offset
  GLint offsetUniformLocation = glGetUniformLocation(shader[SHADER::LINE], "offset");
  glUniform2f(offsetUniformLocation, offset.x, offset.y);

  // rotation ahoy
  GLint thetaUniformLocation = glGetUniformLocation(shader[SHADER::LINE], "theta");
  glUniform1f(thetaUniformLocation, theta);

  GLint edge_thresUniformLocation = glGetUniformLocation(shader[SHADER::LINE], "edge_thres");
  glUniform1f(edge_thresUniformLocation, edge_thres);

  GLint edge_thres2UniformLocation = glGetUniformLocation(shader[SHADER::LINE], "edge_thres2");
  glUniform1f(edge_thres2UniformLocation, edge_thres2);

  GLint spacebarPressedUniformLocation = glGetUniformLocation(shader[SHADER::LINE], "spacebarPressed");
  glUniform1i(spacebarPressedUniformLocation, spacebarPressed);

	return !CheckGLErrors("loadUniforms");
}

//Initialization
void initGL()
{
	//Only call these once - don't call again every time you change geometry
	generateIDs();		//Create VertexArrayObjects and Vertex Buffer Objects and store their handles
	initShader();		//Create shader and store program ID

	initVAO();			//Describe setup of Vertex Array Objects and Vertex Buffer Objects

  // get that big 'ol monkey on me monitor
	texID = createTexture("image1-mandrill.png");
	loadTexture(texID, GL_TEXTURE0, shader[SHADER::LINE], "image");

  //Call these two (or equivalents) every time you change geometry
	generateRect((float)tWidth, (float)tHeight);
	loadBuffer(points, uvs);	//Load geometry into buffers
}


//Draws buffers to screen
void render()
{
	glClearColor(0.f, 0.f, 0.f, 0.f);		//Color to clear the screen with (R, G, B, Alpha)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		//Clear color and depth buffers (Haven't covered yet)

	//Don't need to call these on every draw, so long as they don't change
	glUseProgram(shader[SHADER::LINE]);		//Use LINE program
	glBindVertexArray(vao[VAO::LINES]);		//Use the LINES vertex array

  // load 'em
	loadUniforms();

	glDrawArrays(
			GL_TRIANGLES,		//What shape we're drawing	- GL_TRIANGLES, GL_LINES, GL_POINTS, GL_QUADS, GL_TRIANGLE_STRIP
			0,						//Starting index
			points.size()		//How many vertices
			);

	CheckGLErrors("render");

	progress += 0.01;
}




// ==========================================================================
// PROGRAM ENTRY POINT

int main(int argc, char *argv[])
{
    // initialize the GLFW windowing system
    if (!glfwInit()) {
        cout << "ERROR: GLFW failed to initilize, TERMINATING" << endl;
        return -1;
    }
    glfwSetErrorCallback(ErrorCallback);

    // attempt to create a window with an OpenGL 4.1 core profile context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(WIDTH, HEIGHT, "CPSC 453 OpenGL Boilerplate", 0, 0);
    if (!window) {
        cout << "Program failed to create GLFW window, TERMINATING" << endl;
        glfwTerminate();
        return -1;
    }

    // set keyboard callback function and make our context current (active)
    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);


    // query and print out information about our OpenGL environment
    QueryGLVersion();

	initGL();

    // run an event-triggered main loop
    while (!glfwWindowShouldClose(window))
    {
        // call function to draw our scene
        render();

        // scene is rendered to the back buffer, so swap to front for display
        glfwSwapBuffers(window);

        // sleep until next event before drawing again
        glfwWaitEvents();
	}

	// clean up allocated resources before exit
   deleteIDs();
	glfwDestroyWindow(window);
   glfwTerminate();

   return 0;
}

// ==========================================================================
// SUPPORT FUNCTION DEFINITIONS

// --------------------------------------------------------------------------
// OpenGL utility functions

void QueryGLVersion()
{
    // query opengl version and renderer information
    string version  = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    string glslver  = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    string renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

    cout << "OpenGL [ " << version << " ] "
         << "with GLSL [ " << glslver << " ] "
         << "on renderer [ " << renderer << " ]" << endl;
}

bool CheckGLErrors(string location)
{
    bool error = false;
    for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
    {
        cout << "OpenGL ERROR:  ";
        switch (flag) {
        case GL_INVALID_ENUM:
            cout << location << ": " << "GL_INVALID_ENUM" << endl; break;
        case GL_INVALID_VALUE:
            cout << location << ": " << "GL_INVALID_VALUE" << endl; break;
        case GL_INVALID_OPERATION:
            cout << location << ": " << "GL_INVALID_OPERATION" << endl; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            cout << location << ": " << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl; break;
        case GL_OUT_OF_MEMORY:
            cout << location << ": " << "GL_OUT_OF_MEMORY" << endl; break;
        default:
            cout << "[unknown error code]" << endl;
        }
        error = true;
    }
    return error;
}

// --------------------------------------------------------------------------
// OpenGL shader support functions

// reads a text file with the given name into a string
string LoadSource(const string &filename)
{
    string source;

    ifstream input(filename.c_str());
    if (input) {
        copy(istreambuf_iterator<char>(input),
             istreambuf_iterator<char>(),
             back_inserter(source));
        input.close();
    }
    else {
        cout << "ERROR: Could not load shader source from file "
             << filename << endl;
    }

    return source;
}

// creates and returns a shader object compiled from the given source
GLuint CompileShader(GLenum shaderType, const string &source)
{
    // allocate shader object name
    GLuint shaderObject = glCreateShader(shaderType);

    // try compiling the source as a shader of the given type
    const GLchar *source_ptr = source.c_str();
    glShaderSource(shaderObject, 1, &source_ptr, 0);
    glCompileShader(shaderObject);

    // retrieve compile status
    GLint status;
    glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint length;
        glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
        string info(length, ' ');
        glGetShaderInfoLog(shaderObject, info.length(), &length, &info[0]);
        cout << "ERROR compiling shader:" << endl << endl;
        cout << source << endl;
        cout << info << endl;
    }

    return shaderObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader)
{
    // allocate program object name
    GLuint programObject = glCreateProgram();

    // attach provided shader objects to this program
    if (vertexShader)   glAttachShader(programObject, vertexShader);
    if (fragmentShader) glAttachShader(programObject, fragmentShader);

    // try linking the program with given attachments
    glLinkProgram(programObject);

    // retrieve link status
    GLint status;
    glGetProgramiv(programObject, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint length;
        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
        string info(length, ' ');
        glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
        cout << "ERROR linking shader program:" << endl;
        cout << info << endl;
    }

    return programObject;
}


// ==========================================================================
