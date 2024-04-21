TARGET=polyphase_decimate
OBJS=polyphase_decimate.o wav.o
CARGS=-Wall -Wextra
LINKS=-lpthread

all: $(TARGET)

$(TARGET): $(OBJS)
	gcc $(CARGS) -o $(TARGET) $(OBJS) $(LINKS)

clean:
	rm $(TARGET) *.o
