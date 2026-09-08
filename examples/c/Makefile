CC = clang
TARGET_TRIPLE = x86_64-w64-windows-gnu

# Rust target directory
RUST_TARGET ?= x86_64-pc-windows-gnu
RUST_BIN_DIR = ../../target/$(RUST_TARGET)/release

CFLAGS = --target=$(TARGET_TRIPLE) -O2 -Wall -I../../include
LDFLAGS = --target=$(TARGET_TRIPLE) -L. -lslint_cflat
TARGET = demo.exe
SRCS = main.c

.PHONY: all copy_libs run clean

all: copy_libs $(TARGET)

# Copy the built DLL and import library from the Rust target directory
copy_libs:
	@echo "Copying runtime and import libraries from $(RUST_BIN_DIR)..."
	cp $(RUST_BIN_DIR)/slint_cflat.dll .
	cp $(RUST_BIN_DIR)/libslint_cflat.dll.a .

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET) slint_cflat.dll libslint_cflat.dll.a