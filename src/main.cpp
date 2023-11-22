#include <glad/gl.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <k4a/k4a.h>

#include "camera.hpp"
#include "filesystem.hpp"
#include "shader.hpp"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window, glm::vec3& deltaTPntEye);

// timing
GLfloat deltaTime = 0.0f;	// time between current frame and last frame
GLfloat lastFrame = 0.0f;

// The MAIN function, from here we start the application and run the game loop
int main()
{
    // Check for Kinects
    uint32_t count = k4a_device_get_installed_count();
    if (count == 0)
    {
        printf("No k4a devices attached!\n");
        return 1;
    } else {
        printf("k4a device attached!\n");
    }

    std::cout << "Starting GLFW context, OpenGL 4.6" << std::endl;
    // Init GLFW
    glfwInit();
    // Set all the required options for GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // Get monitor infomation
    // GLFWmonitor* MyMonitor =  glfwGetPrimaryMonitor(); // The primary monitor..
    // const GLFWvidmode* mode = glfwGetVideoMode(MyMonitor);
    // GLuint WIDTH = mode->width;
    // GLuint HEIGHT = mode->height;

    GLuint WIDTH = 1920;
    GLuint HEIGHT = 1080;

    // Create a GLFWwindow object that we can use for GLFW's functions
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", NULL, NULL);
    glfwMakeContextCurrent(window);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Load OpenGL functions, gladLoadGL returns the loaded version, 0 on error.
    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0)
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    // Successfully loaded OpenGL
    std::cout << "Loaded OpenGL " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << std::endl;

    // Define the viewport dimensions
    glViewport(0, 0, WIDTH, HEIGHT);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile our shader zprogram
    // ------------------------------------
    Shader ourShader(FileSystem::getPath("data/shaders/camera.vs").c_str(), FileSystem::getPath("data/shaders/camera.fs").c_str());

    // set up vertex data (and buffer(s)) and configure vertex attributes for walls
    // ---------------------------------------------------------------------------
    float wallsVertices[] = {
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f
    };

    unsigned int wallIndices[] = { 
        // Back Wall
        4, 5, 7,
        4, 6, 7,
        // Left wall
        0, 4, 6,
        0, 2, 6,
        // Right wall
        5, 1, 3,
        5, 7, 3,
        // Floor
        4, 5, 1,
        4, 0, 1
    };  

    // world space positions of our walls
    unsigned int wallsVBO, wallsVAO, wallsEBO;
    glGenVertexArrays(1, &wallsVAO);
    glGenBuffers(1, &wallsVBO);
    glGenBuffers(1, &wallsEBO);

    glBindVertexArray(wallsVAO);

    glBindBuffer(GL_ARRAY_BUFFER, wallsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(wallsVertices), wallsVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wallsEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(wallIndices), wallIndices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // tracker-space
    // -------------

    // Three corners of the screen
    glm::vec3 tPntScreenA = glm::vec3(0.0f,0.0f,0.0f);
    glm::vec3 tPntScreenB = glm::vec3(1.0f,0.0f,0.0f);
    glm::vec3 tPntScreenC = glm::vec3(0.0f,1.0f,0.0f);

    // Orthonomal basis for screen
    glm::vec3 tVecScreenR = glm::normalize(tPntScreenB - tPntScreenA);
    glm::vec3 tVecScreenU = glm::normalize(tPntScreenC - tPntScreenA);
    glm::vec3 tVecScreenN = glm::normalize(glm::cross(tVecScreenR,tVecScreenU));

    // The users eye
    glm::vec3 tPntEye;
    glm::vec3 deltaTPntEye;

    // Vector from eye to screen (set during runtime)
    glm::vec3 tVecEyeScreenA;
    glm::vec3 tVecEyeScreenB;
    glm::vec3 tVecEyeScreenC;

    // Distance from the eye to screen-space-origin (eye intersection on the normal)
    GLfloat tDistEyeScreen;

    // Distances from screen-space-origin to borders of the screen (Calculated at runtime)
    GLfloat lDistScreen;
    GLfloat rDistScreen;
    GLfloat bDistScreen;
    GLfloat tDistScreen;

    // screen-space
    // ------------

    // Clipping distances
    GLfloat nClipDist = 0.1f;
    GLfloat fClipDist = 100.f;

    tPntEye = glm::vec3(0.5f,0.5f,1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----

        // reset vector
        deltaTPntEye.x = 0.0f;
        deltaTPntEye.y = 0.0f;
        deltaTPntEye.z = 0.0f;
        processInput(window,deltaTPntEye);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

        // activate shader
        ourShader.use();

        tPntEye += deltaTPntEye; 
        std::cout << glm::to_string(tPntEye) << std::endl;

        tVecEyeScreenA = tPntScreenA - tPntEye;
        tVecEyeScreenB = tPntScreenB - tPntEye;
        tVecEyeScreenC = tPntScreenC - tPntEye;

        tDistEyeScreen = - glm::dot(tVecScreenN,tVecEyeScreenA);
        GLfloat scaleFactor = nClipDist/tDistEyeScreen;
        lDistScreen = glm::dot(tVecScreenR,tVecEyeScreenA) * scaleFactor;
        rDistScreen = glm::dot(tVecScreenR,tVecEyeScreenB) * scaleFactor;
        bDistScreen = glm::dot(tVecScreenU,tVecEyeScreenA) * scaleFactor;
        tDistScreen = glm::dot(tVecScreenU,tVecEyeScreenC) * scaleFactor;

        // std::cout << " l: " << lDistScreen <<  " r: " << rDistScreen <<  " b: " << bDistScreen <<  " t: " << tDistScreen << std::endl;

        // render Room
        // -----------
        glm::mat4 projection = glm::frustum(lDistScreen, rDistScreen, bDistScreen, tDistScreen, nClipDist, fClipDist);

        // From screen space to the XY plane (should be an indentity matrix currently)
        glm::mat4 basis_change = glm::mat4(
            tVecScreenR.x,tVecScreenR.y,tVecScreenR.z,0.0f,
            tVecScreenU.x,tVecScreenU.y,tVecScreenU.z,0.0f,
            tVecScreenN.x,tVecScreenN.y,tVecScreenN.z,0.0f,
            0.0f,0.0f,0.0f,1.0f);

        glm::mat4 translate = glm::mat4(
            1.0f,0.0f,0.0f,-tPntEye.x,
            0.0f,1.0f,0.0f,-tPntEye.y,
            0.0f,0.0f,1.0f,-tPntEye.z,
            0.0f,0.0f,0.0f,1.0f);

        glm::mat4 tSpaceProjection = projection * basis_change * translate;
        ourShader.setMat4("projection", tSpaceProjection);
        ourShader.setMat4("view", glm::mat4(1.0f));
        ourShader.setMat4("model",glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,0.0f,0.0f)));

        glBindVertexArray(wallsVAO);
        glDrawElements(GL_TRIANGLES, sizeof(wallIndices), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &wallsVAO);
    glDeleteBuffers(1, &wallsVBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window, glm::vec3& deltaTPntEye)
{   
    GLfloat tickChange = 0.01f;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        std::cout << "W" << std::endl;
        deltaTPntEye.y =  tickChange;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        std::cout << "S" << std::endl;
        deltaTPntEye.y = -tickChange;
    }   
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        std::cout << "A" << std::endl;
        deltaTPntEye.x = -tickChange;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        std::cout << "D" << std::endl;
        deltaTPntEye.x = tickChange;
    }
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        std::cout << "Z" << std::endl;
        deltaTPntEye.z = tickChange;
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        std::cout << "X" << std::endl;
        deltaTPntEye.z = -tickChange;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
