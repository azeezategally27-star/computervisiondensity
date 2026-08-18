FROM ubuntu:22.04
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential pkg-config libopencv-dev wget unzip libjson-c-dev
# ONNX Runtime optional: install manually and set ONNXRUNTIME_DIR when building
COPY . /app
WORKDIR /app
RUN make || true
ENTRYPOINT ["./bin/crowd_monitor"]
