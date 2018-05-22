// Assignment 3, Part 1 and 2
//
// Modify this file according to the lab instructions.
//

#include "utils.h"
#include "utils2.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <cstdlib>
#include <algorithm>

#define MAX_PARTICLES 100000

using namespace std;
using namespace glm;

// The attribute locations we will use in the vertex shader
enum AttributeLocation {
    POSITION = 0,
    NORMAL = 1
};

/*
// Struct for representing an indexed triangle mesh
struct Mesh {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;
};

// Struct for representing a vertex array object (VAO) created from a
// mesh. Used for rendering.
struct MeshVAO {
    GLuint vao;
    GLuint vertexVBO;
    GLuint normalVBO;
    GLuint indexVBO;
    int numVertices;
    int numIndices;
};
*/

struct Particle {
    vec3 pos;
    vec3 speed;
    vec4 color;
    float size;
    float angle;
    float weight;
    float life;
};

struct Particles {
    Particle particles[MAX_PARTICLES];
    GLuint billboard;
    GLuint positionSizes;
    GLuint colours;
    GLfloat positionSizesData[4*MAX_PARTICLES];
    GLfloat coloursData[4*MAX_PARTICLES];
    int lastUsedParticle;
    int numParticles;
};

// Struct for resources and state
struct Context {
    int width;
    int height;
    float aspect;
    GLFWwindow *window;
    GLuint program;
    Trackball trackball;
    Particles *particles;
    //Mesh mesh;
    //MeshVAO meshVAO;
    //GLuint defaultVAO;
    //GLuint cubemap;
    float elapsed_time;
    //bool ambient;
    //bool diffuse;
    //bool specular;
    //bool drawNormals;
    //bool gammaCorrect;
    //std::vector<GLuint> cubemaps;
    //int cubemapIdx;
    float zoom;
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

/*
// Returns the absolute path to the 3D model directory
std::string modelDir(void)
{
    std::string rootDir = getEnvVar("ASSIGNMENT3_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT3_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/particles/3d_models/";
}

// Returns the absolute path to the cubemap texture directory
std::string cubemapDir(void)
{
    std::string rootDir = getEnvVar("ASSIGNMENT3_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT3_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/particles/cubemaps/";
}

void loadMesh(const std::string &filename, Mesh *mesh)
{
    OBJMesh obj_mesh;
    objMeshLoad(obj_mesh, filename);
    mesh->vertices = obj_mesh.vertices;
    mesh->normals = obj_mesh.normals;
    mesh->indices = obj_mesh.indices;
}

void createMeshVAO(Context &ctx, const Mesh &mesh, MeshVAO *meshVAO)
{
    // Generates and populates a VBO for the vertices
    glGenBuffers(1, &(meshVAO->vertexVBO));
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->vertexVBO);
    auto verticesNBytes = mesh.vertices.size() * sizeof(mesh.vertices[0]);
    glBufferData(GL_ARRAY_BUFFER, verticesNBytes, mesh.vertices.data(), GL_STATIC_DRAW);

    // Generates and populates a VBO for the vertex normals
    glGenBuffers(1, &(meshVAO->normalVBO));
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->normalVBO);
    auto normalsNBytes = mesh.normals.size() * sizeof(mesh.normals[0]);
    glBufferData(GL_ARRAY_BUFFER, normalsNBytes, mesh.normals.data(), GL_STATIC_DRAW);

    // Generates and populates a VBO for the element indices
    glGenBuffers(1, &(meshVAO->indexVBO));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshVAO->indexVBO);
    auto indicesNBytes = mesh.indices.size() * sizeof(mesh.indices[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesNBytes, mesh.indices.data(), GL_STATIC_DRAW);

    // Creates a vertex array object (VAO) for drawing the mesh
    glGenVertexArrays(1, &(meshVAO->vao));
    glBindVertexArray(meshVAO->vao);
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->vertexVBO);
    glEnableVertexAttribArray(POSITION);
    glVertexAttribPointer(POSITION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->normalVBO);
    glEnableVertexAttribArray(NORMAL);
    glVertexAttribPointer(NORMAL, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshVAO->indexVBO);
    glBindVertexArray(ctx.defaultVAO); // unbinds the VAO

    // Additional information required by draw calls
    meshVAO->numVertices = mesh.vertices.size();
    meshVAO->numIndices = mesh.indices.size();
}
*/

void initializeTrackball(Context &ctx)
{
    double radius = double(std::min(ctx.width, ctx.height)) / 2.0;
    ctx.trackball.radius = radius;
    glm::vec2 center = glm::vec2(ctx.width, ctx.height) / 2.0f;
    ctx.trackball.center = center;
}

void initParticles(Context *ctx)
{
    Particles *particles = (Particles*) malloc(sizeof(struct Particles));
    if (particles == NULL) {
        fprintf(stderr, "Failed to allocate %x bytes of memory. Exiting.", sizeof(struct Particles));
        exit(-1);
    }

    // A quad
    static const GLfloat vertices[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.5f, 0.5f, 0.0f,
        -0.5f, 0.5f, 0.0f
    };

    GLuint billboard;
    GLuint positionSizes;
    GLuint colours;


    // The billboard quad
    // This is done only once
    glGenBuffers(1, &billboard);
    particles->billboard = billboard;

    glBindBuffer(GL_ARRAY_BUFFER, billboard);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


    // The positions and sizes of the particles
    glGenBuffers(1, &positionSizes);
    particles->positionSizes = positionSizes;

    glBindBuffer(GL_ARRAY_BUFFER, positionSizes);
    glBufferData(GL_ARRAY_BUFFER, 4 * MAX_PARTICLES * sizeof(GLfloat), NULL, GL_STREAM_DRAW);


    // The colours of the particles
    glGenBuffers(1, &colours);
    particles->colours = colours;

    glBindBuffer(GL_ARRAY_BUFFER, colours);
    glBufferData(GL_ARRAY_BUFFER, 4 * MAX_PARTICLES * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

    particles->numParticles = 0;
    particles->lastUsedParticle = 0;

    ctx->particles = particles;
}

void init(Context &ctx)
{
    ctx.program = loadShaderProgram(shaderDir() + "particle.vert",
                                    shaderDir() + "particle.frag");

    initParticles(&ctx);

    //loadMesh((modelDir() + "gargo.obj"), &ctx.mesh);
    //createMeshVAO(ctx, ctx.mesh, &ctx.meshVAO);

    //ctx.cubemaps = std::vector<GLuint>();

    /*
    std::vector<std::string> dirs = std::vector<std::string>();
    dirs.push_back("0.125");
    dirs.push_back("0.5");
    dirs.push_back("2");
    dirs.push_back("8");
    dirs.push_back("32");
    dirs.push_back("128");
    dirs.push_back("512");
    dirs.push_back("2048");

    // Load cubemap texture(s)
    for (int i = 0; i < 8; ++i) {
        std::string dir = dirs[i];
        ctx.cubemaps.push_back(loadCubemap(cubemapDir() + "/Forrest/prefiltered/" + dir + "/"));
    }
    */

    initializeTrackball(ctx);
}

/*
// MODIFY THIS FUNCTION
void drawMesh(Context &ctx, GLuint program, const MeshVAO &meshVAO)
{

    glm::vec3 centre = glm::vec3(0.0f);
    glm::vec3 cameraPos = glm::vec3(4.0f,4.0f,4.0f);

    float fov = 0.5f;
    
    glm::vec3 lightPos = glm::vec3(-1.0f,1.0f,1.0f);
    glm::vec3 lightCol = glm::vec3(1.0f,1.0f,1.0f);

    glm::vec3 ambientCol = glm::vec3(0.01f,0.0f,0.0f);
    glm::vec3 diffuseCol = glm::vec3(0.5f,0.0f,0.0f);
    glm::vec3 specularCol = glm::vec3(0.04f);
    float specPow = 32.0f;

    if (!ctx.ambient) {
        ambientCol = glm::vec3(0.0f);
    }

    if (!ctx.diffuse) {
        diffuseCol = glm::vec3(0.0f);
    }

    if (!ctx.specular) {
        specularCol = glm::vec3(0.0f);
    }

    // Define uniforms
    glm::mat4 model = trackballGetRotationMatrix(ctx.trackball);
              model = glm::rotate(model, 0.25f*ctx.elapsed_time, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 view = glm::lookAt(cameraPos, centre, glm::vec3(0.0f,1.0f,0.0f));
    //glm::mat4 projection = glm::ortho(-ctx.aspect, ctx.aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    glm::mat4 projection = glm::perspective(fov*ctx.zoom, ctx.aspect, 0.1f, 100.0f);
    glm::mat4 mv = view * model;
    glm::mat4 mvp = projection * mv;

    glm::mat4 normalMatrix = glm::transpose(glm::inverse(mv));


    // Activate program
    glUseProgram(program);

    // Bind textures
    // ...

    // Pass uniforms
    glUniformMatrix4fv(glGetUniformLocation(program, "u_m"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "u_v"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "u_mv"), 1, GL_FALSE, &mv[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_FALSE, &mvp[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "u_normal_matrix"), 1, GL_FALSE, &normalMatrix[0][0]);
    glUniform1f(glGetUniformLocation(program, "u_time"), ctx.elapsed_time);

    glUniform3fv(glGetUniformLocation(program, "u_light_pos"), 1, &lightPos[0]);
    glUniform3fv(glGetUniformLocation(program, "u_light_col"), 1, &lightCol[0]);

    glUniform1f(glGetUniformLocation(program, "u_spc_pow"), specPow);
    glUniform3fv(glGetUniformLocation(program, "u_amb_col"), 1, &ambientCol[0]);
    glUniform3fv(glGetUniformLocation(program, "u_dif_col"), 1, &diffuseCol[0]);
    glUniform3fv(glGetUniformLocation(program, "u_spc_col"), 1, &specularCol[0]);

    glUniform3fv(glGetUniformLocation(program, "u_camera_pos"), 1, &cameraPos[0]);

    glUniform1i(glGetUniformLocation(program, "u_draw_norm"), ctx.drawNormals);
    glUniform1i(glGetUniformLocation(program, "u_gamma_correct"), ctx.gammaCorrect);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.cubemaps[ctx.cubemapIdx]);

    glUniform1i(glGetUniformLocation(program, "u_cubemap"), 0);

    // Draw!
    glBindVertexArray(meshVAO.vao);
        glDrawElements(GL_TRIANGLES, meshVAO.numIndices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(ctx.defaultVAO);
}
*/

void orphanParticleBuffer()
{
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 4 * sizeof(GLuint), NULL, GL_STREAM_DRAW);
}

void drawParticles(Context *ctx)
{
    /**************
    *  Uniforms  *
    **************/
    
    // Identifiers for the uniform variables
    GLuint camera_up_id = glGetUniformLocation(ctx->program, "camera_up");
    GLuint camera_right_id = glGetUniformLocation(ctx->program, "camera_right");
    GLuint vp_id = glGetUniformLocation(ctx->program, "vp");

    vec3 centre = vec3(0.0f, 0.0f, 0.0f);
    vec3 cameraPos = glm::vec3(4.0f,4.0f,4.0f);
    float fov = 0.5f;

    //mat4 trackball = trackballGetRotationMatrix(ctx->trackball);
    mat4 view = lookAt(cameraPos, centre, vec3(0.0f,1.0f,0.0f));
    mat4 projection = perspective(fov * ctx->zoom, ctx->aspect, 0.1f, 100.0f);
    mat4 vp = projection * view;

    // Camera-local directions for billboarding
    vec3 camera_up = vec3(vp[0][1], vp[1][1], vp[2][1]);
    vec3 camera_right = vec3(vp[0][1], vp[1][0], vp[2][0]);

    // Set uniforms
    glUniform3fv(camera_up_id, 1, &camera_up[0]);
    glUniform3fv(camera_right_id, 1, &camera_right[0]);
    glUniformMatrix4fv(vp_id, 1, GL_FALSE, &vp[0][0]);


    /***************
    *  Particles  *
    ***************/

    // Particle data
    Particles *particles = ctx->particles;
    GLuint billboard = particles->billboard;
    GLuint positionSizes = particles->positionSizes;
    GLuint colours = particles->colours;
    int numParticles = particles->numParticles;

    // Update particle positions and sizes
    glBindBuffer(GL_ARRAY_BUFFER, positionSizes);
    orphanParticleBuffer();
    glBufferSubData(GL_ARRAY_BUFFER, 0, numParticles * sizeof(GLfloat) * 4, particles->positionSizesData);

    // Update particle colours
    glBindBuffer(GL_ARRAY_BUFFER, colours);
    orphanParticleBuffer();
    glBufferSubData(GL_ARRAY_BUFFER, 0, numParticles * sizeof(GLubyte) * 4, particles->coloursData);

    // Attach billboard corners to the vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, billboard);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Attach particle positions and sizes to the vertices
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, positionSizes);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Attach colours to the vertices
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, colours);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_SHORT, GL_TRUE, 0, (void*)0);

    // The billboard is the same for each particle
    glVertexAttribDivisor(0,0);
    // The particle position and sized advance for each particle
    glVertexAttribDivisor(1,1);
    // The particle colours advance for each particle
    glVertexAttribDivisor(2,1);

    // Draw all particle instances
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numParticles);
}

void display(Context *ctx)
{
    glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //glEnable(GL_DEPTH_TEST); // ensures that polygons overlap correctly
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //drawMesh(ctx, ctx.program, ctx.meshVAO);
    
    //updateParticles(ctx)

    drawParticles(ctx);
}

void reloadShaders(Context *ctx)
{
    glDeleteProgram(ctx->program);
    ctx->program = loadShaderProgram(shaderDir() + "particle.vert",
                                     shaderDir() + "particle.frag");
}

/*
void toggleAmbient(Context *ctx)
{
    ctx->ambient = !ctx->ambient;
}

void toggleDiffuse(Context *ctx)
{
    ctx->diffuse = !ctx->diffuse;
}

void toggleSpecular(Context *ctx)
{
    ctx->specular = !ctx->specular;
}

void toggleDrawNormals(Context *ctx)
{
    ctx->drawNormals = !ctx->drawNormals;
}

void toggleGammaCorrection(Context *ctx)
{
    ctx->gammaCorrect = !ctx->gammaCorrect;
}

void cycleCubemaps(Context *ctx)
{
    ctx->cubemapIdx = (ctx->cubemapIdx + 1) % 8;
}
*/

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
    /*
    if (key == GLFW_KEY_A && action == GLFW_PRESS) {
        toggleAmbient(ctx);
    }
    if (key == GLFW_KEY_D && action == GLFW_PRESS) {
        toggleDiffuse(ctx);
    }
    if (key == GLFW_KEY_S && action == GLFW_PRESS) {
        toggleSpecular(ctx);
    }
    if (key == GLFW_KEY_N && action == GLFW_PRESS) {
        toggleDrawNormals(ctx);
    }
    if (key == GLFW_KEY_G && action == GLFW_PRESS) {
        toggleGammaCorrection(ctx);
    }
    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        cycleCubemaps(ctx);
    }
    */
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

int main(void)
{
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
    ctx.zoom = 1.0f;
    /*
    ctx.ambient = true;
    ctx.diffuse = true;
    ctx.specular = true;
    ctx.drawNormals = false;
    ctx.gammaCorrect = true;
    ctx.cubemapIdx = 0;
    */

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

    // Initialize rendering
    //glGenVertexArrays(1, &ctx.defaultVAO);
    //glBindVertexArray(ctx.defaultVAO);
    //glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    init(ctx);

    // Start rendering loop
    while (!glfwWindowShouldClose(ctx.window)) {
        glfwPollEvents();
        ctx.elapsed_time = glfwGetTime();
        ImGui_ImplGlfwGL3_NewFrame();
        display(&ctx);
        ImGui::Render();
        glfwSwapBuffers(ctx.window);
    }

    // Shutdown
    free(ctx.particles);
    glfwDestroyWindow(ctx.window);
    glfwTerminate();
    std::exit(EXIT_SUCCESS);
}
