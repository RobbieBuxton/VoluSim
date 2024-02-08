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
#include <opencv2/imgproc/imgproc.hpp>

#include "main.hpp"
#include "display.hpp"
#include "tracker.hpp"
#include "filesystem.hpp"
#include "shader.hpp"
#include "model.hpp"
#include "image.hpp"

// The MAIN function, from here we start the application and run the game loop
int main()
{
    debugInitPrint();
    GLuint maxPixelWidth = 3840;
    GLuint maxPixelHeight = 2160;
    GLuint pixelWidth = 3840;
    GLuint pixelHeight = 2160;
    GLFWwindow *window = initOpenGL(pixelWidth, pixelHeight);

    // build and compile our shader zprogram
    // ------------------------------------
    Shader modelShader(FileSystem::getPath("data/shaders/camera.vs").c_str(), FileSystem::getPath("data/shaders/camera.fs").c_str());

    // Robbie's Screen
    // Depth is artifical the others are real
    GLfloat dWidth = 70.5f;
    GLfloat dHeight = 39.5f;
    GLfloat dDepth = 39.5f;
    GLfloat pixelScaledWidth = dWidth * ((GLfloat)pixelWidth / (GLfloat)maxPixelWidth);
    GLfloat pixelScaledHeight = dHeight * ((GLfloat)pixelHeight / (GLfloat)maxPixelHeight);
    Display Display(glm::vec3(0.0f, 0.f, 0.f), pixelScaledWidth, pixelScaledHeight, pixelScaledHeight, 0.01f, 1000.0f);

    std::unique_ptr<Tracker> trackerPtr = std::make_unique<Tracker>();

    Model room("data/resources/models/room.obj");
    Model chessSet("data/resources/models/chessSet.obj");

    std::cout << "Finished Load" << std::endl;

    glm::vec3 cameraOffset = glm::vec3(0.0f, 6.0f, -6.0f);
    std::thread trackerThread(pollTracker, trackerPtr.get(), window);

    // render loop
    // -----------
    Image colourCamera = Image(glm::vec2(0.01, 0.99), glm::vec2(0.16, 0.84));
    Image depthCamera = Image(glm::vec2(0.17, 0.99), glm::vec2(0.32, 0.84));
    Image debugInfo = Image(glm::vec2(0.99, 0.89), glm::vec2(0.89, 0.99));

    // Timing variables
    double lastTime = glfwGetTime();
    int nbFrames = 0;

    glm::vec3 lastEyePos = trackerPtr->getEyePos() + cameraOffset;
    // Initialize a counter for eye position changes
    int eyePosChangeCount = 0;

    while (!glfwWindowShouldClose(window))
    {
        // Measure speed
        double currentTime = glfwGetTime();
        nbFrames++;
        // Check if the eye position has changed
        glm::vec3 currentEyePos = trackerPtr->getEyePos() + cameraOffset;
        if (glm::length(currentEyePos - lastEyePos) > std::numeric_limits<float>::epsilon())
        {
            // Eye position has changed
            eyePosChangeCount++;
            lastEyePos = currentEyePos; // Update the last eye position
        }

        if ( currentTime - lastTime >= 1.0 ){
            debugInfo.updateImage(generateDebugPrintBox(eyePosChangeCount));
            nbFrames = 0;
            lastTime += 1.0;
            eyePosChangeCount = 0;
        }

        processInput(window);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model, scaleMatrix, translationMatrix, centeringMatrix;
        // activate shader
        modelShader.use();

        modelShader.setMat4("projection", Display.projectionToEye(currentEyePos));
        modelShader.setVec3("viewPos", currentEyePos);
        modelShader.setVec3("lightPos", glm::vec3(0.0f, Display.height, 80.0f));
        modelShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

        scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(Display.width, Display.height, Display.depth));
        translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, Display.height / 2.0, -Display.depth / 2.0));
        model = translationMatrix * scaleMatrix;
        modelShader.setMat4("model", model);
        modelShader.setVec3("objectColor", glm::vec3(0.5f, 0.5f, 0.5f));

        room.Draw(modelShader);

        centeringMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 2.0, 0.0));
        scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.5, 1.5, 1.5));
        translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, Display.height / 3.0, 0));
        model = translationMatrix * scaleMatrix * centeringMatrix;
        modelShader.setMat4("model", model);
        modelShader.setVec3("objectColor", glm::vec3(0.5f, 0.5f, 0.0f));

        chessSet.Draw(modelShader);

        // Need to convert this to render with opengl rather than opencv
        if (!trackerPtr->getColorImage().empty())
            colourCamera.updateImage(trackerPtr->getColorImage());

        if (!trackerPtr->getDepthImage().empty())
        {
            depthCamera.updateImage(trackerPtr->getDepthImage());
        }

        debugInfo.displayImage();
        colourCamera.displayImage();
        depthCamera.displayImage();

        std::vector<cv::Point3d> pointCloud = trackerPtr->getPointCloud();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    trackerThread.join();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

cv::Mat generateDebugPrintBox(int fps)
{
    cv::Mat img = cv::Mat::zeros(500, 500, CV_8UC3);
    img.setTo(cv::Scalar(255, 255, 255)); // Set the color to white

    // Set the text parameters
    std::string text = "Camera FPS: " + std::to_string(fps);

    int fontFace = cv::FONT_HERSHEY_COMPLEX;
    double fontScale = 1.25;
    int thickness = 2;
    cv::Scalar textColor(0, 0, 0); // Black color for the text
    int baseline = 0;

    // Calculate the text size
    cv::Size textSize = cv::getTextSize(text, fontFace, fontScale, thickness, &baseline);
    baseline += thickness;

    // Center the text
    cv::Point textOrg((img.cols - textSize.width) / 2, (img.rows + textSize.height) / 2);

    // Put the text on the image
    cv::putText(img, text, textOrg, fontFace, fontScale, textColor, thickness);
 
    // Flip along the horizontal axis
    cv::flip(img, img, 0);
    cv::flip(img, img, 1);
    return img;
}

GLFWwindow *initOpenGL(GLuint pixelWidth, GLuint pixelHeight)
{
    std::cout << "Starting GLFW context, OpenGL 4.6" << std::endl;
    // Init GLFW
    glfwInit();
    // Set all the required options for GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // Create a GLFWwindow object that we can use for GLFW's functions
    GLFWwindow *window = glfwCreateWindow(pixelWidth, pixelHeight, "LearnOpenGL", NULL, NULL);
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
    glViewport(0, 0, pixelWidth, pixelHeight);

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
            trackerPtr->update();
        }
        catch (const std::exception &e)
        {
            std::cout << e.what() << '\n';
        }
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

void debugInitPrint()
{
#ifdef DLIB_USE_CUDA
    std::cout << "Dlib is compiled with CUDA support." << std::endl;
#endif
#ifdef DLIB_USE_FFTW
    std::cout << "Dlib is compiled with FFTW support." << std::endl;
#endif
#ifdef DLIB_USE_BLAS
    std::cout << "Dlib is compiled with BLAS support." << std::endl;
#endif
#ifdef DLIB_USE_LAPACK
    std::cout << "Dlib is compiled with LAPACK support." << std::endl;
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
}