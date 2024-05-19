#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "mediapipe.h"

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
#include <optional>
#include <opencv2/core.hpp>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "main.hpp"
#include "display.hpp"
#include "filesystem.hpp"
#include "shader.hpp"
#include "model.hpp"
#include "pointcloud.hpp"
#include "challenge.hpp"
#include "renderer.hpp"

extern "C"
{
	static std::string outputString;

	const char *runSimulation(Mode trackerMode, int challengeNum, float  camera_x, float  camera_y, float  camera_z, float  camera_rot)
	{
		debugInitPrint();
		// GLuint maxPixelWidth = 3840;
		// qw GLuint maxPixelHeight = 2160;
		// GLuint pixelWidth = 3840;
		// GLuint pixelHeight = 2160;
		// GLfloat dWidth = 70.5f;
		// GLfloat dHeight = 39.5f;
		// GLfloat dDepth = 0.01f;

		GLuint pixelWidth = 1200;
		GLuint pixelHeight = 1920;
		GLfloat dHeight = 52.0f;
		GLfloat dWidth = 34.0f;
		GLfloat dDepth = 0.01f;

		GLFWwindow *window = initOpenGL(pixelWidth, pixelHeight);

		bool debug = true;

		// Robbie's Screen
		Display display(glm::vec3(0.0f, 0.f, 0.f), dWidth, dHeight, dDepth, 1.0f, 1000.0f);
		std::shared_ptr<Renderer> renderer = std::make_shared<Renderer>(display);
		std::unique_ptr<Tracker> trackerPtr = std::make_unique<Tracker>(glm::vec3(camera_x, camera_y, camera_z), camera_rot, debug);

		std::thread trackerThread(pollTracker, trackerPtr.get(), window);
		std::thread captureThread(pollCapture, trackerPtr.get(), window);

		// render loop
		// -----------
		Image colourCamera = Image(glm::vec2(0.01, 0.99), glm::vec2(0.16, 0.80));
		Image colourCameraSkeleton = Image(glm::vec2(0.01, 0.99), glm::vec2(0.16, 0.80));
		Image colourCameraSkeletonFace = Image(glm::vec2(0.01, 0.99), glm::vec2(0.16, 0.80));
		Image colourCameraSkeletonHand = Image(glm::vec2(0.01, 0.99), glm::vec2(0.16, 0.80));
		Image colourCameraImportant = Image(glm::vec2(0.01, 0.99), glm::vec2(0.16, 0.80));
		Image depthCamera = Image(glm::vec2(0.17, 0.99), glm::vec2(0.32, 0.80));
		Image depthCameraImportant = Image(glm::vec2(0.17, 0.99), glm::vec2(0.32, 0.80));
		Image debugInfo = Image(glm::vec2(0.99, 0.89), glm::vec2(0.89, 0.99));

		// Timing variables
		double lastTime = glfwGetTime();
		int nbFrames = 0;

		glm::vec3 currentEyePos = glm::vec3(0.0f, 0.0f, 0.0f);

		std::shared_ptr<Hand> hand;
		if (trackerMode == TRACKEROFFSET)
		{
			hand = std::make_shared<Hand>(renderer, glm::vec3(5.0f, 0.0f, 0.0f));
		}
		else
		{
			hand = std::make_shared<Hand>(renderer, glm::vec3(0.0f, 0.0f, 0.0f));
		}
		nlohmann::json jsonOutput;
		// Get current time in milliseconds
		auto startTimeInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
										   std::chrono::system_clock::now().time_since_epoch())
										   .count();
		
		auto currentTimeInMilliseconds = startTimeInMilliseconds;

		// Assign the time to the "startTime" key in the JSON object
		jsonOutput["startTime"] = currentTimeInMilliseconds;
		Challenge challenge = Challenge(renderer, hand, challengeNum);
		while (!trackerPtr->isReady())
		{
			std::cout << "Waiting for tracker" << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		while (!glfwWindowShouldClose(window))
		{
			// Measure speed
			double currentTime = glfwGetTime();

			currentTimeInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
											std::chrono::system_clock::now().time_since_epoch())
											.count();

			if ((currentTimeInMilliseconds - startTimeInMilliseconds) > 60000)
			{
				std::cout << "Timeout: " << (currentTimeInMilliseconds - startTimeInMilliseconds) << "ms" << std::endl;
				glfwSetWindowShouldClose(window, true);
			}

			// Check if the eye position has changed
			if (trackerMode == TRACKER || trackerMode == TRACKEROFFSET)
			{
				std::optional<glm::vec3> leftEyePos = trackerPtr->getLeftEyePos();
				if (leftEyePos.has_value())
				{
					nbFrames++;
					currentEyePos = leftEyePos.value();
				}
				else
				{
					currentEyePos = currentEyePos;
				}
			}
			else if (trackerMode == STATIC)
			{
				currentEyePos = glm::vec3(0.0f, -45.0f, 40.0f);
			}

			if (currentTime - lastTime >= 1.0)
			{
				if (debug)
				{
					debugInfo.updateImage(generateDebugPrintBox(nbFrames));
				}
				nbFrames = 0;
				lastTime += 1.0;
			}

			renderer->updateEyePos(currentEyePos);

			if (debug)
			{
				// Need to convert this to render with opengl rather than opencv
				if (!trackerPtr->getColorImage().empty())
				{
					colourCameraSkeleton.updateImage(trackerPtr->getColorImageSkeletons());
				}
				if (!trackerPtr->getDepthImage().empty())
				{
					depthCameraImportant.updateImage(trackerPtr->getDepthImageImportant());
				}
			}

			hand->updateLandmarks(trackerPtr->getHandLandmarks());
			challenge.update();


			processInput(window);
			renderer->clear();
			hand->draw();
			challenge.draw();

			if (debug)
			{
				renderer->drawImage(debugInfo);
				renderer->drawImage(colourCameraSkeleton);
				renderer->drawImage(depthCameraImportant);
			}
			// renderer->drawRoom();
			// renderer->renderErato();
			if (challenge.isFinished())
			{
				glfwSetWindowShouldClose(window, true);
			}

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
		trackerThread.join();
		captureThread.join();

		if (debug)
		{
			colourCamera.updateImage(trackerPtr->getColorImage());
			colourCameraSkeleton.updateImage(trackerPtr->getColorImageSkeletons());
			colourCameraSkeletonFace.updateImage(trackerPtr->getColorImageSkeletonFace());
			colourCameraSkeletonHand.updateImage(trackerPtr->getColorImageSkeletonHand());

			colourCameraImportant.updateImage(trackerPtr->getColorImageImportant());
			depthCamera.updateImage(trackerPtr->getDepthImage());
			depthCameraImportant.updateImage(trackerPtr->getDepthImageImportant());

			saveDebugInfo(*trackerPtr, colourCamera, colourCameraSkeleton, colourCameraSkeletonFace, colourCameraSkeletonHand, colourCameraImportant, depthCamera, depthCameraImportant, *hand);
		}

		jsonOutput["results"] = challenge.returnJson();
		jsonOutput["trackerLogs"] = trackerPtr->returnJson();
		jsonOutput["finished"] = challenge.isFinished();

		outputString = jsonOutput.dump();

		// glfw: terminate, clearing all previously allocated GLFW resources.
		// ------------------------------------------------------------------
		glfwTerminate();
		return outputString.c_str();
	}
}

void saveDebugInfo(Tracker &trackerPtr, Image &colourCamera, Image &colourCameraSkeleton, Image &colourCameraSkeletonFace, Image &colourCameraSkeletonHand, Image &colourCameraImportant, Image &depthCamera, Image &depthCameraImportant, Hand &hand)
{
	std::string miscPath = "/home/robbieb/Projects/VolumetricSim/misc/";

	std::cout << "Saving data to " << miscPath << std::endl;

	PointCloud pointCloud = PointCloud();
	pointCloud.updateCloud(trackerPtr.getPointCloud());
	std::cout << "Saving Point Cloud" << std::endl;
	pointCloud.save(miscPath + "pointCloud.csv");
	std::cout << "Saving Colour Camera" << std::endl;
	colourCamera.save(miscPath + "colourImage.png");
	colourCameraSkeleton.save(miscPath + "colourImageSkeleton.png");
	colourCameraSkeletonFace.save(miscPath + "colourImageSkeletonFace.png");
	colourCameraSkeletonHand.save(miscPath + "colourImageSkeletonHand.png");
	colourCameraImportant.save(miscPath + "colourImageImportant.png");
	std::cout << "Saving Depth Camera" << std::endl;
	depthCamera.save(miscPath + "depthImage.png");
	depthCameraImportant.save(miscPath + "depthImageImportant.png");

	std::cout << "Saving Eye Pos Left" << std::endl;
	std::optional<glm::vec3> leftEyePos = trackerPtr.getLeftEyePos();
	if (leftEyePos.has_value())
	{
		saveVec3ToCSV(leftEyePos.value(), miscPath + "leftEyePos.csv");
	}

	std::cout << "Saving Eye Pos Right" << std::endl;
	std::optional<glm::vec3> rightEyePos = trackerPtr.getRightEyePos();
	if (rightEyePos.has_value())
	{
		saveVec3ToCSV(rightEyePos.value(), miscPath + "rightEyePos.csv");
	}

	std::cout << "Saving Hand" << std::endl;
	hand.save(miscPath + "hand.csv");

	std::cout << "Data saved" << std::endl;
}

// This should be refactored/removed/done properly
void saveVec3ToCSV(const glm::vec3 &vec, const std::string &filename)
{
	std::ofstream file(filename);

	if (file.is_open())
	{
		// Write the glm::vec3 components to the file separated by commas
		file << vec.x << "," << vec.y << "," << vec.z << std::endl;
		file.close();
	}
	else
	{
		// Handle error
		std::cerr << "Unable to open file: " << filename << std::endl;
	}
}

cv::Mat generateDebugPrintBox(int fps)
{
	// Create a 500x500 image with 4 channels (RGBA), initially fully transparent
	cv::Mat img = cv::Mat::zeros(500, 500, CV_8UC4);

	// Set the text parameters
	std::string text = "Camera FPS: " + std::to_string(fps);
	int fontFace = cv::FONT_HERSHEY_COMPLEX;
	double fontScale = 1.25;
	int thickness = 2;
	// Use white color for the text with full opacity
	cv::Scalar textColor(255, 255, 255, 255);

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

void pollCapture(Tracker *trackerPtr, GLFWwindow *window)
{
	while (!glfwWindowShouldClose(window))
	{
		trackerPtr->getLatestCapture();
		// Wait for 20ms as camera is 30fps
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
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