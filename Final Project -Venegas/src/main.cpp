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

// Light properties
float sh = 100;

// Global variables
glm::vec3 playerPos(0.0f, 0.0f, 0.0f);
float deltaTime = 0;

enum class EnemyType {
    NORMAL,
    FAST
};

// GameObjects
class GameObject {
public:
    glm::vec3 position, size, velocity;
    float rotation;
    GLuint texture;
    glm::vec3 ambientColor, diffuseColor, specularColor;

    GameObject(glm::vec3 pos, glm::vec3 sz, GLuint tex, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular)
        : position(pos), size(sz), texture(tex), ambientColor(ambient), diffuseColor(diffuse), specularColor(specular), velocity(0.0f), rotation(0.0f) {}

    virtual void update(GLfloat dt) = 0;

    virtual void render(const glm::mat4& proj, const glm::mat4& view, GLuint modelParameter, GLuint viewParameter, GLuint projParameter, GLuint modelViewNParameter, GLfloat sh, GLint points) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::scale(model, size);
        glm::mat4 modelView = proj * view * model;
        glm::mat3 modelViewN = glm::mat3(view * model);
        modelViewN = glm::transpose(glm::inverse(modelViewN));

        glUniformMatrix4fv(modelParameter, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewParameter, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projParameter, 1, GL_FALSE, glm::value_ptr(proj));
        glUniformMatrix3fv(modelViewNParameter, 1, GL_FALSE, glm::value_ptr(modelViewN));

        glUniform3fv(kaParameter, 1, glm::value_ptr(ambientColor));
        glUniform3fv(kdParameter, 1, glm::value_ptr(diffuseColor));
        glUniform3fv(ksParameter, 1, glm::value_ptr(specularColor));
        glUniform1fv(shParameter, 1, &sh);

        glDrawArrays(GL_TRIANGLES, 0, points / 3);
    }

    virtual ~GameObject() = default;
};

class Player : public GameObject {
public:
    int health = 10;

    // Player is blue
    Player(glm::vec3 pos, glm::vec3 sz)
        : GameObject(pos, sz, 1, glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), glm::vec3(1, 1, 0)){}

    void update(GLfloat dt) override {
        // Update player position based on input
    }
};


class Enemy : public GameObject {
    public:
    int health = 2;
    float speed = 3.0f;

    // Normal enemy is red
    Enemy(glm::vec3 pos, glm::vec3 sz)
        : GameObject(pos, sz, 1, glm::vec3(1, 0, 0), glm::vec3(1, 0, 0), glm::vec3(1, 0, 0)){}

    void update(GLfloat dt) override {
        // Update enemy position based on AI or other logic
        glm::vec3 direction = playerPos - position;
        float length = glm::length(direction);
        if (length != 0) {
            direction /= length;
        }
        position += direction * speed * dt;
    }
};

class FastEnemy : public GameObject {
    public:
    int health = 3;
    float speed = 5.0f;

    // Fast enemy is yellow
    FastEnemy(glm::vec3 pos, glm::vec3 sz)
        : GameObject(pos, sz, 1, glm::vec3(1, 1, 0), glm::vec3(1, 1, 0), glm::vec3(1, 1, 0)){}

    void update(GLfloat dt) override {
        // Update enemy position based on AI or other logic
        glm::vec3 direction = playerPos - position;
        float length = glm::length(direction);
        if (length != 0) {
            direction /= length;
        }
        position += direction * speed * dt;
    }
};

class Bullet : public GameObject {
    public:
    Bullet(glm::vec3 pos, glm::vec3 sz)
        : GameObject(pos, sz, 1, glm::vec3(0, 0, 1), glm::vec3(0, 1, 0), glm::vec3(1, 0, 0)) {}

    void update(GLfloat dt) override {
        // Move bullet
        position += velocity * dt;
        // Check for collisions and bouncing
        // (Collision code here)
    }
};

// Enemy vector
std::vector<Enemy> enemies;

// Enemy vector
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
float moveSpeed = 0.07f;  // Incrementar la velocidad de movimiento

float enemyPosX = 10.0f;  
float enemyPosY = 10.0f;
float enemySpeed = 0.0006f;

float enemyFastPosX = -10.0f;  
float enemyFastPosY = 10.0f;
float enemyFastSpeed = 0.0009f;

GLuint VAO, VBO;
static bool textureLoaded = false;

bool showMenu = true;

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



// Function to process input and move the player
void processInput(GLFWwindow* window, float deltaTime) {
    float moveSpeed = 5.0f; // Adjust the speed as necessary
    float moveAmount = moveSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        spherePosY -= moveAmount;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        spherePosY += moveAmount;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        spherePosX -= moveAmount;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        spherePosX += moveAmount;
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

    using namespace std;
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

// Spawn enemy in 1, 1 every 5 seconds
void SpawnEnemy(glm::vec2 position, float spawnInterval)
{
    static float spawnTimer = 0.0f;

    spawnTimer += deltaTime;
    if (spawnTimer >= spawnInterval) {
        // Create enemy object
        Enemy newEnemy(glm::vec3(position.x, 0, position.y), glm::vec3(0.4f, 0.4f, 0.4f));
        
        // Add enemy to vector
        enemies.push_back(newEnemy);
        
        spawnTimer = 0.0f;
    }
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

    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Top-Down Shooter", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Cannot open GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    gladLoadGL();
    glViewport(0, 0, 1920, 1080);

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

    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);;

    glClearDepth(100000.0);
    glEnable(GL_DEPTH_TEST);

    float u = 1, v = 1;

    float lastFrame = 0;
    
    // Create player object
    Player player(glm::vec3(0, 0, 0), glm::vec3(0.4f, 0.4f, 0.4f));

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Cosas de Christian
        float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
        processInput(window, deltaTime);

        // Enemigo GameObject
        // Create enemy objects
        SpawnEnemy(glm::vec2(1, 1), 5.0f);

        // Vistas
        glm::mat4 proj = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.01f, 1000.0f);
        glm::mat4 view = glm::lookAt(glm::vec3(0, 5, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, -1));

        // Rendering in the main 
        playerPos = player.position;

        // Render and update enemies in enemy vector
        for (int i = 0; i < enemies.size(); i++) {
            enemies[i].update(deltaTime);

            enemies[i].render(proj, view, modelParameter, viewParameter, projParameter, modelViewNParameter, sh, points);
        }

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
