# Makefile for macOS Virtual CAN Driver
# Requires Xcode command line tools

KEXT_NAME = vcan
KEXT_BUNDLE = $(KEXT_NAME).kext
KEXT_VERSION = 1.0.0

# Build settings
SDKROOT = $(shell xcrun --show-sdk-path)
DEPLOYMENT_TARGET = 10.15
ARCH = $(shell uname -m)

# Compiler and linker flags
CXX = xcrun clang++
CXXFLAGS = -arch $(ARCH) \
           -isysroot $(SDKROOT) \
           -mmacosx-version-min=$(DEPLOYMENT_TARGET) \
           -fno-builtin \
           -fno-stack-protector \
           -fno-common \
           -fno-exceptions \
           -fno-rtti \
           -mkernel \
           -D__KERNEL__ \
           -DKERNEL \
           -DKERNEL_PRIVATE \
           -DDRIVER_PRIVATE \
           -DAPPLE \
           -DNeXT \
           -I$(SDKROOT)/System/Library/Frameworks/Kernel.framework/Headers \
           -I$(SDKROOT)/System/Library/Frameworks/IOKit.framework/Headers \
           -Wall -Wno-unused-parameter

LDFLAGS = -arch $(ARCH) \
          -isysroot $(SDKROOT) \
          -mmacosx-version-min=$(DEPLOYMENT_TARGET) \
          -nostdlib \
          -r \
          -lkmod \
          -lkmodc++ \
          -lcc_kext

# Source files
SOURCES = vcan_simple.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(KEXT_BUNDLE)

# Build the kernel extension bundle
$(KEXT_BUNDLE): $(OBJECTS) Info.plist
	@echo "Creating kernel extension bundle..."
	@mkdir -p $(KEXT_BUNDLE)/Contents/MacOS
	@mkdir -p $(KEXT_BUNDLE)/Contents/Resources
	@cp Info.plist $(KEXT_BUNDLE)/Contents/
	@cp vcan.h $(KEXT_BUNDLE)/Contents/Resources/
	$(CXX) $(LDFLAGS) -o $(KEXT_BUNDLE)/Contents/MacOS/$(KEXT_NAME) $(OBJECTS)
	@echo "Kernel extension built successfully"

# Compile source files
%.o: %.cpp vcan.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(OBJECTS) $(KEXT_BUNDLE)

# Install the kernel extension (requires sudo)
install: $(KEXT_BUNDLE)
	@echo "Installing kernel extension (requires root privileges)..."
	sudo cp -R $(KEXT_BUNDLE) /System/Library/Extensions/
	sudo chown -R root:wheel /System/Library/Extensions/$(KEXT_BUNDLE)
	sudo chmod -R 755 /System/Library/Extensions/$(KEXT_BUNDLE)
	sudo kextcache -system-prelinked-kernel
	@echo "Installation complete. Reboot may be required."

# Uninstall the kernel extension
uninstall:
	@echo "Uninstalling kernel extension (requires root privileges)..."
	sudo kextunload -b com.opensource.vcan || true
	sudo rm -rf /System/Library/Extensions/$(KEXT_BUNDLE)
	sudo kextcache -system-prelinked-kernel
	@echo "Uninstall complete."

# Load the kernel extension
load: $(KEXT_BUNDLE)
	@echo "Loading kernel extension (requires root privileges)..."
	sudo chown -R root:wheel $(KEXT_BUNDLE)
	sudo kextload $(KEXT_BUNDLE)
	sudo chown -R $(shell whoami):staff $(KEXT_BUNDLE)

# Unload the kernel extension
unload:
	@echo "Unloading kernel extension..."
	sudo kextunload -b com.opensource.vcan

# Check if kernel extension is loaded
status:
	@echo "Checking kernel extension status..."
	@kextstat | grep vcan || echo "vcan kernel extension not loaded"

# Create virtual CAN interface
create-interface:
	@echo "Creating virtual CAN interface..."
	sudo ifconfig vcan0 create || echo "Interface creation failed or already exists"
	sudo ifconfig vcan0 up

# Remove virtual CAN interface
remove-interface:
	@echo "Removing virtual CAN interface..."
	sudo ifconfig vcan0 down || true
	sudo ifconfig vcan0 destroy || true

# Show interface information
show-interface:
	@echo "Virtual CAN interface information:"
	@ifconfig vcan0 || echo "vcan0 interface not found"

# Development helpers
debug: CXXFLAGS += -DDEBUG -g
debug: $(KEXT_BUNDLE)

# Code signing (for distribution)
sign: $(KEXT_BUNDLE)
	@echo "Code signing kernel extension..."
	sudo codesign -f -s "Developer ID Application" $(KEXT_BUNDLE)

# Validate the kernel extension
validate: $(KEXT_BUNDLE)
	@echo "Validating kernel extension..."
	kextlibs $(KEXT_BUNDLE)
	kextutil -nt $(KEXT_BUNDLE)

# Help target
help:
	@echo "Available targets:"
	@echo "  all              - Build the kernel extension"
	@echo "  clean            - Remove build artifacts"
	@echo "  install          - Install the kernel extension system-wide"
	@echo "  uninstall        - Remove the kernel extension from system"
	@echo "  load             - Load the kernel extension"
	@echo "  unload           - Unload the kernel extension"
	@echo "  status           - Check if kernel extension is loaded"
	@echo "  create-interface - Create vcan0 network interface"
	@echo "  remove-interface - Remove vcan0 network interface"
	@echo "  show-interface   - Show vcan0 interface information"
	@echo "  debug            - Build with debug symbols"
	@echo "  sign             - Code sign the kernel extension"
	@echo "  validate         - Validate the kernel extension"
	@echo "  help             - Show this help message"

.PHONY: all clean install uninstall load unload status create-interface remove-interface show-interface debug sign validate help