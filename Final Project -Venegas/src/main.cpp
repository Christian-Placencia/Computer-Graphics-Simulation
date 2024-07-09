#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <stdio.h>
#include <iostream>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <string>
#include <vector>
#include <algorithm>
#include <array>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "triangle.h"
#include "helper.h"
#include "objGen.h"
#include "trackball.h"
#include "shaders.h"

#pragma warning(disable : 4996)
#pragma comment(lib, "glfw3.lib")

using namespace std;

TrackBallC trackball;
bool mouseLeft, mouseMid, mouseRight;

// Shader program ID
GLuint shaderProg;
GLint modelParameter;
GLint viewParameter;
GLint projParameter;
GLint modelViewNParameter;

// Materials and lights
GLint kaParameter, kdParameter, ksParameter, shParameter;
GLint laParameter, ldParameter, lsParameter, lPosParameter;

float ksColor[3] = { 1, 1, 0 };
float kaColor[3] = { 0, 0, 0 };
float kdColor[3] = { 0, 0, 1 };
float sh = 100;
float laColor[3] = { 0, 0, 0 };
float ldColor[3] = { 0, 0, 1 };
float lsColor[3] = { 1, 1, 0 };

float enemyKsColor[3] = { 0, 1, 0 };
float enemyKaColor[3] = { 0, 0, 0 };
float enemyKdColor[3] = { 1, 0, 0 };

glm::vec4 lPos;
float ltime = 0;
float timeDelta = 0;

int texAs;
GLint texAsParameter;

GLint points = 0;
GLdouble mouseX, mouseY;

int subdivision = 10;
int wrapu, wrapv;
int minFilter = 0, maxFilter = 0;

float spherePosX = 0.0f;
float spherePosY = 0.0f;
float moveSpeed = 0.05f;  // Incrementar la velocidad de movimiento

float enemyPosX = 4.0f;  // Coordenadas iniciales en la esquina superior derecha
float enemyPosY = 4.0f;
float enemySpeed = 0.02f;

GLuint VAO, VBO;
static bool textureLoaded = false;

void LoadTexture()
{
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    char filename[256];
    strcpy(filename, "textures/check.jpg");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Texture " << filename << " loaded " << width << " x " << height << " pixels" << std::endl;
    }
    else
    {
        std::cout << "Texture " << filename << " not found " << std::endl;
        exit(-1);
    }
    stbi_image_free(data);
}

GLfloat P2(GLfloat u)
{
    return 0.5f * sin(M_PI * u);
}

inline void AddVertex(vector<GLfloat>* a, glm::vec3 A)
{
    a->push_back(A[0]);
    a->push_back(A[1]);
    a->push_back(A[2]);
}

inline void AddUV(vector<GLfloat>* a, glm::vec2 A)
{
    a->push_back(A[0]);
    a->push_back(A[1]);
}

inline glm::vec3 S(GLfloat u, GLfloat v, int scene)
{
    glm::vec3 ret;
    switch (scene) {
    case 1: ret = glm::vec3(P2(u) * sin(2 * M_PI * v), P2(u) * cos(2 * M_PI * v), cos(M_PI * u)); break;
    }
    return ret;
}

inline glm::vec3 Snormal(GLfloat u, GLfloat v, int scene)
{
    float du = 0.05f;
    float dv = 0.05f;

    glm::vec3 sdu, sdv;
    sdu = glm::vec3(S(u, v, scene) - S(u + du, v, scene)) / du;
    sdv = glm::vec3(S(u, v, scene) - S(u, v + dv, scene)) / dv;
    return glm::normalize(glm::cross(sdu, sdv));
}

void CreateRevo(vector<GLfloat>* a, float u, float v, int n, int scene)
{
    GLfloat step = 1.f / n;
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            AddVertex(a, S(i * step, j * step, scene));
            AddVertex(a, Snormal(i * step, j * step, scene));
            AddUV(a, glm::vec2((GLfloat)i * u / n, (GLfloat)j * v / n));

            AddVertex(a, S((i + 1) * step, j * step, scene));
            AddVertex(a, Snormal((i + 1) * step, j * step, scene));
            AddUV(a, glm::vec2((GLfloat)(i + 1) * u / n, (GLfloat)j * v / n));

            AddVertex(a, S((i + 1) * step, (j + 1) * step, scene));
            AddVertex(a, Snormal((i + 1) * step, (j + 1) * step, scene));
            AddUV(a, glm::vec2((GLfloat)(i + 1) * u / n, (GLfloat)(j + 1) * v / n));

            AddVertex(a, S(i * step, j * step, scene));
            AddVertex(a, Snormal(i * step, j * step, scene));
            AddUV(a, glm::vec2((GLfloat)i * u / n, (GLfloat)j * v / n));

            AddVertex(a, S((i + 1) * step, (j + 1) * step, scene));
            AddVertex(a, Snormal((i + 1) * step, (j + 1) * step, scene));
            AddUV(a, glm::vec2((GLfloat)(i + 1) * u / n, (GLfloat)(j + 1) * v / n));

            AddVertex(a, S(i * step, (j + 1) * step, scene));
            AddVertex(a, Snormal(i * step, (j + 1) * step, scene));
            AddUV(a, glm::vec2((GLfloat)i * u / n, (GLfloat)(j + 1) * v / n));
        }
    }
}

void BuildScene(float uu, float vv, int subdiv, int scene) {
    vector<GLfloat> v;
    CreateRevo(&v, uu, vv, subdiv, scene);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    points = v.size();
    glBufferData(GL_ARRAY_BUFFER, points * sizeof(GLfloat), &v[0], GL_STATIC_DRAW);
    v.clear();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    if (!textureLoaded) {
        LoadTexture();
        textureLoaded = true;
    }
}

static void KbdCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) glfwSetWindowShouldClose(window, GLFW_TRUE);

    bool moveUp = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    bool moveDown = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    bool moveLeft = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    bool moveRight = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

    if (moveUp && moveLeft) {
        spherePosY -= moveSpeed / sqrt(2.0f);
        spherePosX -= moveSpeed / sqrt(2.0f);
    }
    else if (moveUp && moveRight) {
        spherePosY -= moveSpeed / sqrt(2.0f);
        spherePosX += moveSpeed / sqrt(2.0f);
    }
    else if (moveDown && moveLeft) {
        spherePosY += moveSpeed / sqrt(2.0f);
        spherePosX -= moveSpeed / sqrt(2.0f);
    }
    else if (moveDown && moveRight) {
        spherePosY += moveSpeed / sqrt(2.0f);
        spherePosX += moveSpeed / sqrt(2.0f);
    }
    else {
        if (moveUp) spherePosY -= moveSpeed;
        if (moveDown) spherePosY += moveSpeed;
        if (moveLeft) spherePosX -= moveSpeed;
        if (moveRight) spherePosX += moveSpeed;
    }
}

void MouseCallback(GLFWwindow* window, double x, double y) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(x, y);
    if (io.WantCaptureMouse) return;

    mouseX = x;
    mouseY = y;
    if (mouseLeft)  trackball.Rotate(mouseX, mouseY);
    if (mouseMid)   trackball.Translate(mouseX, mouseY);
    if (mouseRight) trackball.Zoom(mouseX, mouseY);
}

void MouseButtonCallback(GLFWwindow* window, int button, int state, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseButtonEvent(button, state);
    if (io.WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && state == GLFW_PRESS)
    {
        trackball.Set(window, true, mouseX, mouseY);
        mouseLeft = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && state == GLFW_RELEASE)
    {
        trackball.Set(window, false, mouseX, mouseY);
        mouseLeft = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && state == GLFW_PRESS)
    {
        trackball.Set(window, true, mouseX, mouseY);
        mouseMid = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && state == GLFW_RELEASE)
    {
        trackball.Set(window, true, mouseX, mouseY);
        mouseMid = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && state == GLFW_PRESS)
    {
        trackball.Set(window, true, mouseX, mouseY);
        mouseRight = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && state == GLFW_RELEASE)
    {
        trackball.Set(window, true, mouseX, mouseY);
        mouseRight = false;
    }
}

void InitShaders(GLuint* program)
{
    std::vector<GLuint> shaderList;

    shaderList.push_back(CreateShader(GL_VERTEX_SHADER, LoadShader("shaders/texture.vert")));
    shaderList.push_back(CreateShader(GL_FRAGMENT_SHADER, LoadShader("shaders/texture.frag")));

    *program = CreateProgram(shaderList);

    std::for_each(shaderList.begin(), shaderList.end(), glDeleteShader);

    glUniform1i(glGetUniformLocation(shaderProg, "tex"), 0);
    modelParameter = glGetUniformLocation(shaderProg, "model");
    viewParameter = glGetUniformLocation(shaderProg, "view");
    projParameter = glGetUniformLocation(shaderProg, "proj");
    modelViewNParameter = glGetUniformLocation(shaderProg, "modelViewN");

    kaParameter = glGetUniformLocation(shaderProg, "mat.ka");
    kdParameter = glGetUniformLocation(shaderProg, "mat.kd");
    ksParameter = glGetUniformLocation(shaderProg, "mat.ks");
    shParameter = glGetUniformLocation(shaderProg, "mat.sh");
    laParameter = glGetUniformLocation(shaderProg, "light.la");
    ldParameter = glGetUniformLocation(shaderProg, "light.ld");
    lsParameter = glGetUniformLocation(shaderProg, "light.ls");
    lPosParameter = glGetUniformLocation(shaderProg, "light.lPos");
    texAsParameter = glGetUniformLocation(shaderProg, "texAs");
}

void EnemyIdle()
{
    float directionX = spherePosX - enemyPosX;
    float directionY = spherePosY - enemyPosY;
    float length = sqrt(directionX * directionX + directionY * directionY);
    if (length != 0) {
        directionX /= length;
        directionY /= length;
    }
    enemyPosX += directionX * enemySpeed;
    enemyPosY += directionY * enemySpeed;
}

int main()
{
    bool drawScene = true;
    bool filled = true;
    int scene = 1;
    float repeat = 1.f;

    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1024, 1024, "Lighting", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Cannot open GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    gladLoadGL();
    glViewport(0, 0, 1024, 1024);

    InitShaders(&shaderProg);
    BuildScene(1, 1, subdivision, scene);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glUseProgram(shaderProg);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    glfwSetKeyCallback(window, KbdCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);;

    glClearDepth(100000.0);
    glEnable(GL_DEPTH_TEST);

    float u = 1, v = 1;

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Texturing");
        if (ImGui::SliderFloat("u", &u, 0.1, 10.0, "%f", 0)) {
            BuildScene(u, v, subdivision, scene);
        }
        if (ImGui::SliderFloat("v", &v, 0.1, 10.0, "%f", 0)) {
            BuildScene(u, v, subdivision, scene);
        }
        if (ImGui::SliderInt("Subdivision", &subdivision, 1, 100, "%i", 0)) {
            BuildScene(u, v, subdivision, scene);
        }

        if (ImGui::RadioButton("Sphere", &scene, 1)) {
            BuildScene(u, v, subdivision, scene);
        }

        ImGui::RadioButton("No tex", &texAs, 0); ImGui::SameLine();
        ImGui::RadioButton("Tex as Color", &texAs, 1); ImGui::SameLine();
        ImGui::RadioButton("Tex as diffuse", &texAs, 2); ImGui::SameLine();
        ImGui::RadioButton("Tex as specular", &texAs, 3); ImGui::SameLine();
        ImGui::RadioButton("Tex as ambient", &texAs, 4);

        if (ImGui::Checkbox("Filled", &filled)) {
            if (filled) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            else glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        ImGui::ColorEdit3("Mat. Ambient", kaColor);
        ImGui::ColorEdit3("Mat. Diffuse", kdColor);
        ImGui::ColorEdit3("Mat. Specular", ksColor);
        ImGui::SliderFloat("Shininess", &sh, 0, 200, "%f", 0);

        ImGui::ColorEdit3("Light Ambient", laColor);
        ImGui::ColorEdit3("Light Diffuse", ldColor);
        ImGui::ColorEdit3("Light Specular", lsColor);
        ImGui::SliderFloat("Speed", &timeDelta, 0.0, 0.3, "%f", 0);

        ImGui::End();

        // Actualizar el enemigo
        EnemyIdle();

        glm::mat4 proj = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.01f, 1000.0f);
        glm::mat4 view = glm::lookAt(glm::vec3(0, 5, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, -1));

        // Renderizar el jugador
        glBindVertexArray(VAO);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(spherePosX, 0.0f, spherePosY));
        model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));  // Ajustar el tamaño de la esfera
        glm::mat4 modelView = proj * view * model;
        glm::mat3 modelViewN = glm::mat3(view * model);
        modelViewN = glm::transpose(glm::inverse(modelViewN));

        glUniformMatrix4fv(modelParameter, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewParameter, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projParameter, 1, GL_FALSE, glm::value_ptr(proj));
        glUniformMatrix3fv(modelViewNParameter, 1, GL_FALSE, glm::value_ptr(modelViewN));

        lPos[0] = 100 * sin(ltime);
        lPos[1] = 0.5;
        lPos[2] = -100 * cos(ltime);
        lPos[3] = 1;
        ltime += timeDelta;

        glUniform3fv(kaParameter, 1, kaColor);
        glUniform3fv(kdParameter, 1, kdColor);
        glUniform3fv(ksParameter, 1, ksColor);
        glUniform1fv(shParameter, 1, &sh);

        glUniform3fv(laParameter, 1, laColor);
        glUniform3fv(ldParameter, 1, ldColor);
        glUniform3fv(lsParameter, 1, lsColor);
        glUniform4fv(lPosParameter, 1, glm::value_ptr(lPos));

        glUniform1iv(texAsParameter, 1, &texAs);

        glDrawArrays(GL_TRIANGLES, 0, points / 3);

        // Renderizar el enemigo
        glm::mat4 modelEnemy = glm::translate(glm::mat4(1.0f), glm::vec3(enemyPosX, 0.0f, enemyPosY));
        modelEnemy = glm::scale(modelEnemy, glm::vec3(0.4f, 0.4f, 0.4f));  // Ajustar el tamaño de la esfera
        glm::mat4 modelViewEnemy = proj * view * modelEnemy;
        glm::mat3 modelViewNEnemy = glm::mat3(view * modelEnemy);
        modelViewNEnemy = glm::transpose(glm::inverse(modelViewNEnemy));

        glUniformMatrix4fv(modelParameter, 1, GL_FALSE, glm::value_ptr(modelEnemy));
        glUniformMatrix4fv(viewParameter, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projParameter, 1, GL_FALSE, glm::value_ptr(proj));
        glUniformMatrix3fv(modelViewNParameter, 1, GL_FALSE, glm::value_ptr(modelViewNEnemy));

        glUniform3fv(kaParameter, 1, enemyKaColor);
        glUniform3fv(kdParameter, 1, enemyKdColor);
        glUniform3fv(ksParameter, 1, enemyKsColor);
        glUniform1fv(shParameter, 1, &sh);

        glDrawArrays(GL_TRIANGLES, 0, points / 3);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(shaderProg);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
