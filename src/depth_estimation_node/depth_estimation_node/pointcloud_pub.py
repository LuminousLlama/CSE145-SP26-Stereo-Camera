#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import PointCloud2, PointField, Image
from message_filters import ApproximateTimeSynchronizer, Subscriber
import struct
import numpy as np
import cv2
from cv_bridge import CvBridge
from .stereo import gen_pointcloud_from_disparity, rectification_map
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy

qos = QoSProfile(
    reliability=ReliabilityPolicy.BEST_EFFORT,
    history=HistoryPolicy.KEEP_LAST,
    depth=10
)

camera_matrix_L = np.array([[2.12586235e+03, 0.00000000e+00, 6.08822181e+02],
                             [0.00000000e+00, 2.13144724e+03, 5.00933963e+02],
                             [0.00000000e+00, 0.00000000e+00, 1.00000000e+00]])
dist_coeffs_L = np.array([-0.07949666, 0.29615714, -0.0039992, -0.00234051, -0.83580527])

camera_matrix_R = np.array([[2.07018872e+03, 0.00000000e+00, 6.34785055e+02],
                             [0.00000000e+00, 2.06888960e+03, 4.74850490e+02],
                             [0.00000000e+00, 0.00000000e+00, 1.00000000e+00]])
dist_coeffs_R = np.array([-0.09998571, 0.57469675, -0.00285518, -0.00264086, -1.54810632])

cam1_ext = np.array([[1.00000, 0.00000, 0.00000, 0.00000],
                     [0.00000, 1.00000, 0.00000, 0.00000],
                     [0.00000, 0.00000, 1.00000, 0.00000],
                     [0.00000, 0.00000, 0.00000, 1.00000]])  # extrinsic parameters, camera 1
cam2_ext = np.array([
 [ 9.99738554e-01,  9.53599243e-03, -2.07819485e-02, -2.18013592e-01],
 [-9.35516443e-03,  9.99917683e-01,  8.78112561e-03, -1.14763914e-02],
 [ 2.08639746e-02, -8.58441127e-03,  9.99745469e-01, -1.91280374e-02],
 [ 0.00000000e+00,  0.00000000e+00,  0.00000000e+00,  1.00000000e+00]])  # extrinsic parameters, camera 2

class PointCloudPublisher(Node):
    def __init__(self):
        super().__init__('pc_publisher')
        self.pub = self.create_publisher(PointCloud2, 'points', 10)
        self.bridge = CvBridge()

        self.map1_x = None
        self.map1_y = None
        self.map2_x = None
        self.map2_y = None
        self.P1 = None
        self.P2 = None
        self.Q = None
        self.rectflag = False

        subL = Subscriber(self, Image, '/camera/imageL', qos_profile=qos)
        subR = Subscriber(self, Image, '/camera/imageR', qos_profile=qos)

        self.sync = ApproximateTimeSynchronizer(
            [subL, subR],
            queue_size=10,
            slop=0.01
        )
        self.sync.registerCallback(self.synced_callback)

    def synced_callback(self, msgL, msgR):
        imgL = self.bridge.imgmsg_to_cv2(msgL, desired_encoding='rgb8')
        imgR = self.bridge.imgmsg_to_cv2(msgR, desired_encoding='rgb8')

        if not self.rectflag:
            try:
                self.map1_x, self.map1_y, self.map2_x, self.map2_y, self.P1, self.P2, self.Q = rectification_map(
                    imgL, imgR,
                    camera_matrix_L, dist_coeffs_L,
                    cam1_ext, cam2_ext,
                    camera_matrix_R, dist_coeffs_R
                )
                self.rectflag = True
            except Exception as e:
                self.get_logger().error(f'Rectification failed: {e}')
                return

        if self.map1_x is None:
            print("dah")
            return

        points = gen_pointcloud_from_disparity(
            imgL, imgR,
            self.map1_x, self.map1_y,
            self.map2_x, self.map2_y,
            self.Q
        )

        if points is None or len(points) == 0:
            print("sad")
            return

        msg = PointCloud2()
        msg.header.frame_id = "map"
        msg.header.stamp = msgL.header.stamp
        msg.height = 1
        msg.width = len(points)
        msg.fields = [
            PointField(name='x', offset=0,  datatype=PointField.FLOAT32, count=1),
            PointField(name='y', offset=4,  datatype=PointField.FLOAT32, count=1),
            PointField(name='z', offset=8,  datatype=PointField.FLOAT32, count=1),
            PointField(name='rgb', offset=12, datatype=PointField.UINT32, count=1),
        ]
        msg.is_bigendian = False
        msg.point_step = 16
        msg.row_step = msg.point_step * msg.width

        pts = np.array(points, dtype=np.float32)  # Nx6

        xyz = pts[:, :3].astype(np.float32)
        rgb_arr = pts[:, 3:6].astype(np.uint8)

        # Pack RGB into uint32
        rgb_packed = (rgb_arr[:, 2].astype(np.uint32) |          # B
                    (rgb_arr[:, 1].astype(np.uint32) << 8)  |   # G
                    (rgb_arr[:, 0].astype(np.uint32) << 16))     # R

        # Build structured array in one shot
        cloud = np.zeros(len(pts), dtype=[
            ('x', np.float32), ('y', np.float32), ('z', np.float32),
            ('rgb', np.uint32)
        ])
        cloud['x'] = xyz[:, 0]
        cloud['y'] = xyz[:, 1]
        cloud['z'] = xyz[:, 2]
        cloud['rgb'] = rgb_packed

        msg.data = cloud.tobytes()
        self.pub.publish(msg)

def main():
    rclpy.init()
    node = PointCloudPublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()