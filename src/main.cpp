#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <k4a/k4a.h>
#include <iostream>
#include <unistd.h>
#include <exception>
#include <thread>
#include <chrono>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/cuda.hpp>

#include "main.hpp"
#include "display.hpp"
#include "tracker.hpp"
#include "filesystem.hpp"
#include "shader.hpp"
#include "model.hpp"

// timing
GLfloat deltaTime = 0.0f; // time between current frame and last frame
GLfloat lastFrame = 0.0f;

// The MAIN function, from here we start the application and run the game loop
int main()
{

#ifdef DLIB_USE_CUDA
    std::cout << "Dlib is compiled with CUDA support." << std::endl;
#else
    std::cout << "Dlib is not compiled with CUDA support." << std::endl;
#endif
#ifdef __AVX__
	std::cout << "AVX on" << std::endl;
#endif
#ifdef DLIB_HAVE_SSE2
	std::cout << "DLIB_HAVE_SSE2 on" << std::endl;
#endif
#ifdef DLIB_HAVE_SSE3
	std::cout << "DLIB_HAVE_SSE3 on" << std::endl;
#endif
#ifdef DLIB_HAVE_SSE41
	std::cout << "DLIB_HAVE_SSE41 on" << std::endl;
#endif
#ifdef DLIB_HAVE_AVX
	std::cout << "DLIB_HAVE_AVX on" << std::endl;
#endif


    int num_devices = cv::cuda::getCudaEnabledDeviceCount();
    std::cout << "Number of OpenCV CUDA devices detected: " << num_devices << std::endl;

    if (geteuid() != 0)
    {
        std::cout << "ERROR: Application needs root to be able to use Tracker" << std::endl;
        exit(EXIT_FAILURE);
    }

    GLFWwindow *window = initOpenGL();

    // build and compile our shader zprogram
    // ------------------------------------
    Shader ourShader(FileSystem::getPath("data/shaders/camera.vs").c_str(), FileSystem::getPath("data/shaders/camera.fs").c_str());

    // Robbie's Screen
    // Width = 70.5cm, Height = 39.5cm
    Display Display(glm::vec3(0.0f, 0.f, 0.f), 70.5f, 39.5f, 39.5f, 0.1f, 500.0f);

    std::unique_ptr<Tracker> trackerPtr = std::make_unique<Tracker>();

    Model myModel("data/resources/models/cornell_box.obj");
    std::cout << "Finished Load" << std::endl;

    std::thread trackerThread(pollTracker, trackerPtr.get(), window);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // activate shader
        ourShader.use();

        ourShader.setMat4("projection", Display.projectionToEye(trackerPtr->eyePos));
        ourShader.setMat4("view", glm::mat4(1.0f));

        glm::mat4 model = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(Display.width / 2.0, Display.height / 2.0, Display.depth / 2.0)), glm::vec3(0, -1, -1));

        ourShader.setMat4("model", model);

        myModel.Draw(ourShader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    trackerThread.join();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

GLFWwindow *initOpenGL()
{
    std::cout << "Starting GLFW context, OpenGL 4.6" << std::endl;
    // Init GLFW
    glfwInit();
    // Set all the required options for GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLuint WIDTH = 3840;
    GLuint HEIGHT = 2160;

    // Create a GLFWwindow object that we can use for GLFW's functions
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", NULL, NULL);
    glfwMakeContextCurrent(window);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Load OpenGL functions, gladLoadGL returns the loaded version, 0 on error.
    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0)
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Successfully loaded OpenGL
    std::cout << "Loaded OpenGL " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << std::endl;

    // Define the viewport dimensions
    glViewport(0, 0, WIDTH, HEIGHT);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    return window;
}

void pollTracker(Tracker *trackerPtr, GLFWwindow *window)
{
    while (!glfwWindowShouldClose(window))
    {
        try
        {
            trackerPtr->updateEyePos();
        }
        catch (const std::exception &e)
        {
            // std::cout << e.what() << '\n';
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
