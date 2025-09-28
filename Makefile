CC = gcc

# Compiler flags (separate lines for clarity)
CFLAGS = -O2
CFLAGS += -Wall
CFLAGS += -Wextra

# Include and define flags
INCLUDES = -Iinclude
INCLUDES += -Ithird_party
DEFINES = -DUSE_STB

# Linker flags
LDFLAGS = -pthread
LDFLAGS += -lm

# Directories
SRCDIR = src
BUILDDIR = build
OBJDIR = $(BUILDDIR)/obj
INCLUDEDIR = include

# Target executable name
TARGET = imagemuggle
TARGET_PATH = $(BUILDDIR)/$(TARGET)

# Dynamically find source files
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Default target
all: $(TARGET_PATH)

# Link target executable
$(TARGET_PATH): $(OBJECTS) | $(BUILDDIR)
	$(CC) $^ -o $@ $(LDFLAGS)

# Compile source files to object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@

# Create build directory
$(BUILDDIR):
	mkdir -p $@

# Create object directory
$(OBJDIR):
	mkdir -p $@

# Clean build artifacts
clean:
	rm -rf $(BUILDDIR)

# Rebuild everything
rebuild: clean all

# Phony targets
.PHONY: all clean rebuild
