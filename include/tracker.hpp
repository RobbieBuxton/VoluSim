#ifndef TRACKER_H
#define TRACKER_H

#include <k4a/k4a.h>
#include <glm/glm.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/shape_predictor.h>
#include <dlib/dnn.h>

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

const mp_hand_landmark CONNECTIONS[][2] = {
    {mp_hand_landmark_wrist, mp_hand_landmark_thumb_cmc},
    {mp_hand_landmark_thumb_cmc, mp_hand_landmark_thumb_mcp},
    {mp_hand_landmark_thumb_mcp, mp_hand_landmark_thumb_ip},
    {mp_hand_landmark_thumb_ip, mp_hand_landmark_thumb_tip},
    {mp_hand_landmark_wrist, mp_hand_landmark_index_finger_mcp},
    {mp_hand_landmark_index_finger_mcp, mp_hand_landmark_index_finger_pip},
    {mp_hand_landmark_index_finger_pip, mp_hand_landmark_index_finger_dip},
    {mp_hand_landmark_index_finger_dip, mp_hand_landmark_index_finger_tip},
    {mp_hand_landmark_index_finger_mcp, mp_hand_landmark_middle_finger_mcp},
    {mp_hand_landmark_middle_finger_mcp, mp_hand_landmark_middle_finger_pip},
    {mp_hand_landmark_middle_finger_pip, mp_hand_landmark_middle_finger_dip},
    {mp_hand_landmark_middle_finger_dip, mp_hand_landmark_middle_finger_tip},
    {mp_hand_landmark_middle_finger_mcp, mp_hand_landmark_ring_finger_mcp},
    {mp_hand_landmark_ring_finger_mcp, mp_hand_landmark_ring_finger_pip},
    {mp_hand_landmark_ring_finger_pip, mp_hand_landmark_ring_finger_dip},
    {mp_hand_landmark_ring_finger_dip, mp_hand_landmark_ring_finger_tip},
    {mp_hand_landmark_ring_finger_mcp, mp_hand_landmark_pinky_mcp},
    {mp_hand_landmark_wrist, mp_hand_landmark_pinky_mcp},
    {mp_hand_landmark_pinky_mcp, mp_hand_landmark_pinky_pip},
    {mp_hand_landmark_pinky_pip, mp_hand_landmark_pinky_dip},
    {mp_hand_landmark_pinky_dip, mp_hand_landmark_pinky_tip}};

class Tracker
{
public:
    Tracker(float yRot);
    ~Tracker();
    void update();
    void close();
    glm::vec3 getLeftEyePos();
    glm::vec3 getRightEyePos();
    cv::Mat getDepthImage();
    cv::Mat getColorImage();
    std::vector<glm::vec3> getPointCloud();

private:
    void trackHand(cv::Mat colorImage);
    glm::vec3 calculate3DPos(int x, int y, k4a_calibration_type_t source_type);
    glm::vec3 toScreenSpace(glm::vec3 pos);
    k4a_device_t device;
    k4a_device_configuration_t config;
    k4a_calibration_t calibration;
    k4a_transformation_t transformation;

    glm::vec3 leftEyePos;
    glm::vec3 rightEyePos;
    glm::mat4 toScreenSpaceMat; 
    net_type cnn_face_detector;

    dlib::shape_predictor predictor;

    cv::Mat colorImage;
    cv::Mat depthImage;

    mp_instance *instance;
    mp_poller *landmarks_poller;
    mp_poller *rects_poller;

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

    std::unique_ptr<Capture> captureInstance;
};

#endif