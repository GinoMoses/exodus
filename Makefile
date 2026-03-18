CC 		= gcc
CFLAGS 	= -Wall -Wextra -Werror -pthread -std=c99 -D_DEFAULT_SOURCE
LDFLAGS	 = -lncursesw -lm -pthread

SRC = src/main.c src/cpu.c src/memory.c src/network.c src/system.c src/process.c src/process_compare.c src/ui.c src/input.c
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
