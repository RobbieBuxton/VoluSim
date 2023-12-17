#include "kinect.hpp"
#include <k4a/k4a.h>
#include <iostream>
#include <exception>

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
        config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
        config.color_resolution = K4A_COLOR_RESOLUTION_2160P;
        config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;

        if (K4A_RESULT_SUCCEEDED != k4a_device_start_cameras(device, &config))
        {
            throw FailedToStartKinectException(); // Replace this with correct exception
        }
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

    // Access the depth16 image
    k4a_image_t image = k4a_capture_get_color_image(capture);
    if (image != NULL)
    {
        printf(" | Depth16 res:%4dx%4d stride:%5d\n",
               k4a_image_get_height_pixels(image),
               k4a_image_get_width_pixels(image),
               k4a_image_get_stride_bytes(image));

        // Release the image
        k4a_image_release(image);
    }

    // Release the capture
    k4a_capture_release(capture);
}

void Kinect::close()
{
    // Shut down the camera when finished with application logic
    k4a_device_stop_cameras(device);
    k4a_device_close(device);
}