#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import PointCloud2, PointField, Image
from message_filters import ApproximateTimeSynchronizer, Subscriber
import struct
import numpy as np
import cv2
import cv_bridge
from .stereo import gen_pointcloud_from_params

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
        self.bridge = cv_bridge.CvBridge()

        subL = Subscriber(self, Image, 'camera/imageL')
        subR = Subscriber(self, Image, 'camera/imageR')

        self.sync = ApproximateTimeSynchronizer(
            [subL, subR],
            queue_size=10,
            slop=0.01  # 10ms tolerance
        )
        self.sync.registerCallback(self.synced_callback)

    def synced_callback(self, msgL, msgR):
        imgL = self.bridge.imgmsg_to_cv2(msgL, desired_encoding='bgr8')
        imgR = self.bridge.imgmsg_to_cv2(msgR, desired_encoding='bgr8')

        points = gen_pointcloud_from_params(
            imgL, imgR, camera_matrix, dist_coeffs, cam1_ext, cam2_ext
        )

        msg = PointCloud2()
        msg.header.frame_id = "map"
        msg.header.stamp = msgL.header.stamp  # use camera timestamp, not receive time
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