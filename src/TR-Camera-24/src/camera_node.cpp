
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
  image_transport::Publisher pub = it.advertise("camera/image", 1);

  Camera camera {};
  camera.init(4000); //RMNA 2024 conditions

  rclcpp::WallRate loop_rate(200);
  while (rclcpp::ok()) {
    cv::Mat image = {};
    int status = camera.getImage(image);
    if (status != 0) {
      continue;
    }
    std_msgs::msg::Header hdr;
    sensor_msgs::msg::Image::SharedPtr msg = cv_bridge::CvImage(hdr, "bgr8", image).toImageMsg();
    msg->header.stamp = node->now();
    pub.publish(msg);
    rclcpp::spin_some(node);
    loop_rate.sleep();
  }
}