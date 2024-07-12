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
#include <unordered_map>

#pragma warning(disable : 4996)
#pragma comment(lib, "glfw3.lib")

using namespace std;

TrackBallC trackball;
bool mouseLeft, mouseMid, mouseRight;

// ImGui variables
bool showGameOverScreen = false;

// Shader program ID
GLuint shaderProg;
GLint modelParameter;
GLint viewParameter;
GLint projParameter;
GLint modelViewNParameter;

// Materials and lights parameters
GLint kaParameter, kdParameter, ksParameter, shParameter;
GLint laParameter, ldParameter, lsParameter, lPosParameter;

// Global variables
glm::vec3 playerPos(0.0f, 0.0f, 0.0f);
float deltaTime = 0;
float currentFrame;
int maxPlayerHealth = 10;
int playerHealth = maxPlayerHealth;
float bounds[4] = { -4.5f, 4.5f, -5.0f, 5.0f };

// Light properties
float sh = 100;
glm::vec4 lPos;

// Camera settings
GLFWwindow* window;
glm::mat4 proj = glm::ortho(bounds[0], bounds[1], bounds[2], bounds[3], 0.01f, 1000.0f);
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
GLuint sphereVAO, sphereVBO;
GLuint rectPrismVAO, rectPrismVBO;
GLuint circleVAO, circleVBO;

enum class EnemyType {
    NORMAL,
    FAST
};

enum Shape {
	SPHERE,
	PRISM,
    CIRCLE
};

// Clase Collider para manejar colisiones
class Collider {
private:
    const glm::vec3* positionPtr;
    float* radius;
    float radiusSquared;  // Changed to a normal float

public:
    Collider(const glm::vec3* posPtr, float* r) : positionPtr(posPtr), radius(r), radiusSquared((*r)* (*r)) {}

    bool checkCollision(const Collider& other) const {
        // Calculate the squared distance between the positions
        glm::vec3 diff = *positionPtr - *other.positionPtr;
        float distanceSquared = glm::dot(diff, diff);

        // Calculate the sum of the radii and its square
        float radiiSum = *radius + *other.radius;
        float radiiSumSquared = radiiSum * radiiSum;

        // Check if the squared distance is less than the squared sum of the radii
        return distanceSquared < radiiSumSquared;
    }

    const glm::vec3& getPosition() const {
        return *positionPtr;
    }
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
    Collider collider;
    Shape shape;

    GameObject(glm::vec3 pos, glm::vec3 sz, float colliderRadius, GLuint tex, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, Shape shp)
        : position(pos), size(sz), texture(tex), ambientColor(ambient), diffuseColor(diffuse), specularColor(specular), velocity(0.0f), rotation(0.0f), collider(&position, &size[0]), shape(shp) {}

    virtual void update() {
    }

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

        if (shape == SPHERE) {
            glBindVertexArray(sphereVAO);
        }
        else if (shape == PRISM) {
            glBindVertexArray(rectPrismVAO);
        }
        else if (shape == CIRCLE) {
            glBindVertexArray(circleVAO);
        }

        glDrawArrays(GL_TRIANGLES, 0, points / 3);
    }

    GameObject(const GameObject& other)
        : position(other.position), size(other.size), velocity(other.velocity), rotation(other.rotation),
        texture(other.texture), ambientColor(other.ambientColor), diffuseColor(other.diffuseColor),
        specularColor(other.specularColor), collider(other.collider) {}

    GameObject& operator=(const GameObject& other) {
        if (this != &other) {
            position = other.position;
            size = other.size;
            velocity = other.velocity;
            rotation = other.rotation;
            texture = other.texture;
            ambientColor = other.ambientColor;
            diffuseColor = other.diffuseColor;
            specularColor = other.specularColor;
            collider = other.collider;
        }
        return *this;
    }

    virtual ~GameObject() = default;
};

class Bullet : public GameObject {
public:
    bool active = true;
    float lifetime = 3.0f; // Bullet lifetime in seconds

    Bullet(glm::vec3 pos, glm::vec3 sz, float colliderRadius)
        : GameObject(pos, sz, colliderRadius, 1, glm::vec3(0, 0, 1), glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), SPHERE) {}

    void update() override {
        if (lifetime > 0) {
            position += velocity * deltaTime;
            lifetime -= deltaTime;
        }
        else {
            // active = false;
        }
        handleWallCollision();
    }

    void handleWallCollision() {
        if (position.x <= bounds[0] || position.x >= bounds[1]) {
            velocity.x = -velocity.x;
        }
        if (position.z <= bounds[2] || position.z >= bounds[3]) {
            velocity.z = -velocity.z;
        }
    }

    Bullet(const Bullet& other)
        : GameObject(other), lifetime(other.lifetime) {}

    Bullet& operator=(const Bullet& other) {
        if (this != &other) {
            GameObject::operator=(other);
            lifetime = other.lifetime;
        }
        return *this;
    }
};

std::vector<Bullet> bullets;

class Player : public GameObject {
public:
    float speed = 5.0f;
    float bulletReloadSpeed = 0.2f;
    float reloadTimer = 0.0f;
    float bulletLength = 10.0f;
    bool active = true;

    Player(glm::vec3 pos, glm::vec3 sz, float colliderRadius)
        : GameObject(pos, sz, colliderRadius, 1, glm::vec3(0, 0, 0.2), glm::vec3(0, 0, 1), glm::vec3(1, 1, 0), SPHERE) {}

    void takeDamage(int damage) {
        playerHealth -= damage;
        if (playerHealth <= 0) {
            playerHealth = 0;
           
        }
    }

    void update() override {
        float moveAmount = speed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && position.z < bounds[3])
            position.z -= moveAmount;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && position.z > bounds[2])
            position.z += moveAmount;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && position.x > (bounds[0] + 0.2f))
            position.x -= moveAmount;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && position.x < (bounds[1] - 0.2f))
            position.x += moveAmount;

        reloadTimer += deltaTime;

        // Shoot bullet on mouse click
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            if (reloadTimer >= bulletReloadSpeed) {
                glm::vec3 mouseWorldPos = getMouseWorldPosition();
                glm::vec3 direction = mouseWorldPos - position;

                float lengthXZ = glm::sqrt(direction.x * direction.x + direction.z * direction.z);
				lengthXZ /= bulletLength;

                direction.x /= lengthXZ;
                direction.z /= lengthXZ;

                Bullet newBullet(position, glm::vec3(0.1f, 0.1f, 0.1f), 0.05f);
                newBullet.velocity = direction;
                bullets.push_back(newBullet);

                reloadTimer = 0.0f;
            }
        }
        if (playerHealth <= 0) {
            active = false;
            showGameOverScreen = true;
        }
    }

    glm::vec3 getMouseWorldPosition() {
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        glm::vec4 viewport = glm::vec4(0.0f, 0.0f, 1920.0f, 1080.0f);
        glm::vec3 winCoord = glm::vec3(x, 1080.0f - y, 0.0f);

        glm::vec3 objCoord = glm::unProject(winCoord, view, proj, viewport);
        return objCoord;
    }
};

class Enemy : public GameObject {
public:
    int health = 2;
    float speed = 3.0f;

    // Normal enemy is green
    Enemy(glm::vec3 pos, glm::vec3 sz, float colliderRadius)
        : GameObject(pos, sz, colliderRadius, 1, glm::vec3(0, 0.2, 0), glm::vec3(0, 1, 0), glm::vec3(1, 1, 0), SPHERE) {}

    void update() override {
        GameObject::update();
        glm::vec3 direction = playerPos - position;
        float length = glm::length(direction);
        if (length != 0) {
            direction /= length;
        }
        position += direction * speed * deltaTime;
        handleWallCollision();
    }

    void handleWallCollision() {
        if (position.x <= bounds[0] || position.x >= bounds[1]) {
            velocity.x = -velocity.x;
        }
        if (position.z <= bounds[2] || position.z >= bounds[3]) {
            velocity.z = -velocity.z;
        }
    }
};

class FastEnemy : public GameObject {
public:
    int health = 3;
    float speed = 5.0f;

    // Fast enemy is red
    FastEnemy(glm::vec3 pos, glm::vec3 sz, float colliderRadius)
        : GameObject(pos, sz, colliderRadius, 1, glm::vec3(0.2, 0, 0), glm::vec3(1, 0, 0), glm::vec3(1, 1, 0), SPHERE) {}

    void update() override {
        GameObject::update();
        glm::vec3 direction = playerPos - position;
        float length = glm::length(direction);
        if (length != 0) {
            direction /= length;
        }
        position += direction * speed * deltaTime;
        handleWallCollision();
    }

    void handleWallCollision() {
        if (position.x <= bounds[0] || position.x >= bounds[1]) {
            velocity.x = -velocity.x;
        }
        if (position.z <= bounds[2] || position.z >= bounds[3]) {
            velocity.z = -velocity.z;
        }
    }
};

std::vector<Enemy> enemies;
std::vector<FastEnemy> fastEnemies;

class Wall : public GameObject {
public:
    Wall(glm::vec3 pos, glm::vec3 sz)
        : GameObject(pos, sz, 1.0f, 1, glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), PRISM) {}
};

vector<Wall> walls;

void checkPlayerHealth() {
    // Suponiendo que 'player' es tu objeto jugador y tiene un atributo 'health'
    if (playerHealth <= 0) {
        showGameOverScreen = true;  // Activar la pantalla de Game Over
    }
}

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

    AddFace(a, p0, p1, p5, p4, normalFront, u, v, n);
    AddFace(a, p2, p3, p7, p6, normalBack, u, v, n);
    AddFace(a, p3, p0, p4, p7, normalLeft, u, v, n);
    AddFace(a, p1, p2, p6, p5, normalRight, u, v, n);
    AddFace(a, p3, p2, p6, p7, normalTop, u, v, n);
    AddFace(a, p0, p1, p5, p4, normalBottom, u, v, n);
}

void CreateCircle(vector<GLfloat>* vertices, float radius, int segments) {
    vertices->clear();
    GLfloat angleStep = 2.0f * M_PI / segments;

    // Center vertex
    glm::vec3 centerVertex = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);  // Normal vector for flat circle

    // Add center vertex
    AddVertex(vertices, centerVertex);
    AddVertex(vertices, normal);
    AddUV(vertices, glm::vec2(0.5f, 0.5f));  // Center vertex UV

    // Vertices around the circle
    for (int i = 0; i <= segments; ++i) {
        float angle = i * angleStep;
        float x = radius * cosf(angle);
        float z = radius * sinf(angle);

        glm::vec3 vertex = glm::vec3(x, 0.0f, z);
        glm::vec3 vertexNormal = normal;  // Same normal for all vertices

        AddVertex(vertices, vertex);
        AddVertex(vertices, vertexNormal);

        // Texture coordinates mapping (adjust as necessary)
        float textureX = 0.5f + 0.5f * cosf(angle);
        float textureY = 0.5f + 0.5f * sinf(angle);
        AddUV(vertices, glm::vec2(textureX, textureY));
    }
}


void BuildScene(float uu, float vv, int subdiv, int scene, Shape shape) {
    vector<GLfloat> v;

    if (shape == SPHERE) {
        CreateRevo(&v, uu, vv, subdiv, scene);
    }
    else if (shape == PRISM) {
        CreateRectPrism(&v, 10.0f, 10.0f, 10.0f, uu, vv, subdiv);
    }
    else if (shape == CIRCLE) {
        CreateCircle(&v, uu, subdiv);  // uu as radius and subdiv as segments
    }

    GLuint VAO, VBO;
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

    // Store the VAO and VBO for rendering later
    if (shape == SPHERE) {
        sphereVAO = VAO;
        sphereVBO = VBO;
    } else if (shape == PRISM) {
        rectPrismVAO = VAO;
        rectPrismVBO = VBO;
    } else if (shape == CIRCLE) {
        circleVAO = VAO;
        circleVBO = VBO;
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

    if (spawnTimer >= spawnInterval) {
        if (type == EnemyType::NORMAL) {
            Enemy newEnemy(glm::vec3(position.x, 0, position.y), glm::vec3(0.4f, 0.4f, 0.4f), 0.2f);
            enemies.push_back(newEnemy);
        }
        else if (type == EnemyType::FAST) {
            FastEnemy newEnemy(glm::vec3(position.x, 0, position.y), glm::vec3(0.4f, 0.4f, 0.4f), 0.2f);
            fastEnemies.push_back(newEnemy);
        }
        spawnTimer = 0.0f;
    }
}


bool CheckCollision(GameObject& one, GameObject& two) {
    // collision x-axis?
    bool collisionX = one.position.x + one.size.x >= two.position.x &&
        two.position.x + two.size.x >= one.position.x;
    // collision y-axis?
    bool collisionY = one.position.y + one.size.y >= two.position.y &&
        two.position.y + two.size.y >= one.position.y;
    // collision z-axis?
    bool collisionZ = one.position.z + one.size.z >= two.position.z &&
        two.position.z + two.size.z >= one.position.z;
    // collision only if on all three axes
    return collisionX && collisionY && collisionZ;
}

void CheckBulletEnemyCollisions() {
    for (auto& bullet : bullets) {
        if (!bullet.active) continue;

        for (auto& enemy : enemies) {
            if (CheckCollision(bullet, enemy)) {
                bullet.active = false;  // Deactivate the bullet
                enemy.health -= 1;  // Reduce enemy health
                std::cout << "Collision detected between bullet and enemy\n" << std::endl;

                if (enemy.health <= 0) {
                    std::cout << "Enemy killed!" << std::endl;
                }
                break;  // Exit the inner loop since this bullet is no longer active
            }
        }
    }

    // Remove inactive bullets
    bullets.erase(remove_if(bullets.begin(), bullets.end(), [](Bullet& b) { return !b.active; }), bullets.end());

    // Remove dead enemies
    enemies.erase(remove_if(enemies.begin(), enemies.end(), [](Enemy& e) { return e.health <= 0; }), enemies.end());
}

void CheckPlayerEnemyCollisions(Player& player) {
    for (auto& enemy : enemies) {
        if (CheckCollision(player, enemy)) {
            std::cout << "Colisión detectada entre el jugador y el enemigo!" << std::endl;

            // Desactivar al jugador (puedes establecer una bandera o mover al jugador fuera de la pantalla)
            player.active = false;  // Supongamos que añadimos una bandera 'active' a la clase Player
            player.position = glm::vec3(-1000.0f, -1000.0f, -1000.0f);  // Mover al jugador fuera de la pantalla

            // Opcional: manejar la "muerte" del enemigo
            enemy.health = 0;
            break;  // Salir del bucle ya que el jugador ha colisionado
        }
    }

    // Remover enemigos muertos
    enemies.erase(remove_if(enemies.begin(), enemies.end(), [](Enemy& e) { return e.health <= 0; }), enemies.end());
}

// TODO: Consider have each object have a check collision function

void CreateWalls()
{
    // Create walls on the edges of the scene, defined by the 'bounds' variable
    Wall wall1(glm::vec3(bounds[0], 0, bounds[0]), glm::vec3(1.0f, 5.0f, 10.0f));
    Wall wall2(glm::vec3(bounds[0], 0, bounds[0]), glm::vec3(10.0f, 5.0f, 1.0f));
    Wall wall3(glm::vec3(bounds[0], 0, bounds[1]), glm::vec3(10.0f, 5.0f, 1.0f));
    Wall wall4(glm::vec3(bounds[1], 0, bounds[0]), glm::vec3(1.0f, 5.0f, 10.0f));

    walls.push_back(wall1);
    walls.push_back(wall2);
    walls.push_back(wall3);
    walls.push_back(wall4);
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
    BuildScene(1, 1, subdivision, scene, PRISM);
    BuildScene(1, 1, subdivision, scene, CIRCLE);
    BuildScene(1, 1, subdivision, scene, SPHERE);


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

    CreateWalls();

    Player player(glm::vec3(0, 0, 0), glm::vec3(0.4f, 0.4f, 0.4f), 0.2f);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

         for (int i = 0; i < walls.size(); i++) {
             walls[i].render();
         }

        currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        SpawnEnemy(glm::vec2(2, 2), 12.0f, EnemyType::NORMAL);

        glm::vec4 lPos(100, 100, 100, 100);
        glm::vec3 lightColor(1, 1, 0);

        glUniform4fv(lPosParameter, 1, glm::value_ptr(lPos));
        glUniform3fv(laParameter, 1, glm::value_ptr(lightColor * 1.0f));
        glUniform3fv(ldParameter, 1, glm::value_ptr(lightColor * 1.0f));
        glUniform3fv(lsParameter, 1, glm::value_ptr(lightColor * 1.0f));

        playerPos = player.position;
        player.update();
        player.render();

        for (int i = 0; i < enemies.size(); i++) {
            enemies[i].update();
            enemies[i].render();
        }

        for (int i = 0; i < fastEnemies.size(); i++) {
            fastEnemies[i].update();
            fastEnemies[i].render();
        }

        for (int i = 0; i < bullets.size(); i++) {
            bullets[i].update();
            bullets[i].render();
        }

        CheckBulletEnemyCollisions();
        CheckPlayerEnemyCollisions(player);

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
