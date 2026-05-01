import time

import numpy as np
import cv2
import unittest
import random
#from matplotlib import pyplot as plt

# import os
# print(os.getcwd())

#load a pair of images
#img1 = cv2.imread('../../../../images/left.png')
#img2 = cv2.imread('../../../../images/right.png')
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

#ext1
#1.00000	0.00000	0.00000	0.00000
#0.00000	-1.00000	0.00000	1.00000
#0.00000	0.00000	-1.00000	-10.00000
#0.00000	0.00000	0.00000	1.00000

#ext2
# 0.99444	0.00000	0.10530	0.55578
# 0.00000	-1.00000	0.00000	1.00000
# 0.10530	0.00000	-0.99444	-9.99706
# 0.00000	0.00000	0.00000	1.00000

#K
#623.53830	0.00000	640.00000	0.00000
#0.00000	623.53830	360.00000	0.00000
#0.00000	0.00000	1.00000	0.00000
#0.00000	0.00000	0.00000	1.00000

# cv2.imshow('My Image', img2)
# cv2.waitKey(0)
# cv2.destroyAllWindows()


def extract_R_T(cam1_ext, cam2_ext):
	#Return R, T from cam1 to cam2

	def to_4x4(E):
		E = np.asarray(E)
		if E.shape == (4, 4):
			return E
		if E.shape[0] == 3 and E.shape[1] == 4:
			M = np.eye(4, dtype=E.dtype)
			M[:3, :] = E
			return M
		if E.shape == (3, 3):
			M = np.eye(4, dtype=E.dtype)
			M[:3, :3] = E
			return M
		raise ValueError('Unsupported extrinsic shape: %s' % (E.shape,))

	#if provided as (R,t) tuples
	if isinstance(cam1_ext, tuple) and isinstance(cam2_ext, tuple):
		R1, t1 = cam1_ext
		R2, t2 = cam2_ext
		R_rel = np.asarray(R2).dot(np.asarray(R1).T)
		T_rel = np.asarray(t2).reshape(3, 1) - R_rel.dot(np.asarray(t1).reshape(3, 1))
		return R_rel, T_rel

	E1 = to_4x4(cam1_ext)
	E2 = to_4x4(cam2_ext)

	#relative transform from cam1 to cam2
	rel = np.linalg.inv(E1).dot(E2)
	R = rel[:3, :3]
	T = rel[:3, 3].reshape(3, 1)
	return R, T

def rectification_map(img1, img2, camera_matrix, dist_coeffs, cam1_ext, cam2_ext):
	if img1 is None or img2 is None:
		raise ValueError('Input images not loaded')
	if camera_matrix is None:
		raise ValueError('camera_matrix and dist_coeffs must be provided')

	R, T = extract_R_T(cam1_ext, cam2_ext)

	image_size = (img1.shape[1], img1.shape[0])

	R1, R2, P1, P2, Q, _, _ = cv2.stereoRectify(camera_matrix, dist_coeffs,
												camera_matrix, dist_coeffs,
												image_size, R, T,
												flags=0, #cv2.CALIB_ZERO_DISPARITY,
												alpha=1)

	map1_x, map1_y = cv2.initUndistortRectifyMap(camera_matrix, dist_coeffs, R1, P1, image_size, cv2.CV_16SC2)
	map2_x, map2_y = cv2.initUndistortRectifyMap(camera_matrix, dist_coeffs, R2, P2, image_size, cv2.CV_16SC2)

	return map1_x, map1_y, map2_x, map2_y, P1, P2, Q


def rectify_images(img1, img2, map1_x, map1_y, map2_x, map2_y, out_prefix='images/rectified'):
	#Runs remap to produce rectified images

	rectified1 = cv2.remap(img1, map1_x, map1_y, interpolation=cv2.INTER_LINEAR)
	rectified2 = cv2.remap(img2, map2_x, map2_y, interpolation=cv2.INTER_LINEAR)

	#print(rectified1.shape, rectified2.shape)

	#left_out = out_prefix + '_left.png'
	#right_out = out_prefix + '_right.png'
	#cv2.imwrite(left_out, rectified1)
	#cv2.imwrite(right_out, rectified2)

	return rectified1, rectified2

def disparity_raw(img1_rect, img2_rect, numDisparities=16, blockSize=5, method=1):
	stereo = None
	g1 = cv2.cvtColor(img1_rect, cv2.COLOR_BGR2GRAY)
	g2 = cv2.cvtColor(img2_rect, cv2.COLOR_BGR2GRAY)
	if method == 1:
		stereo = cv2.StereoBM.create(numDisparities=numDisparities, blockSize=blockSize)
	elif method == 2:
		stereo = cv2.StereoSGBM_create(0, numDisparities, blockSize)
	disparity = stereo.compute(g1, g2)
	return disparity.astype('float32') / 16.0

def disparity_WLS(img1_rect, img2_rect, numDisparities=32, blockSize=9, wls_lambda=8000, wls_sigma=0.5, method=1):
	left = None

	g1 = cv2.cvtColor(img1_rect, cv2.COLOR_BGR2GRAY)
	g2 = cv2.cvtColor(img2_rect, cv2.COLOR_BGR2GRAY)

	if method == 1:
		left = cv2.StereoBM_create(numDisparities=numDisparities, blockSize=blockSize)
	elif method == 2:
		left = cv2.StereoSGBM_create(0, numDisparities, blockSize)

	right = cv2.ximgproc.createRightMatcher(left)

	dL = left.compute(g1, g2)
	dR = right.compute(g2, g1)

	wls = cv2.ximgproc.createDisparityWLSFilter(left)
	wls.setLambda(wls_lambda)
	wls.setSigmaColor(wls_sigma)

	disparity = wls.filter(dL, img1_rect, None, dR)
	return disparity.astype('float32') / 16.0

def match_sparse(img1_rect, img2_rect, detector, max_matches=500):
	#detectors: 1 = AKAZE, more to be added
	g1 = cv2.cvtColor(img1_rect, cv2.COLOR_BGR2GRAY)
	g2 = cv2.cvtColor(img2_rect, cv2.COLOR_BGR2GRAY)

	det = None
	norm = None
	if detector == 1:
		det = cv2.AKAZE_create()
		norm = cv2.NORM_HAMMING
	if detector == 2:
		det = cv2.SIFT_create(nfeatures=500)
		norm = cv2.NORM_L2
	if detector == 3:
		det = cv2.ORB_create(nfeatures=500, nlevels=6)
		norm = cv2.NORM_HAMMING

	# Detect keypoints and descriptors
	kp1, des1 = det.detectAndCompute(g1, None)
	kp2, des2 = det.detectAndCompute(g2, None)

	bf = cv2.BFMatcher(norm, crossCheck=True, ) #might want to try FLANN here

	matches = bf.match(des1, des2)
	#matches = sorted(matches, key=lambda x: x.distance)

	return kp1, kp2, matches[:max_matches]


def draw_matches(img1_rect, img2_rect, kp1, kp2, matches, max_matches=50):
	img_matches = cv2.drawMatches(img1_rect, kp1, img2_rect, kp2, matches[:max_matches], None, flags=cv2.DrawMatchesFlags_NOT_DRAW_SINGLE_POINTS)
	return img_matches

def initialize_sparse_pointcloud(img1_rect, img2_rect, kp1, kp2, matches, P1, P2):
	pts1 = []
	pts2 = []
	colors = []

	for m in matches:
		x1, y1 = kp1[m.queryIdx].pt
		x2, y2 = kp2[m.trainIdx].pt

		pts1.append([x1, y1])
		pts2.append([x2, y2])

		xi, yi = int(round(x1)), int(round(y1))
		b, g, r = img1_rect[yi, xi]
		colors.append([r, g, b])

	pts1 = np.array(pts1).T
	pts2 = np.array(pts2).T

	#triangulate
	points_4d = cv2.triangulatePoints(P1[:3, :4], P2[:3, :4], pts1, pts2)
	points_3d = (points_4d[:3] / points_4d[3]).T  # shape (N, 3)
	colors = np.array(colors)  # shape (N, 3)

	pointcloud = np.hstack((points_3d, colors))

	return pointcloud

def initialize_dense_pointcloud(img1_rect, img2_rect, disparity, Q):
	points_3d = cv2.reprojectImageTo3D(disparity, Q)

	#valid disparity values are > 0 and finite
	valid_mask = np.isfinite(disparity) & (disparity > 0)

	pts = points_3d.reshape(-1, 3)
	mask_flat = valid_mask.reshape(-1)

	pts_valid = pts[mask_flat]

	colors = img1_rect.reshape(-1, 3)
	colors_rgb = colors[:, ::-1]
	colors_valid = colors_rgb[mask_flat]

	#Nx6 array: x, y, z, r, g, b
	pointcloud = np.hstack((pts_valid, colors_valid))

	return pointcloud

def gen_pointcloud_from_params(img1, img2, map1_x, map1_y, map2_x, map2_y, P1, P2):
	#equivalent to the main method but with parameters provided

	img1_rect, img2_rect = rectify_images(img1, img2, map1_x, map1_y, map2_x, map2_y)

	if img1_rect is not None and img2_rect is not None:
		kp1, kp2, matches = match_sparse(img1_rect, img2_rect, 3)
		pointcloud = initialize_sparse_pointcloud(img1_rect, img2_rect, kp1, kp2, matches, P1, P2)
		return pointcloud

class PointcloudTests(unittest.TestCase):

	def test_rectification(self):
		img1_rect, img2_rect, P1, P2, Q = None, None, None, None, None
		test_success = True
		try:
			if camera_matrix is not None and cam1_ext is not None and cam2_ext is not None:
				map1_x, map1_y, map2_x, map2_y, P1, P2, Q = rectification_map(img1, img2, camera_matrix, dist_coeffs, cam1_ext, cam2_ext)
				img1_rect, img2_rect = rectify_images(img1, img2, map1_x, map1_y, map2_x, map2_y)
			else:
				print('camera_matrix, dist_coeffs, cam1_ext and cam2_ext are not all set')
				test_success = False
		except Exception as e:
			print('Rectification failed:', e)
			test_success = False
		self.assertTrue(test_success)

		#Check sizes of the two rectified images are the same
		self.assertEqual(img1_rect.shape[0], img2_rect.shape[0])
		self.assertEqual(img1_rect.shape[1], img2_rect.shape[1])

		#Check that the images have some content
		runs = 20
		countblack = 0
		for i in range(runs):
			rand_row = random.randrange(0, img1_rect.shape[0])
			rand_col = random.randrange(0, img1_rect.shape[1])
			if(img1_rect[rand_row, rand_col][0] == 0 and img1_rect[rand_row, rand_col][1] == 0 and img1_rect[rand_row, rand_col][2] == 0):
				countblack += 1

			if(img2_rect[rand_row, rand_col][0] == 0 and img2_rect[rand_row, rand_col][1] == 0 and img2_rect[rand_row, rand_col][2] == 0):
				countblack += 1
		
		self.assertGreater(runs * 1.5, countblack)

	def test_sparse_pointcloud(self):
		img1_rect, img2_rect, P1, P2, Q = None, None, None, None, None
		test_success = True
		try:
			if camera_matrix is not None and cam1_ext is not None and cam2_ext is not None:
				map1_x, map1_y, map2_x, map2_y, P1, P2, Q = rectification_map(img1, img2, camera_matrix, dist_coeffs, cam1_ext, cam2_ext)
				img1_rect, img2_rect = rectify_images(img1, img2, map1_x, map1_y, map2_x, map2_y)
			else:
				print('camera_matrix, dist_coeffs, cam1_ext and cam2_ext are not all set')
				test_success = False
		except Exception as e:
			print('Rectification failed:', e)
			test_success = False
		self.assertTrue(test_success)

		if img1_rect is not None and img2_rect is not None:
			kp1, kp2, matches = match_sparse(img1_rect, img2_rect, 1)
			pointcloud = initialize_sparse_pointcloud(img1_rect, img2_rect, kp1, kp2, matches, P1, P2)

			#check that there are matches
			self.assertGreater(len(matches), 0)

			#check that there are points in the pointcloud
			self.assertGreater(len(pointcloud), 0)

			#check that colors vary across the pointcloud
			runs = 20
			storedcolor = None
			multicolor = False
			for i in range(runs):
				rand_idx = random.randrange(0, len(pointcloud))
				if(storedcolor is None):
					storedcolor = pointcloud[rand_idx]
				else:
					if((pointcloud[rand_idx] != storedcolor).any()):
						multicolor = True
			
			self.assertTrue(multicolor)




if __name__ == '__main__':
	#unittest.main() #to run unit tests

	#test fps
	start_time = time.time()
	frame_count = 0
	map1_x, map1_y, map2_x, map2_y, P1, P2, Q = None, None, None, None, None, None, None

	# only run automatic rectification when intrinsics/extrinsics are provided
	try:
		if camera_matrix is not None and cam1_ext is not None and cam2_ext is not None:
			map1_x, map1_y, map2_x, map2_y, P1, P2, Q = rectification_map(img1, img2, camera_matrix, dist_coeffs, cam1_ext, cam2_ext)
		else:
			print('camera_matrix, dist_coeffs, cam1_ext and cam2_ext are not all set; skipping rectification')
	except Exception as e:
		print('Rectification failed:', e)
	
	while(True):

		img1_rect, img2_rect = rectify_images(img1, img2, map1_x, map1_y, map2_x, map2_y)

		
		if img1_rect is not None and img2_rect is not None:
			#disparity = disparity_WLS(img1_rect, img2_rect, method=1)
			# plt.imshow(disparity,'gray', vmin=0, vmax=disparity.max())
			# plt.show()
			kp1, kp2, matches = match_sparse(img1_rect, img2_rect, 3)
			pointcloud = initialize_sparse_pointcloud(img1_rect, img2_rect, kp1, kp2, matches, P1, P2)
			# pointcloud = initialize_dense_pointcloud(img1_rect, img2_rect, disparity, Q)
			# for i in range(len(pointcloud)):
			# 	print('Point %d: (%.2f, %.2f, %.2f) Color: (%d, %d, %d)' % (i, pointcloud[i][0], pointcloud[i][1], pointcloud[i][2], pointcloud[i][3], pointcloud[i][4], pointcloud[i][5]))
			# img_matches = draw_matches(img1_rect, img2_rect, kp1, kp2, matches)
			# plt.imshow(img_matches)
			# plt.show()
		
		frame_count += 1

		if frame_count % 200 == 0:
			elapsed = time.time() - start_time
			fps = 200.0 / elapsed
			print(f"avg fps: {fps:.2f}")

			start_time = time.time()

    


