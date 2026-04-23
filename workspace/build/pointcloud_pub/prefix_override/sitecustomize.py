import sys
if sys.prefix == '/usr':
    sys.real_prefix = sys.prefix
    sys.prefix = sys.exec_prefix = '/home/sumurthy/Documents/GitHub/CSE145-SP26-Stereo-Camera/workspace/install/pointcloud_pub'
