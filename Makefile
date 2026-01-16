CC      = gcc
CFLAGS  = -std=c18 -Wall -Wextra -O2 -g 
LDLIBS = /usr/lib/libraylib.a -lm -lpthread -ldl -lrt -lX11

TARGET = bmicalc

.PHONY: all clean reformat run vrun

all: $(TARGET)

%: %.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS) 

clean:
	rm -f $(TARGET)

reformat:
	clang-format --style=Chromium -i *.c

run:
	./$(TARGET)

