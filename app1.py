from flask import Flask, render_template, Response, jsonify
import cv2
import torch
import requests
import numpy as np
import time
import threading

app = Flask(__name__)

# YOLOv5 Model Load
model = torch.hub.load('ultralytics/yolov5', 'custom', path='yolov5s.pt')
model.conf = 0.25  # Confidence Threshold

# ESP32-CAM Image URL
ESP32_CAM_URL = "http://192.168.1.31/capture"

detection_result = {"status": "No vehicle detected"}


def get_image():
    """Fetch image from ESP32-CAM"""
    try:
        response = requests.get(ESP32_CAM_URL, timeout=3)#changed to 3 before 5
        img_array = np.array(bytearray(response.content), dtype=np.uint8)
        img = cv2.imdecode(img_array, -1)
        return img
    except:
        print("Failed to fetch image")
        return None


def detect_vehicle():
    """Detect vehicles and send traffic light commands"""
    global detection_result
    while True:
        img = get_image()
        if img is None:
            time.sleep(1)
            continue

        results = model(img)
        detected_objects = results.pandas().xyxy[0]
        frame_width = img.shape[1]
        threshold = frame_width // 2  # Set center of frame as threshold

        right_min = threshold - 240
        right_max = threshold - 1
        left_min = threshold + 1
        left_max = threshold + 240

        left_detected = any(left_min <= obj['xmin'] <= left_max and obj['name'] in ['car', 'truck', 'bus', 'motorcycle'] for _, obj in detected_objects.iterrows())
        right_detected = any(right_min <= obj['xmax'] <= right_max and obj['name'] in ['car', 'truck', 'bus', 'motorcycle'] for _, obj in detected_objects.iterrows())

        if left_detected and right_detected:
            requests.get("http://192.168.1.31/both")  # Red on both sides
            detection_result = {"status": "Vehicles detected on opposite lane, Go slow with caution."}
        elif left_detected:
            requests.get("http://192.168.1.31/left")  # Red on left side
            detection_result = {"status": "Right lane is clear, you're free to go."}
        elif right_detected:
            requests.get("http://192.168.1.31/right")  # Red on right side
            detection_result = {"status": "Left lane is clear, you're free to go"}
        else:
            requests.get("http://192.168.1.31/none")  # Green on both sides
            detection_result = {"status": " "}

        time.sleep(1)  # Update every second

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/video_feed')
def video_feed():
    """Stream images from ESP32-CAM"""
    def generate():
        while True:
            img = get_image()
            if img is not None:
                _, buffer = cv2.imencode('.jpg', img)
                frame = buffer.tobytes()
                yield (b'--frame\r\n' b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
            time.sleep(1)  # Update every second

    return Response(generate(), mimetype='multipart/x-mixed-replace; boundary=frame')


@app.route('/get_status')
def get_status():
    """Return the latest traffic status"""
    return jsonify(detection_result)


if __name__ == '__main__':
    threading.Thread(target=detect_vehicle, daemon=True).start()
    app.run(host='0.0.0.0', port=5000, debug=True)
