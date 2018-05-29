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
    INIT_LIFE,
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
    float initLife;
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
    GLuint initLivesBuffer;
    GLuint coloursBuffer;

    // Data for buffers
    GLfloat positionsData[3*MAX_PARTICLES];
    GLfloat sizesData[MAX_PARTICLES];
    GLfloat livesData[MAX_PARTICLES];
    GLfloat initLivesData[MAX_PARTICLES];
    GLubyte coloursData[4*MAX_PARTICLES];

    int lastUsedParticle;
    int numParticles;
    float spawnRate;
    float initColour[4];
    float finalColour[4];
    float initSize;
    float finalSize;
    float initFuzz;
    float finalFuzz;
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
    bool showQuads;
    bool sortParticles;
    float clearColor[3];
    float alpha;
    float gravity;
    float wind;
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
    std::string rootDir = getEnvVar("PROJECT_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: PROJECT_ROOT is not set." << std::endl;
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

void resetParticles(Particles *particles) {
    for(int i=0; i<MAX_PARTICLES; i++){
        particles->container[i].life = -1.0f;
        particles->container[i].cameraDistance = -1.0f;
    }
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
    GLuint initLives;

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


    // The init life of the particles
    glGenBuffers(1, &initLives);
    particles->initLivesBuffer = initLives;

    glBindBuffer(GL_ARRAY_BUFFER, initLives);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(GLfloat), NULL, GL_STREAM_DRAW);


    // The colours of the particles
    glGenBuffers(1, &colours);
    particles->coloursBuffer = colours;

    glBindBuffer(GL_ARRAY_BUFFER, colours);
    glBufferData(GL_ARRAY_BUFFER, 4 * MAX_PARTICLES * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

    particles->numParticles = 0;
    particles->lastUsedParticle = 0;

    resetParticles(particles);

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

void trackCamera(Context *ctx, vec3 centre) {

    vec3 &cameraPos = ctx->cameraPos;
    mat4 t = translate(glm::mat4(1), -centre);
    mat4 r = trackballGetRotationMatrix(ctx->trackball) * t;
    mat4 tot = translate(r, centre);

    vec4 cameraPosHom = vec4(cameraPos[0], cameraPos[1], cameraPos[2], 1.0f);

    cameraPosHom = tot * cameraPosHom;
    cameraPos[0] = cameraPosHom[0];
    cameraPos[1] = cameraPosHom[1];
    cameraPos[2] = cameraPosHom[2];
    cameraPos /= cameraPosHom[3];
}


void sendUniforms(Context *ctx)
{
    // Identifiers for the uniform variables
    GLuint camera_up_id = glGetUniformLocation(ctx->program, "camera_up");
    GLuint camera_right_id = glGetUniformLocation(ctx->program, "camera_right");
    GLuint vp_id = glGetUniformLocation(ctx->program, "vp");

    GLuint show_quads_id = glGetUniformLocation(ctx->program, "show_quads");
    GLuint alpha_id = glGetUniformLocation(ctx->program, "alpha");

    GLuint init_col_id = glGetUniformLocation(ctx->program, "init_col");
    GLuint final_col_id = glGetUniformLocation(ctx->program, "final_col");

    GLuint init_size_id = glGetUniformLocation(ctx->program, "init_size");
    GLuint final_size_id = glGetUniformLocation(ctx->program, "final_size");

    GLuint init_fuzz_id = glGetUniformLocation(ctx->program, "init_fuzz");
    GLuint final_fuzz_id = glGetUniformLocation(ctx->program, "final_fuzz");

    vec3 centre = vec3(0.0f, 0.0f, 0.2f);

    //trackCamera(ctx, centre);
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
    glUniform3fv(camera_up_id, 1, &camera_up[0]);
    glUniform3fv(camera_right_id, 1, &camera_right[0]);
    glUniformMatrix4fv(vp_id, 1, GL_FALSE, &vp[0][0]);
    glUniform1i(show_quads_id, ctx->showQuads);
    glUniform1f(alpha_id, ctx->alpha);

    glUniform1f(init_size_id, ctx->particles->initSize);
    glUniform1f(final_size_id, ctx->particles->finalSize);

    glUniform1f(init_fuzz_id, ctx->particles->initFuzz);
    glUniform1f(final_fuzz_id, ctx->particles->finalFuzz);

    glUniform4fv(init_col_id, 1, ctx->particles->initColour);
    glUniform4fv(final_col_id, 1, ctx->particles->finalColour);
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
    GLuint initLives = particles->initLivesBuffer;
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

    // Update particle init lives
    glBindBuffer(GL_ARRAY_BUFFER, initLives);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, numParticles * sizeof(GLfloat), particles->initLivesData);

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

    // Attach particle initial lives to the vertices
    glEnableVertexAttribArray(INIT_LIFE);
    glBindBuffer(GL_ARRAY_BUFFER, initLives);
    glVertexAttribPointer(INIT_LIFE, 1, GL_FLOAT, GL_FALSE, 0, nullptr);

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
    // The particle life advance for each particle
    glVertexAttribDivisor(LIFE,1);
    // The particle initial life advance for each particle
    glVertexAttribDivisor(INIT_LIFE,1);
    // The particle colours advance for each particle
    glVertexAttribDivisor(COLOUR,1);

    // Draw all particle instances
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numParticles);

    glDisableVertexAttribArray(POSITION);
    glDisableVertexAttribArray(INSTANCE);
    glDisableVertexAttribArray(COLOUR);
    glDisableVertexAttribArray(SIZE);
    glDisableVertexAttribArray(LIFE);
    glDisableVertexAttribArray(INIT_LIFE);
}

void display(Context *ctx)
{
    glClearColor(ctx->clearColor[0], ctx->clearColor[1], ctx->clearColor[2], 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(ctx->program);

    sendUniforms(ctx);

    drawParticles(ctx);
}

void presetFountain(Context *ctx)
{
    ctx->max_life = 4.0f;
    ctx->min_life = 3.0f;

    ctx->spread = 0.2f;

    ctx->min_speed = 1.0f;
    ctx->max_speed = 2.0f;

    ctx->showQuads = false;
    ctx->sortParticles = true;

    ctx->clearColor[0] = 0.2;
    ctx->clearColor[1] = 0.2;
    ctx->clearColor[2] = 0.2;

    ctx->alpha = 1.0f;

    ctx->particles->spawnRate = 10.0f;

    ctx->gravity = -9.82f;
    ctx->wind = 0.2f;

    ctx->particles->initColour[0] = 102.0f/255.0f;
    ctx->particles->initColour[1] = 141.0f/255.0f;
    ctx->particles->initColour[2] = 181.0f/255.0f;
    ctx->particles->initColour[3] = 25.0f/255.0f;

    ctx->particles->finalColour[0] = 197.0f/255.0f;
    ctx->particles->finalColour[1] = 231.0f/255.0f;
    ctx->particles->finalColour[2] = 1.0f;
    ctx->particles->finalColour[3] = 0.0f;

    ctx->particles->initSize = 0.01f;
    ctx->particles->finalSize = 0.1f;

    ctx->particles->initFuzz = 0.0f;
    ctx->particles->finalFuzz = 0.0f;
}

void presetSmoke(Context *ctx)
{
    ctx->max_life = 7.0f;
    ctx->min_life = 4.0f;

    ctx->spread = PI;

    ctx->min_speed = 0.0f;
    ctx->max_speed = 0.3f;

    ctx->showQuads = false;
    ctx->sortParticles = true;

    ctx->clearColor[0] = 0.2;
    ctx->clearColor[1] = 0.2;
    ctx->clearColor[2] = 0.2;

    ctx->alpha = 1.0f;

    ctx->particles->spawnRate = 10.0f;

    ctx->gravity = 3.5f;
    ctx->wind = -0.2f;

    ctx->particles->initColour[0] = 145.0f/255.0f;
    ctx->particles->initColour[1] = 145.0f/255.0f;
    ctx->particles->initColour[2] = 145.0f/255.0f;
    ctx->particles->initColour[3] = 70.0f/255.0f;

    ctx->particles->finalColour[0] = 51.0f/255.0f;
    ctx->particles->finalColour[1] = 51.0f/255.0f;
    ctx->particles->finalColour[2] = 51.0f/255.0f;
    ctx->particles->finalColour[3] = 10.0f/255.0f;

    ctx->particles->initSize = 0.0f;
    ctx->particles->finalSize = 0.2f;

    ctx->particles->initFuzz = 0.5f;
    ctx->particles->finalFuzz = 0.9f;
}

void presetToonTorch(Context *ctx)
{
    ctx->max_life = 5.0f;
    ctx->min_life = 3.0f;

    ctx->spread = 0.2f;

    ctx->min_speed = 0.611f;
    ctx->max_speed = 1.137f;

    ctx->showQuads = false;
    ctx->sortParticles = true;

    ctx->clearColor[0] = 0.2;
    ctx->clearColor[1] = 0.2;
    ctx->clearColor[2] = 0.2;

    ctx->alpha = 1.0f;

    ctx->particles->spawnRate = 2.0f;

    ctx->gravity = -1.0f;
    ctx->wind = 0.0f;

    ctx->particles->initColour[0] = 200.0f/255.0f;
    ctx->particles->initColour[1] = 0.0f;
    ctx->particles->initColour[2] = 0.0f;
    ctx->particles->initColour[3] = 1.0f;

    ctx->particles->finalColour[0] = 0.0f;
    ctx->particles->finalColour[1] = 0.0f;
    ctx->particles->finalColour[2] = 0.0f;
    ctx->particles->finalColour[3] = 1.0f;

    ctx->particles->initSize = 0.2f;
    ctx->particles->finalSize = 0.0f;

    ctx->particles->initFuzz = 0.0f;
    ctx->particles->finalFuzz = 0.0f;
}

void presetComet(Context *ctx)
{
    ctx->max_life = 0.873f;
    ctx->min_life = 4.128f;

    ctx->spread = 1.269f;

    ctx->min_speed = 0.0f;
    ctx->max_speed = 0.718f;

    ctx->showQuads = false;
    ctx->sortParticles = true;

    ctx->clearColor[0] = 0.2;
    ctx->clearColor[1] = 0.2;
    ctx->clearColor[2] = 0.2;

    ctx->alpha = 1.0f;

    ctx->particles->spawnRate = 20.0f;

    ctx->gravity = 20.0f;
    ctx->wind = 0.0f;

    ctx->particles->initColour[0] = 134.0f/255.0f;
    ctx->particles->initColour[1] = 1.0f;
    ctx->particles->initColour[2] = 0.5f;
    ctx->particles->initColour[3] = 28.0f/255.0f;

    ctx->particles->finalColour[0] = 0.0f;
    ctx->particles->finalColour[1] = 91.0f/255.0f;
    ctx->particles->finalColour[2] = 9.0f/255.0f;
    ctx->particles->finalColour[3] = 99.0f/255.0f;

    ctx->particles->initSize = 0.047f;
    ctx->particles->finalSize = 0.0f;

    ctx->particles->initFuzz = 0.5f;
    ctx->particles->finalFuzz = 0.0f;
}

void presetFire(Context *ctx)
{
    ctx->max_life = 5.0f;
    ctx->min_life = 3.0f;

    ctx->spread = 0.2f;

    ctx->min_speed = 0.611f;
    ctx->max_speed = 1.137f;

    ctx->showQuads = false;
    ctx->sortParticles = true;

    ctx->clearColor[0] = 0.2;
    ctx->clearColor[1] = 0.2;
    ctx->clearColor[2] = 0.2;

    ctx->alpha = 1.0f;

    ctx->particles->spawnRate = 10.0f;

    ctx->gravity = -2.388;
    ctx->wind = 0.1f;

    ctx->particles->initColour[0] = 1.0f;
    ctx->particles->initColour[1] = 131.0f/255.0f;
    ctx->particles->initColour[2] = 64.0f/255.0f;
    ctx->particles->initColour[3] = 47.0f/255.0f;

    ctx->particles->finalColour[0] = 0.0f;
    ctx->particles->finalColour[1] = 0.0f;
    ctx->particles->finalColour[2] = 0.0f;
    ctx->particles->finalColour[3] = 7.0f/255.0f;

    ctx->particles->initSize = 0.01f;
    ctx->particles->finalSize = 0.1f;

    ctx->particles->initFuzz = 0.05f;
    ctx->particles->finalFuzz = 0.4f;
}

void gui(Context *ctx)
{
    ImGui::Begin("Rendering options");

    ImGui::Text("Spawn");

    ImGui::SliderFloat("Particles per ms", &ctx->particles->spawnRate, 0.0f, 20.0f);

    ImGui::SliderFloat("Min life", &ctx->min_life, 0.0f, ctx->max_life);
    ImGui::SliderFloat("Max life", &ctx->max_life, ctx->min_life, 7.0f);

    ImGui::SliderFloat("Spread", &ctx->spread, 0.0f, PI);

    ImGui::SliderFloat("Min speed", &ctx->min_speed, 0.0f, ctx->max_speed);
    ImGui::SliderFloat("Max speed", &ctx->max_speed, ctx->min_speed, 7.0f);

    ImGui::Text("Colour");

    ImGui::ColorEdit4("Initial colour", ctx->particles->initColour);
    ImGui::ColorEdit4("Final colour", ctx->particles->finalColour);

    ImGui::SliderFloat("Initial fuzziness", &ctx->particles->initFuzz, 0.0f, 1.0f);
    ImGui::SliderFloat("Final fuzziness", &ctx->particles->finalFuzz, 0.0f, 1.0f);

    ImGui::Text("Size");

    ImGui::SliderFloat("Initial size", &ctx->particles->initSize, 0.0f, 0.2f);
    ImGui::SliderFloat("Final size", &ctx->particles->finalSize, 0.0f, 0.2f);

    ImGui::Text("Physics");

    ImGui::SliderFloat("Gravity", &ctx->gravity, -20.0f, 20.0f);

    ImGui::SliderFloat("Wind", &ctx->wind, -0.5f, 0.5f);

    ImGui::Text("Presets");

    if (ImGui::Button("Realistic fire")) {
        resetParticles(ctx->particles);
        presetFire(ctx);
    }

    if (ImGui::Button("Toon torch")) {
        resetParticles(ctx->particles);
        presetToonTorch(ctx);
    }

    if (ImGui::Button("Fountain")) {
        resetParticles(ctx->particles);
        presetFountain(ctx);
    }

    if (ImGui::Button("Green comet")) {
        resetParticles(ctx->particles);
        presetComet(ctx);
    }

    if (ImGui::Button("Smoke")) {
        resetParticles(ctx->particles);
        presetSmoke(ctx);
    }

    if (ImGui::CollapsingHeader("Debug")) {
        ImGui::SliderFloat("Alpha", &ctx->alpha, 0.0f, 1.0f);

        ImGui::ColorEdit3("Background", ctx->clearColor);

        ImGui::Checkbox("Show quads", &ctx->showQuads);

        if (ImGui::Button("Reset simulation")) {
            resetParticles(ctx->particles);
        }

    }

    ImGui::Text("Live particles: %6d of %d", ctx->particles->numParticles, MAX_PARTICLES);
    ImGui::Text("Frame rate: %.0f fps", std::trunc(1.0f/ctx->timeDelta));

    ImGui::End();
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
    if (ImGui::GetIO().WantCaptureMouse) {
      // Skip other handling
      return;
    }

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

    return -1;
}

void sortParticles(Particles *particles)
{
    std::sort(particles->container, &(particles->container[MAX_PARTICLES]));
}

void updateParticleData(Particles *particles, float delta)
{
    GLfloat *positionsData = particles->positionsData;
    GLfloat *sizesData = particles->sizesData;
    GLfloat *livesData = particles->livesData;
    GLfloat *initLivesData = particles->initLivesData;
    GLubyte *coloursData = particles->coloursData;

    Particle *container = particles->container;

    int numParticles = particles->numParticles;

    // Update buffers
    int processed = 0;
    for (int i = 0; processed < numParticles; i++){

        Particle& p = container[i];

        if (p.life > 0.0f - delta) {

            positionsData[3*processed+0] = p.pos.x;
            positionsData[3*processed+1] = p.pos.y;
            positionsData[3*processed+2] = p.pos.z;

            sizesData[processed] = p.size;

            livesData[processed] = p.life;

            initLivesData[processed] = p.initLife;

            coloursData[4*processed+0] = p.color.r;
            coloursData[4*processed+1] = p.color.g;
            coloursData[4*processed+2] = p.color.b;
            coloursData[4*processed+3] = p.color.a;

            processed++;
        }
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
    uniform_real_distribution<> azimuth(0, 2*PI);
    uniform_real_distribution<> polar(0, ctx->spread);
    uniform_real_distribution<> speed(glm::min(ctx->min_speed, ctx->max_speed),
                                      glm::max(ctx->min_speed, ctx->max_speed));
    uniform_real_distribution<> rlife(glm::min(ctx->min_life, ctx->max_life),
                                      glm::max(ctx->min_life, ctx->max_life));

    float spawnRate = 1000.0f * ctx->particles->spawnRate;

    // Spawn `spawnRate` particles per second
    int newparticles = (int)(delta * spawnRate);
    if (newparticles > (int)(0.016f * spawnRate))
        newparticles = (int)(0.016f * spawnRate);

    for(int i = 0; i < newparticles; i++){
        int particleIndex = findUnusedParticle(particles);
        if (particleIndex == -1)
          break;

        Particle &p = container[particleIndex];

        p.life = rlife(eng) * STRETCH;
        p.initLife = p.life;

        p.pos = glm::vec3(0, 0.0f, 0.0f);

        float phi = azimuth(eng);
        float theta = polar(eng);
        float r = speed(eng);

        float vx = r * sin(theta) * cos(phi);
        float vy = r * sin(theta) * sin(phi);
        float vz = r * cos(theta);

        p.speed = vec3(vx, vy, vz);


        // Very bad way to generate a random color
        p.color.r = ctx->rand255(ctx->eng);
        p.color.g = ctx->rand255(ctx->eng);
        p.color.b = ctx->rand255(ctx->eng);
        p.color.a = (ctx->rand255(ctx->eng) % 256) / 3;

        p.size = ctx->particles->initSize;
    }

    int numParticles = 0;

    // Simulate
    for (int i = 0; i < MAX_PARTICLES; i++) {

        Particle& p = container[i];

        if (p.life > 0.0f) {

            p.life -= delta;

            // Normalized age
            float age = (p.initLife - p.life) / p.initLife;

            vec3 wind = vec3(0.0f, ctx->wind, 0.0f) * age;

            p.speed += glm::vec3(0.0f, 0.0f, ctx->gravity) * (float)delta * 0.5f;
            p.pos += (p.speed + wind) * (float)delta;
            p.cameraDistance = glm::length2(p.pos - cameraPos);

            p.size = (1-age) * ctx->particles->initSize + age * ctx->particles->finalSize;

            numParticles++;

        } else {
            p.cameraDistance = -1.0f;
        }
    }

    particles->numParticles = numParticles;

    // Sort particles by camera distance for correct blending
    if (ctx->sortParticles) {
        sortParticles(particles);
    }

    updateParticleData(particles, delta);
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
    ctx.window = glfwCreateWindow(ctx.width, ctx.height, "Particles", nullptr,
                                  nullptr);
    ctx.zoom = 0.3f;
    ctx.timeDelta = 0.016f;
    ctx.cameraPos = glm::vec3(4.0f, 0.0f, 0.0f);
    ctx.eng = eng;
    ctx.rand255 = rand255;

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

    presetFire(&ctx);

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
