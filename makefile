CC	:=	gcc
CFLAGS	:=	-Wall -Wextra -Og -march=native -MMD -g
TARGET	:=	main
SRCS	:=	$(wildcard *.c)
HDRS	:=	$(wildcard *.h)
OBJS	:=	$(SRCS:.c=.o)
DEPS	:=	$(OBJS:.o=.d)

DIR		:=	$(notdir $(abspath .))
ZIPFILE	:=	$(DIR).zip

.PHONY: all clean run zip

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET) $(ZIPFILE) $(DEPS)

zip: 
	zip $(ZIPFILE) $(SRCS) $(HDRS) makefile