CC = gcc
CFLAGS = -O2 -Wall -std=c99 `pkg-config --cflags opencv`
LDFLAGS = `pkg-config --libs opencv`
SRCS = src/main.c src/detector_fake.c src/tracker.c src/kde.c src/roi.c src/utils.c src/synthetic_generator.c src/detector_onnx.c
OBJS = $(SRCS:.c=.o)
BIN_DIR = bin
TARGET = $(BIN_DIR)/crowd_monitor
SYN = $(BIN_DIR)/synthetic_generator

# If ONNX Runtime available, user should set ONNXRUNTIME_DIR
ifdef ONNXRUNTIME_DIR
  CFLAGS += -I$(ONNXRUNTIME_DIR)/include
  LDFLAGS += -L$(ONNXRUNTIME_DIR)/lib -lonnxruntime
endif

all: dirs $(TARGET) $(SYN)

dirs:
	mkdir -p $(BIN_DIR)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(filter-out src/synthetic_generator.o,$(OBJS))
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(SYN): src/synthetic_generator.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(BIN_DIR) *.o src/*.o

.PHONY: all clean
