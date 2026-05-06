#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import PointCloud2, PointField, Image
from message_filters import ApproximateTimeSynchronizer, Subscriber
import struct
import numpy as np
import cv2
from sensor_msgs.msg import Image
from cv_bridge import CvBridge

from .stereo import gen_pointcloud_from_params, rectification_map

from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy

qos = QoSProfile(
    reliability=ReliabilityPolicy.BEST_EFFORT,
    history=HistoryPolicy.KEEP_LAST,
    depth=10
)

#load a pair of images 
#img1 = cv2.imread('../images/left.png')
#img2 = cv2.imread('../images/right.png')

camera_matrix_L = np.array([[2.12586235e+03, 0.00000000e+00, 6.08822181e+02],
                             [0.00000000e+00, 2.13144724e+03, 5.00933963e+02],
                             [0.00000000e+00, 0.00000000e+00, 1.00000000e+00]])
dist_coeffs_L = np.array([-0.07949666, 0.29615714, -0.0039992, -0.00234051, -0.83580527])

camera_matrix_R = np.array([[2.07018872e+03, 0.00000000e+00, 6.34785055e+02],
                             [0.00000000e+00, 2.06888960e+03, 4.74850490e+02],
                             [0.00000000e+00, 0.00000000e+00, 1.00000000e+00]])
dist_coeffs_R = np.array([-0.09998571, 0.57469675, -0.00285518, -0.00264086, -1.54810632])

cam1_ext = np.array([[1.00000, 0.00000, 0.00000, 0.00000],
                     [0.00000, -1.00000, 0.00000, 1.00000],
                     [0.00000, 0.00000, -1.00000, -10.00000],
                     [0.00000, 0.00000, 0.00000, 1.00000]])  # extrinsic parameters, camera 1
cam2_ext = np.array([[0.99444, 0.00000, 0.10530, 0.55578],
                     [0.00000, -1.00000, 0.00000, 1.00000],
                     [0.10530, 0.00000, -0.99444, -9.99706],
                     [0.00000, 0.00000, 0.00000, 1.00000]])  # extrinsic parameters, camera 2

class PointCloudPublisher(Node):
    def __init__(self):
        super().__init__('pc_publisher')
        self.pub = self.create_publisher(PointCloud2, 'points', 10)
        self.timer = self.create_timer(0.0, self.publish_points)
        self.map1_x, self.map1_y, self.map2_x, self.map2_y, self.P1, self.P2, self.Q = None, None, None, None, None, None, None
        self.rectflag = False

        self.bridge = CvBridge()

        self.imgL = None
        self.imgR = None

        self.subL = self.create_subscription(
            Image,
            '/camera/imageL',
            self.left_callback,
            qos
        )

        self.subR = self.create_subscription(
            Image,
            '/camera/imageR',
            self.right_callback,
            qos
        )

        

    def left_callback(self, msg):
        self.imgL = self.bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')

    def right_callback(self, msg):
        self.imgR = self.bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')

    def publish_points(self):
        if self.imgL is None or self.imgR is None:
            return
        if not self.rectflag:
            try:
                self.map1_x, self.map1_y, self.map2_x, self.map2_y, self.P1, self.P2, self.Q = rectification_map(self.imgL, self.imgR, camera_matrix_L, dist_coeffs_L, cam1_ext, cam2_ext, camera_matrix_R, dist_coeffs_R)
                self.rectflag = True
            except Exception as e:
                print('Rectification failed:', e)
                return
        if self.map1_x is None:
            return

        msg = PointCloud2()
        msg.header.frame_id = "map"

        # Example data
        # points = [
        #     (1.0, 0.0, 0.0, 255, 0, 0),
        #     (0.0, 1.0, 0.0, 0, 255, 0),
        #     (0.0, 0.0, 1.0, 0, 0, 255),
        # ]

        # Real data
        points = gen_pointcloud_from_params(self.imgL, self.imgR, self.map1_x, self.map1_y, self.map2_x, self.map2_y, self.P1, self.P2)

        msg.height = 1
        msg.width = len(points)

        msg.fields = [
            PointField(name='x', offset=0, datatype=PointField.FLOAT32, count=1),
            PointField(name='y', offset=4, datatype=PointField.FLOAT32, count=1),
            PointField(name='z', offset=8, datatype=PointField.FLOAT32, count=1),
            PointField(name='rgb', offset=12, datatype=PointField.UINT32, count=1),
        ]

        msg.is_bigendian = False
        msg.point_step = 16
        msg.row_step = msg.point_step * msg.width

        data = []
        for x, y, z, r, g, b in points:
            r = int(r)
            g = int(g)
            b = int(b)
            rgb = struct.unpack('I', struct.pack('BBBB', b, g, r, 0))[0]
            data.append(struct.pack('fffI', x, y, z, rgb))

        msg.data = b''.join(data)

        self.pub.publish(msg)

def main():
    rclpy.init()
    print("hi!")
    node = PointCloudPublisher()
    rclpy.spin(node)
    rclpy.shutdown()