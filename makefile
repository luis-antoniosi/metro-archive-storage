CC	:=	gcc
CFLAGS	:=	-std=c11 -Wall -Wextra -Werror -O3 -march=native -MMD
TARGET	:=	main
SRCS	:=	$(wildcard *.c)
OBJS	:=	$(SRCS:.c=.o)
DEPS	:=	$(OBJS:.o=.d)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -o $< -o $@

-include $(DEPS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET) $(DEPS)