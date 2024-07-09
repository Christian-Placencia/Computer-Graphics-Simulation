#version 410
/*	(C) 2022
	Bedrich Benes 
	Purdue University
	bbenes@purdue.edu
*/	


const vec4 lD=vec4(1.0,1.0,1.0,1.0);
const vec4 kd=vec4(1.0,1.0,1.0,1.0);

struct LightS
{
 vec4 lPos; //position of the light
 vec3 la;       //ambient, diffuse, specular
 vec3 ld;
 vec3 ls;
};

struct MaterialS
{
 vec3 ka;       //ambient, diffuse, specular
 vec3 kd;
 vec3 ks;
 float sh;
};


vec4 Lighting(vec3 n, vec4 pos, LightS light, MaterialS mat)
{
  vec3 l=normalize(vec3(light.lPos-pos));
  vec3 v=normalize(-pos.xyz); //camera is at [0,0,0] so the direction to the viewing vector is this
  vec3 r=reflect(l, n);
  float sDot=max(dot(l,n),0.0);
  vec3 diffuse=light.ld*mat.kd*sDot;
  vec3 specular=vec3(0.0);
  if (sDot>0)
  {
    specular=light.ls*mat.ks*pow(max(dot(r,v),0.0),mat.sh);
  }
  vec3 ambient=mat.ka*light.la;
  return vec4(diffuse+specular+ambient,1);
}


layout (location=0) in vec4 iPosCamera;
layout (location=1) in vec3 iNormCamera;
layout (location=2) in vec2 uv;
layout (location=0) out vec4 oColor;
uniform MaterialS mat;
uniform LightS light;
uniform int texAs;


//out vec4 oColor;			//output color
uniform sampler2D tex;		//input texture

void main()
{
    vec4 color=texture(tex, uv);	//get the texure color
	switch (texAs){
		case 0: oColor = Lighting(normalize(iNormCamera),iPosCamera,light,mat);break;
		case 1: oColor=color;break;
		case 2: {
				MaterialS m;	//we canot modify uniform parameters so a copy is required
				m=mat;
				m.kd=vec3(color); //set diffuse from texture
				oColor = Lighting(normalize(iNormCamera),iPosCamera,light,m);
				break;
			}
		case 3: {
				MaterialS m;
				m=mat;
				m.ks=vec3(color); //set specular from texture
				oColor = Lighting(normalize(iNormCamera),iPosCamera,light,m);
				break;
			}
		case 4: {
				MaterialS m;
				m=mat;
				m.ka=vec3(color); //set ambient from texture
				oColor = Lighting(normalize(iNormCamera),iPosCamera,light,m);
				break;
			}
	}

}