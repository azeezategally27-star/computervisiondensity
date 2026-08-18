#!/bin/bash
# capture_frames.sh - capture a short animation sequence using the app's capture mode
DIR=$(cd "$(dirname "$0")" && pwd)
FRAME_DIR="$DIR/frames"
mkdir -p "$FRAME_DIR"
# If the app is not running with the capture button, we can use gtk-recording (headless) but here we just advise
echo "Run the app and press the Capture button to write frames to $FRAME_DIR"
echo "Or use ffmpeg to combine frames into a video after capture:"
echo "ffmpeg -framerate 24 -i $FRAME_DIR/frame_%04d.png -c:v libx264 -pix_fmt yuv420p out.mp4"
