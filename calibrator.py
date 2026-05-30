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
cap.set(cv.CAP_PROP_FOURCC, cv.VideoWriter_fourcc("M","J","P","G"))
cap.set(cv.CAP_PROP_FRAME_WIDTH, 1280)
cap.set(cv.CAP_PROP_FRAME_HEIGHT, 720)
width = cap.get(cv.CAP_PROP_FRAME_WIDTH)
height = cap.get(cv.CAP_PROP_FRAME_HEIGHT)
print(width, height)

cv.namedWindow("frame", cv.WINDOW_GUI_NORMAL)

all_obj_points = []
all_img_points = []

if not cap.isOpened():
    print("Open camera failed, exiting...")
    exit()

while True:
    # Capture frame-by-frame
    ret, frame = cap.read()
    if not ret:
        print("Read frame failed, exiting...")
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
        if len(obj_points) == 0 or len(img_points) == 0:
            continue
        all_obj_points.append(obj_points)
        all_img_points.append(img_points)
    if key == ord('x'):
        ret, cammat, distcoeffs, rv, tv = cv.calibrateCamera( all_obj_points, all_img_points, gray.shape[::-1], None, None )
        print(cammat,distcoeffs)

