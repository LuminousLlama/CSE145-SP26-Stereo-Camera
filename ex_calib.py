#!/usr/bin/env python3
import rclpy
import numpy as np
import cv2
from rclpy.node import Node
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
from message_filters import ApproximateTimeSynchronizer, Subscriber
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy
import dt_apriltags  # pip3 install dt-apriltags

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

TAG_SIZE = 0.200  # 200mm in meters
TAG_ID = 4

class ExtrinsicsCalibrator(Node):
    def __init__(self):
        super().__init__('extrinsics_calibrator')
        self.bridge = CvBridge()
        self.detector = dt_apriltags.Detector(families='tag36h11')

        self.R_list = []
        self.T_list = []
        self.sample_count = 0

        subL = Subscriber(self, Image, '/camera/imageL', qos_profile=qos)
        subR = Subscriber(self, Image, '/camera/imageR', qos_profile=qos)

        self.sync = ApproximateTimeSynchronizer([subL, subR], queue_size=10, slop=0.01)
        self.sync.registerCallback(self.callback)

        self.get_logger().info("Waiting for synced frames...")

    def detect_tag(self, img, camera_matrix, dist_coeffs):
        # Undistort first
        img_undist = cv2.undistort(img, camera_matrix, dist_coeffs)
        gray = cv2.cvtColor(img_undist, cv2.COLOR_RGB2GRAY)

        fx = camera_matrix[0, 0]
        fy = camera_matrix[1, 1]
        cx = camera_matrix[0, 2]
        cy = camera_matrix[1, 2]

        detections = self.detector.detect(
            gray,
            estimate_tag_pose=True,
            camera_params=(fx, fy, cx, cy),
            tag_size=TAG_SIZE
        )

        for d in detections:
            if d.tag_id == TAG_ID:
                return d.pose_R, d.pose_t
        return None, None

    def callback(self, msgL, msgR):
        imgL = self.bridge.imgmsg_to_cv2(msgL, desired_encoding='rgb8')
        imgR = self.bridge.imgmsg_to_cv2(msgR, desired_encoding='rgb8')

        R_L, T_L = self.detect_tag(imgL, camera_matrix_L, dist_coeffs_L)
        R_R, T_R = self.detect_tag(imgR, camera_matrix_R, dist_coeffs_R)

        if R_L is None or R_R is None:
            return

        # Relative pose: R_rel, T_rel transforms from cam L to cam R
        # P_R = R_R * P_world + T_R
        # P_L = R_L * P_world + T_L
        # => P_world = R_L^T * (P_L - T_L)
        # => P_R = R_R * R_L^T * P_L + (T_R - R_R * R_L^T * T_L)

        R_rel = R_R @ R_L.T
        T_rel = T_R - R_rel @ T_L

        self.R_list.append(R_rel)
        self.T_list.append(T_rel)
        self.sample_count += 1

        self.get_logger().info(f"Sample {self.sample_count} — T: {T_rel.flatten()}")

        if self.sample_count >= 30:
            self.compute_final()

    def compute_final(self):
        # Average rotation using mean of rotation matrices
        R_mean = np.mean(self.R_list, axis=0)
        # Re-orthogonalize via SVD
        U, _, Vt = np.linalg.svd(R_mean)
        R_final = U @ Vt

        T_final = np.mean(self.T_list, axis=0)

        self.get_logger().info("\n=== FINAL EXTRINSICS (cam L to cam R) ===")
        self.get_logger().info(f"R:\n{R_final}")
        self.get_logger().info(f"T (meters): {T_final.flatten()}")
        self.get_logger().info(f"T (mm): {T_final.flatten() * 1000}")

        # Print as cam2_ext matrix
        cam2_ext = np.eye(4)
        cam2_ext[:3, :3] = R_final
        cam2_ext[:3, 3] = T_final.flatten() * 1000  # convert to mm
        self.get_logger().info(f"\ncam2_ext =\n{cam2_ext}")

        rclpy.shutdown()

def main():
    rclpy.init()
    node = ExtrinsicsCalibrator()
    rclpy.spin(node)

if __name__ == '__main__':
    main()