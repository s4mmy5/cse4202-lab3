##
# Lab 3: Multiplexed I/O Server
#
# Source using claude.ai
#
# @file
# @version 0.1
CC      = gcc
CFLAGS  = -Wall -Wextra -O0 -g -I$(INC_DIR)
LDFLAGS =

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
INC_DIR = include

SERVER_SRC = $(SRC_DIR)/server.c $(SRC_DIR)/common.c
CLIENT_SRC = $(SRC_DIR)/client.c $(SRC_DIR)/common.c

SERVER_OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SERVER_SRC))
CLIENT_OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(CLIENT_SRC))

SERVER_BIN = $(BIN_DIR)/server
CLIENT_BIN = $(BIN_DIR)/client

.PHONY: all clean dirs

all: dirs $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(CLIENT_BIN): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

dirs:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# end
