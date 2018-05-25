#include "utils.h"
#include "utils2.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include <iostream>
#include <cstdlib>
#include <algorithm>

#include <random>

#define PI 3.1415926535897932384626433832795
#define MAX_PARTICLES 10000
#define STRETCH 0.1f

using namespace std;
using namespace glm;

// The attribute locations we will use in the vertex shader
enum AttributeLocation {
    INSTANCE,
    POSITION,
    SIZE,
    LIFE,
    COLOUR,
    NUM_ATTRIBUTES
};

struct Particle {
    vec3 pos;
    vec3 speed;
    uvec4 color;
    float size;
    float angle;
    float weight;
    float life;
	float cameraDistance;
    bool operator<(const Particle& that) const {
        // Sort in reverse order : far particles drawn first.
        return this->cameraDistance > that.cameraDistance;
    }
};

struct Particles {
    Particle container[MAX_PARTICLES];

    // Buffer identifiers
    GLuint billboardBuffer;
    GLuint positionsBuffer;
    GLuint sizesBuffer;
    GLuint livesBuffer;
    GLuint coloursBuffer;

    // Data for buffers
    GLfloat positionsData[3*MAX_PARTICLES];
    GLfloat sizesData[MAX_PARTICLES];
    GLfloat livesData[MAX_PARTICLES];
    GLubyte coloursData[4*MAX_PARTICLES];

    int lastUsedParticle;
    int numParticles;
};

// Struct for resources and state
struct Context {
    mt19937 eng;
    uniform_int_distribution<> rand255;
    int width;
    int height;
    float aspect;
    vec3 cameraPos;
    GLFWwindow *window;
    GLuint program;
    Trackball trackball;
    GLuint vao;
    Particles *particles;
    float elapsed_time;
    float timeDelta;
    float zoom;
	float max_life;
	float min_life;
	float spread;
	float max_speed;
	float min_speed;
    
};

// Returns the value of an environment variable
std::string getEnvVar(const std::string &name)
{
    char const* value = std::getenv(name.c_str());
    if (value == nullptr) {
        return std::string();
    }
    else {
        return std::string(value);
    }
}

// Returns the absolute path to the shader directory
std::string shaderDir(void)
{
    std::string rootDir = getEnvVar("ASSIGNMENT3_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT3_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/particles/src/shaders/";
}

void initializeTrackball(Context &ctx)
{
    double radius = double(std::min(ctx.width, ctx.height)) / 2.0;
    ctx.trackball.radius = radius;
    glm::vec2 center = glm::vec2(ctx.width, ctx.height) / 2.0f;
    ctx.trackball.center = center;
}

void initParticles(Context *ctx)
{
    Particles *particles = new Particles();
    if (particles == NULL) {
        fprintf(stderr, "Failed to allocate %x bytes of memory. Exiting.", sizeof(struct Particles));
        exit(-1);
    }

    // A quad
    static const GLfloat vertices[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        -0.5f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.0f
    };

    GLuint billboard;
    GLuint positions;
    GLuint sizes;
    GLuint lives;
    GLuint colours;

    // The billboard quad. This is done only once
    glGenBuffers(1, &billboard);
    particles->billboardBuffer = billboard;

    glBindBuffer(GL_ARRAY_BUFFER, billboard);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


    // The positions of the particles
    glGenBuffers(1, &positions);
    particles->positionsBuffer = positions;

    glBindBuffer(GL_ARRAY_BUFFER, positions);
    glBufferData(GL_ARRAY_BUFFER, 3 * MAX_PARTICLES * sizeof(GLfloat), NULL, GL_STREAM_DRAW);


    // The sizes of the particles
    glGenBuffers(1, &sizes);
    particles->sizesBuffer = sizes;

    glBindBuffer(GL_ARRAY_BUFFER, sizes);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(GLfloat), NULL, GL_STREAM_DRAW);


    // The lives of the particles
    glGenBuffers(1, &lives);
    particles->livesBuffer = lives;

    glBindBuffer(GL_ARRAY_BUFFER, lives);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(GLfloat), NULL, GL_STREAM_DRAW);


    // The colours of the particles
    glGenBuffers(1, &colours);
    particles->coloursBuffer = colours;

    glBindBuffer(GL_ARRAY_BUFFER, colours);
    glBufferData(GL_ARRAY_BUFFER, 4 * MAX_PARTICLES * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

    particles->numParticles = 0;
    particles->lastUsedParticle = 0;

    for(int i=0; i<MAX_PARTICLES; i++){
        particles->container[i].life = -1.0f;
        particles->container[i].cameraDistance = -1.0f;
    }

    ctx->particles = particles;
}

void init(Context &ctx)
{
    ctx.program = loadShaderProgram(shaderDir() + "particle.vert",
                                    shaderDir() + "particle.frag");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenVertexArrays(1, &(ctx.vao));
    glBindVertexArray(ctx.vao);

    initParticles(&ctx);

    initializeTrackball(ctx);
}

void sendUniforms(Context *ctx)
{
    // Identifiers for the uniform variables
    GLuint camera_up_id = glGetUniformLocation(ctx->program, "camera_up");
    GLuint camera_right_id = glGetUniformLocation(ctx->program, "camera_right");
    GLuint vp_id = glGetUniformLocation(ctx->program, "vp");
    GLuint max_life_id = glGetUniformLocation(ctx->program, "max_life");

    vec3 centre = vec3(0.0f, 0.0f, 0.2f);
    vec3 cameraPos = ctx->cameraPos;
    float fov = 0.5f;

    //mat4 trackball = trackballGetRotationMatrix(ctx->trackball);
    mat4 view = lookAt(cameraPos, centre, vec3(0.0f,0.0f,1.0f));
    mat4 projection = perspective(fov * ctx->zoom, ctx->aspect, 0.1f, 100.0f);
    mat4 vp = projection * view;

    // Camera-local directions for billboarding
    vec3 camera_up = vec3(view[0][1], view[1][1], view[2][1]);
    vec3 camera_right = vec3(view[0][0], view[1][0], view[2][0]);

    // Set uniforms
    glUniform1f(max_life_id, ctx->max_life * STRETCH);
    glUniform3fv(camera_up_id, 1, &camera_up[0]);
    glUniform3fv(camera_right_id, 1, &camera_right[0]);
    glUniformMatrix4fv(vp_id, 1, GL_FALSE, &vp[0][0]);
}

void drawParticles(Context *ctx)
{
    // Particle data
    Particles *particles = ctx->particles;
    GLuint billboard = particles->billboardBuffer;
    GLuint positions = particles->positionsBuffer;
    GLuint sizes = particles->sizesBuffer;
    GLuint lives = particles->livesBuffer;
    GLuint colours = particles->coloursBuffer;
    int numParticles = particles->numParticles;

    // Update particle positions
    glBindBuffer(GL_ARRAY_BUFFER, positions);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 3 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, numParticles * sizeof(GLfloat) * 3, particles->positionsData);

    // Update particle sizes
    glBindBuffer(GL_ARRAY_BUFFER, sizes);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, numParticles * sizeof(GLfloat), particles->sizesData);

    // Update particle lives
    glBindBuffer(GL_ARRAY_BUFFER, lives);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, numParticles * sizeof(GLfloat), particles->livesData);

    // Update particle colours
    glBindBuffer(GL_ARRAY_BUFFER, colours);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 3 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, numParticles * sizeof(GLubyte) * 4, particles->coloursData);


    // Attach billboard corners to the vertices
    glEnableVertexAttribArray(INSTANCE);
    glBindBuffer(GL_ARRAY_BUFFER, billboard);
    glVertexAttribPointer(INSTANCE, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Attach particle positions to the vertices
    glEnableVertexAttribArray(POSITION);
    glBindBuffer(GL_ARRAY_BUFFER, positions);
    glVertexAttribPointer(POSITION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Attach particle sizes to the vertices
    glEnableVertexAttribArray(SIZE);
    glBindBuffer(GL_ARRAY_BUFFER, sizes);
    glVertexAttribPointer(SIZE, 1, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Attach particle lives to the vertices
    glEnableVertexAttribArray(LIFE);
    glBindBuffer(GL_ARRAY_BUFFER, lives);
    glVertexAttribPointer(LIFE, 1, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Attach colours to the vertices
    glEnableVertexAttribArray(COLOUR);
    glBindBuffer(GL_ARRAY_BUFFER, colours);
    glVertexAttribPointer(COLOUR, 4, GL_UNSIGNED_SHORT, GL_TRUE, 0, nullptr);

    // The billboard is the same for each particle
    glVertexAttribDivisor(INSTANCE,0);
    // The particle position advance for each particle
    glVertexAttribDivisor(POSITION,1);
    // The particle sized advance for each particle
    glVertexAttribDivisor(SIZE,1);
    // The particle sized advance for each particle
    glVertexAttribDivisor(LIFE,1);
    // The particle colours advance for each particle
    glVertexAttribDivisor(COLOUR,1);

    // Draw all particle instances
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numParticles);

    glDisableVertexAttribArray(POSITION);
    glDisableVertexAttribArray(INSTANCE);
    glDisableVertexAttribArray(COLOUR);
    glDisableVertexAttribArray(SIZE);
    glDisableVertexAttribArray(LIFE);
}

void display(Context *ctx)
{
    glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(ctx->program);

    sendUniforms(ctx);

    drawParticles(ctx);
}

//Format is label, variable, step when draggin, minimum draggable value, maximum draggable value
//Bug: If both of a pair are maximized you can drag the max one to any value instead, vice versa if both are minimum
//		putting a max lower than a min or vice versa crashes the program
void gui(Context *ctx)
{
	ImGui::DragFloat("Max life", &ctx->max_life, 0.1f, ctx->min_life, 15.0f);
	ImGui::DragFloat("Min life", &ctx->min_life, 0.1f, 0.0f, ctx->max_life);
	ImGui::DragFloat("Spread", &ctx->spread, 0.1f, 0.0f, 3.0f);
	ImGui::DragFloat("Max speed", &ctx->max_speed, 0.1f, ctx->min_speed, 10.0f);
	ImGui::DragFloat("Min speed", &ctx->min_speed, 0.1f, 0.0f, ctx->max_speed);
}


void reloadShaders(Context *ctx)
{
    glDeleteProgram(ctx->program);
    ctx->program = loadShaderProgram(shaderDir() + "particle.vert",
                                     shaderDir() + "particle.frag");
}

void mouseButtonPressed(Context *ctx, int button, int x, int y)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        ctx->trackball.center = glm::vec2(x, y);
        trackballStartTracking(ctx->trackball, glm::vec2(x, y));
    }
}

void mouseButtonReleased(Context *ctx, int button, int x, int y)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        trackballStopTracking(ctx->trackball);
    }
}

void moveTrackball(Context *ctx, int x, int y)
{
    if (ctx->trackball.tracking) {
        trackballMove(ctx->trackball, glm::vec2(x, y));
    }
}

void errorCallback(int /*error*/, const char* description)
{
    std::cerr << description << std::endl;
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    ctx->zoom -= yoffset*0.05;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Forward event to GUI
    ImGui_ImplGlfwGL3_KeyCallback(window, key, scancode, action, mods);
    if (ImGui::GetIO().WantCaptureKeyboard) { return; }  // Skip other handling

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        reloadShaders(ctx);
    }
}

void charCallback(GLFWwindow* window, unsigned int codepoint)
{
    // Forward event to GUI
    ImGui_ImplGlfwGL3_CharCallback(window, codepoint);
    if (ImGui::GetIO().WantTextInput) { return; }  // Skip other handling
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    // Forward event to GUI
    ImGui_ImplGlfwGL3_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse) { return; }  // Skip other handling

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS) {
        mouseButtonPressed(ctx, button, x, y);
    }
    else {
        mouseButtonReleased(ctx, button, x, y);
    }
}

void cursorPosCallback(GLFWwindow* window, double x, double y)
{
    if (ImGui::GetIO().WantCaptureMouse) { return; }  // Skip other handling   

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    moveTrackball(ctx, x, y);
}

void resizeCallback(GLFWwindow* window, int width, int height)
{
    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    ctx->width = width;
    ctx->height = height;
    ctx->aspect = float(width) / float(height);
    ctx->trackball.radius = double(std::min(width, height)) / 2.0;
    ctx->trackball.center = glm::vec2(width, height) / 2.0f;
    glViewport(0, 0, width, height);
}

int findUnusedParticle(Particles *particles)
{
    int lastUsedParticle = particles->lastUsedParticle;
    Particle *container = particles->container;

    for(int i=lastUsedParticle; i<MAX_PARTICLES; i++){
        if (container[i].life < 0){
            particles->lastUsedParticle = i;
            return i;
        }
    }

    for(int i=0; i<lastUsedParticle; i++){
        if (container[i].life < 0){
            particles->lastUsedParticle = i;
            return i;
        }
    }

    return 0;
}

void sortParticles(Particles *particles)
{
    //Particle *container = particles->container;
    std::sort(particles->container, &(particles->container[MAX_PARTICLES]));
}

void updateParticleData(Particles *particles)
{
    GLfloat *positionsData = particles->positionsData;
    GLfloat *sizesData = particles->sizesData;
    GLfloat *livesData = particles->livesData;
    GLubyte *coloursData = particles->coloursData;

    Particle *container = particles->container;

    int numParticles = particles->numParticles;

    // Update buffers
    for (int i = 0; i < numParticles; i++){

        Particle& p = container[i];

        positionsData[3*i+0] = p.pos.x;
        positionsData[3*i+1] = p.pos.y;
        positionsData[3*i+2] = p.pos.z;

        sizesData[i] = p.size;

        livesData[i] = p.life;

        coloursData[4*i+0] = p.color.r;
        coloursData[4*i+1] = p.color.g;
        coloursData[4*i+2] = p.color.b;
        coloursData[4*i+3] = p.color.a;
    }
}

void simulateParticles(Context *ctx)
{
    Particles *particles = ctx->particles;
    Particle *container = particles->container;
    float delta = ctx->timeDelta * STRETCH;

    vec3 cameraPos = ctx->cameraPos;

    // Uniform distributions for random properties
    mt19937 eng = ctx->eng;
    uniform_real_distribution<> azimuth(0,2*PI);
    uniform_real_distribution<> polar(0,ctx->spread);
    uniform_real_distribution<> speed(ctx->min_speed,ctx->max_speed);

    uniform_real_distribution<> rlife(ctx->min_life, ctx->max_life);

    // Spawn 10 particles per millisecond
    int newparticles = (int)(delta*10000.0);
    if (newparticles > (int)(0.016f*10000.0))
        newparticles = (int)(0.016f*10000.0);

    for(int i=0; i<newparticles; i++){
        int particleIndex = findUnusedParticle(particles);

        Particle &p = container[particleIndex];

        p.life = rlife(eng)*STRETCH;
        p.pos = glm::vec3(0,0.0f,0.0f);

        float phi = azimuth(eng);
        float theta = polar(eng);
        float r = speed(eng);

        float vx = r * sin(theta) * cos(phi);
        float vy = r * sin(theta) * sin(phi);
        float vz = r * cos(theta);

        p.speed =  vec3(vx, vy, vz);


        // Very bad way to generate a random color
        p.color.r = ctx->rand255(ctx->eng);
        p.color.g = ctx->rand255(ctx->eng);
        p.color.b = ctx->rand255(ctx->eng);
        p.color.a = (ctx->rand255(ctx->eng) % 256) / 3;

        p.size = 0.1f;
    }

    int numParticles = 0;

    // Simulate
    for (int i = 0; i < MAX_PARTICLES; i++) {

        Particle& p = container[i];

        if (p.life > 0.0f) {

            p.life -= delta;

            // Normalized age
            float age = (ctx->max_life - p.life)/ctx->max_life;

            vec3 wind = vec3(0.0f, 0.1f, 0.0f) * age;

            p.speed += glm::vec3(0.0f, 0.0f, -9.81f) * (float)delta * 0.5f;
            p.pos += (p.speed + wind) * (float)delta;
            p.cameraDistance = glm::length2(p.pos - cameraPos);

            numParticles++;

        } else {
            p.cameraDistance = -1.0f;
        }
    }

    particles->numParticles = numParticles;

    // Sort particles by camera distance for correct blending
    sortParticles(particles);

    updateParticleData(particles);
}

int main(void)
{
    random_device rd;
    mt19937 eng(rd());
    std::uniform_int_distribution<> rand255(0, 255); // define the range

    Context ctx;

    // Create a GLFW window
    glfwSetErrorCallback(errorCallback);

    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    ctx.width = 500;
    ctx.height = 500;
    ctx.aspect = float(ctx.width) / float(ctx.height);
    ctx.window = glfwCreateWindow(ctx.width, ctx.height, "Particles", nullptr, nullptr);
    ctx.zoom = 0.3f;
    ctx.timeDelta = 0.016f;
    ctx.cameraPos = glm::vec3(4.0f,0.0f,0.0f);
    ctx.eng = eng;
    ctx.rand255 = rand255;
	ctx.max_life = 5.0f;
	ctx.min_life = 3.0f;
	ctx.spread = 0.2f;
	ctx.max_speed = 2.0f;
	ctx.min_speed = 1.0f;

    glfwMakeContextCurrent(ctx.window);
    glfwSetWindowUserPointer(ctx.window, &ctx);

    glfwSetKeyCallback(ctx.window, keyCallback);
    glfwSetCharCallback(ctx.window, charCallback);
    glfwSetMouseButtonCallback(ctx.window, mouseButtonCallback);
    glfwSetCursorPosCallback(ctx.window, cursorPosCallback);
    glfwSetFramebufferSizeCallback(ctx.window, resizeCallback);
    glfwSetScrollCallback(ctx.window, scrollCallback);

    // Load OpenGL functions
    glewExperimental = true;
    GLenum status = glewInit();
    if (status != GLEW_OK) {
        std::cerr << "Error: " << glewGetErrorString(status) << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    // Initialize GUI
    ImGui_ImplGlfwGL3_Init(ctx.window, false /*do not install callbacks*/);

    init(ctx);

    // Start rendering loop
    while (!glfwWindowShouldClose(ctx.window)) {
        glfwPollEvents();
        float newTime = glfwGetTime();
        float timeDelta = newTime - ctx.elapsed_time;
        ctx.elapsed_time = newTime;
        ctx.timeDelta = timeDelta;
        ImGui_ImplGlfwGL3_NewFrame();

		gui(&ctx);

        simulateParticles(&ctx);

        display(&ctx);

        ImGui::Render();
        glfwSwapBuffers(ctx.window);
    }

    // Shutdown
    delete ctx.particles;
    glfwDestroyWindow(ctx.window);
    glfwTerminate();
    std::exit(EXIT_SUCCESS);
}
