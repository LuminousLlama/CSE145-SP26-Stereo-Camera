#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/magnetic_field.hpp>
#include <sensor_msgs/msg/temperature.hpp>
#include <geometry_msgs/msg/vector3_stamped.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <chrono>
#include <thread>

#define BNO055_ADDR     0x28
#define I2C_BUS         "/dev/i2c-1"

#define REG_CHIP_ID         0x00
#define REG_OPR_MODE        0x3D
#define REG_PWR_MODE        0x3E
#define REG_SYS_TRIGGER     0x3F
#define REG_UNIT_SEL        0x3B
#define REG_ACC_DATA_X_LSB  0x08
#define REG_MAG_DATA_X_LSB  0x0E
#define REG_GYR_DATA_X_LSB  0x14
#define REG_EUL_DATA_X_LSB  0x1A
#define REG_QUA_DATA_W_LSB  0x20
#define REG_LIA_DATA_X_LSB  0x28
#define REG_TEMP            0x34
#define REG_CALIB_STAT      0x35

#define MODE_CONFIG     0x00
#define MODE_NDOF       0x0C

class BNO055 {
public:
    int fd;

    BNO055(const char* bus, uint8_t addr) {
        fd = open(bus, O_RDWR);
        if (fd < 0)
            throw std::runtime_error("Failed to open I2C bus");
        if (ioctl(fd, I2C_SLAVE, addr) < 0)
            throw std::runtime_error("Failed to set I2C address");

        uint8_t chip_id = read_byte(REG_CHIP_ID);
        if (chip_id != 0xA0)
            throw std::runtime_error("BNO055 not found, chip ID: " + std::to_string(chip_id));

        reset();
        set_mode(MODE_NDOF);
    }

    ~BNO055() { close(fd); }

    void write_byte(uint8_t reg, uint8_t val) {
        uint8_t buf[2] = {reg, val};
        if (write(fd, buf, 2) != 2)
            throw std::runtime_error("I2C write failed");
    }

    uint8_t read_byte(uint8_t reg) {
        if (write(fd, &reg, 1) != 1)
            throw std::runtime_error("I2C write reg failed");
        uint8_t val;
        if (read(fd, &val, 1) != 1)
            throw std::runtime_error("I2C read failed");
        return val;
    }

    void read_bytes(uint8_t reg, uint8_t* buf, int len) {
        if (write(fd, &reg, 1) != 1)
            throw std::runtime_error("I2C write reg failed");
        if (read(fd, buf, len) != len)
            throw std::runtime_error("I2C read failed");
    }

    void reset() {
        write_byte(REG_SYS_TRIGGER, 0x20);
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
        write_byte(REG_PWR_MODE, 0x00);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        write_byte(REG_SYS_TRIGGER, 0x00);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        write_byte(REG_UNIT_SEL, 0x01);  // m/s^2, rad/s
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    void set_mode(uint8_t mode) {
        write_byte(REG_OPR_MODE, MODE_CONFIG);
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        write_byte(REG_OPR_MODE, mode);
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    void get_calibration(uint8_t& sys, uint8_t& gyro, uint8_t& accel, uint8_t& mag) {
        uint8_t cal = read_byte(REG_CALIB_STAT);
        sys   = (cal >> 6) & 0x03;
        gyro  = (cal >> 4) & 0x03;
        accel = (cal >> 2) & 0x03;
        mag   = cal & 0x03;
    }

    int16_t read_int16(uint8_t* buf, int offset) {
        return (int16_t)(buf[offset] | (buf[offset+1] << 8));
    }

    struct ImuData {
        // Quaternion
        double qw, qx, qy, qz;
        // Euler angles in degrees (global frame, from BNO055 fusion)
        double yaw, roll, pitch;
        // Linear acceleration in m/s^2 (gravity removed)
        double ax, ay, az;
        // Angular velocity in rad/s
        double gx, gy, gz;
        // Magnetic field in uT
        double mx, my, mz;
        // Temperature in C
        double temp;
    };

    ImuData read_data() {
        ImuData data;
        uint8_t buf[8];

        // Quaternion
        read_bytes(REG_QUA_DATA_W_LSB, buf, 8);
        data.qw = read_int16(buf, 0) / 16384.0;
        data.qx = read_int16(buf, 2) / 16384.0;
        data.qy = read_int16(buf, 4) / 16384.0;
        data.qz = read_int16(buf, 6) / 16384.0;

        // Euler angles
        read_bytes(REG_EUL_DATA_X_LSB, buf, 6);
        data.yaw   = read_int16(buf, 0) / 16.0;  // heading 0-360
        data.roll  = read_int16(buf, 2) / 16.0;
        data.pitch = read_int16(buf, 4) / 16.0;

        // Linear acceleration
        read_bytes(REG_LIA_DATA_X_LSB, buf, 6);
        data.ax = read_int16(buf, 0) / 100.0;
        data.ay = read_int16(buf, 2) / 100.0;
        data.az = read_int16(buf, 4) / 100.0;

        // Gyroscope
        read_bytes(REG_GYR_DATA_X_LSB, buf, 6);
        data.gx = read_int16(buf, 0) / 900.0;
        data.gy = read_int16(buf, 2) / 900.0;
        data.gz = read_int16(buf, 4) / 900.0;

        // Magnetometer
        read_bytes(REG_MAG_DATA_X_LSB, buf, 6);
        data.mx = read_int16(buf, 0) / 16.0;
        data.my = read_int16(buf, 2) / 16.0;
        data.mz = read_int16(buf, 4) / 16.0;

        // Temperature
        data.temp = (int8_t)read_byte(REG_TEMP);

        return data;
    }
};

class ImuNode : public rclcpp::Node {
public:
    ImuNode() : Node("imu_node") {
        imu_pub_   = create_publisher<sensor_msgs::msg::Imu>("/imu/data", 10);
        euler_pub_ = create_publisher<geometry_msgs::msg::Vector3Stamped>("/imu/euler", 10);
        mag_pub_   = create_publisher<sensor_msgs::msg::MagneticField>("/imu/mag", 10);
        temp_pub_  = create_publisher<sensor_msgs::msg::Temperature>("/imu/temp", 10);

        try {
            bno_ = std::make_unique<BNO055>(I2C_BUS, BNO055_ADDR);
            RCLCPP_INFO(get_logger(), "BNO055 initialized in NDOF mode");
        } catch (const std::exception& e) {
            RCLCPP_FATAL(get_logger(), "BNO055 init failed: %s", e.what());
            throw;
        }

        timer_ = create_wall_timer(
            std::chrono::milliseconds(10),
            std::bind(&ImuNode::publish, this)
        );

        cal_timer_ = create_wall_timer(
            std::chrono::seconds(5),
            std::bind(&ImuNode::log_calibration, this)
        );
    }

private:
    void publish() {
        BNO055::ImuData d;
        try {
            d = bno_->read_data();
        } catch (const std::exception& e) {
            RCLCPP_WARN(get_logger(), "IMU read failed: %s", e.what());
            return;
        }

        auto stamp = now();
        publish_imu(d, stamp);
        publish_euler(d, stamp);
        publish_mag(d, stamp);
        publish_temp(d, stamp);
    }

    void publish_imu(const BNO055::ImuData& d, const rclcpp::Time& stamp) {
        auto msg = sensor_msgs::msg::Imu();
        msg.header.stamp = stamp;
        msg.header.frame_id = "imu_link";

        msg.orientation.w = d.qw;
        msg.orientation.x = d.qx;
        msg.orientation.y = d.qy;
        msg.orientation.z = d.qz;

        msg.angular_velocity.x = d.gx;
        msg.angular_velocity.y = d.gy;
        msg.angular_velocity.z = d.gz;

        msg.linear_acceleration.x = d.ax;
        msg.linear_acceleration.y = d.ay;
        msg.linear_acceleration.z = d.az;

        msg.orientation_covariance[0] = 0.01;
        msg.orientation_covariance[4] = 0.01;
        msg.orientation_covariance[8] = 0.01;
        msg.angular_velocity_covariance[0] = 0.01;
        msg.angular_velocity_covariance[4] = 0.01;
        msg.angular_velocity_covariance[8] = 0.01;
        msg.linear_acceleration_covariance[0] = 0.01;
        msg.linear_acceleration_covariance[4] = 0.01;
        msg.linear_acceleration_covariance[8] = 0.01;

        imu_pub_->publish(msg);
    }

    void publish_euler(const BNO055::ImuData& d, const rclcpp::Time& stamp) {
        auto msg = geometry_msgs::msg::Vector3Stamped();
        msg.header.stamp = stamp;
        msg.header.frame_id = "imu_link";
        msg.vector.x = d.roll;   // degrees
        msg.vector.y = d.pitch;  // degrees
        msg.vector.z = d.yaw;    // degrees (0-360, north referenced)
        euler_pub_->publish(msg);
    }

    void publish_mag(const BNO055::ImuData& d, const rclcpp::Time& stamp) {
        auto msg = sensor_msgs::msg::MagneticField();
        msg.header.stamp = stamp;
        msg.header.frame_id = "imu_link";
        msg.magnetic_field.x = d.mx * 1e-6;  // uT to T
        msg.magnetic_field.y = d.my * 1e-6;
        msg.magnetic_field.z = d.mz * 1e-6;
        mag_pub_->publish(msg);
    }

    void publish_temp(const BNO055::ImuData& d, const rclcpp::Time& stamp) {
        auto msg = sensor_msgs::msg::Temperature();
        msg.header.stamp = stamp;
        msg.header.frame_id = "imu_link";
        msg.temperature = d.temp;
        temp_pub_->publish(msg);
    }

    void log_calibration() {
        uint8_t sys, gyro, accel, mag;
        bno_->get_calibration(sys, gyro, accel, mag);
        RCLCPP_INFO(get_logger(),
            "Calibration — sys: %d/3, gyro: %d/3, accel: %d/3, mag: %d/3",
            sys, gyro, accel, mag);
    }

    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
    rclcpp::Publisher<geometry_msgs::msg::Vector3Stamped>::SharedPtr euler_pub_;
    rclcpp::Publisher<sensor_msgs::msg::MagneticField>::SharedPtr mag_pub_;
    rclcpp::Publisher<sensor_msgs::msg::Temperature>::SharedPtr temp_pub_;
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::TimerBase::SharedPtr cal_timer_;
    std::unique_ptr<BNO055> bno_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ImuNode>());
    rclcpp::shutdown();
    return 0;
}