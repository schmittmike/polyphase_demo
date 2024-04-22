TARGET=polyphase_decimate
OBJS=polyphase_decimate.o wav.o
CARGS=-ggdb -Wall -Wextra
LINKS=-lpthread
CC+= -g 

all: $(TARGET)

$(TARGET): $(OBJS)
	gcc $(CARGS) -o $(TARGET) $(OBJS) $(LINKS)

clean:
	rm $(TARGET) *.o
