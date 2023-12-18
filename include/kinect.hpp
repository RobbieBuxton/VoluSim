#ifndef KINECT_H
#define KINECT_H

#include <k4a/k4a.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/shape_predictor.h>

class Kinect
{
public:
    Kinect();
    void readFrame();
    void close();

    k4a_device_t device;
    dlib::frontal_face_detector detector;
    dlib::shape_predictor predictor;
};

#endif