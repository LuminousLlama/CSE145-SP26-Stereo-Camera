#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import PointCloud2, PointField
import struct
import numpy as np
import cv2

from .stereo import gen_pointcloud_from_params

#load a pair of images 
img1 = cv2.imread('../images/left.png')
img2 = cv2.imread('../images/right.png')

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
        self.timer = self.create_timer(1.0, self.publish_points)

    def publish_points(self):
        msg = PointCloud2()
        msg.header.frame_id = "map"

        # Example data
        # points = [
        #     (1.0, 0.0, 0.0, 255, 0, 0),
        #     (0.0, 1.0, 0.0, 0, 255, 0),
        #     (0.0, 0.0, 1.0, 0, 0, 255),
        # ]

        # Real data
        points = gen_pointcloud_from_params(img1, img2, camera_matrix, dist_coeffs, cam1_ext, cam2_ext)

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
