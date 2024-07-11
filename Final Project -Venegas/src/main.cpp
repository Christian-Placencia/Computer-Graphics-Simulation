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

// Materials and lights parameters
GLint kaParameter, kdParameter, ksParameter, shParameter;
GLint laParameter, ldParameter, lsParameter, lPosParameter;

// Light properties
float sh = 100;
glm::vec4 lPos;

// Camera settings
GLFWwindow* window;
glm::mat4 proj = glm::ortho(-4.5f, 4.5f, -5.0f, 5.0f, 0.01f, 1000.0f);
// glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1920.0f / 1080.0f, 0.1f, 1000.0f);
glm::mat4 view = glm::lookAt(glm::vec3(0, 5, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, -1));

// Screen settings
GLint points = 0;
GLdouble mouseX, mouseY;
bool showMenu = true;

// Texture settings
int texAs;
GLint texAsParameter;
static bool textureLoaded = false;

// Render settings
int subdivision = 10;
GLuint VAO, VBO;

// Global variables
glm::vec3 playerPos(0.0f, 0.0f, 0.0f);
float deltaTime = 0;
float currentFrame;

enum class EnemyType {
    NORMAL,
    FAST
};

// Function prototypes
void CreateRectPrism(vector<GLfloat>* a, float width, float height, float depth, float u, float v, int n);
void LoadTexture();

// GameObjects
class GameObject {
public:
    glm::vec3 position, size, velocity;
    float rotation;
    GLuint texture;
    glm::vec3 ambientColor, diffuseColor, specularColor;

    GameObject(glm::vec3 pos, glm::vec3 sz, GLuint tex, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular)
        : position(pos), size(sz), texture(tex), ambientColor(ambient), diffuseColor(diffuse), specularColor(specular), velocity(0.0f), rotation(0.0f) {}

    virtual void update() = 0;

    virtual void render() {
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

class Bullet : public GameObject {
    public:
    Bullet(glm::vec3 pos, glm::vec3 sz)
        : GameObject(pos, sz, 1, glm::vec3(0, 0, 1), glm::vec3(0, 1, 0), glm::vec3(1, 0, 0)) {}

    void update() override {
        // Move bullet
        position += velocity * deltaTime;
        // Check for collisions and bouncing
        // (Collision code here)
    }
};

std::vector<Bullet> bullets;

class Player : public GameObject {
public:
    int health = 10;
    float speed = 5.0f;
    float bulletReloadSpeed = 0.2f;
    float reloadTimer = 0.0f;

    // Player is blue
    Player(glm::vec3 pos, glm::vec3 sz)
        : GameObject(pos, sz, 1, glm::vec3(0, 0, 0.2), glm::vec3(0, 0, 1), glm::vec3(1, 1, 0)){}

    void update() override {
        // Update player position based on input
        float moveAmount = speed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            position.z -= moveAmount;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            position.z += moveAmount;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            position.x -= moveAmount;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)  
            position.x += moveAmount;

        // Shoot bullet on mouse click
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            // Only fire when the reload speed has passed
            reloadTimer += deltaTime;

            if (reloadTimer >= bulletReloadSpeed) {
                // Create bullet object and add to vector
                Bullet newBullet(position, glm::vec3(0.1f, 0.1f, 0.1f));
                newBullet.velocity = glm::vec3(0, 0, -10.0f);
                bullets.push_back(newBullet);

                // Reset timer
                reloadTimer = 0.0f;
            }
        }
    }
};


class Enemy : public GameObject {
    public:
    int health = 2;
    float speed = 3.0f;

    // Normal enemy is green
    Enemy(glm::vec3 pos, glm::vec3 sz)
        : GameObject(pos, sz, 1, glm::vec3(0,0.2,0), glm::vec3(0,1,0), glm::vec3(1,1,0)){}

    void update() override {
        // Update enemy position based on AI or other logic
        glm::vec3 direction = playerPos - position;
        float length = glm::length(direction);
        if (length != 0) {
            direction /= length;
        }
        position += direction * speed * deltaTime;
    }
};

class FastEnemy : public GameObject {
    public:
    int health = 3;
    float speed = 5.0f;

    // Fast enemy is red
    FastEnemy(glm::vec3 pos, glm::vec3 sz)
        : GameObject(pos, sz, 1, glm::vec3(0.2, 0, 0), glm::vec3(1, 0, 0), glm::vec3(1, 1, 0)){}

    void update() override {
        // Update enemy position based on AI or other logic
        glm::vec3 direction = playerPos - position;
        float length = glm::length(direction);
        if (length != 0) {
            direction /= length;
        }
        position += direction * speed * deltaTime;
    }
};

// Enemy vector
std::vector<Enemy> enemies;
std::vector<FastEnemy> fastEnemies;

class Wall : public GameObject {
    public:
    Wall(glm::vec3 pos, glm::vec3 sz)
        : GameObject(pos, sz, 1, glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1)){}

    void update() override {
        // Walls are static
    }

    void render() override {
        vector<GLfloat> v;
        CreateRectPrism(&v, size.x, size.y, size.z, 1.0f, 1.0f, subdivision);

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

        GameObject::render();
    }
};

vector<Wall> walls;


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

// Parametric equations for the surface
GLfloat P2(GLfloat u)
{
    return 0.5f * sin(M_PI * u);
}

// Add vertex and normal to the vector
inline void AddVertex(vector<GLfloat>* a, glm::vec3 A)
{
    a->push_back(A[0]);
    a->push_back(A[1]);
    a->push_back(A[2]);
}

// Add UV to the vector
inline void AddUV(vector<GLfloat>* a, glm::vec2 A)
{
    a->push_back(A[0]);
    a->push_back(A[1]);
}

// Surface equation
inline glm::vec3 S(GLfloat u, GLfloat v, int scene)
{
    glm::vec3 ret;
    switch (scene) {
    case 1: ret = glm::vec3(P2(u) * sin(2 * M_PI * v), P2(u) * cos(2 * M_PI * v), cos(M_PI * u)); break;
    }
    return ret;
}

// Normal equation
inline glm::vec3 Snormal(GLfloat u, GLfloat v, int scene)
{
    float du = 0.05f;
    float dv = 0.05f;

    glm::vec3 sdu, sdv;
    sdu = glm::vec3(S(u, v, scene) - S(u + du, v, scene)) / du;
    sdv = glm::vec3(S(u, v, scene) - S(u, v + dv, scene)) / dv;
    return glm::normalize(glm::cross(sdu, sdv));
}

// Create revolution surface
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

// Function to add a face of the rectangular prism
void AddFace(vector<GLfloat>* a, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 normal, float u, float v, int n) {
    GLfloat step = 1.f / n;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            glm::vec2 uv0 = glm::vec2((GLfloat)i * u / n, (GLfloat)j * v / n);
            glm::vec2 uv1 = glm::vec2((GLfloat)(i + 1) * u / n, (GLfloat)j * v / n);
            glm::vec2 uv2 = glm::vec2((GLfloat)(i + 1) * u / n, (GLfloat)(j + 1) * v / n);
            glm::vec2 uv3 = glm::vec2((GLfloat)i * u / n, (GLfloat)(j + 1) * v / n);

            glm::vec3 p0 = v0 + (v1 - v0) * (i * step) + (v3 - v0) * (j * step);
            glm::vec3 p1 = v0 + (v1 - v0) * ((i + 1) * step) + (v3 - v0) * (j * step);
            glm::vec3 p2 = v0 + (v1 - v0) * ((i + 1) * step) + (v3 - v0) * ((j + 1) * step);
            glm::vec3 p3 = v0 + (v1 - v0) * (i * step) + (v3 - v0) * ((j + 1) * step);

            AddVertex(a, p0);
            AddVertex(a, normal);
            AddUV(a, uv0);

            AddVertex(a, p1);
            AddVertex(a, normal);
            AddUV(a, uv1);

            AddVertex(a, p2);
            AddVertex(a, normal);
            AddUV(a, uv2);

            AddVertex(a, p0);
            AddVertex(a, normal);
            AddUV(a, uv0);

            AddVertex(a, p2);
            AddVertex(a, normal);
            AddUV(a, uv2);

            AddVertex(a, p3);
            AddVertex(a, normal);
            AddUV(a, uv3);
        }
    }
}

// Create rectangular prism
void CreateRectPrism(vector<GLfloat>* a, float width, float height, float depth, float u, float v, int n) {
    glm::vec3 p0 = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 p1 = glm::vec3(width, 0.0f, 0.0f);
    glm::vec3 p2 = glm::vec3(width, height, 0.0f);
    glm::vec3 p3 = glm::vec3(0.0f, height, 0.0f);
    glm::vec3 p4 = glm::vec3(0.0f, 0.0f, depth);
    glm::vec3 p5 = glm::vec3(width, 0.0f, depth);
    glm::vec3 p6 = glm::vec3(width, height, depth);
    glm::vec3 p7 = glm::vec3(0.0f, height, depth);

    glm::vec3 normalFront = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 normalBack = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 normalLeft = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 normalRight = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 normalTop = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 normalBottom = glm::vec3(0.0f, -1.0f, 0.0f);

    // Front face
    AddFace(a, p0, p1, p5, p4, normalFront, u, v, n);
    // Back face
    AddFace(a, p2, p3, p7, p6, normalBack, u, v, n);
    // Left face
    AddFace(a, p3, p0, p4, p7, normalLeft, u, v, n);
    // Right face
    AddFace(a, p1, p2, p6, p5, normalRight, u, v, n);
    // Top face
    AddFace(a, p3, p2, p6, p7, normalTop, u, v, n);
    // Bottom face
    AddFace(a, p0, p1, p5, p4, normalBottom, u, v, n);
}


// Build the scene
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

// Mouse and keyboard callback
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

// Function to spawn enemies
void SpawnEnemy(glm::vec2 position, float spawnInterval, EnemyType type = EnemyType::NORMAL)
{
    static float spawnTimer = 0.0f;
    spawnTimer += deltaTime;

    // Check if it's time to spawn a new enemy
    if (spawnTimer >= spawnInterval) {
        // Create enemy object and add to vector
        if (type == EnemyType::NORMAL)
        {
            Enemy newEnemy(glm::vec3(position.x, 0, position.y), glm::vec3(0.4f, 0.4f, 0.4f));
            enemies.push_back(newEnemy);
        }
            
        else if (type == EnemyType::FAST)
        {
            FastEnemy newEnemy(glm::vec3(position.x, 0, position.y), glm::vec3(0.4f, 0.4f, 0.4f));
            fastEnemies.push_back(newEnemy);
        }

        // Reset timer
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

    window = glfwCreateWindow(1920, 1080, "Top-Down Shooter", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Cannot open GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    gladLoadGL();

    int screenWidth, screenHeight;
    glfwGetWindowSize(window, &screenWidth, &screenHeight);

    glViewport(0, 0, screenWidth, screenHeight);

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

    glClearDepth(100000.0);
    glEnable(GL_DEPTH_TEST);

    float u = 1, v = 1;

    float lastFrame = 0;
    
    // Create level walls
    Wall wall1(glm::vec3(-4.5, 0, -4.5), glm::vec3(1.0f, 5.0f, 10.0f));
    Wall wall2(glm::vec3(-4.5, 0, -4.5), glm::vec3(10.0f, 5.0f, 1.0f));
    Wall wall3(glm::vec3(-4.5, 0, 4.5), glm::vec3(10.0f, 5.0f, 1.0f));
    Wall wall4(glm::vec3(4.5, 0, -4.5), glm::vec3(1.0f, 5.0f, 10.0f));

    walls.push_back(wall1);
    walls.push_back(wall2);
    walls.push_back(wall3);
    walls.push_back(wall4);

    // Create player object
    Player player(glm::vec3(0, 0, 0), glm::vec3(0.4f, 0.4f, 0.4f));

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render walls
        // ! Rendering walls makes EVERYTHING a wall
        // for (int i = 0; i < walls.size(); i++) {
        //     walls[i].render();
        // }

        // Time flow
        currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

        // Create enemy objects
        SpawnEnemy(glm::vec2(1, 1), 10.0f, EnemyType::NORMAL);
        SpawnEnemy(glm::vec2(-2, -3), 20.0f, EnemyType::FAST);

        glm::vec4 lPos(100, 100, 100, 100);
        glm::vec3 lightColor(1, 1, 0);

        glUniform4fv(lPosParameter, 1, glm::value_ptr(lPos));
        glUniform3fv(laParameter, 1, glm::value_ptr(lightColor * 1.0f));  // Ambient light
        glUniform3fv(ldParameter, 1, glm::value_ptr(lightColor * 1.0f));  // Diffuse light
        glUniform3fv(lsParameter, 1, glm::value_ptr(lightColor * 1.0f));

        // Show and update the player
        playerPos = player.position;
        player.update();
        player.render();

        // Render and update enemies in enemy vector
        for (int i = 0; i < enemies.size(); i++) {
            enemies[i].update();
            enemies[i].render();
        }

        // Render and update enemies in fast enemy vector
        for (int i = 0; i < fastEnemies.size(); i++) {
            fastEnemies[i].update();
            fastEnemies[i].render();
        }

        // Render and update bullets in bullet vector
        for (int i = 0; i < bullets.size(); i++) {
            bullets[i].update();
            bullets[i].render();
        }


        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    glDeleteProgram(shaderProg);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
