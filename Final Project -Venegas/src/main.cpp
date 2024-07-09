/*
Lighting and texturing
(C) Bedrich Benes 2022
Purdue University
bbenes@purdue.edu
*/

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


#include <iostream>
#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <stdio.h>
#include <iostream>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <string>
#include <vector>			//Standard template library
#include <algorithm>		
#include <array>


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"	//image reading library

#include "triangle.h"  //triangles
#include "helper.h"         
#include "objGen.h"         //to save OBJ file format for 3D printing
#include "trackball.h"
#include "shaders.h"

#pragma warning(disable : 4996)
#pragma comment(lib, "glfw3.lib")


using namespace std;

TrackBallC trackball;
bool mouseLeft, mouseMid, mouseRight;

//shader program ID
GLuint shaderProg;
GLint modelParameter;
GLint viewParameter;
GLint projParameter;
GLint modelViewNParameter;

//now the materials and lights
GLint kaParameter,kdParameter,ksParameter,shParameter;
GLint laParameter,ldParameter,lsParameter,lPosParameter;

float ksColor[3] = { 1,1,0 };
float kaColor[3] = { 0,0,0 };
float kdColor[3] = { 0,0,1 };
float sh = 100;
float laColor[3] = { 0,0,0 };
float ldColor[3] = { 0,0,1 };
float lsColor[3] = { 1,1,0 };
glm::vec4 lPos;
float ltime = 0;
float timeDelta = 0;

int texAs; //the way the texture is intepreted in fragemnt shader
GLint texAsParameter;

GLint points = 0;
GLdouble mouseX, mouseY;

int subdivision=10;
int wrapu, wrapv;
int minFilter=0, maxFilter=0;


void LoadTexture()
{
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	char filename[256];
	strcpy(filename,"textures/check.jpg");

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
	// load image, create texture and generate mipmaps
	int width, height, nrChannels;
	// The FileSystem::getPath(...) is part of the GitHub repository so we can find files on any IDE/platform; replace it with your own image path.
	unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		std::cout << "Texture " << filename << " loaded " << width << " x " << height << " pixels" << std::endl;
	}
	else
	{
		std::cout << "Texture " << filename << " not found " << std::endl;
		exit(-1);
	}
	stbi_image_free(data);
}

//default
GLfloat P1(GLfloat u)
{
	return 0.2 * sin(4 * M_PI * u) + 1.f;
}

//glass
GLfloat P2(GLfloat u)
{
	return (0.2 * sin(2 * M_PI * (1-u)) * (1.2-u) + 0.22);
}


//Cylinder
GLfloat P3(GLfloat u)
{
	return (0.5);
}

inline void AddVertex(vector <GLfloat>* a, glm::vec3 A)
{
	a->push_back(A[0]); a->push_back(A[1]); a->push_back(A[2]);
}

inline void AddUV(vector <GLfloat>* a, glm::vec2 A) 
{
	a->push_back(A[0]); a->push_back(A[1]);
}


inline glm::vec3 S(GLfloat u, GLfloat v, int scene)
{
	glm::vec3 ret;
	switch (scene) {
		case 0: ret = glm::vec3(P1(u) * sin(2 * M_PI * v), u, P1(u) * cos(2 * M_PI * v)); break;
		case 1: ret = glm::vec3(P2(u) * sin(2 * M_PI * v), u, P2(u) * cos(2 * M_PI * v)); break;
		case 2: ret = glm::vec3(P3(u) * sin(2 * M_PI * v), u, P3(u) * cos(2 * M_PI * v)); break;
	}
	return ret;
}

inline glm::vec3 Snormal(GLfloat u, GLfloat v, int scene)
{
	float du = 0.05f;
	float dv = 0.05f;
		
	glm::vec3 sdu, sdv;
	sdu = glm::vec3(S(u, v, scene) - S(u + du, v, scene)) / du;
	sdv = glm::vec3(S(u, v, scene) - S(u, v + dv, scene)) / dv;
	return glm::normalize(glm::cross(sdu, sdv));
}


void CreateRevo(vector <GLfloat>* a, float u, float v, int n, int scene)
{

	GLfloat step = 1.f / n;
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			//lower triangle
			AddVertex(a, S(i * step, j * step, scene)); //vertex
			AddVertex(a, Snormal(i * step, j * step, scene)); //normal
			AddUV(a, glm::vec2((GLfloat)i * u / n, (GLfloat)j * v / n));

			AddVertex(a, S((i + 1) * step, j * step, scene));
			AddVertex(a, Snormal((i + 1) * step, j * step, scene)); //normal
			AddUV(a, glm::vec2((GLfloat)(i + 1) * u / n, (GLfloat)j * v / n));


			AddVertex(a, S((i + 1) * step, (j + 1) * step, scene));
			AddVertex(a, Snormal((i + 1) * step, (j + 1) * step, scene)); //normal
			AddUV(a, glm::vec2((GLfloat)(i + 1) * u / n, (GLfloat)(j + 1) * v / n));

			//upper triangle
			AddVertex(a, S(i * step, j * step, scene));
			AddVertex(a, Snormal(i * step, j * step, scene)); //normal
			AddUV(a, glm::vec2((GLfloat)i * u / n, (GLfloat)j * v / n));

			AddVertex(a, S((i + 1) * step, (j + 1) * step, scene));
			AddVertex(a, Snormal((i + 1) * step, (j + 1) * step, scene)); //normal
			AddUV(a, glm::vec2((GLfloat)(i + 1) * u / n, (GLfloat)(j + 1) * v / n));

			AddVertex(a, S(i * step, (j + 1) * step, scene));
			AddVertex(a, Snormal(i * step, (j + 1) * step, scene)); //normal
			AddUV(a, glm::vec2((GLfloat)i * u / n, (GLfloat)(j + 1) * v / n));
		}
	}
}

void BuildScene(float uu, float vv, int subdiv, int scene) {
	vector<GLfloat> v;
	CreateRevo(&v, uu, vv, subdiv, scene);
	GLuint VBO, VAO;
	static bool textureLoaded = false;

	//make VAO
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	//bind it
	glBindVertexArray(VAO);

	//bind the VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//send the data to the GPU
	points = v.size();
	glBufferData(GL_ARRAY_BUFFER, points * sizeof(GLfloat), &v[0], GL_STATIC_DRAW);
	v.clear(); //no need for the data, it is on the GPU now

	//Configure the attributes

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	// TexCoord attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	if (!textureLoaded) {
		LoadTexture();
		textureLoaded = true;
	}
}


//Quit when ESC is released
static void KbdCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) glfwSetWindowShouldClose(window, GLFW_TRUE);
}

//set the callbacks for the virtual trackball
//this is executed when the mouse is moving
void MouseCallback(GLFWwindow* window, double x, double y) {
	//do not forget to pass the events to ImGUI!
	ImGuiIO& io = ImGui::GetIO();
	io.AddMousePosEvent(x, y);
	if (io.WantCaptureMouse) return; //make sure you do not call this callback when over a menu
//now process them
	mouseX = x;
	mouseY = y;
	//we need to perform an action only if a button is pressed
	if (mouseLeft)  trackball.Rotate(mouseX, mouseY); 
	if (mouseMid)   trackball.Translate(mouseX, mouseY);
	if (mouseRight) trackball.Zoom(mouseX, mouseY);
}


//set the variables when the button is pressed or released
void MouseButtonCallback(GLFWwindow* window, int button, int state, int mods) {
//do not forget to pass the events to ImGUI!
	
	ImGuiIO& io = ImGui::GetIO();
	io.AddMouseButtonEvent(button, state);
	if (io.WantCaptureMouse) return; //make sure you do not call this callback when over a menu

//process them
	if (button == GLFW_MOUSE_BUTTON_LEFT && state == GLFW_PRESS)
	{
		trackball.Set(window, true, mouseX, mouseY);
		mouseLeft = true;
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT  && state == GLFW_RELEASE)
	{
		trackball.Set(window, false, mouseX, mouseY);
		mouseLeft = false;
	}
	if (button == GLFW_MOUSE_BUTTON_MIDDLE && state == GLFW_PRESS)
	{
		trackball.Set(window, true, mouseX, mouseY);
		mouseMid = true;
	}
	if (button == GLFW_MOUSE_BUTTON_MIDDLE && state == GLFW_RELEASE)
	{
		trackball.Set(window, true, mouseX, mouseY);
		mouseMid = false;
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && state == GLFW_PRESS)
	{
		trackball.Set(window, true, mouseX, mouseY);
		mouseRight = true;
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && state == GLFW_RELEASE)
	{
		trackball.Set(window, true, mouseX, mouseY);
		mouseRight = false;
	}
}

void InitShaders(GLuint* program)
{
	std::vector<GLuint> shaderList;

	//load and compile shaders 	
	shaderList.push_back(CreateShader(GL_VERTEX_SHADER, LoadShader("shaders/texture.vert")));
	shaderList.push_back(CreateShader(GL_FRAGMENT_SHADER, LoadShader("shaders/texture.frag")));

	//create the shader program and attach the shaders to it
	*program = CreateProgram(shaderList);

	//delete shaders (they are on the GPU now)
	using namespace std;
	std::for_each(shaderList.begin(), shaderList.end(), glDeleteShader);

	glUniform1i(glGetUniformLocation(shaderProg, "tex"), 0);
	modelParameter = glGetUniformLocation(shaderProg, "model");
	viewParameter = glGetUniformLocation(shaderProg, "view");
	projParameter = glGetUniformLocation(shaderProg, "proj");
	modelViewNParameter = glGetUniformLocation(shaderProg, "modelViewN");
	//now the materials and lights
	kaParameter = glGetUniformLocation(shaderProg, "mat.ka");
	kdParameter = glGetUniformLocation(shaderProg, "mat.kd");
	ksParameter = glGetUniformLocation(shaderProg, "mat.ks");
	shParameter = glGetUniformLocation(shaderProg, "mat.sh");
	//now the light properties
	laParameter = glGetUniformLocation(shaderProg, "light.la");
	ldParameter = glGetUniformLocation(shaderProg, "light.ld");
	lsParameter = glGetUniformLocation(shaderProg, "light.ls");
	lPosParameter = glGetUniformLocation(shaderProg, "light.lPos");
	texAsParameter= glGetUniformLocation(shaderProg, "texAs");
}


int main()
{
	bool drawScene = true;
	bool filled = true;
	int scene = 2;
	float repeat = 1.f;

	glfwInit();

	//negotiate with the OpenGL
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//make OpenGL window
	GLFWwindow* window = glfwCreateWindow(1024, 1024, "Lighting", NULL, NULL);
	//is all OK?
	if (window == NULL)
	{
		std::cout << "Cannot open GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	//Paste the window to the current context
	glfwMakeContextCurrent(window);

	//Load GLAD to configure OpenGL
	gladLoadGL();
	//Set the viewport
	glViewport(0, 0, 1024, 1024);

	//once the OpenGL context is done, build the scene and compile shaders
	InitShaders(&shaderProg);
	BuildScene(1,1,subdivision, scene);

	//Background color
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//Use shader
	glUseProgram(shaderProg);

	// Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	glfwSetKeyCallback(window, KbdCallback); //set keyboard callback to quit
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);;

	glClearDepth(100000.0);
	glEnable(GL_DEPTH_TEST);

	float u = 1, v = 1; //uv coordinates for mapping

	// Main while loop
	while (!glfwWindowShouldClose(window))
	{
		//Clean the window
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ImGUI window creation
		ImGui::Begin("Texturing");
		//checkbox to render or not the scene
		if (ImGui::SliderFloat("u", &u, 0.1, 10.0, "%f", 0)) {
			BuildScene(u, v, subdivision, scene);
		}
		if (ImGui::SliderFloat("v", &v, 0.1, 10.0, "%f", 0)) {
			BuildScene(u, v, subdivision,scene);
		}
		if (ImGui::SliderInt("Subdivision", &subdivision, 1, 100, "%i", 0)) {
			BuildScene(u, v, subdivision, scene);
		}


		if (ImGui::RadioButton("Sine", &scene, 0)) BuildScene(u,v,  subdivision, scene); ImGui::SameLine();
		if (ImGui::RadioButton("Glass",   &scene, 1)) BuildScene(u, v, subdivision, scene); ImGui::SameLine();
		if (ImGui::RadioButton("Cylinder",  &scene, 2)) BuildScene(u, v, subdivision, scene); 

		ImGui::RadioButton("No tex",          &texAs, 0); ImGui::SameLine();
		ImGui::RadioButton("Tex as Color",    &texAs, 1); ImGui::SameLine();
		ImGui::RadioButton("Tex as diffuse",  &texAs, 2); ImGui::SameLine();
		ImGui::RadioButton("Tex as specular", &texAs, 3); ImGui::SameLine();
		ImGui::RadioButton("Tex as ambient",  &texAs, 4);


		if (ImGui::Checkbox("Filled", &filled)) {
			if (filled) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		  		   else glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	    }
		ImGui::ColorEdit3("Mat. Ambient", kaColor);
		ImGui::ColorEdit3("Mat. Diffuse", kdColor);
		ImGui::ColorEdit3("Mat. Specular", ksColor);
		ImGui::SliderFloat("Shininess", &sh, 0, 200, "%f", 0);

		ImGui::ColorEdit3("Light Ambient", laColor);
		ImGui::ColorEdit3("Light Diffuse", ldColor);
		ImGui::ColorEdit3("Light Specular",lsColor);
		ImGui::SliderFloat("Speed", &timeDelta, 0.0, 0.3, "%f", 0);

		// Ends the window
		ImGui::End();

		//set the projection matrix
		glm::mat4 proj = glm::perspective(40.f, 1.f, 0.01f, 1000.f);
		//set the viewing matrix (looking from [0,0,5] to [0,0,0])
		glm::mat4 view = glm::lookAt(glm::vec3(0,0,5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		//get the modeling matrix from the trackball
		glm::mat4 model = trackball.Set3DViewCameraMatrix();
		//premultiply the modelViewProjection matrix
		glm::mat4 modelView = proj * view * model;
		//and send it to the vertex shader
		glm::mat3 modelViewN = glm::mat3(view * model);
		modelViewN = glm::transpose(glm::inverse(modelViewN));

		glUniformMatrix4fv(modelParameter, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(viewParameter, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projParameter, 1, GL_FALSE, glm::value_ptr(proj));
		glUniformMatrix3fv(modelViewNParameter, 1, GL_FALSE, glm::value_ptr(modelViewN));


		lPos[0] = 100 * sin(ltime);
		lPos[1] = 0.5;
		lPos[2] = -100 * cos(ltime);
		lPos[3] = 1;
		ltime += timeDelta;
//		lPos = proj * view * model * lPos;

		//now the material
		glUniform3fv(kaParameter, 1, kaColor);
		glUniform3fv(kdParameter, 1, kdColor);
		glUniform3fv(ksParameter, 1, ksColor);
		glUniform1fv(shParameter, 1, &sh);

		//and the light
		glUniform3fv(laParameter, 1, laColor);
		glUniform3fv(ldParameter, 1, ldColor);
		glUniform3fv(lsParameter, 1, lsColor);
		glUniform4fv(lPosParameter, 1, glm::value_ptr(lPos));

		//pass the way the texure is interpreted to the fragment shader
		glUniform1iv(texAsParameter, 1, &texAs);


		glDrawArrays(GL_TRIANGLES, 0, points/3);

		// Renders the ImGUI elements
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		//Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		//make sure events are served
		glfwPollEvents();
	}
	//Cleanup
	glDeleteProgram(shaderProg);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
