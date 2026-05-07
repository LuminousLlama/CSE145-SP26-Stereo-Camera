#include "cv_bridge/cv_bridge.hpp"
#include "image_transport/image_transport.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgcodecs.hpp"
#include "rclcpp/rclcpp.hpp"
#include "include/Camera.h"
#include "std_msgs/msg/header.hpp"
#include "sensor_msgs/msg/image.hpp"
#include <thread>
#include <future>
#include <chrono>

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::NodeOptions options;
  rclcpp::Node::SharedPtr node = rclcpp::Node::make_shared("image_publisher", options);

  // Declare parameters with defaults
  node->declare_parameter("exposure_time", 16000.0);
  node->declare_parameter("trigger_mode", false);
  node->declare_parameter("serial_l", std::string("BC24484AAK00009"));
  node->declare_parameter("serial_r", std::string("BC24484AAK00010"));

  // Get parameter values
  double exposure_time = node->get_parameter("exposure_time").as_double();
  bool trigger_mode    = node->get_parameter("trigger_mode").as_bool();
  std::string serial_l = node->get_parameter("serial_l").as_string();
  std::string serial_r = node->get_parameter("serial_r").as_string();

  image_transport::ImageTransport it(node);
  auto pubL = node->create_publisher<sensor_msgs::msg::Image>("camera/imageL", 
    rclcpp::QoS(1).best_effort());
  auto pubR = node->create_publisher<sensor_msgs::msg::Image>("camera/imageR", 
    rclcpp::QoS(1).best_effort());

  Camera cameraL(serial_l);
  Camera cameraR(serial_r);

  cameraL.init(exposure_time, trigger_mode);
  cameraR.init(exposure_time, trigger_mode);
  
  while (rclcpp::ok()) {
    cv::Mat imageL, imageR;

    // Grab and convert both cameras in parallel using async
    auto t0 = std::chrono::steady_clock::now();
    auto futureL = std::async(std::launch::async, [&]() {
      return cameraL.getImage(imageL);
    });
    auto futureR = std::async(std::launch::async, [&]() {
      return cameraR.getImage(imageR);
    });

    int statusL = futureL.get();
    int statusR = futureR.get();

    if (statusL != 0 || statusR != 0) {
        continue;  // skip if either failed
    }
    auto t1 = std::chrono::steady_clock::now();

    auto stamp = node->now();

    // Replace the loaned message blocks with this:
    auto msgL = std::make_unique<sensor_msgs::msg::Image>();
    msgL->header.stamp = stamp;
    msgL->header.frame_id = "camera_left";
    msgL->height = imageL.rows;
    msgL->width = imageL.cols;
    msgL->encoding = "bgr8";
    msgL->is_bigendian = false;
    msgL->step = imageL.cols * 3;
    // msgL->data.resize(imageL.total() * imageL.elemSize());
    // memcpy(msgL->data.data(), imageL.data, msgL->data.size());
    msgL->data.assign(imageL.data, imageL.data + imageL.total() * imageL.elemSize());
    pubL->publish(std::move(msgL));

    auto msgR = std::make_unique<sensor_msgs::msg::Image>();
    msgR->header.stamp = stamp;
    msgR->header.frame_id = "camera_right";
    msgR->height = imageR.rows;
    msgR->width = imageR.cols;
    msgR->encoding = "bgr8";
    msgR->is_bigendian = false;
    msgR->step = imageR.cols * 3;
    // msgL->data.resize(imageL.total() * imageL.elemSize());
    // memcpy(msgL->data.data(), imageL.data, msgL->data.size());
    msgR->data.assign(imageR.data, imageR.data + imageR.total() * imageR.elemSize());
    pubR->publish(std::move(msgR));
    auto t2 = std::chrono::steady_clock::now();

    printf("%ldms %ldms\n", (t1-t0).count(), (t2-t1).count());

    rclcpp::spin_some(node);
  }
}
