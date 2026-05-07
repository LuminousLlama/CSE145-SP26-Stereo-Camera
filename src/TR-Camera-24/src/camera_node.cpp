#include "cv_bridge/cv_bridge.hpp"
#include "image_transport/image_transport.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgcodecs.hpp"
#include "rclcpp/rclcpp.hpp"
#include "include/Camera.h"
#include "sensor_msgs/msg/image.hpp"
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <chrono>

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("image_publisher");

  // Declare parameters with defaults
  node->declare_parameter("exposure_time", 4000.0);
  node->declare_parameter("trigger_mode", false);
  node->declare_parameter("half_res", false);
  node->declare_parameter("serial_l", std::string("BC24484AAK00009"));
  node->declare_parameter("serial_r", std::string("BC24484AAK00010"));

  // Get parameter values
  double exposure_time = node->get_parameter("exposure_time").as_double();
  bool trigger_mode    = node->get_parameter("trigger_mode").as_bool();
  bool half_res        = node->get_parameter("half_res").as_bool();
  std::string serial_l = node->get_parameter("serial_l").as_string();
  std::string serial_r = node->get_parameter("serial_r").as_string();

  auto pubL = image_transport::create_publisher(node.get(), "camera/imageL",
    rclcpp::QoS(1).best_effort().get_rmw_qos_profile());
  auto pubR = image_transport::create_publisher(node.get(), "camera/imageR",
    rclcpp::QoS(1).best_effort().get_rmw_qos_profile());

  Camera cameraL(serial_l);
  Camera cameraR(serial_r);

  cameraL.init(exposure_time, trigger_mode, half_res);
  cameraR.init(exposure_time, trigger_mode, half_res);

  sensor_msgs::msg::Image msgL, msgR;
  msgL.encoding = "rgb8"; msgR.encoding = "rgb8";
  msgL.is_bigendian = false; msgR.is_bigendian = false;
  msgL.header.frame_id = "camera_left";
  msgR.header.frame_id = "camera_right";
  bool allocated = false;

  // Persistent memcpy thread state
  bool running = true;
  cv::Mat imageL, imageR;

  std::mutex copyMutexL, copyMutexR;
  std::condition_variable copyCvL, copyCvR;
  bool doCopyL = false, doCopyR = false;
  bool copyDoneL = false, copyDoneR = false;

  std::thread copyThreadL([&]() {
    while (running) {
      std::unique_lock<std::mutex> lock(copyMutexL);
      copyCvL.wait(lock, [&]{ return doCopyL || !running; });
      if (!running) break;
      memcpy(msgL.data.data(), imageL.data, imageL.total() * imageL.elemSize());
      doCopyL = false;
      copyDoneL = true;
      copyCvL.notify_one();
    }
  });

  std::thread copyThreadR([&]() {
    while (running) {
      std::unique_lock<std::mutex> lock(copyMutexR);
      copyCvR.wait(lock, [&]{ return doCopyR || !running; });
      if (!running) break;
      memcpy(msgR.data.data(), imageR.data, imageR.total() * imageR.elemSize());
      doCopyR = false;
      copyDoneR = true;
      copyCvR.notify_one();
    }
  });

  while (rclcpp::ok()) {
    auto t0 = std::chrono::steady_clock::now();

    // Grab both cameras in parallel with async
    auto futureL = std::async(std::launch::async, [&]() {
      return cameraL.getImage(imageL);
    });
    auto futureR = std::async(std::launch::async, [&]() {
      return cameraR.getImage(imageR);
    });

    int statusL = futureL.get();
    int statusR = futureR.get();

    if (statusL != 0 || statusR != 0) {
      continue;
    }

    auto t1 = std::chrono::steady_clock::now();

    if (!allocated) {
      msgL.width  = imageL.cols; msgL.height = imageL.rows;
      msgL.step   = imageL.cols * 3;
      msgR.width  = imageR.cols; msgR.height = imageR.rows;
      msgR.step   = imageR.cols * 3;
      msgL.data.resize(imageL.total() * imageL.elemSize());
      msgR.data.resize(imageR.total() * imageR.elemSize());
      allocated = true;
    }

    auto stamp = node->now();
    msgL.header.stamp = stamp;
    msgR.header.stamp = stamp;

    // Trigger persistent memcpy threads
    {
      std::lock_guard<std::mutex> lock(copyMutexL);
      copyDoneL = false;
      doCopyL = true;
      copyCvL.notify_one();
    }
    {
      std::lock_guard<std::mutex> lock(copyMutexR);
      copyDoneR = false;
      doCopyR = true;
      copyCvR.notify_one();
    }

    // Wait for both copies to finish
    {
      std::unique_lock<std::mutex> lock(copyMutexL);
      copyCvL.wait(lock, [&]{ return copyDoneL; });
    }
    {
      std::unique_lock<std::mutex> lock(copyMutexR);
      copyCvR.wait(lock, [&]{ return copyDoneR; });
    }

    pubL.publish(msgL);
    pubR.publish(msgR);

    auto t2 = std::chrono::steady_clock::now();
    auto capture_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    auto publish_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    RCLCPP_INFO_THROTTLE(node->get_logger(), *node->get_clock(), 1000,
      "capture=%ld ms publish=%ld ms", capture_ms, publish_ms);

    rclcpp::spin_some(node);
  }

  running = false;
  copyCvL.notify_all();
  copyCvR.notify_all();
  copyThreadL.join();
  copyThreadR.join();
  rclcpp::shutdown();
  return 0;
}