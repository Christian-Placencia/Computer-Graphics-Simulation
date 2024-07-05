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
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <string>
#include <vector>			//Standard template library class
#include <GL/glew.h>
#include <GL/glut.h>
//glm
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/half_float.hpp>
//in house created libraries
#include "shaders.h"    
#include "shapes.h"    

#pragma warning(disable : 4996)
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "glut32.lib")

using namespace std;

bool needRedisplay=false;
GLfloat  sign=+1;
GLint stacks=5, slices=5;
ShapesC* cube;

//shader program ID
GLuint shaderProgram;
GLfloat ftime=0.f;
glm::mat4 view=glm::mat4(1.0);
glm::mat4 proj=glm::perspective(80.0f,//fovy
				  		         1.0f,//aspect
						        0.01f,1000.f); //near, far
GLint modelParameter;
GLint viewParameter;
GLint projParameter;

//the main window size
GLint wWindow=800;
GLint hWindow=800;

/*********************************
Some OpenGL-related functions
**********************************/

//called when a window is reshaped
void Reshape(int w, int h)
{
  glViewport(0,0,w, h);       
  glEnable(GL_DEPTH_TEST);
//remember the settings for the camera
  wWindow=w;
  hWindow=h;
}

void Arm(glm::mat4 m)
{
//let's use instancing
	m=glm::translate(m,glm::vec3(0,0.5,0.0));
	glm::mat4 tmp=m;
	cube->SetMVP(glm::scale(m,glm::vec3(0.1f,1.0f,0.1f)));
	cube->Render();

//optimized
	//const glm::mat4 e(1.0);
	//m=glm::translate(m,glm::vec3(0.0,0.5,0.0));
	//m=glm::rotate(m,-20.0f*ftime,glm::vec3(0.0,0.0,1.0));
	//const glm::mat4 hlp=glm::translate(e,glm::vec3(0.0,0.5,0.0))*
	//	                glm::scale    (e,glm::vec3(0.1f,1.0f,0.1f));
	//cube->SetMVP(m*hlp);


	m=glm::translate(m,glm::vec3(0.0,0.5,0.0));
	m=glm::rotate(m,-20.0f*ftime,glm::vec3(0.0,0.0,1.0));
	m=glm::translate(m,glm::vec3(0.0,0.5,0.0));
	cube->SetMVP(glm::scale(m,glm::vec3(0.1f,1.0f,0.1f)));
	cube->Render();
}

//the main rendering function
void RenderObjects()
{
	const int range=70;
	glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	glColor3f(0,0,0);
	glPointSize(2);
	glLineWidth(2);
	//set the projection and view once for the scene
	glUniformMatrix4fv(projParameter,1,GL_FALSE,glm::value_ptr(proj));
	view=glm::lookAt(glm::vec3(125*sin(ftime/40.f),3.f+3*sin(ftime / 80.f),25*cos(ftime/40.f)),//eye
				     glm::vec3(0,0,0),  //destination
				     glm::vec3(0,1,0)); //up

	glUniformMatrix4fv(viewParameter,1,GL_FALSE,glm::value_ptr(view));
	for (int i=-range;i<range;i++)
	{
		for (int j=-range;j<range;j++)
		{
			glm::mat4 m=glm::translate(glm::mat4(1.0),glm::vec3(2*i,0,2*j));
			Arm(m);
		}
	}
}
	
void Idle(void)
{
  glClearColor(0.1,0.1,0.1,1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  ftime+=0.1;
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
	switch(a)
	{
 	  case 27 : exit(0);break;
	  case 32 : {needRedisplay=!needRedisplay;break;}
	  case 'S': 
	  case 's': {sign=-sign;break;}
	  case 'a': {stacks++;break;}
	  case 'A': {
				  stacks--; 
				  if (stacks<2) stacks=2;
				  break;
				}
	  case 'b': {slices++;break;}
	  case 'B': {
				  slices--; 
				  if (slices<2) slices=2;
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
   	switch(a)
	{
 	  case GLUT_KEY_LEFT  : 
		  {
			  break;
		  }
	  case GLUT_KEY_RIGHT : 
		  {
			break;
		  }
 	  case GLUT_KEY_DOWN    : 
		  {
			break;
		  }
	  case GLUT_KEY_UP  :
		  {
			break;
		  }

	}
	glutPostRedisplay();
}

//called when a special key is released
void SpecKbdRelease(int a, int x, int y)
{
	switch(a)
	{
 	  case GLUT_KEY_LEFT  : 
		  {
			  break;
		  }
	  case GLUT_KEY_RIGHT : 
		  {
			  break;
		  }
 	  case GLUT_KEY_DOWN  : 
		  {
			break;
		  }
	  case GLUT_KEY_UP  :
		  {
			break;
		  }
	}
	glutPostRedisplay();
}


void Mouse(int button,int state,int x,int y)
{
	cout << "Location is " << "[" << x << "'" << y << "]" << endl;
}


GLuint InitializeProgram(GLuint *program)
{
	std::vector<GLuint> shaderList;

//load and compile shaders 	
	shaderList.push_back(CreateShader(GL_VERTEX_SHADER,   LoadShader("shaders/transform.vert")));
	shaderList.push_back(CreateShader(GL_FRAGMENT_SHADER, LoadShader("shaders/passthrough.frag")));

//create the shader program and attach the shaders to it
	*program = CreateProgram(shaderList);

//delete shaders (they are on the GPU now)
	std::for_each(shaderList.begin(), shaderList.end(), glDeleteShader);

	modelParameter=glGetUniformLocation(*program,"model");
	viewParameter =glGetUniformLocation(*program,"view");
	projParameter =glGetUniformLocation(*program,"proj");

	return modelParameter;
}

void InitShapes(GLuint modelParameter)
{
//create one unit cube in the origin
	cube=new CubeC();
	cube->SetMVP(glm::mat4(1.0));
	cube->SetMatrixParamToShader(modelParameter);
}

int main(int argc, char **argv)
{ 
  glutInitDisplayString("stencil>=2 rgb double depth samples");
  glutInit(&argc, argv);
  glutInitWindowSize(wWindow,hWindow);
  glutInitWindowPosition(500,100);
  glutCreateWindow("Model View Projection GLSL");
  GLenum err = glewInit();
  if (GLEW_OK != err){
   fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
  }
  glutDisplayFunc(Display);
  glutIdleFunc(Idle);
  glutMouseFunc(Mouse);
  glutReshapeFunc(Reshape);
  glutKeyboardFunc(Kbd); //+ and -
  glutSpecialUpFunc(SpecKbdRelease); //smooth motion
  glutSpecialFunc(SpecKbdPress);
  GLuint modelParameter=InitializeProgram(&shaderProgram);
  InitShapes(modelParameter);
  glutMainLoop();
  return 0;        
}
	