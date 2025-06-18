# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Overview

This repository contains both Linux and macOS implementations of virtual CAN (vcan) interfaces. It provides virtual Controller Area Network interfaces that allow CAN frames to be sent and received without requiring physical CAN hardware.

## Architecture

### Linux Implementation
**Core Module**: `linux_vcan.c` - Complete Linux kernel module implementation
- Virtual CAN network interface driver
- Supports standard CAN, CAN-FD, and CAN-XL frame formats  
- Optional echo mode for testing CAN core functionality
- Implements standard Linux network device operations

**Key Components**:
- `vcan_tx()` - Transmit function handling frame transmission and loopback
- `vcan_rx()` - Receive function for processing incoming frames
- `vcan_setup()` - Network device initialization and configuration
- Module parameter `echo` - Enables/disables driver-level echo for testing

### macOS Implementation
**Core Files**:
- `vcan.cpp` - IOKit-based kernel extension for macOS
- `vcan.h` - CAN frame structures compatible with Linux SocketCAN
- `Info.plist` - Kernel extension bundle configuration
- `Makefile` - Comprehensive build system with multiple targets

**Architecture**:
- `VirtualCANInterface` class extending IOService
- Support for up to 16 virtual CAN interfaces
- Compatible CAN frame formats (CAN, CAN-FD, CAN-XL)
- Optional echo mode matching Linux functionality
- Network statistics and BPF packet capture support

## Common Development Commands

### Building
```bash
# macOS kernel extension
make all                # Build the kernel extension
make debug             # Build with debug symbols
make validate          # Validate the kernel extension

# Linux module (traditional)
make -C /lib/modules/$(uname -r)/build M=$(PWD) modules
```

### Installation and Management
```bash
# macOS
make install           # Install system-wide (requires SIP disabled)
make load             # Load the kernel extension
make unload           # Unload the kernel extension
make status           # Check load status

# Interface management
make create-interface  # Create vcan0 interface
make remove-interface # Remove vcan0 interface
make show-interface   # Show interface information
```

## Development Context

Originally from Volkswagen Group Electronic Research team, distributed under dual BSD/GPL license. Both implementations create virtual CAN interfaces for:
- CAN protocol testing and development
- CAN application debugging without physical hardware
- CAN network simulation and validation

## Technical Details

**Common Features**:
- **Device Type**: Virtual CAN network interface
- **MTU Support**: CAN_MTU (16), CANFD_MTU (72), and CAN-XL variable MTU
- **Frame Formats**: Standard CAN, CAN-FD, and CAN-XL
- **Echo Mode**: Optional driver-level echo for testing
- **Compatibility**: SocketCAN-compatible frame structures

**Platform Differences**:
- Linux: Uses netdev subsystem and standard network device operations
- macOS: Uses IOKit framework with custom network interface implementation