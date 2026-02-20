CC 		= gcc
CFLAGS 	= -Wall -Wextra -Werror

SRC = src/main.c src/cpu.c
OBJ = $(SRC:src/%.c=build/%.o)

TARGET = build/exodus

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

build/%.o: src/%.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build

.PHONY: all clean
