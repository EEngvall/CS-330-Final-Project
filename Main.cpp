#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define _USE_MATH_DEFINES
#include <math.h>


// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h"

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Module 5 Milestone - Erik Engvall"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbos[2];     // Handles for the vertex buffer objects
        GLuint nIndices;    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Triangle mesh data
    GLMesh gBook1;
    GLMesh gBook2;
    GLMesh gBook3;
    GLMesh gTable;
    GLMesh gRoundTable;
    GLMesh gLight;
    // Shader program
    GLuint gProgramId;
    GLuint gLightProgramId;
    GLuint gBookTex;
    GLuint gBookTex2;
    GLuint gMarbleTex;
    GLuint gParchmentTex;
    GLuint gStain;

    glm::vec2 gUVScale(5.0f, 5.0f);
    GLint gTexWrapMode = GL_REPEAT;

    //camera
    Camera gCamera(glm::vec3(0.0f, 5.0f, 5.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    bool ortho = false;
    GLfloat fov = 45.0f;


    // Define camera attributes
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
    glm::mat4 viewMatrix;

    // Subject position and scale
    glm::vec3 gCenterPosition(0.0f, 0.0f, 0.0f);
    glm::vec3 gCenterScale(1.0f);
    glm::vec3 gBook1Position(0.0f, 0.0f, 0.0f);
    glm::vec3 gBook1Scale(1.0f);
    glm::vec3 gBook2Position(0.0f, 0.40, -0.125f);
    glm::vec3 gBook2Scale(1.0f);
    glm::vec3 gBook3Position(0.0f, 0.60f, -0.2f);
    glm::vec3 gBook3Scale(1.0f);
    glm::vec3 gTablePosition(0.0f, -0.25f, 0.0f);
    glm::vec3 gTableScale(1.0f);


    // Object and light color
    glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
    glm::vec3 gLightColor(0.99f, 0.95f, 0.86f);

    // Light position and scale
    glm::vec3 gLightPosition(5.0f, 8.0f, 8.0f);
    glm::vec3 gLightScale(1.0f);

}


/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateBook1(GLMesh& mesh);
void UCreateBook2(GLMesh& mesh);
void UCreateBook3(GLMesh& mesh);
void UCreateTable(GLMesh& mesh);
void UCreateLight(GLMesh& mesh);
void UCreateRoundTable(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    vertexTextureCoordinate = textureCoordinate;
}
);


/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,
    in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor;
// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform sampler2D uTextureBase;
uniform sampler2D uTextureExtra;
uniform bool multipleTextures;
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;

void main()
{

    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    //Calculate Ambient lighting*/
    float ambientStrength = 0.2f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit

    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube

    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light

    vec3 diffuse = impact * lightColor; // Generate diffuse light color


    //Calculate Specular lighting*/
    float specularIntensity = 1.0f; // Set specular light strength
    float highlightSize = 32.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);

    vec3 specular = specularIntensity * specularComponent * lightColor;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

    vec3 fullDiffuse = diffuse;
    vec3 fullSpecular = specular;
    // Calculate phong result
    vec3 phong = (ambient + fullDiffuse + fullSpecular) * textureColor.xyz;

    fragmentColor = texture(uTextureBase, vertexTextureCoordinate) * vec4(phong, 1.0); // Send lighting results to GPU
    if (multipleTextures)
    {
        vec4 extraTexture = texture(uTextureExtra, vertexTextureCoordinate) * vec4(phong, 1.0);
        if (extraTexture.a != 0.0)
            fragmentColor = extraTexture;
    }

}
);

/*Light Shader Source Code*/
const GLchar* lightVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

        //Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Light Fragment Shader Source Code*/
const GLchar* lightFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing light color (smaller pyramid) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);

//Function to flip image vertically
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}



int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateBook1(gBook1); // Calls the function to create the Vertex Buffer Object
    UCreateBook2(gBook2);
    UCreateBook3(gBook3);
    UCreateTable(gTable);
    UCreateLight(gLight);
    UCreateRoundTable(gRoundTable);
    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;
    if (!UCreateShaderProgram(lightVertexShaderSource, lightFragmentShaderSource, gLightProgramId))
        return EXIT_FAILURE;

    // Load textures
    const char* texFilename = "img/BookTexture3.jpg";
    if (!UCreateTexture(texFilename, gBookTex))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "img/MarbleTexture.jpg";
    if (!UCreateTexture(texFilename, gMarbleTex))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "img/BookTexture4.jpg";
    if (!UCreateTexture(texFilename, gBookTex2))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "img/ParchmentTexture.jpg";
    if (!UCreateTexture(texFilename, gParchmentTex))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "img/Stain.jpg";
    if (!UCreateTexture(texFilename, gStain))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTextureBase"), 0);
    // We set the texture as texture unit 1
    glUniform1i(glGetUniformLocation(gProgramId, "uTextureExtra"), 1);
    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }


    // Release mesh data
    UDestroyMesh(gBook1);
    UDestroyMesh(gBook2);
    UDestroyMesh(gBook3);
    UDestroyMesh(gTable);
    UDestroyMesh(gLight);
    UDestroyMesh(gRoundTable);

    //Release Texture data
    UDestroyTexture(gBookTex);
    UDestroyTexture(gBookTex2);
    UDestroyTexture(gMarbleTex);
    UDestroyTexture(gParchmentTex);
    UDestroyTexture(gStain);

    // Release shader program
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLightProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    bool keypress = false;


    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
        //cout << "You pressed W! ";
        keypress = true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
        //cout << "You pressed S! ";
        keypress = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
        //cout << "You pressed A! ";
        keypress = true;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
        //cout << "You pressed D! ";
        keypress = true;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);
        //cout << "You pressed Q! ";
        keypress = true;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        gCamera.ProcessKeyboard(UP, gDeltaTime);
        //cout << "You pressed E! ";
        keypress = true;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    {
        ortho = !ortho;

    }

    //if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    //{
    //    gLightPosition.x = gLightPosition.x + 1;
    //    cout << "Light Position X: " << gLightPosition.x << endl;
    //}

    if (keypress)
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        //cout << "Cursor at position (" << x << ", " << y << ")" << endl;
    }
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Functioned called to render a frame
void URender()
{
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // Set the shader to be used
    glUseProgram(gProgramId);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = glm::translate(gCenterPosition) * glm::scale(gCenterScale);

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    glm::mat4 projection;
    // Setup views and projections
    if (ortho) {
        float scale = 50;

        projection = glm::ortho(-(800.0f / scale), 800.0f / scale, -(600.0f / scale), (600.0f / scale), 4.5f, 6.5f);
    }
    else {
        projection = glm::perspective(glm::radians(gCamera.Zoom), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
    }

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Pyramid Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));


    //Draws Bottom Book
    model = glm::translate(gBook1Position) * glm::scale(gBook1Scale);

    // Reference matrix uniforms from the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    // Pass matrix data to the Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(gBook1.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gParchmentTex);
    glDrawElements(GL_TRIANGLES, gBook1.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle

    //Draws Middle Book
    model = glm::translate(gBook2Position) * glm::scale(gBook2Scale);

    // Reference matrix uniforms from the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    // Pass matrix data to the Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glBindVertexArray(gBook2.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBookTex);
    glDrawElements(GL_TRIANGLES, gBook2.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle

    //Draws Top Book
    model = glm::translate(gBook3Position) * glm::scale(gBook3Scale);

    // Reference matrix uniforms from the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    // Pass matrix data to the Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glBindVertexArray(gBook3.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBookTex2);
    glDrawElements(GL_TRIANGLES, gBook3.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle

    ////Draws Table
    //model = glm::translate(gCenterPosition) * glm::scale(gCenterScale);

    //// Reference matrix uniforms from the Shader program
    //modelLoc = glGetUniformLocation(gProgramId, "model");
    //viewLoc = glGetUniformLocation(gProgramId, "view");
    //projLoc = glGetUniformLocation(gProgramId, "projection");

    //// Pass matrix data to the Shader program's matrix uniforms
    //glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    //glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    //glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    //glBindVertexArray(gTable.vao);
    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, gMarbleTex);
    //glDrawElements(GL_TRIANGLES, gTable.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle

    //Draws Round Table
    model = glm::translate(gTablePosition) * glm::scale(gTableScale);

    // Reference matrix uniforms from the Shader program
    modelLoc = glGetUniformLocation(gProgramId, "model");
    viewLoc = glGetUniformLocation(gProgramId, "view");
    projLoc = glGetUniformLocation(gProgramId, "projection");

    // Pass matrix data to the Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(gRoundTable.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gMarbleTex);
    glDrawElements(GL_TRIANGLES, gRoundTable.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle



    //Draws Light
    //----------------
    glUseProgram(gLightProgramId);

    //Transform the smaller pyramid used as a visual que for the light source
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);

    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLightProgramId, "model");
    viewLoc = glGetUniformLocation(gLightProgramId, "view");
    projLoc = glGetUniformLocation(gLightProgramId, "projection");

    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(gLight.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gParchmentTex);
    glDrawElements(GL_TRIANGLES, gLight.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
    glUseProgram(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Implements the UCreateBook1 function
void UCreateBook1(GLMesh& mesh)
{
    GLfloat width = 1.0f;
    GLfloat height = 0.25f;
    GLfloat depth = 1.0f;
    // Position and Color data
    GLfloat verts[] = {

        // Vertex Positions    // +/- Normals          //Textture Coordinates
        width, height, 0.0f,      0.0f,  0.0f,  1.0f,       1.0f, 1.0f,                        // 0 Top Right Close (Lower Cube)
        width, -height, 0.0f,     0.0f,  0.0f,  1.0f,       0.0f, 1.0f,                        // 1 Bottom Right Close (Lower Cube)
        -width, -height, 0.0f,    0.0f,  0.0f,  1.0f,       0.0f, 0.0f,                        // 2 Bottom Left Close (Lower Cube)
        -width,  height, 0.0f,    0.0f,  0.0f,  1.0f,       1.0f, 0.0f,                        // 3 Top Left Close (lower Cube)

        width, -height, -depth,    1.0f,  0.0f,  0.0f,       1.0f, 0.0f,                        // 4 Bottom Right Far (Lower Cube)
        width,  height, -depth,    1.0f,  0.0f,  0.0f,       1.0f, 1.0f,                        // 5 Top Right Far (Lower Cube)
        width, height, 0.0f,      1.0f,  0.0f,  0.0f,       0.0f, 1.0f,                        // 6 Top Right Close (Lower Cube)
        width, -height, 0.0f,     1.0f,  0.0f,  0.0f,       0.0f, 0.0f,                        // 7 Bottom Right Close (Lower Cube)


        width,  height, -depth,    -1.0f,  0.0f,  0.0f,       0.0f, 1.0f,                       // 8 Top Right Far (Lower Cube)
        width, -height, -depth,    -1.0f,  0.0f,  0.0f,       0.0f, 0.0f,                       // 9 Bottom Right Far (Lower Cube)
        -width, -height, -depth,   -1.0f,  0.0f,  0.0f,       1.0f, 0.0f,                       // 10 Bottom Left Far (Lower Cube)
        -width,  height, -depth,   -1.0f,  0.0f,  0.0f,       1.0f, 1.0f,                       // 11 Top Left Far (Lower Cube)

        -width, -height, 0.0f,    -1.0f,  0.0f,  0.0f,       0.0f, 1.0f,                       // 12 Bottom Left Close (Lower Cube)
        -width,  height, 0.0f,    -1.0f,  0.0f,  0.0f,       1.0f, 1.0f,                       // 13 Top Left Close (lower Cube)
        -width,  height, -depth,   -1.0f,  0.0f,  0.0f,       1.0f, 0.0f,                       // 14 Top Left Far (Lower Cube)
        -width, -height, -depth,   -1.0f,  0.0f,  0.0f,       0.0f, 0.0f,                       // 15 Bottom Left Far (Lower Cube)

        width, height, 0.0f,      0.0f,  1.0f,  0.0f,       0.0f, 1.0f,                        // 16 Top Right Close (Lower Cube)
        -width,  height, 0.0f,    0.0f,  1.0f,  0.0f,       1.0f, 1.0f,                        // 17 Top Left Close (lower Cube)
        -width,  height, -depth,   0.0f,  1.0f,  0.0f,       1.0f, 0.0f,                        // 18 Top Left Far (Lower Cube)
        width,  height, -depth,    0.0f,  1.0f,  0.0f,       0.0f, 0.0f,                        // 19 Top Right Far (Lower Cube)




    };

    // Index data to share position data
    GLushort indices[] = {
        0, 1, 3,  //Create Bottom Cube
        1, 2, 3,
        4, 5, 6,
        6, 7, 4,
        8, 9, 10,
        10, 11, 8,
        12, 13, 14,
        14, 15, 12,
        16, 17, 18,
        18, 19, 16


    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

void UCreateBook2(GLMesh& mesh)
{
    GLfloat width = 0.8f;
    GLfloat height = 0.15f;
    GLfloat depth = 0.8f;
    // Position and Color data
    GLfloat verts[] = {

        // Vertex Positions    // +/- Normals          //Textture Coordinates
width, height, 0.0f,      0.0f,  0.0f,  1.0f,       1.0f, 1.0f,                        // 0 Top Right Close (Lower Cube)
width, -height, 0.0f,     0.0f,  0.0f,  1.0f,       0.0f, 1.0f,                        // 1 Bottom Right Close (Lower Cube)
-width, -height, 0.0f,    0.0f,  0.0f,  1.0f,       0.0f, 0.0f,                        // 2 Bottom Left Close (Lower Cube)
-width,  height, 0.0f,    0.0f,  0.0f,  1.0f,       1.0f, 0.0f,                        // 3 Top Left Close (lower Cube)

width, -height, -depth,    1.0f,  0.0f,  0.0f,       1.0f, 0.0f,                        // 4 Bottom Right Far (Lower Cube)
width,  height, -depth,    1.0f,  0.0f,  0.0f,       1.0f, 1.0f,                        // 5 Top Right Far (Lower Cube)
width, height, 0.0f,      1.0f,  0.0f,  0.0f,       0.0f, 1.0f,                        // 6 Top Right Close (Lower Cube)
width, -height, 0.0f,     1.0f,  0.0f,  0.0f,       0.0f, 0.0f,                        // 7 Bottom Right Close (Lower Cube)


width,  height, -depth,    -1.0f,  0.0f,  0.0f,       0.0f, 1.0f,                       // 8 Top Right Far (Lower Cube)
width, -height, -depth,    -1.0f,  0.0f,  0.0f,       0.0f, 0.0f,                       // 9 Bottom Right Far (Lower Cube)
-width, -height, -depth,   -1.0f,  0.0f,  0.0f,       1.0f, 0.0f,                       // 10 Bottom Left Far (Lower Cube)
-width,  height, -depth,   -1.0f,  0.0f,  0.0f,       1.0f, 1.0f,                       // 11 Top Left Far (Lower Cube)

-width, -height, 0.0f,    -1.0f,  0.0f,  0.0f,       0.0f, 1.0f,                       // 12 Bottom Left Close (Lower Cube)
-width,  height, 0.0f,    -1.0f,  0.0f,  0.0f,       1.0f, 1.0f,                       // 13 Top Left Close (lower Cube)
-width,  height, -depth,   -1.0f,  0.0f,  0.0f,       1.0f, 0.0f,                       // 14 Top Left Far (Lower Cube)
-width, -height, -depth,   -1.0f,  0.0f,  0.0f,       0.0f, 0.0f,                       // 15 Bottom Left Far (Lower Cube)

width, height, 0.0f,      0.0f,  1.0f,  0.0f,       0.0f, 1.0f,                        // 16 Top Right Close (Lower Cube)
-width,  height, 0.0f,    0.0f,  1.0f,  0.0f,       1.0f, 1.0f,                        // 17 Top Left Close (lower Cube)
-width,  height, -depth,   0.0f,  1.0f,  0.0f,       1.0f, 0.0f,                        // 18 Top Left Far (Lower Cube)
width,  height, -depth,    0.0f,  1.0f,  0.0f,       0.0f, 0.0f,                        // 19 Top Right Far (Lower Cube)







    };

    // Index data to share position data
    GLushort indices[] = {
        0, 1, 3,  //Create Bottom Cube
        1, 2, 3,
        4, 5, 6,
        6, 7, 4,
        8, 9, 10,
        10, 11, 8,
        12, 13, 14,
        14, 15, 12,
        16, 17, 18,
        18, 19, 16

    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

void UCreateBook3(GLMesh& mesh)
{
    GLfloat width = 0.7f;
    GLfloat height = 0.10f;
    GLfloat depth = 0.5f;
    // Position and Color data
    GLfloat verts[] = {

        // Vertex Positions    // +/- Normals          //Textture Coordinates
        width, height, 0.0f,      0.0f,  0.0f,  1.0f,       1.0f, 1.0f,                        // 0 Top Right Close (Lower Cube)
        width, -height, 0.0f,     0.0f,  0.0f,  1.0f,       0.0f, 1.0f,                        // 1 Bottom Right Close (Lower Cube)
        -width, -height, 0.0f,    0.0f,  0.0f,  1.0f,       0.0f, 0.0f,                        // 2 Bottom Left Close (Lower Cube)
        -width,  height, 0.0f,    0.0f,  0.0f,  1.0f,       1.0f, 0.0f,                        // 3 Top Left Close (lower Cube)

        width, -height, -depth,    1.0f,  0.0f,  0.0f,       1.0f, 0.0f,                        // 4 Bottom Right Far (Lower Cube)
        width,  height, -depth,    1.0f,  0.0f,  0.0f,       1.0f, 1.0f,                        // 5 Top Right Far (Lower Cube)
        width, height, 0.0f,      1.0f,  0.0f,  0.0f,       0.0f, 1.0f,                        // 6 Top Right Close (Lower Cube)
        width, -height, 0.0f,     1.0f,  0.0f,  0.0f,       0.0f, 0.0f,                        // 7 Bottom Right Close (Lower Cube)


        width,  height, -depth,    -1.0f,  0.0f,  0.0f,       0.0f, 1.0f,                       // 8 Top Right Far (Lower Cube)
        width, -height, -depth,    -1.0f,  0.0f,  0.0f,       0.0f, 0.0f,                       // 9 Bottom Right Far (Lower Cube)
        -width, -height, -depth,   -1.0f,  0.0f,  0.0f,       1.0f, 0.0f,                       // 10 Bottom Left Far (Lower Cube)
        -width,  height, -depth,   -1.0f,  0.0f,  0.0f,       1.0f, 1.0f,                       // 11 Top Left Far (Lower Cube)

        -width, -height, 0.0f,    -1.0f,  0.0f,  0.0f,       0.0f, 1.0f,                       // 12 Bottom Left Close (Lower Cube)
        -width,  height, 0.0f,    -1.0f,  0.0f,  0.0f,       1.0f, 1.0f,                       // 13 Top Left Close (lower Cube)
        -width,  height, -depth,   -1.0f,  0.0f,  0.0f,       1.0f, 0.0f,                       // 14 Top Left Far (Lower Cube)
        -width, -height, -depth,   -1.0f,  0.0f,  0.0f,       0.0f, 0.0f,                       // 15 Bottom Left Far (Lower Cube)

        width, height, 0.0f,      0.0f,  1.0f,  0.0f,       0.0f, 1.0f,                        // 16 Top Right Close (Lower Cube)
        -width,  height, 0.0f,    0.0f,  1.0f,  0.0f,       1.0f, 1.0f,                        // 17 Top Left Close (lower Cube)
        -width,  height, -depth,   0.0f,  1.0f,  0.0f,       1.0f, 0.0f,                        // 18 Top Left Far (Lower Cube)
        width,  height, -depth,    0.0f,  1.0f,  0.0f,       0.0f, 0.0f,                        // 19 Top Right Far (Lower Cube)








    };

    // Index data to share position data
    GLushort indices[] = {
        0, 1, 3,  //Create Bottom Cube
        1, 2, 3,
        4, 5, 6,
        6, 7, 4,
        8, 9, 10,
        10, 11, 8,
        12, 13, 14,
        14, 15, 12,
        16, 17, 18,
        18, 19, 16

    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

// Implements the UCreateTable function
void UCreateTable(GLMesh& mesh)
{
    // Position and Color data
    GLfloat verts[] = {

        // Vertex Positions         // +/- Normals                 //Texture Coordinates
        5.0f, -0.25f, -5.0f,        0.0f, 1.0f, 0.0f,              1.0f, 1.0f,                       //24 Back Right of Plane
        5.0f, -0.25f, 5.0f,         0.0f, 1.0f, 0.0f,              1.0f, 0.0f,                       //25 Front Right of Plane
        -5.0f, -0.25f, 5.0f,        0.0f, 1.0f, 0.0f,              0.0f, 0.0f,                       //26 Front Left of Plane
        -5.0f, -0.25f, -5.0f,       0.0f, 1.0f, 0.0f,              0.0f, 1.0f,                       //27 Back Left of Plane

    };

    // Index data to share position data
    GLushort indices[] = {
        0, 1, 2,
        2, 3, 0

    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

// Implements the UCreateLight function
void UCreateLight(GLMesh& mesh)
{
    // Position and Color data
    GLfloat verts[] = {

        // Vertex Positions    // Colors (r,g,b,a)          //Textture Coordinates
        0.25f, 0.25f, 0.0f,     1.0f, 1.0f, 1.0f, 1.0f,       1.0f, 1.0f,                       // 0 Top Right Close (Lower Cube)
        0.25f, -0.25f, 0.0f,    1.0f, 1.0f, 1.0f, 1.0f,       0.0f, 1.0f,                       // 1 Bottom Right Close (Lower Cube)
        -0.25f, -0.25f, 0.0f,   1.0f, 1.0f, 1.0f, 1.0f,       0.0f, 0.0f,                       // 2 Bottom Left Close (Lower Cube)
        -0.25f,  0.25f, 0.0f,   1.0f, 1.0f, 1.0f, 1.0f,       1.0f, 0.0f,                       // 3 Top Left Close (lower Cube)

        0.25f, -0.25f, -1.0f,   1.0f, 1.0f, 1.0f, 1.0f,       1.0f, 0.0f,                       // 4 Bottom Right Far (Lower Cube)
        0.25f,  0.25f, -1.0f,   1.0f, 1.0f, 1.0f, 1.0f,       1.0f, 1.0f,                       // 5 Top Right Far (Lower Cube)
        0.25f, 0.25f, 0.0f,     1.0f, 1.0f, 1.0f, 1.0f,       0.0f, 1.0f,                       // 6 Top Right Close (Lower Cube)
        0.25f, -0.25f, 0.0f,    1.0f, 1.0f, 1.0f, 1.0f,       0.0f, 0.0f,                       // 7 Bottom Right Close (Lower Cube)


        0.25f,  0.25f, -1.0f,   1.0f, 1.0f, 1.0f, 1.0f,       0.0f, 1.0f,                       // 8 Top Right Far (Lower Cube)
        0.25f, -0.25f, -1.0f,   1.0f, 1.0f, 1.0f, 1.0f,       0.0f, 0.0f,                       // 9 Bottom Right Far (Lower Cube)
        -0.25f, -0.25f, -1.0f,  1.0f, 1.0f, 1.0f, 1.0f,       1.0f, 0.0f,                       // 10 Bottom Left Far (Lower Cube)
        -0.25f,  0.25f, -1.0f,  1.0f, 1.0f, 1.0f, 1.0f,       1.0f, 1.0f,                       // 11 Top Left Far (Lower Cube)

        -0.25f, -0.25f, 0.0f,   1.0f, 1.0f, 1.0f, 1.0f,       0.0f, 1.0f,                       // 12 Bottom Left Close (Lower Cube)
        -0.25f,  0.25f, 0.0f,   1.0f, 1.0f, 1.0f, 1.0f,       1.0f, 1.0f,                       // 13 Top Left Close (lower Cube)
        -0.25f,  0.25f, -1.0f,  1.0f, 1.0f, 1.0f, 1.0f,       1.0f, 0.0f,                       // 14 Top Left Far (Lower Cube)
        -0.25f, -0.25f, -1.0f,  1.0f, 1.0f, 1.0f, 1.0f,       0.0f, 0.0f,                       // 15 Bottom Left Far (Lower Cube)

        0.25f, 0.25f, 0.0f,     1.0f, 1.0f, 1.0f, 1.0f,       0.0f, 1.0f,                       // 16 Top Right Close (Lower Cube)
        -0.25f,  0.25f, 0.0f,   1.0f, 1.0f, 1.0f, 1.0f,       1.0f, 1.0f,                       // 17 Top Left Close (lower Cube)
        -0.25f,  0.25f, -1.0f,  1.0f, 1.0f, 1.0f, 1.0f,       1.0f, 0.0f,                       // 18 Top Left Far (Lower Cube)
        0.25f,  0.25f, -1.0f,   1.0f, 1.0f, 1.0f, 1.0f,       0.0f, 0.0f,                       // 19 Top Right Far (Lower Cube)




    };

    // Index data to share position data
    GLushort indices[] = {
        0, 1, 3,  //Create Bottom Cube
        1, 2, 3,
        4, 5, 6,
        6, 7, 4,
        8, 9, 10,
        10, 11, 8,
        12, 13, 14,
        14, 15, 12,
        16, 17, 18,
        18, 19, 16


    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerColor = 4;
    const GLuint floatsPerUV = 2;

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerColor)));
    glEnableVertexAttribArray(2);
}

// Implements the UCreateRoundTable function
void UCreateRoundTable(GLMesh& mesh)
{
    GLfloat x = 0;
    GLfloat y = 0;
    GLfloat z = 0;
    GLfloat radius = 4;
    GLuint numberOfSides = 6;
    GLuint numberOfVertices = numberOfSides + 1;
    //GLuint indices[18];
    
    GLfloat doublePi = 2.0f * M_PI;

    GLfloat* circleVerticesX = new GLfloat[numberOfVertices];
    GLfloat* circleVerticesY = new GLfloat[numberOfVertices];
    GLfloat* circleVerticesZ = new GLfloat[numberOfVertices];
    GLfloat allCircleVertices[63]; /*= new GLfloat[numberOfVertices * numberOfSides];*/

    circleVerticesX[0] = x;
    circleVerticesY[0] = y;
    circleVerticesZ[0] = z;

    //Loop to determine angles between vertices
    for (int i = 1; i < numberOfVertices; i++)
    {
        circleVerticesX[i] = x + (radius * cos(i * doublePi / numberOfSides));
        circleVerticesY[i] = y;
        circleVerticesZ[i] = z + (radius * sin(i * doublePi / numberOfSides));
    }
    
    //Loop to fill array with vertices
    for (int i = 0; i < numberOfVertices; i++)
    {
        allCircleVertices[i * 8] = circleVerticesX[i];
        allCircleVertices[(i * 8) + 1] = circleVerticesY[i];
        allCircleVertices[(i * 8) + 2] = circleVerticesZ[i];
        allCircleVertices[(i * 8) + 3] = 0.0f;
        allCircleVertices[(i * 8) + 4] = 1.0f;
        allCircleVertices[(i * 8) + 5] = 0.0f;


        if ((i * 8 + 6) == 6)
        {
            allCircleVertices[(i * 8) + 6] = 0.5f;
        }
        else
        {
            allCircleVertices[(i * 8) + 6] = 0.0f;

        }
        if ((i * 8 + 7) == 7)
        {
            allCircleVertices[(i * 8) + 7] = 0.5f;

        }
        else
        {
            allCircleVertices[(i * 8) + 7] = 0.0f;

        }
        if ((i * 8 + 8) == 22 || (i * 8 + 8) == 30 || (i * 8 + 8) == 38)
        {
            allCircleVertices[(i * 8) + 8] = 1.0f;
        }
        else
        {
            allCircleVertices[(i * 8) + 8] = 0.0f;
        }
    }

    //for (int i = 0; i < 63; i++)
    //{
    //    cout << allCircleVertices[i * 8] << endl;
    //    cout << allCircleVertices[(i * 8) + 1] << endl;
    //    cout << allCircleVertices[(i * 8) + 2] << endl;
    //    cout << allCircleVertices[(i * 8) + 3] << endl;
    //    cout << allCircleVertices[(i * 8) + 4] << endl;
    //    cout << allCircleVertices[(i * 8) + 5] << endl;
    //    cout << allCircleVertices[(i * 8) + 6] << endl;
    //    cout << allCircleVertices[(i * 8) + 7] << endl;
    //    cout << allCircleVertices[(i * 8) + 8] << endl;

    //}




    //GLfloat allCircleVertices[] = {
    //0.0f, 0.0f, 0.0f,             0.0f, 1.0f, 0.0f,           0.5f, 0.5f,
    //3.0f, 0.0f, 4.0f,             0.0f, 1.0f, 0.0f,           0.0f, 0.0f, 
    //-3.0f, 0.0f, 4.0f,            0.0f, 1.0f, 0.0f,           1.0f, 0.0f, 
    //-5.0f, 0.0f, 0.0f,            0.0f, 1.0f, 0.0f,           0.0f, 0.0f, 
    //-3.0f, 0.0f, -4.0f,           0.0f, 1.0f, 0.0f,           1.0f, 0.0f, 
    //3.0f, 0.0f, -4.0f,            0.0f, 1.0f, 0.0f,           0.0f, 0.0f, 
    //5.0f, 0.0f, 0.0f,             0.0f, 1.0f, 0.0f,           1.0f, 0.0f, 

    //};




    GLushort indices[] = {
        0, 2, 1,
        0, 3, 2,
        0, 4, 3,
        0, 5, 4,
        0, 6, 5,
        0, 1, 6
    };

    /*FIX STILL NEEDED*/
    ////For loop to fill Indices array with correct indices based on number of sides
    //for (int i = 0; i < numberOfSides; i++)
    //{
    //    if (i == (numberOfSides - 1))
    //    {
    //        indices[i * 3] = 0;
    //        indices[(i * 3) + 1] = indices[2];
    //        indices[(i * 3) + 2] = numberOfSides;
    //        cout << indices[i * 3] << ", " << indices[(i * 3) + 1] << ", " << indices[(i * 3) + 2] << endl;
    //    }
    //    else
    //    {
    //        indices[i * 3] = 0;
    //        indices[(i * 3) + 1] = i + 2;
    //        indices[(i * 3) + 2] = i + 1;
    //        cout << indices[i * 3] << ", " << indices[(i * 3) + 1] << ", " << indices[(i * 3) + 2] << endl;
    //    }

    //}



 


    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(allCircleVertices), allCircleVertices, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
   
}


void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(2, mesh.vbos);
}

/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}

void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
    //cout << "Mouse at (" << xpos << ", " << ypos << ")" << endl;
}

//Controls the camera's speed using the scroll wheel, this should be updated in the future to account for negative values. 
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.MovementSpeed += yoffset;
    cout << "Camera Speed = " << gCamera.MovementSpeed << endl;
    //cout << "Mouse wheel (" << xoffset << ", " << yoffset << ")" << endl;
}

//Controls the actions of the left, right, middle and undefined mouse button clicks.  Currently set just to print out the action. 
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}



