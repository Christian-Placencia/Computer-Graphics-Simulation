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

GLuint VAO[3], VBO[3];

int CompileShaders() {
    // Vertex Shader
    const char* vsSrc = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "uniform float scale;\n"
		"uniform float translation_x;\n"
		"uniform float translation_y;\n"
		"uniform float rotation;\n"
        "void main()\n"
        "{\n"
        "   float cosTheta = cos(rotation);\n"
        "   float sinTheta = sin(rotation);\n"
        "   vec3 rotatedPos;\n"
        "   rotatedPos.x = aPos.x * cosTheta - aPos.y * sinTheta;\n"
        "   rotatedPos.y = aPos.x * sinTheta + aPos.y * cosTheta;\n"
        "   rotatedPos.z = aPos.z;\n"
        "   gl_Position = vec4(scale * (rotatedPos.x + translation_x), scale * (rotatedPos.y + translation_y), scale * rotatedPos.z, 1.0);\n"
        "}\0";

    // Fragment Shader
    const char* fsSrc = "#version 330 core\n"
        "out vec4 col;\n"
        "uniform vec4 color;\n"
        "void main()\n"
        "{\n"
        "   col = color;\n"
        "}\n\0";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSrc, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSrc, NULL);
    glCompileShader(fs);

    GLuint shaderProg = glCreateProgram();
    glAttachShader(shaderProg, vs);
    glAttachShader(shaderProg, fs);
    glLinkProgram(shaderProg);

    glDeleteShader(vs);
    glDeleteShader(fs);
    return shaderProg;
}

void BuildScene(GLuint& VBO, GLuint& VAO, int shape) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    float vertices[100 * 3];

    if (shape == 0) {  // Triangle
        float tempVertices[] = {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.0f,  0.5f, 0.0f
        };
        memcpy(vertices, tempVertices, sizeof(tempVertices));
    }
    else if (shape == 1) {  // Square
        float tempVertices[] = {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.5f,  0.5f, 0.0f,
            -0.5f,  0.5f, 0.0f
        };
        memcpy(vertices, tempVertices, sizeof(tempVertices));
    }
    else if (shape == 2) {  // Hexagon
        float tempVertices[] = {
			-0.25f, -0.5f, 0.0f,
			 0.25f, -0.5f, 0.0f,
			 0.5f,  0.0f, 0.0f,
			 0.25f,  0.5f, 0.0f,
			-0.25f,  0.5f, 0.0f,
			-0.5f,  0.0f, 0.0f
        };
        memcpy(vertices, tempVertices, sizeof(tempVertices));
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static void KbdCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Simple", NULL, NULL);
    if (window == NULL) {
        std::cout << "Cannot open GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    gladLoadGL();
    glViewport(0, 0, 800, 800);

    int shaderProg = CompileShaders();

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glUseProgram(shaderProg);
    glPointSize(5);
    glLineWidth(5);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    bool drawTriangle = true;
    bool drawSquare = false;  // Initially set to false
	bool drawHexagon = false;  // Initially set to false
    float scale = 1.0f;
	float translation_x = 0.0f;
	float translation_y = 0.0f;
	float rotation = 0.0f;
    float colorTriangle[4] = { 0.2f, 0.2f, 0.8f, 1.0f };  // Color for triangle
    float colorSquare[4] = { 0.8f, 0.2f, 0.2f, 1.0f };  // Color for square
	float colorHexagon[4] = { 0.2f, 0.8f, 0.2f, 1.0f };  // Color for hexagon

    glfwSetKeyCallback(window, KbdCallback);

    BuildScene(VBO[0], VAO[0], 0);  // Triangle
    BuildScene(VBO[1], VAO[1], 1);  // Square
    BuildScene(VBO[2], VAO[2], 2);  // Hexagon

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glUseProgram(shaderProg);

        if (drawTriangle) {
            glBindVertexArray(VAO[0]);
            glUniform4fv(glGetUniformLocation(shaderProg, "color"), 1, colorTriangle);
            glDrawArrays(GL_LINE_LOOP, 0, 3);
        }
        if (drawSquare) {
            glBindVertexArray(VAO[1]);
            glUniform4fv(glGetUniformLocation(shaderProg, "color"), 1, colorSquare);
            glDrawArrays(GL_LINE_LOOP, 0, 4);
        }
        if (drawHexagon) {
            glBindVertexArray(VAO[2]);
            glUniform4fv(glGetUniformLocation(shaderProg, "color"), 1, colorHexagon);
            glDrawArrays(GL_LINE_LOOP, 0, 6);
        }

        ImGui::Begin("CS 535");
        ImGui::Text("Let there be OpenGL!");
        ImGui::Checkbox("Draw Triangle", &drawTriangle);
        ImGui::Checkbox("Draw Square", &drawSquare);
		ImGui::Checkbox("Draw Hexagon", &drawHexagon);
        ImGui::SliderFloat("Scale", &scale, -3.0f, 3.0f);
        ImGui::SliderFloat("Translation x", &translation_x, -3.0f, 3.0f);
        ImGui::SliderFloat("Translation y", &translation_y, -3.0f, 3.0f);
        ImGui::SliderFloat("Rotate", &rotation, -3.0f, 3.0f);
        ImGui::ColorEdit4("Triangle Color", colorTriangle);
        ImGui::ColorEdit4("Square Color", colorSquare);
        ImGui::ColorEdit4("Hexagon Color", colorHexagon);
        ImGui::End();

        glUniform1f(glGetUniformLocation(shaderProg, "scale"), scale);
        glUniform1f(glGetUniformLocation(shaderProg, "translation_x"), translation_x);
        glUniform1f(glGetUniformLocation(shaderProg, "translation_y"), translation_y);
        glUniform1f(glGetUniformLocation(shaderProg, "rotation"), rotation);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(2, VAO);
    glDeleteBuffers(2, VBO);
    glDeleteProgram(shaderProg);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
