CC = gcc
CFLAGS = -Wall -g
LIBS = -lavformat -lavcodec -lavutil

TARGET = demuxer
SRC = mp4demux.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET)