# Crowd Monitor (pure C) — YOLOv8 ONNX detector added

This repository implements real-time crowd density monitoring in pure C.

Overview
--------
- Input: webcam, RTSP URL, or synthetic MP4 fallback
- Detection: optional YOLOv8 ONNX via ONNX Runtime C API, OR fallback motion/contour detector (no model required)
- Tracking: IoU/centroid-based tracker with persistent IDs
- Density heatmap: Gaussian kernel density estimation rendered as an overlay
- ROI alerts: polygon ROI in config.json; trigger visual warning when count in ROI > threshold
- Telemetry overlay: FPS, total count, ROI count, zone density

Quick demo (works without any model)
1. Install dependencies (see below).
2. Build:
   make
3. Generate synthetic video (creates sample.mp4):
   ./bin/synthetic_generator sample.mp4 640 360 60
4. Run the monitor using the synthetic video and fallback detector:
   ./bin/crowd_monitor --source sample.mp4 --use-onnx 0 --config config.json

To run with webcam:
   ./bin/crowd_monitor --source 0 --use-onnx 0 --config config.json

To enable ONNX YOLOv8 (optional)
- Export YOLOv8 (Ultralytics) to ONNX with person class (COCO class 0). Example:
  python -m pip install ultralytics
  python - <<EOF
  from ultralytics import YOLO
  model = YOLO('yolov8n.pt')
  model.export(format='onnx', opset=12, simplify=True, dynamic=False)
  EOF
- Put the exported `yolov8.onnx` into the repo root or point --model to its path.
- Install ONNX Runtime (CPU) and set ONNXRUNTIME_DIR to the install prefix containing include/ and lib/
- Run (CPU-only):
  export ONNXRUNTIME_DIR=/path/to/onnxruntime
  make
  ./bin/crowd_monitor --source sample.mp4 --use-onnx 1 --model yolov8.onnx --config config.json

Notes
-----
- The ONNX detector implemented here expects a standard Ultralytics YOLOv8 export with shape (1,N,85) output (xywh + conf + class scores). If your model differs, you may need to adjust detector_onnx.c postprocessing parameters (input size, output parsing).
- ONNX Runtime GPU/CUDA builds are supported if you install the GPU runtime and update LDFLAGS accordingly; the Makefile supports linking via ONNXRUNTIME_DIR but GPU runtimes often use different libs—see ONNX Runtime docs.

License: MIT
