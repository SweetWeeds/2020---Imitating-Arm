import cv2
#from src.hand_tracker import HandTracker
from src.multi_hand_tracker import *
#from src.multi_hand_tracker import is_right_hand
import numpy as np

WINDOW = "Hand Tracking"
PALM_MODEL_PATH = "models/palm_detection_without_custom_op.tflite"
#LANDMARK_MODEL_PATH = "models/hand_landmark.tflite"
LANDMARK_MODEL_PATH = "models/hand_landmark_3d.tflite"
ANCHORS_PATH = "models/anchors.csv"

POINT_COLOR = (0, 255, 0)
CONNECTION_COLOR = (255, 0, 0)
THICKNESS = 2

cv2.namedWindow(WINDOW)
capture = cv2.VideoCapture(0)

if capture.isOpened():
    hasFrame, frame = capture.read()
else:
    hasFrame = False

#        8   12  16  20
#        |   |   |   |
#        7   11  15  19
#    4   |   |   |   |
#    |   6   10  14  18
#    3   |   |   |   |
#    |   5---9---13--17
#    2    \         /
#     \    \       /
#      1    \     /
#       \    \   /
#        ------0-
connections = [
    (0, 1), (1, 2), (2, 3), (3, 4),
    (5, 6), (6, 7), (7, 8),
    (9, 10), (10, 11), (11, 12),
    (13, 14), (14, 15), (15, 16),
    (17, 18), (18, 19), (19, 20),
    (0, 5), (5, 9), (9, 13), (13, 17), (0, 17)
]

angle_pos = [
    1, 2, 3,
    5, 6, 7,
    9, 10, 11,
    13, 14, 15,
    17, 18, 19
]

angle = {'left':{}, 'right':{}}

detector = MultiHandTracker3D(
    PALM_MODEL_PATH,
    LANDMARK_MODEL_PATH,
    ANCHORS_PATH
)

while hasFrame:
    image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    hands, _ = detector(image)
    if hands is not None:
        for hand in hands:
            if hand is None:
                continue
            #print(hand)
            hand = np.array(hand)
            points = hand
            for point in points:
                x, y, z = point[:]
                cv2.circle(frame, (int(x), int(y)), THICKNESS * 2, POINT_COLOR, THICKNESS)
            for idx, connection in enumerate(connections):
                x0, y0, z0 = points[connection[0]]
                x1, y1, z1 = points[connection[1]]
                cv2.line(frame, (int(x0), int(y0)), (int(x1), int(y1)), CONNECTION_COLOR, THICKNESS)
            side = is_right_hand(points)
            if side == None:
                continue
            for pos in angle_pos:
                if side == True:
                    side = 'right'
                else:
                    side = 'left'
                angle[side].update({pos : calc_angle(pos, connection, points)})
            print(angle[side][6])
    cv2.imshow(WINDOW, frame)
    hasFrame, frame = capture.read()
    key = cv2.waitKey(1)
    if key == 27:
        break

capture.release()
cv2.destroyAllWindows()
