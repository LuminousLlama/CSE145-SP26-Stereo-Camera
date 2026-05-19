#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "cv_bridge/cv_bridge.hpp"
#include <opencv2/opencv.hpp>

class VIOAdapter : public rclcpp::Node {
public:
    VIOAdapter() : Node("vio_adapter") {
        // Subscribe to your existing topics
        imu_sub_ = create_subscription<sensor_msgs::msg::Imu>(
            "/imu/data", 100,
            [this](sensor_msgs::msg::Imu::SharedPtr msg) {
                // VINS needs raw accel (with gravity), not linear accel
                // Make sure your IMU node publishes raw accel
                imu_pub_->publish(*msg);
            });

        imgL_sub_ = create_subscription<sensor_msgs::msg::Image>(
            "/camera/imageL", 10,
            [this](sensor_msgs::msg::Image::SharedPtr msg) {
                // Convert to grayscale — VINS works on grayscale
                auto cv_img = cv_bridge::toCvShare(msg, "rgb8");
                cv::Mat gray;
                cv::cvtColor(cv_img->image, gray, cv::COLOR_RGB2GRAY);
                auto gray_msg = cv_bridge::CvImage(
                    msg->header, "mono8", gray).toImageMsg();
                imgL_pub_->publish(*gray_msg);
            });

        imgR_sub_ = create_subscription<sensor_msgs::msg::Image>(
            "/camera/imageR", 10,
            [this](sensor_msgs::msg::Image::SharedPtr msg) {
                auto cv_img = cv_bridge::toCvShare(msg, "rgb8");
                cv::Mat gray;
                cv::cvtColor(cv_img->image, gray, cv::COLOR_RGB2GRAY);
                auto gray_msg = cv_bridge::CvImage(
                    msg->header, "mono8", gray).toImageMsg();
                imgR_pub_->publish(*gray_msg);
            });

        // Republish on topics VINS subscribes to
        imu_pub_  = create_publisher<sensor_msgs::msg::Imu>("/vins/data", 100);
        imgL_pub_ = create_publisher<sensor_msgs::msg::Image>("/vins/imageL", 10);
        imgR_pub_ = create_publisher<sensor_msgs::msg::Image>("/vins/imageR", 10);

        RCLCPP_INFO(get_logger(), "VIO adapter started — converting to grayscale for VINS");
    }

private:
    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr imgL_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr imgR_sub_;
    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr imgL_pub_;
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr imgR_pub_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<VIOAdapter>());
    rclcpp::shutdown();
    return 0;
}