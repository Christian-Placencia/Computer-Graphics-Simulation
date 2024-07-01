#include <iostream>
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "helper.h"


//Some simple rendering routines using old fixed-pipeline OpenGL
//draws line from a to b with color 
void DrawLine(glm::vec3 a, glm::vec3 b, glm::vec3 color) {

	glColor3f(color.r, color.g, color.b);
	glBegin(GL_LINES);
	glVertex3f(a.x,a.y,a.z);
	glVertex3f(b.x,b.y,b.z);
	glEnd();
}

//draws point at a with color 
void DrawPoint(glm::vec3 a, glm::vec3 color) {

	glColor3f(color.r, color.g, color.b);
	glPointSize(5);
	glBegin(GL_POINTS);
	glVertex3f(a.x, a.y, a.z);
	glEnd();
}



//display coordinate system
void CoordSyst() {
	glm::vec3 a, b, c;
	glm::vec3 origin(0, 0, 0);
	glm::vec3 red(1, 0, 0), green(0, 1, 0), blue(0, 0, 1), almostBlack(0.1f, 0.1f, 0.1f), yellow(1, 1, 0);

	//draw the coordinate system 
	a=glm::vec3(10, 0, 0);
	b=glm::vec3(0, 10, 0);
	c=glm::cross(a, b); //use cross product to find the last vector
	glLineWidth(4);
	DrawLine(origin, a, red);
	DrawLine(origin, b, green);
	DrawLine(origin, c, blue);
	DrawPoint(origin, almostBlack);
	DrawPoint(a, red);
	DrawPoint(b, green);
	DrawPoint(c, blue);
	glLineWidth(1);
}