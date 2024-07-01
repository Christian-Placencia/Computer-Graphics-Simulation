/*
 Ruled surface                
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
#include <array>

#include "triangle.h"  //triangles
#include "helper.h"         
#include "objGen.h"         //to save OBJ file format for 3D printing
#include "trackball.h"

#pragma warning(disable : 4996)
#pragma comment(lib, "glfw3.lib")


using namespace std;

TrackBallC trackball;
bool mouseLeft, mouseMid, mouseRight;

vector <TriangleC> tri;   //all the triangles will be stored here
std::string filename = "geometry.obj";

GLuint points = 0; //number of points to display the object
int steps = 12;//# of subdivisions
int pointSize = 5;
int lineWidth = 1;
GLdouble mouseX, mouseY;

//Vertex array object and vertex buffer object indices 
GLuint VAO, VBO;


inline void AddVertex(vector <GLfloat>* a, glm::vec3 A) //check this!
{
	a->push_back(A[0]); a->push_back(A[1]); a->push_back(A[2]);
}


glm::vec3 P(GLfloat t)
{
	return glm::vec3(0.3 * cos(2 * M_PI * t + M_PI / 2), 0, 0.6 * sin(2 * M_PI * t + M_PI / 2));
}

inline glm::vec3 Q(GLfloat t)
{
	return glm::vec3(0.6 * cos(2 * M_PI * t), 1, 0.6 * sin(2 * M_PI * t));
}

inline glm::vec3 S(GLfloat u, GLfloat t)
{
	return glm::vec3(u * P(t) + (1 - u) * Q(t));
}

void CreateRuled(vector <GLfloat>* vv, int n)
{
	GLfloat step = 1.f / n;
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			//lower triangle
			AddVertex(vv, S(i * step, j * step));
			AddVertex(vv, S((i + 1) * step, j * step));
			AddVertex(vv, S((i + 1) * step, (j + 1) * step));
			//upper triangle
			AddVertex(vv, S(i * step, j * step));
			AddVertex(vv, S((i + 1) * step, (j + 1) * step));
			AddVertex(vv, S(i * step, (j + 1) * step));
		}
	}
}


int CompileShaders() {
	//Vertex Shader
	const char* vsSrc= "#version 330 core\n"
		"layout (location = 0) in vec4 iPos;\n"
		"uniform mat4 modelview;\n"
		"void main()\n"
		"{\n"
		"   vec4 oPos=modelview*iPos;\n"
		"   gl_Position = vec4(oPos.x, oPos.y, oPos.z, oPos.w);\n"
		"}\0";

	//Fragment Shader
	const char* fsSrc = "#version 330 core\n"
		"out vec4 col;\n"
		"uniform vec4 color;\n"
		"void main()\n"
		"{\n"
		"   col = color;\n"
		"}\n\0";

	//Create VS object
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	//Attach VS src to the Vertex Shader Object
	glShaderSource(vs, 1, &vsSrc, NULL);
	//Compile the vs
	glCompileShader(vs);

	//The same for FS
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fsSrc, NULL);
	glCompileShader(fs);

	//Get shader program object
	GLuint shaderProg = glCreateProgram();
	//Attach both vs and fs
	glAttachShader(shaderProg, vs);
	glAttachShader(shaderProg, fs);
	//Link all
	glLinkProgram(shaderProg);

	//Clear the VS and FS objects
	glDeleteShader(vs);
	glDeleteShader(fs);
	return shaderProg;
}

void BuildScene(GLuint& VBO, GLuint& VAO, int n) { //return VBO and VAO values n is the subdivision
	vector<GLfloat> v;
	CreateRuled(&v, n);
	//now get it ready for saving as OBJ
	tri.clear();
	for (unsigned int i = 0; i < v.size(); i += 9) { //stride 3 - 3 vertices per triangle
		TriangleC tmp;
		glm::vec3 a, b, c;
		a=glm::vec3(v[i], v[i + 1], v[i + 2]);
		b=glm::vec3(v[i + 3], v[i + 4], v[i + 5]);
		c = glm::vec3(v[i + 6], v[i + 7], v[i + 8]);
		tmp.Set(a, b, c); //store them for 3D export
		tri.push_back(tmp);
	}

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

	//Configure the attributes
//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//Make it valid
	glEnableVertexAttribArray(0);

	v.clear(); //no need for the data, it is on the GPU now

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


int main()
{
	glfwInit();

	//negotiate with the OpenGL
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//make OpenGL window
	GLFWwindow* window = glfwCreateWindow(800, 800, "Simple", NULL, NULL);
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
	glViewport(0, 0, 800, 800);

	//once the OpenGL context is done, build the scene and compile shaders
	BuildScene(VBO, VAO, steps);
	int shaderProg = CompileShaders();
	GLint modelviewParameter = glGetUniformLocation(shaderProg, "modelview");

	//Background color
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//Use shader
	glUseProgram(shaderProg);
	glPointSize(pointSize);

	// Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	bool drawScene = true;
	float color[4] = { 0.8f, 0.8f, 0.2f, 1.0f };
	//send the color to the fragment shader
	glUniform4f(glGetUniformLocation(shaderProg, "color"), color[0], color[1], color[2], color[3]);


	glfwSetKeyCallback(window, KbdCallback); //set keyboard callback to quit
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);;

	// Main while loop
	while (!glfwWindowShouldClose(window))
	{
		//Clean the window
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ImGUI window creation
		ImGui::Begin("Ruled Surface");
		//checkbox to render or not the scene
		ImGui::Checkbox("Draw Scene", &drawScene);
		//checkbox to render or not the scene
		if (ImGui::Button("Save OBJ")) {
			SaveOBJ(&tri, filename);
			//ImGui::OpenPopup("Saved");
		}
		//color picker
		if (ImGui::SliderInt("Mesh Subdivision", &steps, 1, 100, "%d", 0)) {
			BuildScene(VBO, VAO, steps); //rebuild scene if the subdivision has changed
		}
		if (ImGui::SliderInt("point Size", &pointSize, 1, 10, "%d", 0)) {
			glPointSize(pointSize); //set the new point size if it has been changed			
		}
		if (ImGui::SliderInt("line width", &lineWidth, 1, 10, "%d", 0)) {
			glLineWidth(lineWidth); //set the new point size if it has been changed			
		}
		if (ImGui::ColorEdit4("Color", color)) { //set the new color only if it has changed
			glUniform4f(glGetUniformLocation(shaderProg, "color"), color[0], color[1], color[2], color[3]);
		}

		// Ends the window
		ImGui::End();

		//set the projection matrix
		glm::mat4 proj = glm::perspective(65.f, 1.f, 0.01f, 1000.f);
		//set the viewing matrix (looking from [0,0,5] to [0,0,0])
		glm::mat4 view = glm::lookAt(glm::vec3(0,0,5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		//get the modeling matrix from the trackball
		glm::mat4 model = trackball.Set3DViewCameraMatrix();
		//premultiply the modelViewProjection matrix
		glm::mat4 modelView = proj * view * model;
		//and send it to the vertex shader
		glUniformMatrix4fv(modelviewParameter, 1, GL_FALSE, glm::value_ptr(modelView));
		
		if (drawScene) {
			glDrawArrays(GL_POINTS, 0, points / 3);
			glDrawArrays(GL_TRIANGLES, 0, points / 3);
		}

		// Renders the ImGUI elements
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		//Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		//make sure events are served
		glfwPollEvents();
	}
	//Cleanup
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shaderProg);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
