#ifndef TRACKER_H
#define TRACKER_H

#include <k4a/k4a.h>
#include <glm/glm.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/shape_predictor.h>

class Tracker
{
public:
    Tracker();
    ~Tracker();
    glm::vec3 readFrame();
    void close();

    k4a_device_t device;
    k4a_device_configuration_t config;
    k4a_calibration_t calibration;
    k4a_transformation_t transformation;
    dlib::frontal_face_detector detector;
    dlib::shape_predictor predictor;
private:
    class Capture {
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