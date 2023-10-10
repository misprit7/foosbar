#!/usr/bin/python3

import numpy as np
import cv2 as cv
import glob

# termination criteria
criteria = (cv.TERM_CRITERIA_EPS + cv.TERM_CRITERIA_MAX_ITER, 30, 0.001)

w = 7
h = 7

# prepare object points, like (0,0,0), (1,0,0), (2,0,0) ....,(6,5,0)
objp = np.zeros((w*h,3), np.float32)
objp[:,:2] = np.mgrid[0:w,0:h].T.reshape(-1,2)
# objp = np.array([
#     [0,0,0],
#     [0,23.6,0],
#     [0,93.1-52.2,0],
#     [103-52.2,3.7,-11.7],
#     [103-52.2,3.7+68-52.2,-11.7],
#     [128.1-52.2,3.7,-11.7],
#     [128.1-52.2,3.7+(68-52.2)/2,-11.7],
#     [128.1-52.2,3.7+68-52.2,-11.7],
#     [139.3-52.2,0,0],
#     [139.3-52.2,23.6,0],
# ])

# Arrays to store object points and image points from all the images.
objpoints = [] # 3d point in real world space
imgpoints = [] # 2d points in image plane.
images = sorted(glob.glob('../assets/calibration/*.jpg'))

for fname in images:
    img = cv.imread(fname)
    gray = cv.cvtColor(img, cv.COLOR_BGR2GRAY)

    # Find the chess board corners
    ret, corners = cv.findChessboardCorners(gray, (w,h), None)

    # If found, add object points, image points (after refining them)
    if ret == True:
        objpoints.append(objp)

    # corners2 = cv.cornerSubPix(gray,corners, (11,11), (-1,-1), criteria)
    imgpoints.append(corners)
    # Draw and display the corners
    cv.drawChessboardCorners(img, (w,h), corners, ret)
    # cv.imshow('img', img)
    # cv.waitKey(100000)

img = cv.imread('../assets/calibration/01.jpg')
ih, iw = img.shape[:2]
ret, mtx, dist, rvecs, tvecs = cv.calibrateCamera(objpoints, imgpoints, (iw,ih), None, None)

newcameramtx, roi = cv.getOptimalNewCameraMatrix(mtx, dist, (iw,ih), 1, (iw,ih))
dst = cv.undistort(img, mtx, dist, None, newcameramtx)

# crop the image
x, y, w, h = roi
dst = dst[y:y+h, x:x+w]
cv.imshow('dst', dst)
cv.imshow('img', img)
cv.waitKey(100000)

cv.destroyAllWindows()
