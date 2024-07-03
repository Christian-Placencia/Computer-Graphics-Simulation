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

inline void AddVertex(vector <GLfloat>* a, glm::vec3 A) {
	a->push_back(A[0]); a->push_back(A[1]); a->push_back(A[2]);
}

// Superficie de revolución 1 (Cono sin base)
glm::vec3 Cone(float u, float v) {
	float theta = 2 * M_PI * u;
	float x = (1 - v) * cos(theta);
	float y = v;
	float z = (1 - v) * sin(theta);
	return glm::vec3(x, y, z);
}

void CreateCone(vector<GLfloat>& vv, int n) {
	float step = 1.f / n;
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			AddVertex(&vv, Cone(i * step, j * step));
			AddVertex(&vv, Cone((i + 1) * step, j * step));
			AddVertex(&vv, Cone((i + 1) * step, (j + 1) * step));

			AddVertex(&vv, Cone(i * step, j * step));
			AddVertex(&vv, Cone((i + 1) * step, (j + 1) * step));
			AddVertex(&vv, Cone(i * step, (j + 1) * step));
		}
	}
}

// Superficie de revolución 2 (Antena de TV)
glm::vec3 SandClock(float u, float v) {
	float theta = 2 * M_PI * u;
	float r = 0.5 + 0.5 * cos(2 * M_PI * v);
	float x = r * cos(theta);
	float y = 2 * v - 1;  // Adjusted height
	float z = r * sin(theta);
	return glm::vec3(x, y, z);
}

void CreateSandClock(vector<GLfloat>& vv, int n) {
	float step = 1.f / n;
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			AddVertex(&vv, SandClock(i * step, j * step));
			AddVertex(&vv, SandClock((i + 1) * step, j * step));
			AddVertex(&vv, SandClock((i + 1) * step, (j + 1) * step));

			AddVertex(&vv, SandClock(i * step, j * step));
			AddVertex(&vv, SandClock((i + 1) * step, (j + 1) * step));
			AddVertex(&vv, SandClock(i * step, (j + 1) * step));
		}
	}
}

void CreateCube(vector<GLfloat>& vv, int n) {
	glm::vec3 vertices[8] = {
		glm::vec3(-1.0f, -1.0f, -1.0f),
		glm::vec3(1.0f, -1.0f, -1.0f),
		glm::vec3(1.0f,  1.0f, -1.0f),
		glm::vec3(-1.0f,  1.0f, -1.0f),
		glm::vec3(-1.0f, -1.0f,  1.0f),
		glm::vec3(1.0f, -1.0f,  1.0f),
		glm::vec3(1.0f,  1.0f,  1.0f),
		glm::vec3(-1.0f,  1.0f,  1.0f)
	};

	unsigned int faces[6][4] = {
		{0, 1, 2, 3},
		{4, 5, 6, 7},
		{0, 1, 5, 4},
		{2, 3, 7, 6},
		{0, 3, 7, 4},
		{1, 2, 6, 5}
	};

	for (int f = 0; f < 6; ++f) {
		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < n; ++j) {
				float u0 = (float)i / n;
				float v0 = (float)j / n;
				float u1 = (float)(i + 1) / n;
				float v1 = (float)(j + 1) / n;

				glm::vec3 p0 = (1 - u0) * (1 - v0) * vertices[faces[f][0]] +
					u0 * (1 - v0) * vertices[faces[f][1]] +
					u0 * v0 * vertices[faces[f][2]] +
					(1 - u0) * v0 * vertices[faces[f][3]];

				glm::vec3 p1 = (1 - u1) * (1 - v0) * vertices[faces[f][0]] +
					u1 * (1 - v0) * vertices[faces[f][1]] +
					u1 * v0 * vertices[faces[f][2]] +
					(1 - u1) * v0 * vertices[faces[f][3]];

				glm::vec3 p2 = (1 - u1) * (1 - v1) * vertices[faces[f][0]] +
					u1 * (1 - v1) * vertices[faces[f][1]] +
					u1 * v1 * vertices[faces[f][2]] +
					(1 - u1) * v1 * vertices[faces[f][3]];

				glm::vec3 p3 = (1 - u0) * (1 - v1) * vertices[faces[f][0]] +
					u0 * (1 - v1) * vertices[faces[f][1]] +
					u0 * v1 * vertices[faces[f][2]] +
					(1 - u0) * v1 * vertices[faces[f][3]];

				AddVertex(&vv, p0);
				AddVertex(&vv, p1);
				AddVertex(&vv, p2);

				AddVertex(&vv, p0);
				AddVertex(&vv, p2);
				AddVertex(&vv, p3);
			}
		}
	}
}

void CreateDiamond(vector<GLfloat>& vv, int n) {
	glm::vec3 vertices[6] = {
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, -1.0f, 0.0f),
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(-1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, 0.0f, -1.0f)
	};

	unsigned int indices[24] = {
		0, 2, 4,
		0, 4, 3,
		0, 3, 5,
		0, 5, 2,
		1, 2, 4,
		1, 4, 3,
		1, 3, 5,
		1, 5, 2
	};

	for (unsigned int i = 0; i < 24; i++) {
		AddVertex(&vv, vertices[indices[i]]);
	}
}


inline glm::vec3 Sphere(GLfloat u, GLfloat v) {
	return glm::vec3(cos(2 * M_PI * u) * sin(M_PI * v),
		sin(2 * M_PI * u) * sin(M_PI * v),
		cos(M_PI * v));
}

void CreateSphere(vector<GLfloat>& vertices, int n) {
	GLfloat step = 1.f / n;
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			AddVertex(&vertices, Sphere(i * step, j * step));
			AddVertex(&vertices, Sphere((i + 1) * step, j * step));
			AddVertex(&vertices, Sphere(i * step, (j + 1) * step));

			AddVertex(&vertices, Sphere((i + 1) * step, j * step));
			AddVertex(&vertices, Sphere((i + 1) * step, (j + 1) * step));
			AddVertex(&vertices, Sphere(i * step, (j + 1) * step));
		}
	}
}

// Pirámide
void CreatePyramid(vector<GLfloat>& vv, int n) {
	glm::vec3 vertices[5] = {
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(-1.0f, -1.0f, -1.0f),
		glm::vec3(1.0f, -1.0f, -1.0f),
		glm::vec3(1.0f, -1.0f, 1.0f),
		glm::vec3(-1.0f, -1.0f, 1.0f)
	};

	unsigned int indices[18] = {
		0, 1, 2,
		0, 2, 3,
		0, 3, 4,
		0, 4, 1,
		1, 2, 3,
		1, 3, 4
	};

	for (unsigned int i = 0; i < 18; i++) {
		AddVertex(&vv, vertices[indices[i]]);
	}
}



void CreateCylinder(vector<GLfloat>& vv, int n) {
	for (int i = 0; i < n; ++i) {
		float theta = 2.0f * M_PI * float(i) / float(n);
		float nextTheta = 2.0f * M_PI * float(i + 1) / float(n);

		AddVertex(&vv, glm::vec3(cos(theta), 1.0f, sin(theta)));
		AddVertex(&vv, glm::vec3(cos(nextTheta), 1.0f, sin(nextTheta)));
		AddVertex(&vv, glm::vec3(cos(nextTheta), 0.0f, sin(nextTheta)));

		AddVertex(&vv, glm::vec3(cos(theta), 1.0f, sin(theta)));
		AddVertex(&vv, glm::vec3(cos(nextTheta), 0.0f, sin(nextTheta)));
		AddVertex(&vv, glm::vec3(cos(theta), 0.0f, sin(theta)));
	}
}

int CompileShaders() {
	//Vertex Shader
	const char* vsSrc = "#version 330 core\n"
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

void BuildScene(GLuint& VBO, GLuint& VAO, int n, void(*CreateObjectFunc)(vector<GLfloat>&, int)) {
	vector<GLfloat> v;
	CreateObjectFunc(v, n);
	// now get it ready for saving as OBJ
	tri.clear();
	for (unsigned int i = 0; i < v.size(); i += 9) { //stride 3 - 3 vertices per triangle
		TriangleC tmp;
		glm::vec3 a, b, c;
		a = glm::vec3(v[i], v[i + 1], v[i + 2]);
		b = glm::vec3(v[i + 3], v[i + 4], v[i + 5]);
		c = glm::vec3(v[i + 6], v[i + 7], v[i + 8]);
		tmp.Set(a, b, c); //store them for 3D export
		tri.push_back(tmp);
	}

	// make VAO
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	// bind it
	glBindVertexArray(VAO);

	// bind the VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// send the data to the GPU
	points = v.size();
	glBufferData(GL_ARRAY_BUFFER, points * sizeof(GLfloat), &v[0], GL_STATIC_DRAW);

	// Configure the attributes
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	// Make it valid
	glEnableVertexAttribArray(0);

	v.clear(); // no need for the data, it is on the GPU now
}

static void KbdCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void MouseCallback(GLFWwindow* window, double x, double y) {
	ImGuiIO& io = ImGui::GetIO();
	io.AddMousePosEvent(x, y);
	if (io.WantCaptureMouse) return; //make sure you do not call this callback when over a menu
	mouseX = x;
	mouseY = y;
	if (mouseLeft)  trackball.Rotate(mouseX, mouseY);
	if (mouseMid)   trackball.Translate(mouseX, mouseY);
	if (mouseRight) trackball.Zoom(mouseX, mouseY);
}

void MouseButtonCallback(GLFWwindow* window, int button, int state, int mods) {
	ImGuiIO& io = ImGui::GetIO();
	io.AddMouseButtonEvent(button, state);
	if (io.WantCaptureMouse) return; //make sure you do not call this callback when over a menu
	if (button == GLFW_MOUSE_BUTTON_LEFT && state == GLFW_PRESS)
	{
		trackball.Set(window, true, mouseX, mouseY);
		mouseLeft = true;
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && state == GLFW_RELEASE)
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

int main() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 800, "Simple", NULL, NULL);
	if (window == NULL) {
		std::cout << "Cannot open GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	gladLoadGL();
	glViewport(0, 0, 800, 800);

	BuildScene(VBO, VAO, steps, CreateCone);
	int shaderProg = CompileShaders();
	GLint modelviewParameter = glGetUniformLocation(shaderProg, "modelview");

	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUseProgram(shaderProg);
	glPointSize(pointSize);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	bool drawScene = true;
	bool fillShape = false;
	float color[4] = { 0.8f, 0.8f, 0.2f, 1.0f };
	glUniform4f(glGetUniformLocation(shaderProg, "color"), color[0], color[1], color[2], color[3]);

	glfwSetKeyCallback(window, KbdCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);

	int objectType = 0; // 0: Cone, 1: SandClock, 2: Cube, 3: Diamond, 4: Sphere , 5: Cylinder, 6: Pyramid

	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Object Selector");
		ImGui::Checkbox("Draw Scene", &drawScene);

		if (ImGui::Button("Save OBJ")) {
			SaveOBJ(&tri, filename);
		}

		ImGui::Checkbox("Fill Shape", &fillShape);

		if (ImGui::SliderInt("Mesh Subdivision", &steps, 1, 100, "%d", 0)) {
			switch (objectType) {
			case 0: BuildScene(VBO, VAO, steps, CreateCone); break;
			case 1: BuildScene(VBO, VAO, steps, CreateSandClock); break;
			case 2: BuildScene(VBO, VAO, steps, CreateCube); break;
			case 3: BuildScene(VBO, VAO, steps, CreateDiamond); break;
			case 4: BuildScene(VBO, VAO, steps, CreateSphere); break;
			case 5: BuildScene(VBO, VAO, steps, CreateCylinder); break;
			case 6: BuildScene(VBO, VAO, steps, CreatePyramid); break;
			}
		}
		if (ImGui::SliderInt("point Size", &pointSize, 1, 10, "%d", 0)) {
			glPointSize(pointSize);
		}
		if (ImGui::SliderInt("line width", &lineWidth, 1, 10, "%d", 0)) {
			glLineWidth(lineWidth);
		}
		if (ImGui::ColorEdit4("Color", color)) {
			glUniform4f(glGetUniformLocation(shaderProg, "color"), color[0], color[1], color[2], color[3]);
		}
		const char* items[] = { "Cone", "Sand Clock", "Cube", "Diamond", "Sphere", "Cylinder", "Pyramid" };
		if (ImGui::Combo("Object Type", &objectType, items, IM_ARRAYSIZE(items))) {
			switch (objectType) {
			case 0: BuildScene(VBO, VAO, steps, CreateCone); break;
			case 1: BuildScene(VBO, VAO, steps, CreateSandClock); break;
			case 2: BuildScene(VBO, VAO, steps, CreateCube); break;
			case 3: BuildScene(VBO, VAO, steps, CreateDiamond); break;
			case 4: BuildScene(VBO, VAO, steps, CreateSphere); break;
			case 5: BuildScene(VBO, VAO, steps, CreateCylinder); break;
			case 6: BuildScene(VBO, VAO, steps, CreatePyramid); break;
			}
		}

		ImGui::End();

		glm::mat4 proj = glm::perspective(65.f, 1.f, 0.01f, 1000.f);
		glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		glm::mat4 model = trackball.Set3DViewCameraMatrix();
		glm::mat4 modelView = proj * view * model;
		glUniformMatrix4fv(modelviewParameter, 1, GL_FALSE, glm::value_ptr(modelView));

		if (drawScene) {
			glDrawArrays(GL_POINTS, 0, points / 3);
			glDrawArrays(GL_TRIANGLES, 0, points / 3);

			glPolygonMode(GL_FRONT_AND_BACK, fillShape ? GL_FILL : GL_LINE);
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shaderProg);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
