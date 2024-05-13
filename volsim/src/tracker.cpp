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

Tracker::Tracker(glm::vec3 initCameraOffset, float yRot)
{

	cameraOffset = initCameraOffset;

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
	mp_add_option_float(builder, "palmdetectioncpu__TensorsToDetectionsCalculator", "min_score_thresh", 0.5);
	mp_add_option_double(builder, "handlandmarkcpu__ThresholdingCalculator", "threshold", 0.5);
	mp_add_side_packet(builder, "num_hands", mp_create_packet_int(1));
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

	// Rotation into the same basis as screenSpace
	toScreenSpaceMat = glm::rotate(glm::mat4(1.0f), glm::radians(yRot), glm::vec3(1.0f, 0.0f, 0.0f));

	// Create a new tracking frame
	trackF = std::make_unique<TrackingFrame>();

	// Make sure the capture is initialized
	getLatestCapture();
}

glm::vec3 Tracker::calculate3DPos(int x, int y, k4a_calibration_type_t source_type, std::shared_ptr<Capture> capture)
{
	uint16_t depth;
	if (source_type == K4A_CALIBRATION_TYPE_COLOR)
	{
		uint16_t *depthBuffer = reinterpret_cast<uint16_t *>(k4a_image_get_buffer(capture->colorSpace.depthImage));
		int index = y * capture->colorSpace.width + x;
		depth = depthBuffer[index];
	}
	else if (source_type == K4A_CALIBRATION_TYPE_DEPTH)
	{
		uint16_t *depthBuffer = reinterpret_cast<uint16_t *>(k4a_image_get_buffer(capture->depthSpace.depthImage));
		int index = y * capture->depthSpace.width + x;
		depth = depthBuffer[index];
	}
	else
	{
		throw std::runtime_error("Invalid source type");
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
	if ((trackF->lastCapture == nullptr) || (trackF->lastCapture->depthSpace.depthImage == NULL))
	{
		return pointCloud;
	}
	k4a_image_t pointCloudImage;

	// Create an image to hold the point cloud data
	k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM,
					 trackF->lastCapture->depthSpace.width,
					 trackF->lastCapture->depthSpace.height,
					 trackF->lastCapture->depthSpace.width * 3 * (int)sizeof(int16_t),
					 &pointCloudImage);

	// Transform the depth image to a point cloud
	k4a_transformation_depth_image_to_point_cloud(transformation, trackF->lastCapture->depthSpace.depthImage, K4A_CALIBRATION_TYPE_DEPTH, pointCloudImage);

	// Convert pointCloudImage to std::vector<glm::vec3>
	int16_t *pointCloudData = reinterpret_cast<int16_t *>(k4a_image_get_buffer(pointCloudImage));
	for (int i = 0; i < trackF->lastCapture->depthSpace.width * trackF->lastCapture->depthSpace.height; ++i)
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

void Tracker::createNewTrackingFrame(cv::Mat inputColorImage, std::shared_ptr<Capture> capture)
{
	// Store the frame data in an image structure.
	mp_image image;
	image.data = inputColorImage.data;
	image.width = inputColorImage.cols;
	image.height = inputColorImage.rows;
	image.format = mp_image_format_srgb;

	// Wrap the image in a packet and process it.
	CHECK_MP_RESULT(mp_process(instance, mp_create_packet_image(image)))

	// Start face tracking
	dlib::cv_image<dlib::bgr_pixel> dlib_img = dlib::cv_image<dlib::bgr_pixel>(inputColorImage);

	std::vector<dlib::matrix<dlib::rgb_pixel>> batch;
	dlib::matrix<dlib::rgb_pixel> dlib_matrix;
	dlib::assign_image(dlib_matrix, dlib_img);
	batch.push_back(dlib_matrix);

	auto detections = cnn_face_detector(batch);

	// Get first from batch (I think need to double check this)
	std::vector<dlib::mmod_rect> dets = detections[0];

	// If face detected
	if (!dets.empty())
	{
		std::unique_ptr<FaceLandmarks> face = std::make_unique<FaceLandmarks>();

		face->box = {(int)(dets[0].rect.left() + dets[0].rect.right()) / 2,
					 (int)(dets[0].rect.top() + dets[0].rect.bottom()) / 2,
					 (int)(dets[0].rect.right() - dets[0].rect.left()),
					 (int)(dets[0].rect.bottom() - dets[0].rect.top()),
					 (float)0.0f};

		dlib::full_object_detection detection = predictor(dlib_img, dets[0].rect);
		for (int j = 0; j < 5; j++)
		{
			face->landmarks[j] = {detection.part(j).x(), detection.part(j).y()};
		}
		face->capture = capture;
		trackF->face = std::move(face);
	}

	// Wait until the image has been processed.
	CHECK_MP_RESULT(mp_wait_until_idle(instance))


	// Get hand landmarks
	// Making the assumption if there is a landmark packet there is a rect packet
	if (mp_get_queue_size(landmarks_poller) > 0)
	{
		mp_packet *landmark_packet = mp_poll_packet(landmarks_poller);
		mp_multi_face_landmark_list *hand_landmarks_list = mp_get_norm_multi_face_landmarks(landmark_packet);

		mp_packet *rects_packet = mp_poll_packet(rects_poller);
		mp_rect_list *rects = mp_get_norm_rects(rects_packet);

		if (hand_landmarks_list->length > 0)
		{
			mp_landmark_list &landmarks = hand_landmarks_list->elements[0];
			std::unique_ptr<HandLandmarks> hand = std::make_unique<HandLandmarks>();
			for (int j = 0; j < landmarks.length; j++)
			{
				const mp_landmark &p = landmarks.elements[j];
				float x = (float)inputColorImage.cols * p.x;
				float y = (float)inputColorImage.rows * p.y;
				// Mediapipe scales z with x
				float z = (float)inputColorImage.cols * p.z;

				hand->landmarks[j] = {x, y, z};
			}

			const mp_rect &rect = rects->elements[0];
			hand->box = {(int)(inputColorImage.cols * rect.x_center),
						 (int)(inputColorImage.rows * rect.y_center),
						 (int)(inputColorImage.cols * rect.width),
						 (int)(inputColorImage.rows * rect.height),
						 rect.rotation};

			hand->capture = capture;
			trackF->hand = std::move(hand);
		}

		mp_destroy_multi_face_landmarks(hand_landmarks_list);
		mp_destroy_packet(landmark_packet);
		mp_destroy_rects(rects);
		mp_destroy_packet(rects_packet);
	}
	else
	{
		trackF->hand = nullptr;
	}
}

void Tracker::getLatestCapture()
{
	while (true)
	{
		try
		{
			latestCapture = std::make_shared<Capture>(device, transformation);
			return;
		}
		catch (const std::exception &e)
		{
			// wait 1 millisecond
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

void Tracker::update()
{
	if (trackF->lastCapture == latestCapture)
	{
		return;
	}
	try
	{
		// Define time points and durations
		std::chrono::high_resolution_clock::time_point startOverall, stopOverall;
		std::chrono::high_resolution_clock::time_point start, stop;
		std::chrono::milliseconds durationCapture, durationGPUOperations, durationTracking, durationOverall;

		startOverall = std::chrono::high_resolution_clock::now();

		// Step 1: Capture Instance
		start = std::chrono::high_resolution_clock::now();

		stop = std::chrono::high_resolution_clock::now();
		durationCapture = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

		// Steps 2, 3, and 4: GPU Operations (Upload to GPU, GPU Processing, Download from GPU)
		start = std::chrono::high_resolution_clock::now();

		// Upload to GPU
		cv::Mat bgraImage(latestCapture->colorSpace.height, latestCapture->colorSpace.width, CV_8UC4, k4a_image_get_buffer(latestCapture->colorSpace.colorImage), (size_t)k4a_image_get_stride_bytes(latestCapture->colorSpace.colorImage));
		cv::cuda::GpuMat bgraImageGpu, bgrImageGpu;
		bgraImageGpu.upload(bgraImage);

		// GPU Processing
		cv::cuda::cvtColor(bgraImageGpu, bgrImageGpu, cv::COLOR_BGRA2BGR);
		cv::cuda::pyrDown(bgrImageGpu, bgrImageGpu);

		// Download from GPU
		cv::Mat processedBgrImage;
		bgrImageGpu.download(processedBgrImage);

		stop = std::chrono::high_resolution_clock::now();
		durationGPUOperations = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

		// Step 5: Run the tracking
		start = std::chrono::high_resolution_clock::now();
		try
		{
			createNewTrackingFrame(processedBgrImage, latestCapture);

			// Normalize and map depth image (example of further processing)
			cv::Mat dImage(latestCapture->colorSpace.height, latestCapture->colorSpace.width, CV_16U, k4a_image_get_buffer(latestCapture->colorSpace.depthImage), (size_t)k4a_image_get_stride_bytes(latestCapture->colorSpace.depthImage));
			cv::Mat normalisedDImage, colorDepthImage;
			cv::normalize(dImage, normalisedDImage, 0, 255, cv::NORM_MINMAX, CV_8U);
			cv::applyColorMap(normalisedDImage, colorDepthImage, cv::COLORMAP_JET);

			colorDepthImage.copyTo(depthImage);
			bgraImage.copyTo(colorImage);
			
			debugDraw();
		}
		catch (const std::exception &e)
		{
			std::cerr << "Failed to create new tracking frame" << std::endl;
		}

		stop = std::chrono::high_resolution_clock::now();
		durationTracking = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

		stopOverall = std::chrono::high_resolution_clock::now();
		durationOverall = std::chrono::duration_cast<std::chrono::milliseconds>(stopOverall - startOverall);

		trackF->lastCapture = latestCapture;

		// Print all durations at the end
		std::cout << "Capture Instance:  " << durationCapture.count() << " ms\n"
		          << "GPU Operations:    " << durationGPUOperations.count() << " ms\n"
		          << "Tracking:          " << durationTracking.count() << " ms\n"
		          << "Total Time:        " << durationOverall.count() << " ms" << std::endl
		          << std::endl;
	}
	catch (const std::exception &e)
	{
		throw;
	}
}

void Tracker::debugDraw()
{
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

	if (!trackF)
	{
		return;
	}

	colorImage.copyTo(colorImageSkeletons);
	colorImage.copyTo(colorImageImportant);
	depthImage.copyTo(depthImageImportant);

	if (trackF->face)
	{
		cv::Point2f faceCenter((float)trackF->face->box.x, (float)trackF->face->box.y);
		cv::Point2f faceSize((float)trackF->face->box.width, (float)trackF->face->box.height);
		float faceRotation = (float)trackF->face->box.rotation * (180.0f / (float)CV_PI);

		// Draw hand bounding boxes as blue rectangles.
		cv::Point2f faceVertices[4];
		cv::RotatedRect(faceCenter, faceSize, faceRotation).points(faceVertices);
		for (int j = 0; j < 4; j++)
		{
			cv::line(colorImageSkeletons, cv::Point(faceVertices[j].x * 2,faceVertices[j].y * 2), cv::Point(faceVertices[(j + 1) % 4].x * 2, faceVertices[(j + 1) % 4].y * 2), CV_RGB(0, 0, 255), 2);
		}
		for (unsigned long j = 0; j < 5; j++)
		{
			cv::circle(colorImageSkeletons, cv::Point(trackF->face->landmarks[j].x * 2, trackF->face->landmarks[j].y * 2), 3, cv::Scalar(0, 0, 255), -1);
		}
		cv::Point leftEyeCenter = cv::Point2f((trackF->face->landmarks[0].x + trackF->face->landmarks[1].x), (trackF->face->landmarks[0].y + trackF->face->landmarks[1].y));
		cv::circle (colorImageImportant, leftEyeCenter, 10, cv::Scalar(55, 124, 255), -1);
		cv::circle (depthImageImportant, leftEyeCenter, 10, cv::Scalar(55, 124, 255), -1);
	}

	if (trackF->hand)
	{
		for (unsigned long j = 0; j < 21; j++)
		{
			for (const auto &connection : CONNECTIONS)
			{
				const glm::vec2 &p1 = trackF->hand->landmarks[connection[0]];
				const glm::vec2 &p2 = trackF->hand->landmarks[connection[1]];
				cv::line(colorImageSkeletons, {(int)p1.x * 2, (int)p1.y * 2}, {(int)p2.x * 2, (int)p2.y * 2}, CV_RGB(0, 255, 0), 2);
			}
		}

		for (unsigned long j = 0; j < 21; j++)
		{
			float minZ = std::numeric_limits<float>::max();
			float maxZ = std::numeric_limits<float>::min();

			// Find the minimum and maximum z values

			for (const auto &landmark : trackF->hand->landmarks)
			{
				minZ = std::min(minZ, landmark.z);
				maxZ = std::max(maxZ, landmark.z);
			}

			for (const auto &landmark : trackF->hand->landmarks)
			{
				// Normalize the z value
				float normalizedZ = (landmark.z - minZ) / (maxZ - minZ);

				// Calculate the color based on the normalized z value
				cv::Scalar color(0, 0, 255 * (1 - normalizedZ));

				cv::circle(colorImageSkeletons, cv::Point(landmark.x * 2, landmark.y * 2), 5, color, -1);
			}
		}

		cv::Point2f thumb = cv::Point2f((float)trackF->hand->landmarks[mp_hand_landmark_thumb_tip].x * 2, (float)trackF->hand->landmarks[mp_hand_landmark_thumb_tip].y * 2);
		cv::circle (colorImageImportant, thumb, 10, cv::Scalar(163, 69, 143), -1);
		cv::circle (depthImageImportant, thumb, 10, cv::Scalar(163, 69, 143), -1);
		cv::Point2f index = cv::Point2f((float)trackF->hand->landmarks[mp_hand_landmark_index_finger_tip].x * 2, (float)trackF->hand->landmarks[mp_hand_landmark_index_finger_tip].y * 2);
		cv::circle (colorImageImportant, index, 10, cv::Scalar(163, 69, 143), -1);
		cv::circle (depthImageImportant, index, 10, cv::Scalar(163, 69, 143), -1);


		cv::Point2f handCenter((float)trackF->hand->box.x, (float)trackF->hand->box.y);
		cv::Point2f handSize((float)trackF->hand->box.width, (float)trackF->hand->box.height);
		float handRotation = (float)trackF->hand->box.rotation * (180.0f / (float)CV_PI);

		// Draw hand bounding boxes as blue rectangles.
		cv::Point2f handVertices[4];
		cv::RotatedRect(handCenter, handSize, handRotation).points(handVertices);
		for (int j = 0; j < 4; j++)
		{
			cv::line(colorImageSkeletons, cv::Point(handVertices[j].x * 2, handVertices[j].y * 2), cv::Point(handVertices[(j + 1) % 4].x*2,handVertices[(j + 1) % 4].y*2) , CV_RGB(0, 0, 255), 2);
		}
	}
}

glm::vec3 Tracker::getFilteredPoint(glm::vec3 point, std::shared_ptr<Capture> capture)
{
	glm::vec3 bestPoint = calculate3DPos(point.x * 2, point.y * 2, K4A_CALIBRATION_TYPE_COLOR, capture);
	float minZ = bestPoint.z;

	// Iterate over a 3x3 grid centered on the original point
	for (int dx = -1; dx <= 1; dx++)
	{
		for (int dy = -1; dy <= 1; dy++)
		{
			if (dx == 0 && dy == 0)
				continue; // Skip the center point since it's already considered

			glm::vec3 newPoint = calculate3DPos(point.x * 2 + dx, point.y * 2 + dy, K4A_CALIBRATION_TYPE_COLOR, capture);
			if (newPoint.z < minZ)
			{
				minZ = newPoint.z;
				bestPoint = newPoint;
			}
		}
	}

	return bestPoint;
}

std::optional<std::vector<glm::vec3>> Tracker::getHandLandmarks()
{
	auto currentTimeInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
										 std::chrono::system_clock::now().time_since_epoch())
										 .count();

	if (trackF && trackF->hand)
	{
		std::vector<glm::vec3> landmarks;

		// There will always be a thumb if there is a finger
		if (trackF->hand->cachedIndexFinger.has_value())
		{
			landmarks.push_back(trackF->hand->cachedIndexFinger.value());
			landmarks.push_back(trackF->hand->cachedThumb.value());
		}
		else
		{
			glm::vec3 landmarkIndexFinger = trackF->hand->landmarks[mp_hand_landmark_index_finger_tip];
			// Correct for down sample
			glm::vec3 posIndexFinger = toScreenSpace(getFilteredPoint(landmarkIndexFinger, trackF->hand->capture));
			trackF->hand->cachedIndexFinger = posIndexFinger;
			landmarks.push_back(posIndexFinger);

			glm::vec3 landmarkThumb = trackF->hand->landmarks[mp_hand_landmark_thumb_tip];
			// Correct for down sample
			glm::vec3 posThumb = toScreenSpace(getFilteredPoint(landmarkThumb, trackF->hand->capture));
			trackF->hand->cachedThumb = posThumb;

			landmarks.push_back(posThumb);
			// Add 'index' finger details to 'hand'.
			nlohmann::json hand;
			hand["time"] = currentTimeInMilliseconds;
			hand["index"] = {
				{"x", posIndexFinger.x},
				{"y", posIndexFinger.y},
				{"z", posIndexFinger.z}};

			// Add 'thumb' details to 'hand'.
			hand["thumb"] = {
				{"x", posThumb.x},
				{"y", posThumb.y},
				{"z", posThumb.z}};

			// Push the 'hand' object to 'jsonLog' under key 'hand'.
			jsonLog["hand"].push_back(hand);
		}

		if (glm::distance(landmarks[0], landmarks[1]) < 18.0f)
		{
			return landmarks;
		}
	}

	return {};
}

std::optional<glm::vec3> Tracker::getLeftEyePos()

{
	auto currentTimeInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
										 std::chrono::system_clock::now().time_since_epoch())
										 .count();

	if (trackF && trackF->face)
	{
		if (trackF->face->cachedLeftEye.has_value())
		{
			return trackF->face->cachedLeftEye.value();
		}

		// We don't need to div by 2 because we pyradown the image
		cv::Point eye = cv::Point(
			trackF->face->landmarks[0].x + trackF->face->landmarks[1].x,
			trackF->face->landmarks[0].y + trackF->face->landmarks[1].y);

		glm::vec3 translatedEye = toScreenSpace(calculate3DPos(eye.x, eye.y, K4A_CALIBRATION_TYPE_COLOR, trackF->face->capture));
		jsonLog["leftEye"].push_back({{"time", currentTimeInMilliseconds},
									  {"x", translatedEye.x},
									  {"y", translatedEye.y},
									  {"z", translatedEye.z}});
		trackF->face->cachedLeftEye = translatedEye;
		return translatedEye;
	}
	return {};
}

std::optional<glm::vec3> Tracker::getRightEyePos()
{
	auto currentTimeInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
										 std::chrono::system_clock::now().time_since_epoch())
										 .count();
	if (trackF && trackF->face)
	{
		if (trackF->face->cachedRightEye.has_value())
		{
			return trackF->face->cachedRightEye.value();
		}
		// We don't need to div by 2 because we pyradown the image
		cv::Point eye = cv::Point(
			trackF->face->landmarks[2].x + trackF->face->landmarks[3].x,
			trackF->face->landmarks[2].y + trackF->face->landmarks[3].y);

		glm::vec3 translatedEye = toScreenSpace(calculate3DPos(eye.x, eye.y, K4A_CALIBRATION_TYPE_COLOR, trackF->face->capture));
		jsonLog["rightEye"].push_back({{"time", currentTimeInMilliseconds},
									   {"x", translatedEye.x},
									   {"y", translatedEye.y},
									   {"z", translatedEye.z}});
		trackF->face->cachedRightEye = translatedEye;
		return translatedEye;
	}
	return {};
}

cv::Mat Tracker::getDepthImage()
{
	return depthImage;
}

cv::Mat Tracker::getDepthImageImportant()
{
	return depthImageImportant;
}

cv::Mat Tracker::getColorImage()
{
	return colorImage;
}

cv::Mat Tracker::getColorImageImportant()
{
	return colorImageImportant;
}

cv::Mat Tracker::getColorImageSkeletons()
{
	return colorImageSkeletons;
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
	return glm::vec3(toScreenSpaceMat * glm::vec4(pos, 1.0f)) + cameraOffset;
}

Tracker::Capture::~Capture()
{
	k4a_image_release(colorSpace.colorImage);
	k4a_image_release(colorSpace.depthImage);
	k4a_image_release(depthSpace.depthImage);
}

nlohmann::json Tracker::returnJson()
{
	return jsonLog;
}
