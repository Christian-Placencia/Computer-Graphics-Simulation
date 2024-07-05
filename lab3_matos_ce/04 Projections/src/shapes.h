#include <stdio.h>
#include <iostream>
#include <string.h>
#include <math.h>
#include <string>
#include <vector>
#include <GL/glut.h>
#include <glm/glm.hpp>


using namespace std;

//Base class for shapes
class ShapesC
{
public:
	virtual void SetMatrixParamToShader(GLuint uniform);
	virtual void SetMVP(glm::mat4 tmp);
	virtual void SetColor(GLubyte r,GLubyte b,GLubyte g);
	virtual void Render();

protected:
  GLuint	 modelParameter;	 //shader uniform variable
  glm::mat4  model;				 //modeling matrix
  GLubyte    color[3];
};

//derived class from ShapesC
class SphereC: public ShapesC
{
public:
	SphereC();
	SphereC(int stacks, int slices, GLfloat r);
	virtual void Render();		

private:
	vector <GLfloat> *a;
	vector <GLfloat> data;
	GLuint vaID;
	GLuint buffer;
	GLuint points;
	GLuint stacks, slices;
	GLfloat r;
	void SphereC::Generate(int stacks, int slices, GLfloat r);
	void SphereC::InitArrays();
};

//derived class from ShapesC
class CubeC: public ShapesC
{
public:
	CubeC();
	virtual void Render();		
private:
	vector <GLfloat> *a;
	vector <GLfloat> data;
	GLuint vaID;
	GLuint buffer;
	GLuint points;
	void Generate();
	void InitArrays();
};
