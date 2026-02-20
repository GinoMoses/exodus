CC 		= gcc
CFLAGS 	= -Wall -Wextra -Werror
LDFLAGS	 = -lncurses

SRC = src/main.c src/cpu.c src/ui.c
OBJ = $(SRC:src/%.c=build/%.o)

TARGET = build/exodus

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

build/%.o: src/%.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build

.PHONY: all clean
