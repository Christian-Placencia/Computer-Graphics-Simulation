#ifndef __TRIANGLE_H__
#define __TRIANGLE_H__

#include <memory.h>
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"


class TriangleC
{
public:
	//constructors
	TriangleC(void);

	TriangleC(glm::vec3 aa, glm::vec3 bb, glm::vec3 cc)
	{
		a = aa; b = bb; c = cc;
	};

	TriangleC(const TriangleC *newv);

	TriangleC(const TriangleC &newv);

	//destructor
	~TriangleC() {}	//empty

	inline void Set(glm::vec3 aa, glm::vec3 bb, glm::vec3 cc){
		a=aa;
		b=bb;
		c=cc;
	}

/*
	//Accessors kept for compatibility
	inline void SetX(float x) {v[0]=x;}
	inline void SetY(float y) {v[1]=y;}
	inline void SetZ(float z) {v[2]=z;}

	float GetX() const {return v[0];}	//public accessor functions
	float GetY() const {return v[1];}	//inline, const
	float GetZ() const {return v[2];}

	float x() const {return v[0];}	//public accessor functions
	float y() const {return v[1];}	//inline, const
	float z() const {return v[2];}


	void Zero(void);

	void One(void);
*/
//protected:
	//member variables
	glm::vec3 a,b,c;
};

#endif	//TriangleC_H