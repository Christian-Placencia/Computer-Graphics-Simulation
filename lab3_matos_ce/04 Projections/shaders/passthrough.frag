#version 330
out vec4 outputColor;
smooth in vec4 theColor;

void main()
{
//	outputColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);
	outputColor = theColor;
}
