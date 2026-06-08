CC	:=	gcc
CFLAGS	:=	-Wall -Wextra -Werror -Wpedantic -march=native -MMD
TARGET	:=	main
SRCS	:=	$(wildcard *.c) $(wildcard */*.c)
HDRS	:=	$(wildcard *.h) $(wildcard */*.h)
OBJS	:=	$(SRCS:.c=.o)
DEPS	:=	$(OBJS:.o=.d)

VFLAGS	:=	--leak-check=full --show-leak-kinds=all --track-origins=yes -s

DIR		:=	$(notdir $(abspath .))
ZIPFILE	:=	$(DIR).zip

.PHONY: all debug run valgrind zip clean

all: CFLAGS += -O3
all: $(TARGET)

# Build for debugging (e.g.: valgrind or gdb, so no optimizations)
debug: CFLAGS += -g -O0
debug: clean $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

run: all
	./$(TARGET)	

valgrind: debug
	valgrind $(VFLAGS) ./$(TARGET)

zip: 
	zip $(ZIPFILE) $(SRCS) $(HDRS) makefile

clean:
	rm -f $(OBJS) $(TARGET) $(ZIPFILE) $(DEPS)