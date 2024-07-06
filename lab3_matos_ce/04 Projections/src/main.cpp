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
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/half_float.hpp>
#include "shaders.h"
#include "shapes.h"

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

int planet1Moons = 3;
int planet2Moons = 2;
int planet3Moons = 4;

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

void DrawPlanetAndMoons(glm::mat4 m, int numMoons, glm::vec3 planetPosition, glm::vec3 planetScale, float planetRotationSpeed, float* moonOrbitRadii, glm::vec3* moonScales, float* moonRotationSpeeds, int* moonDirections)
{
    // Rotate around the sun

    // Draw planet
    DrawObject(m, sphere, planetScale, glm::vec3(0.0f, 1.0f, 0.0f), ftime * 50.0f);

    // Draw moons
    for (int i = 0; i < numMoons; i++) {
        glm::mat4 moon_m = m; // Create a copy of the planet's matrix
        moon_m = glm::rotate(moon_m, ftime * moonRotationSpeeds[i] * moonDirections[i], glm::vec3(0.0f, 1.0f, 0.0f));
        
        DrawObject(moon_m, sphere, moonScales[i], glm::vec3(0.0f, 1.0f, 0.0f), ftime * moonRotationSpeeds[i] * moonDirections[i]);
    }
}

//the main rendering function
void RenderObjects()
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor3f(0, 0, 0);
    glPointSize(2);
    glLineWidth(2);
    //set the projection and view once for the scene
    glUniformMatrix4fv(projParameter, 1, GL_FALSE, glm::value_ptr(proj));
    view = glm::lookAt(glm::vec3(125 * sin(ftime / 40.f), 3.f + 3 * sin(ftime / 80.f), 25 * cos(ftime / 40.f)), //eye
        glm::vec3(0, 0, 0),  //destination
        glm::vec3(0, 1, 0)); //up

    glUniformMatrix4fv(viewParameter, 1, GL_FALSE, glm::value_ptr(view));

    glm::mat4 m = glm::mat4(1.0);

    // Draw Sun
    DrawObject(m, sphere, glm::vec3(2.0f), glm::vec3(0.0f, 1.0f, 0.0f), ftime * 10.0f);

    // Draw planets and moons

    // Parameters for planet 1
    float planet1MoonOrbitRadii[3] = { 2.0f, 3.0f, 4.0f };
    glm::vec3 planet1MoonScales[3] = { glm::vec3(0.1f), glm::vec3(0.15f), glm::vec3(0.2f) };
    float planet1MoonRotationSpeeds[3] = { 50.0f, 40.0f, 30.0f };
    int planet1MoonDirections[3] = { 1, -1, 1 };

    // Parameters for planet 2
    float planet2MoonOrbitRadii[2] = { 3.0f, 4.5f };
    glm::vec3 planet2MoonScales[2] = { glm::vec3(0.15f), glm::vec3(0.2f) };
    float planet2MoonRotationSpeeds[2] = { 60.0f, 45.0f };
    int planet2MoonDirections[2] = { -1, 1 };

    // Parameters for planet 3
    float planet3MoonOrbitRadii[4] = { 3.5f, 5.0f, 6.5f, 8.0f };
    glm::vec3 planet3MoonScales[4] = { glm::vec3(0.1f), glm::vec3(0.15f), glm::vec3(0.2f), glm::vec3(0.25f) };
    float planet3MoonRotationSpeeds[4] = { 40.0f, 35.0f, 30.0f, 25.0f };
    int planet3MoonDirections[4] = { 1, -1, 1, -1 };

    // Draw planets with moons
    DrawPlanetAndMoons(m, planet1Moons, glm::vec3(10.0f, 0.0f, 0.0f), glm::vec3(0.5f), 5.0f, planet1MoonOrbitRadii, planet1MoonScales, planet1MoonRotationSpeeds, planet1MoonDirections);
    DrawPlanetAndMoons(m, planet2Moons, glm::vec3(20.0f, 0.0f, 0.0f), glm::vec3(0.7f), 15.0f, planet2MoonOrbitRadii, planet2MoonScales, planet2MoonRotationSpeeds, planet2MoonDirections);
    DrawPlanetAndMoons(m, planet3Moons, glm::vec3(30.0f, 0.0f, 0.0f), glm::vec3(0.9f), 2.5f, planet3MoonOrbitRadii, planet3MoonScales, planet3MoonRotationSpeeds, planet3MoonDirections);
}

void Idle(void)
{
    if (rotationEnabled) {
        ftime += 0.01; // Decreased the speed
    }

    glClearColor(0.1, 0.1, 0.1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);
    RenderObjects();
    glutSwapBuffers();
}

void Display(void)
{

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
    case 'a': { stacks++; break; }
    case 'A': {
        stacks--;
        if (stacks < 2) stacks = 2;
        break;
    }
    case 'b': { slices++; break; }
    case 'B': {
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
    case GLUT_KEY_LEFT:
    {
        break;
    }
    case GLUT_KEY_RIGHT:
    {
        break;
    }
    case GLUT_KEY_DOWN:
    {
        break;
    }
    case GLUT_KEY_UP:
    {
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
    case GLUT_KEY_LEFT:
    {
        break;
    }
    case GLUT_KEY_RIGHT:
    {
        break;
    }
    case GLUT_KEY_DOWN:
    {
        break;
    }
    case GLUT_KEY_UP:
    {
        break;
    }
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
    glutInitDisplayString("stencil>=2 rgb double depth samples");
    glutInit(&argc, argv);
    glutInitWindowSize(wWindow, hWindow);
    glutInitWindowPosition(500, 100);
    glutCreateWindow("Model View Projection GLSL");
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }
    glutDisplayFunc(Display);
    glutIdleFunc(Idle);
    glutMouseFunc(Mouse);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Kbd); //+ and -
    glutSpecialUpFunc(SpecKbdRelease); //smooth motion
    glutSpecialFunc(SpecKbdPress);

    GLuint modelParameter = InitializeProgram(&shaderProgram);
    InitShapes(modelParameter);
    glutMainLoop();
    return 0;
}
