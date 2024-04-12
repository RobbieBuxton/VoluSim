#include <k4a/k4a.h>
#include <iostream>
#include <algorithm>
#include <exception>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/cudaimgproc.hpp>

#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <dlib/opencv.h>
#include <dlib/dnn.h>

#include "tracker.hpp"
#include "filesystem.hpp"
#include "mediapipe.h"

class TrackerException : public std::exception
{
private:
    std::string message;

public:
    explicit TrackerException(const char *msg) : message(msg) {}

    virtual const char *what() const throw()
    {
        return message.c_str();
    }
};

class NoTrackersDetectedException : public TrackerException
{
public:
    NoTrackersDetectedException() : TrackerException("No Trackers were detected") {}
};

class FailedToOpenTrackerException : public TrackerException
{
public:
    FailedToOpenTrackerException() : TrackerException("Cannot open Tracker") {}
};

class FailedToStartTrackerException : public TrackerException
{
public:
    FailedToStartTrackerException() : TrackerException("Cannot start the Tracker cameras") {}
};

class FailedToDetectFaceException : public TrackerException
{
public:
    FailedToDetectFaceException() : TrackerException("Could not detect face from capture") {}
};

Tracker::Tracker(float yRot)
{
    // Check for Trackers
    uint32_t count = k4a_device_get_installed_count();
    if (count == 0)
    {
        throw NoTrackersDetectedException();
    }

    std::cout << "k4a device attached!" << std::endl;

    // Open the first plugged in Tracker device
    device = NULL;
    if (K4A_FAILED(k4a_device_open(K4A_DEVICE_DEFAULT, &device)))
    {
        throw FailedToOpenTrackerException();
    }

    // Get the size of the serial number
    size_t serial_size = 0;
    k4a_device_get_serialnum(device, NULL, &serial_size);

    // Allocate memory for the serial, then acquire it
    char *serial = (char *)(malloc(serial_size));
    k4a_device_get_serialnum(device, serial, &serial_size);
    printf("Opened device: %s\n", serial);
    free(serial);

    config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;
    config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
    config.color_resolution = K4A_COLOR_RESOLUTION_1080P;
    config.depth_mode = K4A_DEPTH_MODE_WFOV_2X2BINNED;
    config.synchronized_images_only = true;

    if (K4A_RESULT_SUCCEEDED != k4a_device_start_cameras(device, &config))
    {
        k4a_device_close(device);
        throw FailedToStartTrackerException();
    }

    // Display the color image
    k4a_device_get_calibration(device, config.depth_mode, config.color_resolution, &calibration);
    transformation = k4a_transformation_create(&calibration);

    dlib::deserialize(FileSystem::getPath("data/shape_predictor_5_face_landmarks.dat").c_str()) >> predictor;
    dlib::deserialize(FileSystem::getPath("data/mmod_human_face_detector.dat").c_str()) >> cnn_face_detector;

    // configure mediapipe
    std::string srcPath = FileSystem::getPath("data/");
    std::string landmarkPath = FileSystem::getPath("data/mediapipe/modules/hand_landmark/hand_landmark_tracking_cpu.binarypb");
    mp_set_resource_dir(srcPath.c_str());

    // Load the binary graph and specify the input stream name.
    mp_instance_builder *builder = mp_create_instance_builder(landmarkPath.c_str(), "image");

    // Configure the graph with node options and side packets.
    mp_add_option_float(builder, "palmdetectioncpu__TensorsToDetectionsCalculator", "min_score_thresh", 0.6);
    mp_add_option_double(builder, "handlandmarkcpu__ThresholdingCalculator", "threshold", 0.2);
    mp_add_side_packet(builder, "num_hands", mp_create_packet_int(2));
    mp_add_side_packet(builder, "model_complexity", mp_create_packet_int(1));
    mp_add_side_packet(builder, "use_prev_landmarks", mp_create_packet_bool(true));

    // Create an instance from the instance builder.
    instance = mp_create_instance(builder);
    CHECK_MP_RESULT(instance)

    // Create poller for the hand landmarks.
    landmarks_poller = mp_create_poller(instance, "multi_hand_landmarks");
    CHECK_MP_RESULT(landmarks_poller)

    // Create a poller for the hand rectangles.
    rects_poller = mp_create_poller(instance, "hand_rects");
    CHECK_MP_RESULT(rects_poller)

    // Start the graph.
    CHECK_MP_RESULT(mp_start(instance))

    // Head distance 50cm
    trackingFrame = std::make_unique<TrackingFrame>();

    // Rotation into the same basis as screenSpace
    toScreenSpaceMat = glm::rotate(glm::mat4(1.0f), glm::radians(yRot), glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 Tracker::calculate3DPos(int x, int y, k4a_calibration_type_t source_type)
{
    uint16_t depth;
    if (source_type == K4A_CALIBRATION_TYPE_COLOR)
    {
        uint16_t *depthBuffer = reinterpret_cast<uint16_t *>(k4a_image_get_buffer(captureInstance->colorSpace.depthImage));
        int index = y * captureInstance->colorSpace.width + x;
        depth = depthBuffer[index];
    }
    else if (source_type == K4A_CALIBRATION_TYPE_DEPTH)
    {
        uint16_t *depthBuffer = reinterpret_cast<uint16_t *>(k4a_image_get_buffer(captureInstance->depthSpace.depthImage));
        int index = y * captureInstance->depthSpace.width + x;
        depth = depthBuffer[index];
    }

    k4a_float2_t pointK4APoint = {static_cast<float>(x), static_cast<float>(y)};
    k4a_float3_t cameraPoint;
    int valid;
    if (K4A_RESULT_SUCCEEDED != k4a_calibration_2d_to_3d(&calibration, &pointK4APoint, depth, source_type, K4A_CALIBRATION_TYPE_DEPTH, &cameraPoint, &valid))
    {
        // std::cout << "Failed to convert from 2d to 3d" << std::endl;
        // exit(EXIT_FAILURE);
    }

    if (!valid)
    {
        // std::cout << "Failed to convert to valid 3d coords" << std::endl;
        // exit(EXIT_FAILURE);
    }
    return glm::vec3((-(float)cameraPoint.xyz.x) / 10.0, -((float)cameraPoint.xyz.y) / 10.0, ((float)cameraPoint.xyz.z) / 10.0);
}

std::vector<glm::vec3> Tracker::getPointCloud()
{
    std::vector<glm::vec3> pointCloud;
    if ((captureInstance == nullptr) || (captureInstance->depthSpace.depthImage == NULL))
    {
        return pointCloud;
    }
    k4a_image_t pointCloudImage;

    // Create an image to hold the point cloud data
    k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM,
                     captureInstance->depthSpace.width,
                     captureInstance->depthSpace.height,
                     captureInstance->depthSpace.width * 3 * (int)sizeof(int16_t),
                     &pointCloudImage);

    // Transform the depth image to a point cloud
    k4a_transformation_depth_image_to_point_cloud(transformation, captureInstance->depthSpace.depthImage, K4A_CALIBRATION_TYPE_DEPTH, pointCloudImage);

    // Convert pointCloudImage to std::vector<glm::vec3>
    int16_t *pointCloudData = reinterpret_cast<int16_t *>(k4a_image_get_buffer(pointCloudImage));
    for (int i = 0; i < captureInstance->depthSpace.width * captureInstance->depthSpace.height; ++i)
    {
        int index = i * 3;
        float x = -static_cast<float>(pointCloudData[index]) / 10.0f;
        float y = -static_cast<float>(pointCloudData[index + 1]) / 10.0f;
        float z = static_cast<float>(pointCloudData[index + 2]) / 10.0f;
        pointCloud.push_back(toScreenSpace(glm::vec3(x, y, z)));
    }

    // Remember to release the point cloud image after use
    k4a_image_release(pointCloudImage);

    return pointCloud;
}

void Tracker::createNewTrackingFrame(cv::Mat colorImage)
{
    std::unique_ptr<TrackingFrame> newTrackingFrame = std::make_unique<TrackingFrame>();
    // Store the frame data in an image structure.
    mp_image image;
    image.data = colorImage.data;
    image.width = colorImage.cols;
    image.height = colorImage.rows;
    image.format = mp_image_format_srgb;

    // Wrap the image in a packet and process it.
    CHECK_MP_RESULT(mp_process(instance, mp_create_packet_image(image)))

    // Start face tracking
    dlib::cv_image<dlib::bgr_pixel> dlib_img = dlib::cv_image<dlib::bgr_pixel>(colorImage);

    std::vector<dlib::matrix<dlib::rgb_pixel>> batch;
    dlib::matrix<dlib::rgb_pixel> dlib_matrix;
    dlib::assign_image(dlib_matrix, dlib_img);
    batch.push_back(dlib_matrix);

    auto detections = cnn_face_detector(batch);

    // Get first from batch (I think need to double check this)
    std::vector<dlib::mmod_rect> dets = detections[0];

    if (dets.empty())
    {
        throw FailedToDetectFaceException();
    }

    for (int i = 0; i < dets.size(); i++)
    {
        newTrackingFrame->faces.push_back(dets[i].rect);
        newTrackingFrame->faceLandmarks.push_back(predictor(dlib_img, dets[i].rect));
    }

    std::vector<cv::Point> eyes;
    for (int i = 0; i < newTrackingFrame->faces.size(); i++)
    {
        // We don't need to div by 2 because we pyradown the image
        eyes.push_back(cv::Point(
            newTrackingFrame->faceLandmarks[i].part(0).x() + newTrackingFrame->faceLandmarks[i].part(1).x(),
            newTrackingFrame->faceLandmarks[i].part(0).y() + newTrackingFrame->faceLandmarks[i].part(1).y()));
        eyes.push_back(cv::Point(
            newTrackingFrame->faceLandmarks[i].part(2).x() + newTrackingFrame->faceLandmarks[i].part(3).x(),
            newTrackingFrame->faceLandmarks[i].part(2).y() + newTrackingFrame->faceLandmarks[i].part(3).y()));

        // Left eye
        newTrackingFrame->leftEyePos = calculate3DPos(eyes[0].x, eyes[0].y, K4A_CALIBRATION_TYPE_COLOR);
        newTrackingFrame->rightEyePos = calculate3DPos(eyes[1].x, eyes[1].y, K4A_CALIBRATION_TYPE_COLOR);
    }

    // Wait until the image has been processed.
    CHECK_MP_RESULT(mp_wait_until_idle(instance))

    // Draw the hand landmarks onto the frame.
    if (mp_get_queue_size(landmarks_poller) > 0)
    {
        mp_packet *packet = mp_poll_packet(landmarks_poller);
        newTrackingFrame->handLandmarks = mp_get_norm_multi_face_landmarks(packet);
        mp_destroy_packet(packet);
    }

    // Draw the hand rectangles onto the frame.
    if (mp_get_queue_size(rects_poller) > 0)
    {
        mp_packet *packet = mp_poll_packet(rects_poller);
        newTrackingFrame->rects = mp_get_norm_rects(packet);
        mp_destroy_packet(packet);
    }
    trackingFrame = std::move(newTrackingFrame);
}

void Tracker::update()
{
    try
    {

        auto start = std::chrono::high_resolution_clock::now();
        
        captureInstance = std::make_unique<Capture>(device, transformation);

        // Directly upload BGRA image to GPU and convert to BGR
        cv::cuda::GpuMat bgraImageGpu, bgrImageGpu;
        cv::Mat bgraImage(captureInstance->colorSpace.height, captureInstance->colorSpace.width, CV_8UC4, k4a_image_get_buffer(captureInstance->colorSpace.colorImage), (size_t)k4a_image_get_stride_bytes(captureInstance->colorSpace.colorImage));
        bgraImageGpu.upload(bgraImage);
        cv::cuda::cvtColor(bgraImageGpu, bgrImageGpu, cv::COLOR_BGRA2BGR);

        // Perform any GPU-based image processing
        cv::cuda::pyrDown(bgrImageGpu, bgrImageGpu);

        // Download the processed image from GPU to CPU for further processing with Dlib
        cv::Mat processedBgrImage;
        bgrImageGpu.download(processedBgrImage);

        cv::Mat dImage(captureInstance->colorSpace.height, captureInstance->colorSpace.width, CV_16U, k4a_image_get_buffer(captureInstance->colorSpace.depthImage), (size_t)k4a_image_get_stride_bytes(captureInstance->colorSpace.depthImage));

        cv::Mat normalisedDImage, colorDepthImage;

        createNewTrackingFrame(processedBgrImage);

        auto stop = std::chrono::high_resolution_clock::now();

        // Calculate the duration in milliseconds
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

        std::cout << "Latency: " << duration.count() << " milliseconds" << std::endl;

        cv::normalize(dImage, normalisedDImage, 0, 255, cv::NORM_MINMAX, CV_8U);
        cv::applyColorMap(normalisedDImage, colorDepthImage, cv::COLORMAP_JET);

        colorDepthImage.copyTo(depthImage);
        processedBgrImage.copyTo(colorImage);

        debugDraw(colorImage);

        cv::waitKey(1);
    }
    catch (const std::exception &e)
    {
        throw;
    }
}

void Tracker::debugDraw(cv::Mat colorImage)
{
    // Draw Detected faces on output
    for (unsigned long i = 0; i < trackingFrame->faces.size(); i++)
    {
        cv::rectangle(colorImage, cv::Point(trackingFrame->faces[i].left(), trackingFrame->faces[i].top()), cv::Point(trackingFrame->faces[i].right(), trackingFrame->faces[i].bottom()), CV_RGB(0, 0, 255), 2);
        for (unsigned long j = 0; j < trackingFrame->faceLandmarks[i].num_parts(); j++)
        {
            cv::circle(colorImage, cv::Point(trackingFrame->faceLandmarks[i].part(j).x(), trackingFrame->faceLandmarks[i].part(j).y()), 3, cv::Scalar(0, 0, 255), -1);
        }
    }
    if (trackingFrame->handLandmarks != NULL)
    {
        for (int i = 0; i < trackingFrame->handLandmarks->length; i++)
        {
            const mp_landmark_list &hand = trackingFrame->handLandmarks->elements[i];

            // Draw hand connections as green lines.
            for (const auto &connection : CONNECTIONS)
            {
                const mp_landmark &p1 = hand.elements[connection[0]];
                const mp_landmark &p2 = hand.elements[connection[1]];
                float x1 = (float)colorImage.cols * p1.x;
                float y1 = (float)colorImage.rows * p1.y;
                float x2 = (float)colorImage.cols * p2.x;
                float y2 = (float)colorImage.rows * p2.y;

                cv::line(colorImage, {(int)x1, (int)y1}, {(int)x2, (int)y2}, CV_RGB(0, 255, 0), 2);
            }

            // Draw hand landmarks as red dots.
            for (int j = 0; j < hand.length; j++)
            {
                const mp_landmark &p = hand.elements[j];
                float x = (float)colorImage.cols * p.x;
                float y = (float)colorImage.rows * p.y;
                cv::circle(colorImage, cv::Point((int)x, (int)y), 4, CV_RGB(255, 0, 0), -1);
            }
        }
    }
    if (trackingFrame->rects != NULL)
    {
        for (int i = 0; i < trackingFrame->rects->length; i++)
        {
            const mp_rect &rect = trackingFrame->rects->elements[i];

            cv::Point2f center((float)colorImage.cols * rect.x_center, (float)colorImage.rows * rect.y_center);
            cv::Point2f size((float)colorImage.cols * rect.width, (float)colorImage.rows * rect.height);
            float rotation = (float)rect.rotation * (180.0f / (float)CV_PI);

            // Draw hand bounding boxes as blue rectangles.
            cv::Point2f vertices[4];
            cv::RotatedRect(center, size, rotation).points(vertices);
            for (int j = 0; j < 4; j++)
            {
                cv::line(colorImage, vertices[j], vertices[(j + 1) % 4], CV_RGB(0, 0, 255), 2);
            }
        }
    }
}

glm::vec3 Tracker::getLeftEyePos()
{
    if (trackingFrame->faceLandmarks.size() == 0)
    {
        return glm::vec3(0.0f);
    }

    // We don't need to div by 2 because we pyradown the image
    cv::Point eye = cv::Point(
        trackingFrame->faceLandmarks[0].part(0).x() + trackingFrame->faceLandmarks[0].part(1).x(),
        trackingFrame->faceLandmarks[0].part(0).y() + trackingFrame->faceLandmarks[0].part(1).y());

    return toScreenSpace(calculate3DPos(eye.x, eye.y, K4A_CALIBRATION_TYPE_COLOR));
}

glm::vec3 Tracker::getRightEyePos()
{
    if (trackingFrame->faceLandmarks.size() == 0)
    {
        return glm::vec3(0.0f);
    }

    // We don't need to div by 2 because we pyradown the image
    cv::Point eye = cv::Point(
        trackingFrame->faceLandmarks[0].part(2).x() + trackingFrame->faceLandmarks[3].part(1).x(),
        trackingFrame->faceLandmarks[0].part(2).y() + trackingFrame->faceLandmarks[3].part(1).y());

    return toScreenSpace(calculate3DPos(eye.x, eye.y, K4A_CALIBRATION_TYPE_COLOR));
}

cv::Mat Tracker::getDepthImage()
{
    return depthImage;
}

cv::Mat Tracker::getColorImage()
{
    return colorImage;
}

Tracker::~Tracker()
{
    // Shut down the camera when finished with application logic
    k4a_device_stop_cameras(device);
    k4a_device_close(device);
}

class CaptureException : public std::exception
{
private:
    std::string message;

public:
    explicit CaptureException(const char *msg) : message(msg) {}

    virtual const char *what() const throw()
    {
        return message.c_str();
    }
};

class CaptureTimeoutException : public CaptureException
{
public:
    CaptureTimeoutException() : CaptureException("Timed out waiting for a capture") {}
};

class CaptureReadFailedException : public CaptureException
{
public:
    CaptureReadFailedException() : CaptureException("Failed to read a capture") {}
};

Tracker::Capture::Capture(k4a_device_t device, k4a_transformation_t transformation)
{
    // Capture a depth frame
    switch (k4a_device_get_capture(device, &capture, timeout))
    {
    case K4A_WAIT_RESULT_SUCCEEDED:
        break;
    case K4A_WAIT_RESULT_TIMEOUT:
        throw CaptureTimeoutException();
    case K4A_WAIT_RESULT_FAILED:
        throw CaptureReadFailedException();
    }

    colorSpace.colorImage = k4a_capture_get_color_image(capture);
    colorSpace.width = k4a_image_get_width_pixels(colorSpace.colorImage);
    colorSpace.height = k4a_image_get_height_pixels(colorSpace.colorImage);
    depthSpace.depthImage = k4a_capture_get_depth_image(capture);
    depthSpace.width = k4a_image_get_width_pixels(depthSpace.depthImage);
    depthSpace.height = k4a_image_get_height_pixels(depthSpace.depthImage);
    if (K4A_RESULT_FAILED == k4a_image_create(K4A_IMAGE_FORMAT_DEPTH16, colorSpace.width, colorSpace.height, colorSpace.width * (int)sizeof(uint16_t), &colorSpace.depthImage))
    {
        k4a_image_release(colorSpace.colorImage);
        k4a_image_release(depthSpace.depthImage);
        k4a_capture_release(capture);
        std::cout << "Failed to create empty transformed_depth_image" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (K4A_RESULT_FAILED == k4a_transformation_depth_image_to_color_camera(transformation, depthSpace.depthImage, colorSpace.depthImage))
    {
        k4a_image_release(colorSpace.colorImage);
        k4a_image_release(colorSpace.depthImage);
        k4a_image_release(depthSpace.depthImage);
        k4a_capture_release(capture);
        std::cout << "Failed to create transformed_depth_image" << std::endl;
        exit(EXIT_FAILURE);
    }
}

glm::vec3 Tracker::toScreenSpace(glm::vec3 pos)
{
    return glm::vec3(toScreenSpaceMat * glm::vec4(pos, 1.0f));
}

Tracker::Capture::~Capture()
{
    k4a_image_release(colorSpace.colorImage);
    k4a_image_release(colorSpace.depthImage);
    k4a_image_release(depthSpace.depthImage);
}

Tracker::TrackingFrame::~TrackingFrame()
{
    if (handLandmarks != NULL)
    {
        mp_destroy_multi_face_landmarks(handLandmarks);
    }
    if (rects != NULL)
    {
        mp_destroy_rects(rects);
    }
}