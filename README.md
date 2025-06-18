# Virtual CAN Driver for macOS

A kernel extension that provides virtual CAN (Controller Area Network) interfaces for macOS, similar to the Linux vcan driver.

## Features

- Virtual CAN interfaces compatible with Linux SocketCAN
- Support for CAN, CAN-FD, and CAN-XL frame formats
- Optional echo mode for testing
- Multiple virtual interfaces (up to 16)
- BPF packet capture support
- Network statistics tracking

## Requirements

- macOS 10.15 (Catalina) or later
- Xcode command line tools
- System Integrity Protection (SIP) may need to be disabled for installation

## Building

```bash
make all
```

## Installation

**Warning:** Loading unsigned kernel extensions requires disabling System Integrity Protection (SIP) on modern macOS versions. This significantly reduces system security and is not recommended for production use.

### Prerequisites

1. **Disable SIP** (required for unsigned kernel extensions):
   - Boot into Recovery Mode (Command+R during startup)
   - Open Terminal and run: `csrutil disable`
   - Reboot normally
   - Verify SIP is disabled: `csrutil status`

2. **Set proper ownership** (kernel extensions must be owned by root):
   ```bash
   sudo chown -R root:wheel vcan.kext
   ```

### Loading the Extension

**Option 1: Direct loading (development/testing)**
```bash
sudo kextload vcan.kext
```

**Option 2: System installation (permanent)**
```bash
make install  # Installs to /System/Library/Extensions/
make load     # Loads the installed extension
```

### Current Status

The kernel extension builds successfully but requires either:
- **SIP disabled** for unsigned extensions, OR
- **Valid code signature** from Apple Developer Program

## Usage

### Creating Virtual CAN Interfaces

```bash
# Create and bring up a virtual CAN interface
make create-interface

# Or manually:
sudo ifconfig vcan0 create
sudo ifconfig vcan0 up
```

### Checking Status

```bash
# Check if kernel extension is loaded
make status

# Show interface information
make show-interface
```

### Removing Interfaces

```bash
# Remove virtual CAN interface
make remove-interface

# Or manually:
sudo ifconfig vcan0 down
sudo ifconfig vcan0 destroy
```

## Programming Interface

The driver provides a network interface that can be used with standard BSD socket APIs:

```c
#include "vcan.h"
#include <sys/socket.h>

// Create CAN socket
int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);

// Bind to interface
struct sockaddr_can addr;
addr.can_family = AF_CAN;
addr.can_ifindex = if_nametoindex("vcan0");
bind(sock, (struct sockaddr*)&addr, sizeof(addr));

// Send CAN frame
struct can_frame frame;
frame.can_id = 0x123;
frame.can_dlc = 8;
memcpy(frame.data, "DEADBEEF", 8);
write(sock, &frame, sizeof(frame));

// Receive CAN frame
read(sock, &frame, sizeof(frame));
```

## Debugging

Build with debug symbols:
```bash
make debug
```

Check kernel logs:
```bash
sudo dmesg | grep vcan
```

## Uninstalling

```bash
# Unload and remove kernel extension
make unload
make uninstall
```

## Troubleshooting

### Common Issues

1. **"Operation not permitted" errors**
   - Ensure SIP is disabled
   - Run commands with sudo

2. **Kernel extension won't load**
   - Check that the extension is properly signed
   - Verify compatibility with your macOS version

3. **Interface creation fails**
   - Ensure the kernel extension is loaded
   - Check that you have root privileges

### Validation

Validate the kernel extension before loading:
```bash
make validate
```

## Architecture

The driver consists of:

- **vcan.cpp**: Main kernel extension implementing the virtual CAN interface
- **vcan.h**: CAN frame structures and constants compatible with Linux SocketCAN
- **Info.plist**: Kernel extension bundle information
- **Makefile**: Build system with comprehensive targets

## License

This code is based on the Linux vcan driver from Volkswagen Group Electronic Research and is distributed under the same dual BSD/GPL license.

## Limitations

- Kernel extensions are deprecated in favor of DriverKit on newer macOS versions
- Requires SIP to be disabled for installation
- May not work with all macOS security configurations
- Limited to 16 virtual interfaces maximum
