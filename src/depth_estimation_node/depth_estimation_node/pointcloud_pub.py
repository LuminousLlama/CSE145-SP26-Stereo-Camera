#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import PointCloud2, PointField, Image
from nav_msgs.msg import Odometry
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

camera_matrix_L = np.array([[1062.931175, 0.0,         304.411091],
                             [0.0,         1065.723620, 250.466982],
                             [0.0,         0.0,         1.0       ]])
dist_coeffs_L = np.array([-0.07949666, 0.29615714, -0.0039992, -0.00234051, -0.83580527])

camera_matrix_R = np.array([[1035.094360, 0.0,         317.392528],
                             [0.0,         1034.444800, 237.425245],
                             [0.0,         0.0,         1.0       ]])
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


def quat_to_rot(x, y, z, w) -> np.ndarray:
    """Quaternion to 3x3 rotation matrix, pure numpy."""
    return np.array([
        [1 - 2*(y*y + z*z),     2*(x*y - z*w),     2*(x*z + y*w)],
        [    2*(x*y + z*w), 1 - 2*(x*x + z*z),     2*(y*z - x*w)],
        [    2*(x*z - y*w),     2*(y*z + x*w), 1 - 2*(x*x + y*y)],
    ], dtype=np.float64)

# T_ros_vins = np.array([
#     [ 0,  1,  0,  0],
#     [ 1,  0,  0,  0],
#     [ 0,  0,  1,  0],
#     [ 0,  0,  0,  1]
# ], dtype=np.float64)

T_ros_vins = np.eye(4)

def odom_to_transform(msg: Odometry) -> np.ndarray:
    """Convert nav_msgs/Odometry pose to a 4x4 world-from-camera transform."""
    p = msg.pose.pose.position
    q = msg.pose.pose.orientation
    T = np.eye(4, dtype=np.float64)
    T[:3, :3] = quat_to_rot(q.x, q.y, q.z, q.w)
    T[:3,  3] = [p.x, p.y, p.z]
    return T_ros_vins @ T


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

        # Pose buffer: parallel lists kept sorted by timestamp (ns)
        # Sized for ~10 s at 30 Hz VIO output
        self._pose_stamps: list[int]        = []
        self._pose_Ts:     list[np.ndarray] = []
        self._BUFFER_SIZE = 30
        self._MAX_AGE_MS  = 2000  # reject lookup if nearest pose is older than this

        # Subscribe to VINS odometry independently (not time-synced with images —
        # it arrives at its own rate and we do a manual timestamp lookup instead)
        odom_qos = QoSProfile(
            reliability=ReliabilityPolicy.BEST_EFFORT,
            history=HistoryPolicy.KEEP_LAST,
            depth=50
        )
        self.create_subscription(Odometry, '/odometry',
                                 self._odom_cb, odom_qos)

        subL = Subscriber(self, Image, '/camera/imageL', qos_profile=qos)
        subR = Subscriber(self, Image, '/camera/imageR', qos_profile=qos)

        self.sync = ApproximateTimeSynchronizer(
            [subL, subR],
            queue_size=10,
            slop=0.01
        )
        self.sync.registerCallback(self.synced_callback)

    # Pose buffer
    def _odom_cb(self, msg: Odometry):
        t_ns = rclpy.time.Time.from_msg(msg.header.stamp).nanoseconds
        T    = odom_to_transform(msg)

        self._pose_stamps.append(t_ns)
        self._pose_Ts.append(T)

        if len(self._pose_stamps) > self._BUFFER_SIZE:
            self._pose_stamps.pop(0)
            self._pose_Ts.pop(0)

    def _lookup_pose(self, stamp_ns: int) -> np.ndarray | None:
        """Nearest-neighbour pose lookup. Returns 4x4 transform or None."""
        if not self._pose_stamps:
            return None

        # scan from newest backwards — the match is almost always at the tail
        best_idx = 0
        best_dt  = abs(self._pose_stamps[0] - stamp_ns)
        for i in range(len(self._pose_stamps) - 1, -1, -1):
            dt = abs(self._pose_stamps[i] - stamp_ns)
            if dt < best_dt:
                best_dt  = dt
                best_idx = i
            if self._pose_stamps[i] < stamp_ns:
                break  # gone past — earlier entries can only be worse

        dt_ms = best_dt / 1e6
        if dt_ms > self._MAX_AGE_MS:
            self.get_logger().warn(
                f'Pose lookup: nearest pose is {dt_ms:.1f} ms away — skipping world transform')
            # return None

        return self._pose_Ts[-1]

    # Stereo callback
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
            return

        points = gen_pointcloud_from_disparity(
            imgL, imgR,
            self.map1_x, self.map1_y,
            self.map2_x, self.map2_y,
            self.Q,
            0 #1 for yolo, 0 for standard pointcloud
        )

        if points is None or len(points) == 0:
            return

        pts     = np.array(points, dtype=np.float32)  # Nx6: x,y,z,r,g,b
        xyz     = pts[:, :3].astype(np.float64)
        rgb_arr = pts[:, 3:6].astype(np.uint8)

        # --- look up the world pose for this frame's timestamp and apply it ---
        stamp_ns    = rclpy.time.Time.from_msg(msgL.header.stamp).nanoseconds
        T_world_cam = self._lookup_pose(stamp_ns)

        if T_world_cam is not None:
            # homogeneous transform: (4x4) @ (4xN) -> (3xN)
            ones  = np.ones((len(xyz), 1), dtype=np.float64)
            xyz_h = np.hstack([xyz, ones])               # Nx4
            xyz   = (T_world_cam @ xyz_h.T).T[:, :3]    # Nx3
            frame_id = 'world'
        else:
            self.get_logger().warn('No VIO pose available — publishing in camera frame')
            frame_id = 'world'

        xyz = xyz.astype(np.float32)

        # --- pack RGB ---
        rgb_packed = (rgb_arr[:, 2].astype(np.uint32) |
                     (rgb_arr[:, 1].astype(np.uint32) << 8)  |
                     (rgb_arr[:, 0].astype(np.uint32) << 16))

        cloud = np.zeros(len(pts), dtype=[
            ('x', np.float32), ('y', np.float32), ('z', np.float32),
            ('rgb', np.uint32)
        ])
        cloud['x'] = xyz[:, 0]
        cloud['y'] = xyz[:, 1]
        cloud['z'] = xyz[:, 2]
        cloud['rgb'] = rgb_packed

        msg = PointCloud2()
        msg.header.stamp    = msgL.header.stamp   # preserve original camera timestamp
        msg.header.frame_id = frame_id
        msg.height     = 1
        msg.width      = len(pts)
        msg.fields     = [
            PointField(name='x',   offset=0,  datatype=PointField.FLOAT32, count=1),
            PointField(name='y',   offset=4,  datatype=PointField.FLOAT32, count=1),
            PointField(name='z',   offset=8,  datatype=PointField.FLOAT32, count=1),
            PointField(name='rgb', offset=12, datatype=PointField.UINT32,  count=1),
        ]
        msg.is_bigendian = False
        msg.point_step   = 16
        msg.row_step     = msg.point_step * msg.width
        msg.data         = cloud.tobytes()
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