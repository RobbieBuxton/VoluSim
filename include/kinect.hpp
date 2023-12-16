#ifndef KINECT_H
#define KINECT_H

#include <k4a/k4a.h>

class Kinect
{
public:
    Kinect();
    void readFrame();
    void close();

    k4a_device_t device;
};

#endif