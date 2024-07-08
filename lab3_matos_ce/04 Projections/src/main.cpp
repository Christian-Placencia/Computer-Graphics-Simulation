/**********************************/
/* Simple transformations	      */
/* (C) Bedrich Benes 2021         */
/* bbenes@purdue.edu              */
/**********************************/
#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <string.h>
#include <iostream>
#include <ctime>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <string>
#include <vector>
#include <GL/glew.h>

#define _LIB
#define FREEGLUT_LIB_PRAGMAS 0

#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/half_float.hpp>
#include "shaders.h"
#include "shapes.h"

// Graphical interface
// #include "imgui.h"
#include "imgui_impl_glut.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include "imgui.h"


#pragma warning(disable : 4996)
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "glut32.lib")

using namespace std;

bool needRedisplay = false;
bool rotationEnabled = true;
GLfloat sign = +1;
GLint stacks = 5, slices = 5;
ShapesC* cube;
ShapesC* sphere;

//shader program ID
GLuint shaderProgram;
GLfloat ftime = 0.f;
glm::mat4 view = glm::mat4(1.0);
glm::mat4 proj = glm::perspective(80.0f, //fovy
    1.0f, //aspect
    0.01f, 1000.f); //near, far
GLint modelParameter;
GLint viewParameter;
GLint projParameter;

//the main window size
GLint wWindow = 800;
GLint hWindow = 800;

//--------------------------------

class Moon {
public:
	float orbitRadius;
	glm::vec3 scale;
	float rotationSpeed;
	int direction;

	Moon(float orbitRadius, glm::vec3 scale, float rotationSpeed, int direction) {
		this->orbitRadius = orbitRadius;
		this->scale = scale;
		this->rotationSpeed = rotationSpeed;
		this->direction = direction;
	}

    // Current position getter
    glm::vec3 getPosition() {
        // Rotate aroun the planet
		glm::mat4 m = glm::mat4(1.0);
        m = glm::rotate(m, glm::radians(ftime * rotationSpeed * direction), glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::translate(m, glm::vec3(orbitRadius, 0.0f, 0.0f));
        glm::vec3 position = glm::vec3(m[3]);
		return position;
	}

};

class Planet {
public:
	glm::vec3 position;
	glm::vec3 scale;
	float rotationSpeed;
	std::vector<Moon> moons;

	Planet(glm::vec3 position, glm::vec3 scale, float rotationSpeed, std::vector<Moon> moons) {
		this->position = position;
		this->scale = scale;
		this->rotationSpeed = rotationSpeed;
		this->moons = moons;
	}

	// Current position getter
    glm::vec3 getPosition() {
		glm::mat4 m = glm::mat4(1.0);
		m = glm::rotate(m, ftime * rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
		m = glm::translate(m, position);
		glm::vec3 position = glm::vec3(m[3]);
		return position;
    }
};

// Define the planets and moons
vector<Planet> planets;

// Camera settings
glm::vec3 cameraOffset = glm::vec3(0.0f, 10.0f, 10.0f);

// Define the places for the camera
enum CameraMode { SUN, PLANET1, PLANET2, PLANET3, MOON1_1, MOON1_2, MOON1_3, MOON2_1, MOON2_2, MOON3_1, MOON3_2, MOON3_3, MOON3_4 };
int currentCameraMode = 0; // SUN, PLANET1, PLANET2, PLANET3, MOON1_1, MOON1_2, MOON1_3, MOON2_1, MOON2_2, MOON3_1, MOON3_2, MOON3_3, MOON3_4

/*********************************
Some OpenGL-related functions
**********************************/

//called when a window is reshaped
void Reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glEnable(GL_DEPTH_TEST);
    //remember the settings for the camera
    wWindow = w;
    hWindow = h;
}

void DrawObject(glm::mat4 m, ShapesC* object, glm::vec3 scale, glm::vec3 rotation_axis, float rotation_angle)
{
    m = glm::rotate(m, glm::radians(rotation_angle), rotation_axis);
    m = glm::scale(m, scale);
    object->SetMVP(m);
    object->Render();
}

// Take a planet and its moons and draw them.
void DrawPlanetAndMoons(glm::mat4 m, Planet& planet)
{
    // Rotate around the sun
	m = glm::rotate(m, ftime * planet.rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));

	// Translate to planet's orbit distance
	m = glm::translate(m, planet.position);

	// Draw planet
	DrawObject(m, sphere, planet.scale, glm::vec3(0.0f, 1.0f, 0.0f), ftime * 50.0f);

	// Draw moons
	for (Moon& moon : planet.moons) {
		glm::mat4 moon_m = m; // Create a copy of the planet's matrix
		moon_m = glm::rotate(moon_m, glm::radians(ftime * moon.rotationSpeed * moon.direction), glm::vec3(0.0f, 1.0f, 0.0f));
		moon_m = glm::translate(moon_m, glm::vec3(moon.orbitRadius, 0.0f, 0.0f));
		DrawObject(moon_m, sphere, moon.scale, glm::vec3(0.0f, 1.0f, 0.0f), ftime * moon.rotationSpeed * moon.direction);
	}
}



// The main rendering function
void RenderObjects()
{
	// Set the background color
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor3f(0, 0, 0);
    glPointSize(2);
    glLineWidth(2);

    // Set the projection and view once for the scene
    glUniformMatrix4fv(projParameter, 1, GL_FALSE, glm::value_ptr(proj));
    glm::vec3 eye;
    glm::vec3 target;

	// Set the camera position based on the current camera mode
    switch (currentCameraMode)
    {
    case 0:
        target = glm::vec3(0.0f, 0.0f, 0.0f);
        break;
    case 1:
        target = planets[0].getPosition();
        break;
    case 2:
        target = planets[1].getPosition();
        break;
    case 3:
        target = planets[2].getPosition();
        break;
    case 4:
        target = planets[0].moons[0].getPosition() + planets[0].getPosition();;
        break;
    case 5:
        target = planets[0].moons[1].getPosition() + planets[0].getPosition();;
        break;
    case 6:
        target = planets[0].moons[2].getPosition() + planets[0].getPosition();;
        break;
    case 7:
        target = planets[1].moons[0].getPosition() + planets[1].getPosition();;
        break;
    case 8:
        target = planets[1].moons[1].getPosition() + planets[1].getPosition();;
        break;
    case 9:
        target = planets[2].moons[0].getPosition() + planets[2].getPosition();;
        break;
    case 10:
        target = planets[2].moons[1].getPosition() + planets[2].getPosition();;
        break;
    case 11:
        target = planets[2].moons[2].getPosition() + planets[2].getPosition();;
        break;
    case 12:
        target = planets[2].moons[3].getPosition() + planets[2].getPosition();;
        break;
    }

    eye = target + cameraOffset;

	// Set the view matrix
    view = glm::lookAt(eye, target, glm::vec3(0, 1, 0));
    glUniformMatrix4fv(viewParameter, 1, GL_FALSE, glm::value_ptr(view));

    // Clear the buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set the model matrix to the identity matrix
    glm::mat4 m = glm::mat4(1.0);

    // Draw Sun
    DrawObject(m, sphere, glm::vec3(2.0f), glm::vec3(0.0f, 1.0f, 0.0f), ftime * 10.0f);

    // Draw planets with moons
	for (Planet& planet : planets) {
		DrawPlanetAndMoons(m, planet);
	}

    // Flush the graphis pipeline
    glFlush();
}

void Idle(void)
{
    if (rotationEnabled) {
        ftime += 0.01; // Decreased the speed
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();

    // Create ImGui window
    ImGui::Begin("Controls");
    ImGui::Text("Camera Offset");
    ImGui::SliderFloat("X", &cameraOffset[0], -50.0f, 50.0f);
    ImGui::SliderFloat("Y", &cameraOffset[1], -50.0f, 50.0f);
    ImGui::SliderFloat("Z", &cameraOffset[2], -50.0f, 50.0f);
	ImGui::Text("Camera Mode");
    const char* planets[] = { 
        "Sun", 
        "Planet 1", 
        "Planet 2", 
        "Planet 3", 
        "Moon 1-1", 
        "Moon 1-2", 
        "Moon 1-3", 
        "Moon 2-1",
        "Moon 2-2",
        "Moon 3-1",
		"Moon 3-2",
        "Moon 3-3",
        "Moon 3-4"};
    if (ImGui::Combo("Planet", &currentCameraMode, planets, IM_ARRAYSIZE(planets))) {
        switch (currentCameraMode) {
        case 0: { currentCameraMode = SUN; break; }
        case 1: { currentCameraMode = PLANET1; break; }
        case 2: { currentCameraMode = PLANET2; break; }
        case 3: { currentCameraMode = PLANET3; break; }
        case 4: { currentCameraMode = MOON1_1; break; }
        case 5: { currentCameraMode = MOON1_2; break; }
        case 6: { currentCameraMode = MOON1_3; break; }
        case 7: { currentCameraMode = MOON2_1; break; }
        case 8: { currentCameraMode = MOON2_2; break; }
        case 9: { currentCameraMode = MOON3_1; break; }
        case 10: { currentCameraMode = MOON3_2; break; }
        case 11: { currentCameraMode = MOON3_3; break; }
        case 12: { currentCameraMode = MOON3_4; break; }
        }
    }
    ImGui::End();

    ImGui::Render();

    // Clear the screen
    glClearColor(0.1, 0.1, 0.1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

    RenderObjects();

    // Render ImGui
    
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glutSwapBuffers();
}

//keyboard callback
void Kbd(unsigned char a, int x, int y)
{
    switch (a)
    {
    case 27: exit(0); break;
    case 32: { needRedisplay = !needRedisplay; break; }
    case 'S':
    case 's': { sign = -sign; break; }
    case 'a': { currentCameraMode = SUN; break; }
    case 'b': { currentCameraMode = PLANET1; break; }
    case 'c': { currentCameraMode = PLANET2; break; }
    case 'd': { currentCameraMode = PLANET3; break; }
    case 'e': { currentCameraMode = MOON1_1; break; }
    case 'f': { currentCameraMode = MOON1_2; break; }
    case 'g': { currentCameraMode = MOON1_3; break; }
	case 'h': { currentCameraMode = MOON2_1; break; }
	case 'i': { currentCameraMode = MOON2_2; break; }
	case 'j': { currentCameraMode = MOON3_1; break; }
	case 'k': { currentCameraMode = MOON3_2; break; }
	case 'l': { currentCameraMode = MOON3_3; break; }
	case 'm': { currentCameraMode = MOON3_4; break; }
    
    // Move offset with keys
	case 37: { cameraOffset[0] -= 1.0f; break; } // Left
	case 38: { cameraOffset[2] += 1.0f; break; } // Up
	case 39: { cameraOffset[0] += 1.0f; break; } // Right
	case 40: { cameraOffset[2] -= 1.0f; break; } // Down

    case '1': { stacks++; break; }
    case '2': {
        stacks--;
        if (stacks < 2) stacks = 2;
        break;
    }
    case '3': { slices++; break; }
    case '4': {
        slices--;
        if (slices < 2) slices = 2;
        break;
    }
    case '+': break;
    case '-': break;
    }
    glutPostRedisplay();
}

//special keyboard callback
void SpecKbdPress(int a, int x, int y)
{
    switch (a)
    {
    case GLUT_KEY_LEFT: { 
		cameraOffset[0] -= 10.0f;
        break; 
    }
    case GLUT_KEY_RIGHT: { 
		cameraOffset[0] += 10.0f;
        break; 
    }
    case GLUT_KEY_DOWN: { 
		cameraOffset[2] -= 10.0f;
        break; 
    }
    case GLUT_KEY_UP: { 
		cameraOffset[2] += 10.0f;
        break; 
    }
    }
    glutPostRedisplay();
}

//called when a special key is released
void SpecKbdRelease(int a, int x, int y)
{
    switch (a)
    {
    case GLUT_KEY_LEFT: { break; }
    case GLUT_KEY_RIGHT: { break; }
    case GLUT_KEY_DOWN: { break; }
    case GLUT_KEY_UP: { break; }
    }
    glutPostRedisplay();
}

void Mouse(int button, int state, int x, int y)
{
    cout << "Location is " << "[" << x << "'" << y << "]" << endl;
}

GLuint InitializeProgram(GLuint* program)
{
    std::vector<GLuint> shaderList;

    //load and compile shaders
    shaderList.push_back(CreateShader(GL_VERTEX_SHADER, LoadShader("shaders/transform.vert")));
    shaderList.push_back(CreateShader(GL_FRAGMENT_SHADER, LoadShader("shaders/passthrough.frag")));

    //create the shader program and attach the shaders to it
    *program = CreateProgram(shaderList);

    //delete shaders (they are on the GPU now)
    std::for_each(shaderList.begin(), shaderList.end(), glDeleteShader);

    modelParameter = glGetUniformLocation(*program, "model");
    viewParameter = glGetUniformLocation(*program, "view");
    projParameter = glGetUniformLocation(*program, "proj");

    return modelParameter;
}

// Set up the planets and moons
void InitializePlanets()
{
    // Planet 1
    vector<Moon> moons1;
    moons1.push_back(Moon(2.0f, glm::vec3(0.1f), 5000.0f, 1));
    moons1.push_back(Moon(3.0f, glm::vec3(0.15f), 4000.0f, -1));
    moons1.push_back(Moon(4.0f, glm::vec3(0.2f), 30000.0f, 1));
    planets.push_back(Planet(glm::vec3(10.0f, 0.0f, 0.0f), glm::vec3(0.5f), 5.0f, moons1));

    // Planet 2
    vector<Moon> moons2;
    moons2.push_back(Moon(3.0f, glm::vec3(0.15f), 60.0f, -1));
    moons2.push_back(Moon(4.5f, glm::vec3(0.2f), 45.0f, 1));
    planets.push_back(Planet(glm::vec3(20.0f, 0.0f, 0.0f), glm::vec3(0.7f), 15.0f, moons2));

    // Planet 3
    vector<Moon> moons3;
    moons3.push_back(Moon(3.5f, glm::vec3(0.1f), 40.0f, 1));
    moons3.push_back(Moon(5.0f, glm::vec3(0.15f), 35.0f, -1));
    moons3.push_back(Moon(6.5f, glm::vec3(0.2f), 30.0f, 1));
    moons3.push_back(Moon(8.0f, glm::vec3(0.25f), 25.0f, -1));
    planets.push_back(Planet(glm::vec3(30.0f, 0.0f, 0.0f), glm::vec3(0.9f), 2.5f, moons3));
}

void InitShapes(GLuint modelParameter)
{
    //create one unit cube in the origin
    cube = new CubeC();
    cube->SetMVP(glm::mat4(1.0));
    cube->SetMatrixParamToShader(modelParameter);

    //create one unit sphere in the origin
    sphere = new SphereC();
    sphere->SetMVP(glm::mat4(1.0));
    sphere->SetMatrixParamToShader(modelParameter);
}

int main(int argc, char** argv)
{
	InitializePlanets();

    glutInitDisplayString("stencil>=2 rgb double depth samples");
    glutInit(&argc, argv);
    glutInitWindowSize(wWindow, hWindow);
    glutInitWindowPosition(500, 100);
    glutCreateWindow("Model View Projection GLSL");
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }

    // Initialize ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplGLUT_Init();
    ImGui_ImplOpenGL3_Init();

    glutIdleFunc(Idle);
    glutMouseFunc(Mouse);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Kbd); //+ and -
    glutSpecialUpFunc(SpecKbdRelease); //smooth motion
    glutSpecialFunc(SpecKbdPress);

    GLuint modelParameter = InitializeProgram(&shaderProgram);
    InitShapes(modelParameter);
    glutMainLoop();

    //// Cleanup ImGui
    //ImGui_ImplOpenGL2_Shutdown();
    //ImGui_ImplGLUT_Shutdown();
    //ImGui::DestroyContext();


    return 0;
}
