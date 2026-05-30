#!/usr/bin/python3
import numpy as np
import cv2 as cv
import glob

# termination criteria
criteria = (cv.TERM_CRITERIA_EPS + cv.TERM_CRITERIA_MAX_ITER, 30, 0.001)

# marker dictionary
aruco_dict = cv.aruco.getPredefinedDictionary(cv.aruco.DICT_6X6_250)

# (Ch)Aruco boards
squares_x=10
squares_y=8

cboard = cv.aruco.CharucoBoard(
    size=(squares_x, squares_y),
    squareLength=1.0,
    markerLength=0.75,
    dictionary=aruco_dict,
)

cboard.setLegacyPattern(True)

gboard = cv.aruco.GridBoard(
    size=(4,5),
    markerLength=5,
    markerSeparation=1,
    dictionary=aruco_dict
)

board = cboard

charuco_detector = cv.aruco.CharucoDetector(cboard)

cap = cv.VideoCapture(0,cv.CAP_V4L2)
cap.set(cv.CAP_PROP_FOURCC, cv.VideoWriter_fourcc(*"MJPG"))
cap.set(cv.CAP_PROP_FRAME_WIDTH, 1280)
cap.set(cv.CAP_PROP_FRAME_HEIGHT, 720)
width = cap.get(cv.CAP_PROP_FRAME_WIDTH)
height = cap.get(cv.CAP_PROP_FRAME_HEIGHT)
print(width, height)

cv.namedWindow('frame', cv.WINDOW_GUI_NORMAL)

all_obj_points = []
all_img_points = []

if not cap.isOpened():
    print("Cannot open camera")
    exit()

while True:
    # Capture frame-by-frame
    ret, frame = cap.read()

    # if frame is read correctly ret is True
    if not ret:
        print("Can't receive frame (stream end?). Exiting ...")
        break

    # Our operations on the frame come here
    gray = cv.cvtColor(frame, cv.COLOR_BGR2GRAY)

    charuco_corners, charuco_ids, marker_corners, marker_ids = charuco_detector.detectBoard(gray)

    obj_points = []
    img_points = []

    if marker_ids is not None and len(marker_ids) >= 0:
        obj_points, img_points = gboard.matchImagePoints( marker_corners, marker_ids ) # or charuco_{corners,ids}

    cv.aruco.drawDetectedMarkers(gray, marker_corners, marker_ids)

    if charuco_corners is not None and len(charuco_corners) >= 0:
        cv.aruco.drawDetectedCornersCharuco( gray, charuco_corners, charuco_ids, cornerColor=(0, 255, 0) )

    # Display the resulting frame
    cv.imshow('frame', gray)
    key = cv.waitKey(10)
    if key == ord('q'):
        break
    if key == ord('s'):
        all_obj_points.append(obj_points)
        all_img_points.append(img_points)
    if key == ord('x'):
        ret, cammat, distcoeffs, rv, tv = cv.calibrateCamera( all_obj_points, all_img_points, gray.shape[::-1], None, None )
        print(cammat,distcoeffs)

exit

# prepare object points, like (0,0,0), (1,0,0), (2,0,0) ....,(6,5,0)
objp = np.zeros((6*7,3), np.float32)
objp[:,:2] = np.mgrid[0:7,0:6].T.reshape(-1,2)
 
# Arrays to store object points and image points from all the images.
objpoints = [] # 3d point in real world space
imgpoints = [] # 2d points in image plane.
 
images = glob.glob('*.jpg')
 
for fname in images:
    img = cv.imread(fname)
    gray = cv.cvtColor(img, cv.COLOR_BGR2GRAY)
 
    # Find the chess board corners
    ret, corners = cv.findChessboardCorners(gray, (7,6), None)
 
    # If found, add object points, image points (after refining them)
    if ret == True:
        objpoints.append(objp)
 
        corners2 = cv.cornerSubPix(gray,corners, (11,11), (-1,-1), criteria)
        imgpoints.append(corners2)
 
        # Draw and display the corners
        cv.drawChessboardCorners(img, (7,6), corners2, ret)
        cv.imshow('img', img)
        cv.waitKey(500)
 
cv.destroyAllWindows()
