
#include "cv_bridge/cv_bridge.hpp"
#include "image_transport/image_transport.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgcodecs.hpp"
#include "rclcpp/rclcpp.hpp"
#include "include/Camera.h"
#include "std_msgs/msg/header.hpp"

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::NodeOptions options;
  
  rclcpp::Node::SharedPtr node = rclcpp::Node::make_shared("image_publisher", options);
  image_transport::ImageTransport it(node);
  image_transport::Publisher pubL = it.advertise("camera/imageL", 1);
  image_transport::Publisher pubR = it.advertise("camera/imageR", 1);

  Camera cameraL("ABCD");
  Camera cameraR("1234");

  cameraL.init(16000, true);
  cameraR.init(16000, true);
  
  rclcpp::WallRate loop_rate(100);
  while (rclcpp::ok()) {
    cv::Mat imageL, imageR;
    int statusL = 0, statusR = 0;

    // Grab both cameras in parallel
    std::thread grabL([&]() {
        statusL = cameraL.getImage(imageL);
    });
    std::thread grabR([&]() {
        statusR = cameraR.getImage(imageR);
    });

    grabL.join();  // wait for both to finish
    grabR.join();

    if (statusL != 0 || statusR != 0) {
        continue;  // skip if either failed
    }

    // Publish with same timestamp for sync
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