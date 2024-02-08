#include <k4a/k4a.h>
#include <iostream>
#include <algorithm>
#include <exception>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/cudaimgproc.hpp>

#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <dlib/opencv.h>
#include <dlib/dnn.h>

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

std::vector<cv::Point3d> Tracker::getPointCloud()
{
    std::vector<cv::Point3d> pointCloud;
    if (captureInstance == nullptr)
    {
        return pointCloud;
    }
    if (captureInstance->dDepthImage == NULL)
    {
        return pointCloud;
    }
    int height = k4a_image_get_height_pixels(captureInstance->dDepthImage);
    int width = k4a_image_get_width_pixels(captureInstance->dDepthImage);

    // Get the depth image buffer
    uint16_t* depthBuffer = reinterpret_cast<uint16_t*>(k4a_image_get_buffer(captureInstance->dDepthImage));

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            // Calculate the index for the current (x, y) pixel
            int index = y * width + x;

            // Access the depth value at (x, y)
            uint16_t depth = depthBuffer[index];

            k4a_float2_t k4a_point = {static_cast<float>(x), static_cast<float>(y)};
            k4a_float3_t camera_point;
            int valid;
            if (K4A_RESULT_SUCCEEDED != k4a_calibration_2d_to_3d(&calibration, &k4a_point, depth, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_COLOR, &camera_point, &valid))
            {
                // std::cout << "Failed to convert from 2d to 3d" << std::endl;
                continue;
            }

            if (!valid)
            {
                // std::cout << "Failed to convert to valid 3d coords" << std::endl;
                continue;
            }
            pointCloud.push_back(cv::Point3d(camera_point.xyz.x / 10.0, camera_point.xyz.y / 10.0, camera_point.xyz.z / 10.0));
        }
    }
    
    return pointCloud;
}

void Tracker::update()
{
    try
    {
        captureInstance = std::make_unique<Capture>(device, transformation);

        // Directly upload BGRA image to GPU and convert to BGR
        cv::cuda::GpuMat bgraImageGpu, bgrImageGpu;
        cv::Mat bgraImage(k4a_image_get_height_pixels(captureInstance->cColorImage), k4a_image_get_width_pixels(captureInstance->cColorImage), CV_8UC4, k4a_image_get_buffer(captureInstance->cColorImage), (size_t)k4a_image_get_stride_bytes(captureInstance->cColorImage));
        bgraImageGpu.upload(bgraImage);
        cv::cuda::cvtColor(bgraImageGpu, bgrImageGpu, cv::COLOR_BGRA2BGR);

        // Perform any GPU-based image processing
        cv::cuda::pyrDown(bgrImageGpu, bgrImageGpu);
        
        // Download the processed image from GPU to CPU for further processing with Dlib
        cv::Mat processedBgrImage;
        bgrImageGpu.download(processedBgrImage);
        
        dlib::cv_image<dlib::bgr_pixel> dlib_img = dlib::cv_image<dlib::bgr_pixel>(processedBgrImage);

        std::vector<dlib::matrix<dlib::rgb_pixel>> batch;
        dlib::matrix<dlib::rgb_pixel> dlib_matrix;
        dlib::assign_image(dlib_matrix, dlib_img);
        batch.push_back(dlib_matrix);

        cv::Mat dImage(k4a_image_get_height_pixels(captureInstance->cDepthImage), k4a_image_get_width_pixels(captureInstance->cDepthImage), CV_16U, k4a_image_get_buffer(captureInstance->cDepthImage), (size_t)k4a_image_get_stride_bytes(captureInstance->cDepthImage));

        cv::Mat normalisedDImage, colorDepthImage;
        cv::normalize(dImage, normalisedDImage, 0, 255, cv::NORM_MINMAX, CV_8U);
        cv::applyColorMap(normalisedDImage, colorDepthImage, cv::COLORMAP_JET);

        colorDepthImage.copyTo(depthImage);
        processedBgrImage.copyTo(colorImage);

        std::vector<dlib::rectangle> faces;
        std::vector<dlib::full_object_detection> landmarks;

        auto detections = cnn_face_detector(batch);

        // Get first from batch (I think need to double check this)
        std::vector<dlib::mmod_rect> dets = detections[0];

        if (dets.empty())
        {
            throw FailedToDetectFaceException();
        }

        for (int i = 0; i < dets.size(); i++)
        {
            faces.push_back(dets[i].rect);
            landmarks.push_back(predictor(dlib_img, dets[i].rect));
        }

        std::vector<cv::Point> eyes;
        for (int i = 0; i < faces.size(); i++) {
            // We don't need to div by 2 because we pyradown the image
            eyes.push_back(cv::Point(
                landmarks[i].part(0).x() + landmarks[i].part(1).x(),
                landmarks[i].part(0).y() + landmarks[i].part(1).y()));
            eyes.push_back(cv::Point(
                landmarks[i].part(2).x() + landmarks[i].part(3).x(),
                landmarks[i].part(2).y() + landmarks[i].part(3).y()));

            ushort depth = dImage.at<ushort>(eyes[0].y, eyes[0].x);
            k4a_float2_t k4a_point = {static_cast<float>(eyes[0].x), static_cast<float>(eyes[0].y)};
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
        }

        //Draw Detected faces on output
        for (unsigned long i = 0; i < faces.size(); i++)
        {
            cv::rectangle(colorImage, cv::Point(faces[i].left(), faces[i].top()), cv::Point(faces[i].right(), faces[i].bottom()), cv::Scalar(0, 255, 0), 2);
            for (unsigned long j = 0; j < landmarks[i].num_parts(); j++)
            {
                cv::circle(colorImage, cv::Point(landmarks[i].part(j).x(), landmarks[i].part(j).y()), 3, cv::Scalar(0, 0, 255), -1);
            }
        }

        //Draw location depth is sampled from
        for (unsigned long i = 0; i < eyes.size(); i++)
        {
            cv::circle(depthImage, eyes[i], 10, cv::Scalar(255, 0, 0), -1);
        }

        cv::waitKey(1);
    }
    catch (const std::exception &e)
    {
        throw;
    }
}


glm::vec3 Tracker::getEyePos()
{
    return eyePos;
}

cv::Mat Tracker::getDepthImage()
{
    return depthImage;
}

cv::Mat Tracker::getColorImage()
{
    return colorImage;
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

