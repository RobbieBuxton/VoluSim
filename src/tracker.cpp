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
        leftEyePos = glm::vec3(0.0f, 0.0f, 50.0f);
    }
}

glm::vec3 Tracker::calculate3DPos(int x, int y, k4a_calibration_type_t source_type)
{
    uint16_t depth;
    if (source_type == K4A_CALIBRATION_TYPE_COLOR)
    {
        uint16_t *depthBuffer = reinterpret_cast<uint16_t *>(k4a_image_get_buffer(captureInstance->colorSpace.depthImage));
        int index = y * captureInstance->colorSpace.width + x;
        depth = depthBuffer[index];
    }
    else if (source_type == K4A_CALIBRATION_TYPE_DEPTH)
    {
        uint16_t *depthBuffer = reinterpret_cast<uint16_t *>(k4a_image_get_buffer(captureInstance->depthSpace.depthImage));
        int index = y * captureInstance->depthSpace.width + x;
        depth = depthBuffer[index];
    }

    k4a_float2_t pointK4APoint = {static_cast<float>(x), static_cast<float>(y)};
    k4a_float3_t cameraPoint;
    int valid;
    if (K4A_RESULT_SUCCEEDED != k4a_calibration_2d_to_3d(&calibration, &pointK4APoint, depth, source_type, K4A_CALIBRATION_TYPE_COLOR, &cameraPoint, &valid))
    {
        // std::cout << "Failed to convert from 2d to 3d" << std::endl;
        // exit(EXIT_FAILURE);
    }

    if (!valid)
    {
        // std::cout << "Failed to convert to valid 3d coords" << std::endl;
        // exit(EXIT_FAILURE);
    }
    return glm::vec3((-(float)cameraPoint.xyz.x) / 10.0, -((float)cameraPoint.xyz.y) / 10.0, ((float)cameraPoint.xyz.z) / 10.0);
}


std::vector<glm::vec3> Tracker::getPointCloudOld()
{
    std::vector<glm::vec3> pointCloud;
    if ((captureInstance == nullptr) || (captureInstance->depthSpace.depthImage == NULL))
    {
        return pointCloud;
    }

    for (int y = 0; y < captureInstance->depthSpace.height; y++)
    {
        for (int x = 0; x < captureInstance->depthSpace.width; x++)
        {
            pointCloud.push_back(calculate3DPos(x, y, K4A_CALIBRATION_TYPE_DEPTH));
        }
    }

    return pointCloud;
}

std::vector<glm::vec3> Tracker::getPointCloud()
{
    std::vector<glm::vec3> pointCloud;
    if ((captureInstance == nullptr) || (captureInstance->depthSpace.depthImage == NULL))
    {
        return pointCloud;
    }
    k4a_image_t pointCloudImage;

    // Create an image to hold the point cloud data
    k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM,
                     captureInstance->depthSpace.width,
                     captureInstance->depthSpace.height,
                     captureInstance->depthSpace.width * 3 * (int)sizeof(int16_t),
                     &pointCloudImage);

    // Transform the depth image to a point cloud
    k4a_transformation_depth_image_to_point_cloud(transformation, captureInstance->depthSpace.depthImage, K4A_CALIBRATION_TYPE_DEPTH, pointCloudImage);

    // Convert pointCloudImage to std::vector<glm::vec3>
    int16_t* pointCloudData = reinterpret_cast<int16_t*>(k4a_image_get_buffer(pointCloudImage));
    for (int i = 0; i < captureInstance->depthSpace.width * captureInstance->depthSpace.height; ++i)
    {
        int index = i * 3;
        float x = -static_cast<float>(pointCloudData[index]) / 10.0f;
        float y = -static_cast<float>(pointCloudData[index + 1]) / 10.0f;
        float z = static_cast<float>(pointCloudData[index + 2]) / 10.0f;
        pointCloud.push_back(glm::vec3(x, y, z));
    }

    // Remember to release the point cloud image after use
    k4a_image_release(pointCloudImage);

    return pointCloud;
}

void Tracker::update()
{
    try
    {
        captureInstance = std::make_unique<Capture>(device, transformation);

        // Directly upload BGRA image to GPU and convert to BGR
        cv::cuda::GpuMat bgraImageGpu, bgrImageGpu;
        cv::Mat bgraImage(captureInstance->colorSpace.height, captureInstance->colorSpace.width, CV_8UC4, k4a_image_get_buffer(captureInstance->colorSpace.colorImage), (size_t)k4a_image_get_stride_bytes(captureInstance->colorSpace.colorImage));
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

        cv::Mat dImage(captureInstance->colorSpace.height, captureInstance->colorSpace.width, CV_16U, k4a_image_get_buffer(captureInstance->colorSpace.depthImage), (size_t)k4a_image_get_stride_bytes(captureInstance->colorSpace.depthImage));

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
        for (int i = 0; i < faces.size(); i++)
        {
            // We don't need to div by 2 because we pyradown the image
            eyes.push_back(cv::Point(
                landmarks[i].part(0).x() + landmarks[i].part(1).x(),
                landmarks[i].part(0).y() + landmarks[i].part(1).y()));
            eyes.push_back(cv::Point(
                landmarks[i].part(2).x() + landmarks[i].part(3).x(),
                landmarks[i].part(2).y() + landmarks[i].part(3).y()));

            // Left eye
            leftEyePos = calculate3DPos(eyes[0].x, eyes[0].y, K4A_CALIBRATION_TYPE_COLOR);
            rightEyePos = calculate3DPos(eyes[1].x, eyes[1].y, K4A_CALIBRATION_TYPE_COLOR);
        }

        // Draw Detected faces on output
        for (unsigned long i = 0; i < faces.size(); i++)
        {
            cv::rectangle(colorImage, cv::Point(faces[i].left(), faces[i].top()), cv::Point(faces[i].right(), faces[i].bottom()), cv::Scalar(0, 255, 0), 2);
            for (unsigned long j = 0; j < landmarks[i].num_parts(); j++)
            {
                cv::circle(colorImage, cv::Point(landmarks[i].part(j).x(), landmarks[i].part(j).y()), 3, cv::Scalar(0, 0, 255), -1);
            }
        }

        // Draw location depth is sampled from
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

glm::vec3 Tracker::getLeftEyePos()
{
    return leftEyePos;
}

glm::vec3 Tracker::getRightEyePos()
{
    return rightEyePos;
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

    colorSpace.colorImage = k4a_capture_get_color_image(capture);
    colorSpace.width = k4a_image_get_width_pixels(colorSpace.colorImage);
    colorSpace.height = k4a_image_get_height_pixels(colorSpace.colorImage);
    depthSpace.depthImage = k4a_capture_get_depth_image(capture);
    depthSpace.width = k4a_image_get_width_pixels(depthSpace.depthImage);
    depthSpace.height = k4a_image_get_height_pixels(depthSpace.depthImage);
    if (K4A_RESULT_FAILED == k4a_image_create(K4A_IMAGE_FORMAT_DEPTH16, colorSpace.width, colorSpace.height, colorSpace.width * (int)sizeof(uint16_t), &colorSpace.depthImage))
    {
        k4a_image_release(colorSpace.colorImage);
        k4a_image_release(depthSpace.depthImage);
        k4a_capture_release(capture);
        std::cout << "Failed to create empty transformed_depth_image" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (K4A_RESULT_FAILED == k4a_transformation_depth_image_to_color_camera(transformation, depthSpace.depthImage, colorSpace.depthImage))
    {
        k4a_image_release(colorSpace.colorImage);
        k4a_image_release(colorSpace.depthImage);
        k4a_image_release(depthSpace.depthImage);
        k4a_capture_release(capture);
        std::cout << "Failed to create transformed_depth_image" << std::endl;
        exit(EXIT_FAILURE);
    }
}

Tracker::Capture::~Capture()
{
    k4a_image_release(colorSpace.colorImage);
    k4a_image_release(colorSpace.depthImage);
    k4a_image_release(depthSpace.depthImage);
}
