#include <stdio.h>
#include <iostream>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <string>
#include <vector>
#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "shapes.h"

using namespace std;

//help procedure that send values from glm::vec3 to a STL vector of float
//used for creating VBOs
inline void AddVertex(vector <GLfloat> *a, const glm::vec3 *v)
{
	a->push_back(v->x);
	a->push_back(v->y);
	a->push_back(v->z);
}

void ShapesC::Render()
{
	cout << "Base class cannot render" << "\n";
}

void ShapesC::SetMVP(glm::mat4 tmp)
{
	model=tmp;
}

void ShapesC::SetMatrixParamToShader(GLuint uniform)
{
  modelParameter=uniform;
}

void ShapesC::SetColor(GLubyte r,GLubyte b,GLubyte g)
{
	color[0]=r;
	color[1]=g;
	color[2]=b;
}

void SphereC::Render()
{
	glBindVertexArray(vaID);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glEnableVertexAttribArray(0);
    glUniformMatrix4fv(modelParameter,1,GL_FALSE,glm::value_ptr(model));
	glDrawArrays(GL_TRIANGLES, 0, 3*points);
}

void SphereC::Generate(int stacks, int slices, GLfloat r)
{
	glm::vec3 v;

	GLfloat deltaTheta=2*M_PI/(GLfloat)slices;
	GLfloat deltaPhi  =  M_PI/(GLfloat)stacks;

	for (GLint i=0;i<stacks;i++)
	{
		GLfloat phi=i*deltaPhi;
		for (GLint j=0;j<slices;j++)
		{
			GLfloat theta=j*deltaTheta;
//the first triangle
			v=glm::vec3(r*cos(theta)*sin(phi),
				        r*sin(theta)*sin(phi),
				        r*cos(phi));
			AddVertex(&data,&v);
			v=glm::vec3(r*cos(theta+deltaTheta)*sin(phi),
				        r*sin(theta+deltaTheta)*sin(phi),
                        r*cos(phi));
			AddVertex(&data,&v);
			v=glm::vec3(r*cos(theta)*sin(phi+deltaPhi),
				        r*sin(theta)*sin(phi+deltaPhi),
				        r*cos(phi+deltaPhi));
			AddVertex(&data,&v);
//the second triangle
			v=glm::vec3(r*cos(theta+deltaTheta)*sin(phi),
				             r*sin(theta+deltaTheta)*sin(phi),
				             r*cos(phi));
			AddVertex(&data,&v);
			v=glm::vec3(r*cos(theta)*sin(phi+deltaPhi),
				        r*sin(theta)*sin(phi+deltaPhi),
				        r*cos(phi+deltaPhi));
			AddVertex(&data,&v);
			v=glm::vec3(r*cos(theta+deltaTheta)*sin(phi+deltaPhi),
				        r*sin(theta+deltaTheta)*sin(phi+deltaPhi),
				        r*cos(phi+deltaPhi));
			AddVertex(&data,&v);
		}
	}
}

void SphereC::InitArrays()
{
	glGenVertexArrays(1,&vaID);
	glBindVertexArray(vaID);
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	points=data.size();
	glBufferData(GL_ARRAY_BUFFER, points*sizeof(GLfloat), &data[0], GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0); 
	glEnableVertexAttribArray(0);
	data.clear(); //no need for the data, it is on the GPU now
}

SphereC::SphereC()
{
	Generate(10,10,1.f);	
	InitArrays();
}

SphereC::SphereC(int stacks, int slices, GLfloat r)
{
	this->stacks=stacks;
	this->slices=slices;
	this->r=r;
	Generate(stacks,slices,r);	
	InitArrays();
}

void CubeC::InitArrays()
{
	glGenVertexArrays(1,&vaID);
	glBindVertexArray(vaID);
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	points=data.size();
	glBufferData(GL_ARRAY_BUFFER, points*sizeof(GLfloat), &data[0], GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0); 
	glEnableVertexAttribArray(0);
	data.clear(); //no need for the data, it is on the GPU now
}


CubeC::CubeC()
{
	Generate();	
	InitArrays();
}

void CubeC::Generate()
{
	const glm::vec3 A=glm::vec3(-0.5f,-0.5f,-0.5f);
	const glm::vec3 B=glm::vec3(+0.5f,-0.5f,-0.5f);
	const glm::vec3 C=glm::vec3(+0.5f,+0.5f,-0.5f);
	const glm::vec3 D=glm::vec3(-0.5f,+0.5f,-0.5f);
	const glm::vec3 E=glm::vec3(-0.5f,-0.5f,+0.5f);
	const glm::vec3 F=glm::vec3(+0.5f,-0.5f,+0.5f);
	const glm::vec3 G=glm::vec3(+0.5f,+0.5f,+0.5f);
	const glm::vec3 H=glm::vec3(-0.5f,+0.5f,+0.5f);
	AddVertex(&data,&A);AddVertex(&data,&B);AddVertex(&data,&C);
	AddVertex(&data,&A);AddVertex(&data,&C);AddVertex(&data,&D);
	AddVertex(&data,&A);AddVertex(&data,&E);AddVertex(&data,&F);
	AddVertex(&data,&A);AddVertex(&data,&F);AddVertex(&data,&B);
	AddVertex(&data,&B);AddVertex(&data,&F);AddVertex(&data,&G);
	AddVertex(&data,&B);AddVertex(&data,&G);AddVertex(&data,&C);
	AddVertex(&data,&C);AddVertex(&data,&G);AddVertex(&data,&H);
	AddVertex(&data,&C);AddVertex(&data,&H);AddVertex(&data,&D);
	AddVertex(&data,&D);AddVertex(&data,&H);AddVertex(&data,&E);
	AddVertex(&data,&D);AddVertex(&data,&E);AddVertex(&data,&A);
	AddVertex(&data,&E);AddVertex(&data,&F);AddVertex(&data,&G);
	AddVertex(&data,&E);AddVertex(&data,&G);AddVertex(&data,&H);
}


void CubeC::Render()
{
	glBindVertexArray(vaID);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glEnableVertexAttribArray(0);
    glUniformMatrix4fv(modelParameter,1,GL_FALSE,glm::value_ptr(model));
	glDrawArrays(GL_TRIANGLES, 0, 3*points);
}

