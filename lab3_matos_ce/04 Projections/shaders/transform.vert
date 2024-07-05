#version 400
/*	(C) 2012 
	Bedrich Benes 
	Purdue University
	bbenes<#!&^#>purdue<.>edu
*/	


layout (location=0) in vec4 iPosition;
out vec4 theColor;
uniform mat4 model;
uniform mat4 view;
uniform  mat4 proj;
void main()					
{
	gl_Position=proj*view*model*iPosition;	
	theColor=vec4(1,1,0,1);			//set the output color
}
