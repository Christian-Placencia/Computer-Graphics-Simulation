#version 410
/*	(C) 2022
	Bedrich Benes 
	Purdue University
	bbenes@purdue.edu
*/	


layout (location=0) in vec4 iPosition;  //input position
layout (location=1) in vec3 iNormal;	//normal vector
layout (location=2) in vec2 iuv;		//texture coordinates 
layout (location=0) out vec4 oPosCam;
layout (location=1) out vec3 oNormalCam;
layout (location=2) out vec2 ouv;
uniform mat4 model;	//three matrices for projections (4x4)
uniform mat4 view;
uniform mat4 proj;
uniform mat3 modelViewN; //modeview for normals (3x3)


void main()					
{
	ouv=iuv; //texture coordinates from the input are passed on the output
	oNormalCam=normalize(modelViewN*iNormal);	  //n to camera coords
	oPosCam=view*model*iPosition;				  //v to camera coords	
	gl_Position = proj*view*model*iPosition;      //standard vertex out	          
}
