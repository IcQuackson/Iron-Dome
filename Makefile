SRC_DIR = ./src
INCLUDE_DIR = ./include
BIN_DIR = ./bin
CC = gcc
CFLAGS = -Wall -Wextra -O2 -I$(INCLUDE_DIR)
TARGET = iron_dome
RESOURCES = /home /etc

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean: kill
	rm -rf $(BIN_DIR) $(TARGET)

re: clean all

run: kill all
	sudo ./$(TARGET) $(RESOURCES)

kill:
	- sudo killall $(TARGET) 2> /dev/null

.PHONY: all clean re run
