#include "kinect.hpp"
#include <k4a/k4a.h>
#include <iostream>
#include <exception>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>

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
        k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
        config.camera_fps = K4A_FRAMES_PER_SECOND_30;
        config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
        config.color_resolution = K4A_COLOR_RESOLUTION_1080P;
        config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;

        if (K4A_RESULT_SUCCEEDED != k4a_device_start_cameras(device, &config))
        {
            throw FailedToStartKinectException(); // Replace this with correct exception
        }

        detector = dlib::get_frontal_face_detector();
        dlib::deserialize("/home/robbieb/Imperial/IndividualProject/VolumetricSim/data/shape_predictor_68_face_landmarks.dat") >> predictor;
    }
}

void Kinect::readFrame()
{
    // Capture a depth frame
    k4a_capture_t capture = NULL;
    switch (k4a_device_get_capture(device, &capture, 17))
    {
    case K4A_WAIT_RESULT_SUCCEEDED:
        break;
    case K4A_WAIT_RESULT_TIMEOUT:
        std::cout << "Timed out waiting for a capture" << std::endl;
        return;
        break;
    case K4A_WAIT_RESULT_FAILED:
        std::cout << "Failed to read a capture" << std::endl;
        return;
    }

    
    dlib::image_window win;
    // Display the color image
    k4a_image_t color_image = k4a_capture_get_color_image(capture);
    if (color_image != nullptr)
    {
        //This is pretty inefficent, needs a refactor to directly convert into bgr instead of bgra then bgr

        // Split the 4-channel image into separate channels
        const cv::Mat bgraImage(k4a_image_get_height_pixels(color_image), k4a_image_get_width_pixels(color_image), CV_8UC4, k4a_image_get_buffer(color_image),(size_t)k4a_image_get_stride_bytes(color_image));
        std::vector<cv::Mat> channels;
        cv::split(bgraImage, channels);

        // Create a 3-channel image by merging the first 3 channels (assuming BGR order)
        cv::Mat bgrImage;
        cv::merge(std::vector<cv::Mat>{channels[0], channels[1], channels[2]}, bgrImage);

        dlib::cv_image<dlib::bgr_pixel> dlib_img = dlib::cv_image<dlib::bgr_pixel>(bgrImage);
        auto dets = detector(dlib_img);
        std::cout << "Number of faces detected: " << dets.size() << std::endl;

        // Now we will go ask the shape_predictor to tell us the pose of
        // each face we detected.
        std::vector<dlib::full_object_detection> shapes;
        for (unsigned long j = 0; j < dets.size(); ++j)
        {
            dlib::full_object_detection shape = predictor(dlib_img, dets[j]);
            std::cout << "number of parts: "<< shape.num_parts() << std::endl;
            std::cout << "pixel position of first part:  " << shape.part(0) << std::endl;
            std::cout << "pixel position of second part: " << shape.part(1) << std::endl;
            // You get the idea, you can get all the face part locations if
            // you want them.  Here we just store them in shapes so we can
            // put them on the screen.
            shapes.push_back(shape);
        }

        for (const auto& shape : shapes) {
            // Draw circles at each facial landmark
            for (int i = 36; i < 48; i++) {
                // Get the position of the facial landmark
                dlib::point pt = shape.part(i);

                // Convert dlib point to OpenCV Point (x, y)
                cv::Point cvPt(pt.x(), pt.y());

                // Draw a circle at the facial landmark position
                cv::circle(bgrImage, cvPt, 5, cv::Scalar(255, 0, 0), -1); // You can adjust circle size and color here
            }
        }

        cv::imshow("Color Image", bgrImage);
        // Release the color_image
        k4a_image_release(color_image);
    }

    // // Display the depth image
    // k4a_image_t depth_image = k4a_capture_get_depth_image(capture);
    // if (depth_image != nullptr)
    // {
    //     int depth_width = k4a_image_get_width_pixels(depth_image);
    //     int depth_height = k4a_image_get_height_pixels(depth_image);
    //     cv::Mat depth_mat(depth_height, depth_width, CV_16U, k4a_image_get_buffer(depth_image),(size_t)k4a_image_get_stride_bytes(depth_image));
    //     cv::Mat depth_display;
    //     cv::normalize(depth_mat, depth_display, 0, 255, cv::NORM_MINMAX, CV_8U);
    //     cv::imshow("Depth Image", depth_display);

    //     k4a_image_release(depth_image);
    // }

    // k4a_image_t ir_image = k4a_capture_get_ir_image(capture);
    // if (ir_image != nullptr)
    // {
    //     cv::Mat ir_mat(k4a_image_get_height_pixels(ir_image), k4a_image_get_width_pixels(ir_image),
    //                     CV_16U, (void*)k4a_image_get_buffer(ir_image),(size_t)k4a_image_get_stride_bytes(ir_image));
    //     cv::Mat ir_8bit;
    //     ir_mat.convertTo(ir_8bit, CV_8U);
    //     cv::imshow("IR Image", ir_8bit);

    //     k4a_image_release(ir_image);
    // }

    cv::waitKey(1);
    // Release the capture
    k4a_capture_release(capture);
}

void Kinect::close()
{
    // Shut down the camera when finished with application logic
    k4a_device_stop_cameras(device);
    k4a_device_close(device);
}