#include "cv_bridge/cv_bridge.hpp"
#include "image_transport/image_transport.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgcodecs.hpp"
#include "rclcpp/rclcpp.hpp"
#include "include/Camera.h"
#include "std_msgs/msg/header.hpp"
#include <thread>

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
  image_transport::Publisher pubL = it.advertise("camera/imageL", 1);
  image_transport::Publisher pubR = it.advertise("camera/imageR", 1);

  Camera cameraL(serial_l);
  Camera cameraR(serial_r);

  cameraL.init(exposure_time, trigger_mode);
  cameraR.init(exposure_time, trigger_mode);
  rclcpp::WallRate loop_rate(100);
  while (rclcpp::ok()) {
    cv::Mat imageL, imageR;
    int statusL = 0, statusR = 0;

    std::thread grabL([&]() { statusL = cameraL.getImage(imageL); });
    std::thread grabR([&]() { statusR = cameraR.getImage(imageR); });
    grabL.join();
    grabR.join();

    if (statusL != 0 || statusR != 0) {
        continue;  // skip if either failed
    }

    auto stamp = node->now();

    std_msgs::msg::Header hdrL;
    auto msgL = cv_bridge::CvImage(hdrL, "bgr8", imageL).toImageMsg();
    msgL->header.stamp = stamp;
    pubL.publish(msgL);

    std_msgs::msg::Header hdrR;
    auto msgR = cv_bridge::CvImage(hdrR, "bgr8", imageR).toImageMsg();
    msgR->header.stamp = stamp;
    pubR.publish(msgR);

    rclcpp::spin_some(node);
    loop_rate.sleep();
  }
}