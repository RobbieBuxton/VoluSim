#ifndef KINECT_H
#define KINECT_H

#include <k4a/k4a.h>
#include <glm/glm.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/shape_predictor.h>

class Kinect
{
public:
    Kinect();
    glm::vec3 readFrame();
    void close();

    k4a_device_t device;
    k4a_device_configuration_t config;
    k4a_calibration_t calibration;
    k4a_transformation_t transformation;
    dlib::frontal_face_detector detector;
    dlib::shape_predictor predictor;
};

#endif