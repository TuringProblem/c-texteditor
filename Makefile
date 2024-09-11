CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99
EXEC = editor
SRC_DIR = src/com/c/text-editor
MAIN = $(SRC_DIR)/main.c

$(EXEC): $(MAIN)
	$(CC) $(CFLAGS) $< -o $@

.PHONY: clean run

clean:
	rm -f $(EXEC)

run: $(EXEC)
	./$(EXEC)
