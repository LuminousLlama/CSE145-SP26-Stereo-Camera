#include "cv_bridge/cv_bridge.hpp"
#include "image_transport/image_transport.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgcodecs.hpp"
#include "rclcpp/rclcpp.hpp"
#include "include/Camera.h"
#include "sensor_msgs/msg/image.hpp"
#include <thread>
#include <future>
#include <chrono>

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("image_publisher");

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

  auto pubL = image_transport::create_publisher(node.get(), "camera/imageL",
    rclcpp::QoS(1).best_effort().get_rmw_qos_profile());
  auto pubR = image_transport::create_publisher(node.get(), "camera/imageR",
    rclcpp::QoS(1).best_effort().get_rmw_qos_profile());

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

    // Use cv_bridge
    sensor_msgs::msg::Image::SharedPtr msgL;
    sensor_msgs::msg::Image::SharedPtr msgR;

    std::thread buildL([&]() {
      auto cv_msg = cv_bridge::CvImage(
        std_msgs::msg::Header(),
        "bgr8",
        imageL
      );
      cv_msg.header.stamp = stamp;
      cv_msg.header.frame_id = "camera_left";

      msgL = cv_msg.toImageMsg();
    });

    std::thread buildR([&]() {
      auto cv_msg = cv_bridge::CvImage(
        std_msgs::msg::Header(),
        "bgr8",
        imageR
      );
      cv_msg.header.stamp = stamp;
      cv_msg.header.frame_id = "camera_right";

      msgR = cv_msg.toImageMsg();
    });

    buildL.join();
    buildR.join();

    pubL.publish(msgL);
    pubR.publish(msgR);

    auto t2 = std::chrono::steady_clock::now();

    auto capture_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    auto publish_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    RCLCPP_INFO(node->get_logger(),
      "capture=%ld ms publish=%ld ms",
      capture_ms,
      publish_ms);

    rclcpp::spin_some(node);
  }
  rclcpp::shutdown();
  return 0;
}