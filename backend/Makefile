CC = gcc
CFLAGS = -Wall -I./include -DMG_ENABLE_LINES=1 -DMG_ENABLE_HTTP=1 -DMG_ENABLE_IPV6=1 `pkg-config --cflags openssl`
LIBS = `pkg-config --libs openssl` -lpthread

SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include

# Add mongoose.c to sources
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/server.c $(SRC_DIR)/mongoose.c
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

TARGET = server

.PHONY: all clean directories

all: directories $(TARGET)

directories:
	@mkdir -p $(OBJ_DIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
