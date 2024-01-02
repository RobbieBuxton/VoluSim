#ifndef TRACKER_H
#define TRACKER_H

#include <k4a/k4a.h>
#include <glm/glm.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/shape_predictor.h>
#include <dlib/dnn.h>

template <long num_filters, typename SUBNET>
using con5d = dlib::con<num_filters, 5, 5, 2, 2, SUBNET>;
template <long num_filters, typename SUBNET>
using con5 = dlib::con<num_filters, 5, 5, 1, 1, SUBNET>;

template <typename SUBNET>
using downsampler = dlib::relu<dlib::affine<con5d<32, dlib::relu<dlib::affine<con5d<32, dlib::relu<dlib::affine<con5d<16, SUBNET>>>>>>>>>;
template <typename SUBNET>
using rcon5 = dlib::relu<dlib::affine<con5<45, SUBNET>>>;

using net_type = dlib::loss_mmod<dlib::con<1, 9, 9, 1, 1, rcon5<rcon5<rcon5<downsampler<dlib::input_rgb_image_pyramid<dlib::pyramid_down<6>>>>>>>>;

class Tracker
{
public:
    Tracker();
    ~Tracker();
    void updateEyePos();
    void close();

    k4a_device_t device;
    k4a_device_configuration_t config;
    k4a_calibration_t calibration;
    k4a_transformation_t transformation;
    net_type cnn_face_detector;
    dlib::shape_predictor predictor;
    glm::vec3 eyePos;

private:
    class Capture
    {
    public:
        Capture(k4a_device_t device, k4a_transformation_t transformation);
        ~Capture();
        // c prefix means in colour coord space, d means in depth/ir coord space
        k4a_image_t cColorImage;
        k4a_image_t cDepthImage;
        // k4a_image_t cIRImage;
        // k4a_image_t dColorImage;
        k4a_image_t dDepthImage;
        // k4a_image_t dIRImage;
    private:
        k4a_capture_t capture = NULL;
        int32_t timeout = 17;
    };
};

#endif