#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import PointCloud2, PointField
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

camera_matrix = np.array([[623.53830, 0.00000, 640.00000], 
                            [0.00000, 623.53830, 360.00000], 
                            [0.00000, 0.00000, 1.00000]])
cam1_ext = np.array([[1.00000, 0.00000, 0.00000, 0.00000], 
					 [0.00000,	-1.00000,	0.00000,	1.00000],
					 [0.00000,	0.00000,	-1.00000,	-10.00000],
					 [0.00000,	0.00000,	0.00000,	1.00000]]) #extrinsic parameters, camera 1
cam2_ext = np.array([[0.99444, 0.00000, 0.10530, 0.55578], 
					 [0.00000,	-1.00000,	0.00000,	1.00000], 
					 [0.10530,	0.00000,	-0.99444,	-9.99706], 
					 [0.00000,	0.00000,	0.00000,	1.00000]]) #extrinsic parameters, camera 2
dist_coeffs = None #distortion coefficients

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
        elif not self.rectflag:
            try:
                if camera_matrix is not None and cam1_ext is not None and cam2_ext is not None:
                    self.map1_x, self.map1_y, self.map2_x, self.map2_y, self.P1, self.P2, self.Q = rectification_map(self.imgL, self.imgR, camera_matrix, dist_coeffs, cam1_ext, cam2_ext)
                    self.rectflag = True
                else:
                    print('camera_matrix, dist_coeffs, cam1_ext and cam2_ext are not all set; skipping rectification')
            except Exception as e:
                print('Rectification failed:', e)

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
    node = PointCloudPublisher()
    rclpy.spin(node)
    rclpy.shutdown()
