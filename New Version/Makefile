# ─────────────────────────────────────────────────────────────────────
# Makefile — Smart Backup Utility
# Auto-discovers all .c files under src/; compiles with GTK+ 3.0
# ─────────────────────────────────────────────────────────────────────

CC      = gcc
CFLAGS  = -Wall -Wextra -Iinclude $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS = $(shell pkg-config --libs   gtk+-3.0)

TARGET  = build/smart_backup.exe

# Auto-discover every .c file under src/
SRCS    = $(shell find src -name "*.c")

# Mirror src/ structure inside build/ for object files
OBJS    = $(patsubst src/%.c, build/obj/%.o, $(SRCS))

# ── Default target ────────────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
	@echo "Build successful: $(TARGET)"

# ── Compile rule ─────────────────────────────────────────────────────
build/obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ── Clean ─────────────────────────────────────────────────────────────
clean:
	rm -rf build/obj build/*.exe
	@echo "Clean complete"

.PHONY: all clean
