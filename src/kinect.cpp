#include "kinect.hpp"
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

class KinectException : public std::exception
{
private:
    std::string message;

public:
    explicit KinectException(const char *msg) : message(msg) {}

    virtual const char *what() const throw()
    {
        return message.c_str();
    }
};

class NoKinectsDetectedException : public KinectException
{
public:
    NoKinectsDetectedException() : KinectException("No Kinects were detected") {}
};

class FailedToOpenKinectException : public KinectException
{
public:
    FailedToOpenKinectException() : KinectException("Cannot open Kinect") {}
};

class FailedToStartKinectException : public KinectException
{
public:
    FailedToStartKinectException() : KinectException("Cannot start the Kinect cameras") {}
};

Kinect::Kinect()
{
    // Check for Kinects
    uint32_t count = k4a_device_get_installed_count();
    if (count == 0)
    {
        throw NoKinectsDetectedException();
    }
    else
    {
        std::cout << "k4a device attached!" << std::endl;

        // Open the first plugged in Kinect device
        device = NULL;
        if (K4A_FAILED(k4a_device_open(K4A_DEVICE_DEFAULT, &device)))
        {
            throw FailedToOpenKinectException();
        }

        // Get the size of the serial number
        size_t serial_size = 0;
        k4a_device_get_serialnum(device, NULL, &serial_size);

        // Allocate memory for the serial, then acquire it
        char *serial = (char *)(malloc(serial_size));
        k4a_device_get_serialnum(device, serial, &serial_size);
        printf("Opened device: %s\n", serial);
        free(serial);

        // Configure a stream of 4096x3072 BRGA color data at 15 frames per second
        config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
        config.camera_fps = K4A_FRAMES_PER_SECOND_30;
        // BGRA is not native to kinect might be worth switching to MJPEG
        config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
        config.color_resolution = K4A_COLOR_RESOLUTION_2160P;
        config.depth_mode = K4A_DEPTH_MODE_WFOV_2X2BINNED;
        config.synchronized_images_only = true;

        if (K4A_RESULT_SUCCEEDED != k4a_device_start_cameras(device, &config))
        {
            throw FailedToStartKinectException();
        }

        // Display the color image
        k4a_device_get_calibration(device, config.depth_mode, config.color_resolution, &calibration);
        transformation = k4a_transformation_create(&calibration);

        detector = dlib::get_frontal_face_detector();
        dlib::deserialize("/home/robbieb/Imperial/IndividualProject/VolumetricSim/data/shape_predictor_5_face_landmarks.dat") >> predictor;
    }
}

glm::vec3 Kinect::readFrame()
{
    std::cout << "----" << std::endl;
    // Capture a depth frame
    k4a_capture_t capture = NULL;
    switch (k4a_device_get_capture(device, &capture, 17))
    {
    case K4A_WAIT_RESULT_SUCCEEDED:
        break;
    case K4A_WAIT_RESULT_TIMEOUT:
        std::cout << "Timed out waiting for a capture" << std::endl;
        return glm::vec3(0,0,-1);
        break;
    case K4A_WAIT_RESULT_FAILED:
        std::cout << "Failed to read a capture" << std::endl;
        return glm::vec3(0,0,-1);
    }

    k4a_image_t color_image = k4a_capture_get_color_image(capture);
    k4a_image_t depth_image = k4a_capture_get_depth_image(capture);
    k4a_image_t transformed_depth_image;
    if (K4A_RESULT_FAILED == k4a_image_create(K4A_IMAGE_FORMAT_DEPTH16, k4a_image_get_width_pixels(color_image), k4a_image_get_height_pixels(color_image), k4a_image_get_width_pixels(color_image) *  (int)sizeof(uint16_t), &transformed_depth_image))
    {
        std::cout << "#####: Failed to create empty transformed_depth_image" << std::endl;
        return glm::vec3(0,0,-1);
    }
    if (K4A_RESULT_FAILED == k4a_transformation_depth_image_to_color_camera(transformation, depth_image, transformed_depth_image))
    {
        std::cout << "#####: Failed to create transformed_depth_image" << std::endl;
        return glm::vec3(0,0,-1);
    }
    
    glm::vec3 headPos;
    if (transformed_depth_image != nullptr && color_image != nullptr)
    {
        int depth_width = k4a_image_get_width_pixels(transformed_depth_image);
        int depth_height = k4a_image_get_height_pixels(transformed_depth_image);
        cv::Mat depth_mat(depth_height, depth_width, CV_16U, k4a_image_get_buffer(transformed_depth_image),(size_t)k4a_image_get_stride_bytes(transformed_depth_image));
        cv::Mat depth_display;
        cv::normalize(depth_mat, depth_display, 0, 255, cv::NORM_MINMAX, CV_8U);
    
        // This is pretty inefficent, needs a refactor to directly convert into bgr instead of bgra then bgr
        cv::Mat bgraImage(k4a_image_get_height_pixels(color_image), k4a_image_get_width_pixels(color_image), CV_8UC4, k4a_image_get_buffer(color_image), (size_t)k4a_image_get_stride_bytes(color_image));

        std::vector<cv::Mat> channels;
        cv::split(bgraImage, channels);

        // Create a 3-channel image by merging the first 3 channels (assuming BGR order)
        cv::Mat bgrImage;
        cv::merge(std::vector<cv::Mat>{channels[0], channels[1], channels[2]}, bgrImage);

        cv::cuda::GpuMat bgrImageGpu(bgrImage);

        int scale_factor = 2;
        for (int i = 0; i < scale_factor; i++)
        {
            cv::cuda::pyrDown(bgrImageGpu, bgrImageGpu);
        }

        cv::Mat processedBgrImage(bgrImageGpu);

        dlib::cv_image<dlib::bgr_pixel> dlib_img = dlib::cv_image<dlib::bgr_pixel>(processedBgrImage);
        auto dets = detector(dlib_img);
        std::cout << "Number of faces detected: " << dets.size() << std::endl;

        if (dets.size() == 0){
            return glm::vec3(0,0,-1);
        }

        for (unsigned long j = 0; j < dets.size(); ++j)
        {
            dlib::full_object_detection shape = predictor(dlib_img, dets[j]);
            cv::Point leftEye(
                (shape.part(0).x() + shape.part(1).x()) * pow(2,scale_factor) / 2.0,
                (shape.part(0).y() + shape.part(1).y()) * pow(2,scale_factor) / 2.0);
            cv::Point rightEye(
                (shape.part(2).x() + shape.part(3).x()) * pow(2,scale_factor) / 2.0,
                (shape.part(2).y() + shape.part(3).y()) * pow(2,scale_factor) / 2.0);

            cv::circle(depth_display, leftEye, 10, cv::Scalar(0, 255, 0), -1);
            cv::circle(depth_display, rightEye, 10, cv::Scalar(0, 255, 0), -1);
            cv::imshow("Color Image", depth_display);

            ushort depth = depth_mat.at<ushort>(leftEye.x,leftEye.y);
            std::cout << "Depth:" << depth << " x: " << leftEye.x << " y " << leftEye.y << std::endl;
            k4a_float2_t k4a_point;
            k4a_point.xy.x = leftEye.x;
            k4a_point.xy.y = leftEye.y;
            k4a_float3_t camera_point;
            int valid; 
            k4a_calibration_2d_to_3d(&calibration,&k4a_point,depth,K4A_CALIBRATION_TYPE_COLOR,K4A_CALIBRATION_TYPE_COLOR,&camera_point,&valid);
            
            if (valid) {
                headPos = glm::vec3(-((float)camera_point.xyz.x)/10.0, ((float)camera_point.xyz.y)/10.0, ((float)camera_point.xyz.z)/10.0);
            } else {
                headPos = glm::vec3(0,0,-1);
            }
        }

        // Release the color_image
        k4a_image_release(transformed_depth_image);
        k4a_image_release(color_image);
    }

    k4a_image_release(depth_image);
    cv::waitKey(1);
    // Release the capture
    k4a_capture_release(capture);
    
    return headPos;
}

void Kinect::close()
{
    // Shut down the camera when finished with application logic
    k4a_device_stop_cameras(device);
    k4a_device_close(device);
}