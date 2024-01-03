#include <k4a/k4a.h>
#include <iostream>
#include <exception>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/cudawarping.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <dlib/opencv.h>
#include <dlib/dnn.h>

#include <chrono>

#include "tracker.hpp"
#include "filesystem.hpp"

class TrackerException : public std::exception
{
private:
    std::string message;

public:
    explicit TrackerException(const char *msg) : message(msg) {}

    virtual const char *what() const throw()
    {
        return message.c_str();
    }
};

class NoTrackersDetectedException : public TrackerException
{
public:
    NoTrackersDetectedException() : TrackerException("No Trackers were detected") {}
};

class FailedToOpenTrackerException : public TrackerException
{
public:
    FailedToOpenTrackerException() : TrackerException("Cannot open Tracker") {}
};

class FailedToStartTrackerException : public TrackerException
{
public:
    FailedToStartTrackerException() : TrackerException("Cannot start the Tracker cameras") {}
};

class FailedToDetectFaceException : public TrackerException
{
public:
    FailedToDetectFaceException() : TrackerException("Could not detect face from capture") {}
};

Tracker::Tracker()
{
    // Check for Trackers
    uint32_t count = k4a_device_get_installed_count();
    if (count == 0)
    {
        throw NoTrackersDetectedException();
    }
    else
    {
        std::cout << "k4a device attached!" << std::endl;

        // Open the first plugged in Tracker device
        device = NULL;
        if (K4A_FAILED(k4a_device_open(K4A_DEVICE_DEFAULT, &device)))
        {
            throw FailedToOpenTrackerException();
        }

        // Get the size of the serial number
        size_t serial_size = 0;
        k4a_device_get_serialnum(device, NULL, &serial_size);

        // Allocate memory for the serial, then acquire it
        char *serial = (char *)(malloc(serial_size));
        k4a_device_get_serialnum(device, serial, &serial_size);
        printf("Opened device: %s\n", serial);
        free(serial);

        config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
        config.camera_fps = K4A_FRAMES_PER_SECOND_30;
        config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
        config.color_resolution = K4A_COLOR_RESOLUTION_1080P;
        config.depth_mode = K4A_DEPTH_MODE_WFOV_2X2BINNED;
        config.synchronized_images_only = true;

        if (K4A_RESULT_SUCCEEDED != k4a_device_start_cameras(device, &config))
        {
            k4a_device_close(device);
            throw FailedToStartTrackerException();
        }

        // Display the color image
        k4a_device_get_calibration(device, config.depth_mode, config.color_resolution, &calibration);
        transformation = k4a_transformation_create(&calibration);

        dlib::deserialize(FileSystem::getPath("data/shape_predictor_5_face_landmarks.dat").c_str()) >> predictor;
        dlib::deserialize(FileSystem::getPath("data/mmod_human_face_detector.dat").c_str()) >> cnn_face_detector;

        // Head distance 50cm
        eyePos = glm::vec3(0.0f, 0.0f, 50.0f);
    }
}

void Tracker::updateEyePos()
{
    try
    {
        using std::chrono::duration;
        using std::chrono::duration_cast;
        using std::chrono::high_resolution_clock;
        using std::chrono::milliseconds;
        auto tStart = high_resolution_clock::now();
        Tracker::Capture capture(device, transformation);
        auto tCapture = high_resolution_clock::now();

        int depth_width = k4a_image_get_width_pixels(capture.cDepthImage);
        int depth_height = k4a_image_get_height_pixels(capture.cDepthImage);
        cv::Mat depth_mat(depth_height, depth_width, CV_16U, k4a_image_get_buffer(capture.cDepthImage), (size_t)k4a_image_get_stride_bytes(capture.cDepthImage));
        cv::Mat depth_display;
        cv::normalize(depth_mat, depth_display, 0, 255, cv::NORM_MINMAX, CV_8U);

        // This is pretty inefficent, needs a refactor to directly convert into bgr instead of bgra then bgr
        cv::Mat bgraImage(k4a_image_get_height_pixels(capture.cColorImage), k4a_image_get_width_pixels(capture.cColorImage), CV_8UC4, k4a_image_get_buffer(capture.cColorImage), (size_t)k4a_image_get_stride_bytes(capture.cColorImage));

        std::vector<cv::Mat> channels;
        cv::split(bgraImage, channels);

        // Create a 3-channel image by merging the first 3 channels (assuming BGR order)
        cv::Mat bgrImage;
        cv::merge(std::vector<cv::Mat>{channels[0], channels[1], channels[2]}, bgrImage);

        cv::cuda::GpuMat bgrImageGpu(bgrImage);

        int scale_factor = 1;
        for (int i = 0; i < scale_factor; i++)
        {
            cv::cuda::pyrDown(bgrImageGpu, bgrImageGpu);
        }

        cv::Mat processedBgrImage(bgrImageGpu);
        auto tOpenCVProcessing = high_resolution_clock::now();

        dlib::cv_image<dlib::bgr_pixel> dlib_img = dlib::cv_image<dlib::bgr_pixel>(processedBgrImage);

        std::vector<dlib::matrix<dlib::rgb_pixel>> batch;
        dlib::matrix<dlib::rgb_pixel> dlib_matrix;
        dlib::assign_image(dlib_matrix, dlib_img);
        batch.push_back(dlib_matrix);

        auto detections = cnn_face_detector(batch);
        auto tFace = high_resolution_clock::now();

        // Get first from batch (I think need to double check this)
        std::vector<dlib::mmod_rect> dets = detections[0];

        if (dets.empty())
        {
            cv::Mat flipped_depth_display;
            cv::flip(depth_display, flipped_depth_display, 1);
            // cv::imshow("Color Image", flipped_depth_display);
            cv::waitKey(1);
            throw FailedToDetectFaceException();
        }

        for (unsigned long j = 0; j < dets.size(); ++j)
        {
            dlib::rectangle rect = dets[j].rect;
            dlib::full_object_detection shape = predictor(dlib_img, rect);
            auto tLandmarks = high_resolution_clock::now();
            // This is slightly inaccurate because down down pyramid has size Size((src.cols+1)/2, (src.rows+1)/2)
            cv::Point leftEye(
                (shape.part(0).x() + shape.part(1).x()) * pow(2, scale_factor) / 2.0,
                (shape.part(0).y() + shape.part(1).y()) * pow(2, scale_factor) / 2.0);
            cv::circle(depth_display, leftEye, 10, cv::Scalar(0, 255, 0), -1);

            ushort depth = depth_mat.at<ushort>(leftEye.y, leftEye.x);
            k4a_float2_t k4a_point = {static_cast<float>(leftEye.x), static_cast<float>(leftEye.y)};
            k4a_float3_t camera_point;
            int valid;
            if (K4A_RESULT_SUCCEEDED != k4a_calibration_2d_to_3d(&calibration, &k4a_point, depth, K4A_CALIBRATION_TYPE_COLOR, K4A_CALIBRATION_TYPE_COLOR, &camera_point, &valid))
            {
                std::cout << "Failed to convert from 2d to 3d" << std::endl;
                exit(EXIT_FAILURE);
            }

            if (!valid)
            {
                std::cout << "Failed to convert to valid 3d coords" << std::endl;
                exit(EXIT_FAILURE);
            }
            eyePos = glm::vec3((-(float)camera_point.xyz.x) / 10.0, -((float)camera_point.xyz.y) / 10.0, ((float)camera_point.xyz.z) / 10.0);

            auto tEnd = high_resolution_clock::now();
            /* Getting number of milliseconds as a double. */
            duration<double, std::milli> ms_double = tStart - tEnd;
            duration<double, std::milli> ms_capture = tStart - tCapture;
            duration<double, std::milli> ms_opencv = tCapture - tOpenCVProcessing;
            duration<double, std::milli> ms_face = tOpenCVProcessing - tFace;
            duration<double, std::milli> ms_landmarks = tFace - tLandmarks;
            std::cout << "Total:   " << -ms_double.count() << "ms" << std::endl;
            std::cout << "Capture: " << -ms_capture.count() << "ms" << std::endl;
            std::cout << "OpenCV:  " << -ms_opencv.count() << "ms" << std::endl;
            std::cout << "Dlib f:  " << -ms_face.count() << "ms" << std::endl;
            std::cout << "Dlib l:  " << -ms_landmarks.count() << "ms" << std::endl;
            std::cout << "----" << std::endl;
        }
        cv::Mat flipped_depth_display;
        cv::flip(depth_display, flipped_depth_display, 1);
        // cv::imshow("Color Image", flipped_depth_display);
        cv::waitKey(1);
    }
    catch (const std::exception &e)
    {
        throw;
    }
}

Tracker::~Tracker()
{
    // Shut down the camera when finished with application logic
    k4a_device_stop_cameras(device);
    k4a_device_close(device);
}

class CaptureException : public std::exception
{
private:
    std::string message;

public:
    explicit CaptureException(const char *msg) : message(msg) {}

    virtual const char *what() const throw()
    {
        return message.c_str();
    }
};

class CaptureTimeoutException : public CaptureException
{
public:
    CaptureTimeoutException() : CaptureException("Timed out waiting for a capture") {}
};

class CaptureReadFailedException : public CaptureException
{
public:
    CaptureReadFailedException() : CaptureException("Failed to read a capture") {}
};

Tracker::Capture::Capture(k4a_device_t device, k4a_transformation_t transformation)
{
    // Capture a depth frame
    switch (k4a_device_get_capture(device, &capture, timeout))
    {
    case K4A_WAIT_RESULT_SUCCEEDED:
        break;
    case K4A_WAIT_RESULT_TIMEOUT:
        throw CaptureTimeoutException();
    case K4A_WAIT_RESULT_FAILED:
        throw CaptureReadFailedException();
    }

    cColorImage = k4a_capture_get_color_image(capture);
    dDepthImage = k4a_capture_get_depth_image(capture);
    if (K4A_RESULT_FAILED == k4a_image_create(K4A_IMAGE_FORMAT_DEPTH16, k4a_image_get_width_pixels(cColorImage), k4a_image_get_height_pixels(cColorImage), k4a_image_get_width_pixels(cColorImage) * (int)sizeof(uint16_t), &cDepthImage))
    {
        k4a_image_release(cColorImage);
        k4a_image_release(dDepthImage);
        k4a_capture_release(capture);
        std::cout << "Failed to create empty transformed_depth_image" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (K4A_RESULT_FAILED == k4a_transformation_depth_image_to_color_camera(transformation, dDepthImage, cDepthImage))
    {
        k4a_image_release(cColorImage);
        k4a_image_release(cDepthImage);
        k4a_image_release(dDepthImage);
        k4a_capture_release(capture);
        std::cout << "Failed to create transformed_depth_image" << std::endl;
        exit(EXIT_FAILURE);
    }
}

Tracker::Capture::~Capture()
{
    k4a_image_release(cColorImage);
    k4a_image_release(cDepthImage);
    k4a_image_release(dDepthImage);
}