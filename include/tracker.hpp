#ifndef TRACKER_H
#define TRACKER_H

#include <k4a/k4a.h>
#include <glm/glm.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/shape_predictor.h>
#include <dlib/dnn.h>
#include <optional>

#include "mediapipe.h"

template <long num_filters, typename SUBNET>
using con5d = dlib::con<num_filters, 5, 5, 2, 2, SUBNET>;
template <long num_filters, typename SUBNET>
using con5 = dlib::con<num_filters, 5, 5, 1, 1, SUBNET>;

template <typename SUBNET>
using downsampler = dlib::relu<dlib::affine<con5d<32, dlib::relu<dlib::affine<con5d<32, dlib::relu<dlib::affine<con5d<16, SUBNET>>>>>>>>>;
template <typename SUBNET>
using rcon5 = dlib::relu<dlib::affine<con5<45, SUBNET>>>;

using net_type = dlib::loss_mmod<dlib::con<1, 9, 9, 1, 1, rcon5<rcon5<rcon5<downsampler<dlib::input_rgb_image_pyramid<dlib::pyramid_down<6>>>>>>>>;

#define CHECK_MP_RESULT(result)                            \
    if (!result)                                           \
    {                                                      \
        const char *error = mp_get_last_error();           \
        std::cerr << "[MediaPipe] " << error << std::endl; \
        mp_free_error(error);                              \
        std::exit(1);                                      \
    }

class Tracker
{
public:
    Tracker(glm::vec3 initCameraOffset, float yRot);
    ~Tracker();
    void update();
    void close();
    std::optional<glm::vec3> getLeftEyePos();
    std::optional<glm::vec3> getRightEyePos();
    std::optional<std::vector<glm::vec3>> getHandLandmarks();
    cv::Mat getDepthImage();
    cv::Mat getColorImage();
    std::vector<glm::vec3> getPointCloud();
    void getLatestCapture();
    
private:
    class Capture
    {
    public:
        Capture(k4a_device_t device, k4a_transformation_t transformation);
        ~Capture();
        struct ImageSpace
        {
            int height;
            int width;
            k4a_image_t colorImage;
            k4a_image_t depthImage;
            // k4a_image_t IRImage;
        };

        // color coord space
        ImageSpace colorSpace;
        // depth/ir coord space
        ImageSpace depthSpace;

    private:
        k4a_capture_t capture = NULL;
        int32_t timeout = 17;
    };

    void createNewTrackingFrame(cv::Mat inputColorImage, std::shared_ptr<Capture> cInst);
    void debugDraw(cv::Mat inputColorImage);
    glm::vec3 calculate3DPos(int x, int y, k4a_calibration_type_t source_type, std::shared_ptr<Capture> capture);
    glm::vec3 toScreenSpace(glm::vec3 pos);
    glm::vec3 getFilteredPoint(glm::vec3 point, std::shared_ptr<Capture> capture);
    glm::vec3 cameraOffset;
    

    std::shared_ptr<Capture> latestCapture;

    k4a_device_t device;
    k4a_device_configuration_t config;
    k4a_calibration_t calibration;
    k4a_transformation_t transformation;

    glm::mat4 toScreenSpaceMat;
    net_type cnn_face_detector;

    dlib::shape_predictor predictor;

    cv::Mat colorImage;
    cv::Mat depthImage;

    mp_instance *instance;
    mp_poller *landmarks_poller;
    mp_poller *rects_poller;

    struct Rectangle
    {
        int x;
        int y;
        int width;
        int height;
        float rotation;
    };

    struct HandLandmarks
    {
        std::shared_ptr<Capture> capture;
        glm::vec3 landmarks[21];
        Rectangle box;
    };

    struct FaceLandmarks
    {
        std::shared_ptr<Capture> capture;
        glm::vec2 landmarks[5];
        Rectangle box;
    };

    struct TrackingFrame
    {
        std::shared_ptr<Capture> debugCapture;
        std::unique_ptr<FaceLandmarks> face;
        std::unique_ptr<HandLandmarks> hand;
    };
    std::unique_ptr<TrackingFrame> trackF;
};

#endif