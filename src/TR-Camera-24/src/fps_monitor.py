#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, QoSReliabilityPolicy, QoSHistoryPolicy
from sensor_msgs.msg import Image
import time

class FPSMonitor(Node):
    def __init__(self):
        super().__init__('fps_monitor')

        self.timesL = []
        self.timesR = []
        self.window = 100

        qos = QoSProfile(
            reliability=QoSReliabilityPolicy.BEST_EFFORT,
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=1
        )

        self.create_subscription(Image, 'camera/imageL', self.callbackL, qos)
        self.create_subscription(Image, 'camera/imageR', self.callbackR, qos)

        self.create_timer(1.0, self.print_fps)

    def callbackL(self, msg):
        self.timesL.append(time.monotonic())
        if len(self.timesL) > self.window:
            self.timesL.pop(0)

    def callbackR(self, msg):
        self.timesR.append(time.monotonic())
        if len(self.timesR) > self.window:
            self.timesR.pop(0)

    def print_fps(self):
        fpsL = self.calc_fps(self.timesL)
        fpsR = self.calc_fps(self.timesR)
        self.get_logger().info(f"imageL: {fpsL:.2f} fps | imageR: {fpsR:.2f} fps")

    def calc_fps(self, times):
        if len(times) < 2:
            return 0.0
        elapsed = times[-1] - times[0]
        if elapsed == 0:
            return 0.0
        return (len(times) - 1) / elapsed

def main():
    rclpy.init()
    node = FPSMonitor()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == "__main__":
    main()
