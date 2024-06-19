//Based on http://www.glfw.org and on https://github.com/VictorGordan/opengl-tutorials 
/*
(C) Bedrich Benes 2022
Purdue University
bbenes@purdue.edu
*/

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


#include <iostream>
#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"


//Vertex array object and vertex buffer object indices 
GLuint VAO, VBO;


int CompileShaders() {
	//Vertex Shader
	const char* vsSrc= "#version 330 core\n"
		"layout (location = 0) in vec3 aPos;\n"
		"uniform float scale;\n"
		"void main()\n"
		"{\n"
		"   gl_Position = vec4(scale* aPos.x, scale* aPos.y, scale* aPos.z, 1.0);\n"
		"}\0";

	//Fragment Shader
	const char* fsSrc = "#version 330 core\n"
		"out vec4 col;\n"
		"uniform vec4 color;\n"
		"void main()\n"
		"{\n"
		"   col = color;\n"
		"}\n\0";

	//Create VS object
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	//Attach VS src to the Vertex Shader Object
	glShaderSource(vs, 1, &vsSrc, NULL);
	//Compile the vs
	glCompileShader(vs);

	//The same for FS
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fsSrc, NULL);
	glCompileShader(fs);

	//Get shader program object
	GLuint shaderProg = glCreateProgram();
	//Attach both vs and fs
	glAttachShader(shaderProg, vs);
	glAttachShader(shaderProg, fs);
	//Link all
	glLinkProgram(shaderProg);

	//Clear the VS and FS objects
	glDeleteShader(vs);
	glDeleteShader(fs);
	return shaderProg;
}

void BuildScene(GLuint& VBO, GLuint& VAO) { //return VBO and VAO values
	// Vertices coordinates
	const float K = 0.5f;
	glm::vec3 vertices[] =
	{
		glm::vec3(-K, -K, 0.0f ),
		glm::vec3(K, -K, 0.0f),
		glm::vec3(0,  K, 0.0f)
	};

	//make VAO
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	//bind it
	glBindVertexArray(VAO);

	//bind the VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//send the data to the GPU
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//Configure the attributes
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	//Make it valid
	glEnableVertexAttribArray(0);

	//Bind both the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

//Quit when ESC is released
static void KbdCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) glfwSetWindowShouldClose(window, GLFW_TRUE);
}


int main()
{
	glfwInit();

	//negotiate with OpenGL
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//make OpenGL windo
	GLFWwindow* window = glfwCreateWindow(800, 800, "Simple", NULL, NULL);
	//is all OK?
	if (window == NULL)
	{
		std::cout << "Cannot open GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	//Paste the window to the current context
	glfwMakeContextCurrent(window);

	//Load GLAD to configure OpenGL
	gladLoadGL();
	//Set the viewport
	glViewport(0, 0, 800, 800);

	//once the OpenGL context is done, build the scene and compile shaders
	BuildScene(VBO, VAO);
	int shaderProg = CompileShaders();

	//Backg color
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	//Use shader
	glUseProgram(shaderProg);
	//Bind the VAO
	glBindVertexArray(VAO);
	glPointSize(5);

	// Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	bool drawTriangle = true;
	float scale = 1.0f;
	float color[4] = { 0.2f, 0.2f, 0.8f, 1.0f };

	glfwSetKeyCallback(window, KbdCallback); //set keyboard callback to quit

	// Main while loop
	while (!glfwWindowShouldClose(window))
	{
		//Clean the window
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		//Draw the scene
		//glDrawArrays(GL_TRIANGLES, 0, 3);

		if (drawTriangle) {
			glDrawArrays(GL_POINTS, 0, 3);
			glDrawArrays(GL_LINE_LOOP, 0, 3);
		}

		// ImGUI window creation
		ImGui::Begin("CS 535");
		// Text that appears in the window
		ImGui::Text("Let there be OpenGL!");
		// Checkbox that appears in the window
		ImGui::Checkbox("Draw Triangle", &drawTriangle);
		// Slider that appears in the window
		ImGui::SliderFloat("Scale", &scale, 0.5f, 2.0f);
		// Fancy color editor that appears in the window
		ImGui::ColorEdit4("Color", color);
		// Ends the window
		ImGui::End();

		// Export variables to shader
		glUseProgram(shaderProg);
		glUniform1f(glGetUniformLocation(shaderProg, "scale"), scale);
		glUniform4f(glGetUniformLocation(shaderProg, "color"), color[0], color[1], color[2], color[3]);

		// Renders the ImGUI elements
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		//Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		//make sure events are served
		glfwPollEvents();
	}

	//Cleanup
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shaderProg);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
