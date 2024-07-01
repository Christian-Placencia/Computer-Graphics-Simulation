#ifndef __helper_h__
#define __helper_h__

#include "helper.h"


/*********************************
Some OpenGL-related functions DO NOT TOUCH
**********************************/
//displays the text message in the GL window
void GLMessage(char *message);

//called when a window is reshaped
void Reshape(int w, int h);

//Some simple rendering routines using old fixed-pipeline OpenGL
//draws line from a to b with color 
void DrawLine(glm::vec3 a, glm::vec3 b, glm::vec3 color);

//draws point at a with color 
void DrawPoint(glm::vec3 a, glm::vec3 color);

//OpenGL freeglut stuff
void Idle(void);

//OpenGL freeglut stuff
void Display(void);


//display coordinate system
void CoordSyst();


#endif