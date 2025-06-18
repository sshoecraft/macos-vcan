#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <net/if.h>
#include <net/if_var.h>
#include <net/if_types.h>
#include <net/kpi_interface.h>
#include <net/bpf.h>
#include <sys/sockio.h>
#include "vcan.h"

#define VCAN_DRIVER_VERSION "1.0.0"
#define VCAN_MAX_INTERFACES 16

class VirtualCANInterface : public IOService
{
    OSDeclareDefaultStructors(VirtualCANInterface)

public:
    virtual bool init(OSDictionary *properties = 0) override;
    virtual void free() override;
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
    
private:
    ifnet_t interface;
    bool echo_mode;
    UInt32 interface_index;
    
    static errno_t vcan_output(ifnet_t interface, mbuf_t packet);
    static errno_t vcan_ioctl(ifnet_t interface, u_long command, void *data);
    
    errno_t output_handler(mbuf_t packet);
    errno_t ioctl_handler(u_long command, void *data);
    void input_handler(mbuf_t packet);
    
    bool create_interface();
    void destroy_interface();
    
    static VirtualCANInterface *interfaces[VCAN_MAX_INTERFACES];
    static UInt32 interface_count;
    static IOLock *interface_lock;
};

#define super IOService
OSDefineMetaClassAndStructors(VirtualCANInterface, IOService)

VirtualCANInterface *VirtualCANInterface::interfaces[VCAN_MAX_INTERFACES] = {0};
UInt32 VirtualCANInterface::interface_count = 0;
IOLock *VirtualCANInterface::interface_lock = NULL;

bool VirtualCANInterface::init(OSDictionary *properties)
{
    if (!super::init(properties)) {
        return false;
    }
    
    interface = NULL;
    echo_mode = false;
    interface_index = 0;
    
    // Check for echo mode parameter
    OSObject *echo_param = getProperty("echo");
    if (echo_param) {
        OSBoolean *echo_bool = OSDynamicCast(OSBoolean, echo_param);
        if (echo_bool) {
            echo_mode = echo_bool->isTrue();
        }
    }
    
    return true;
}

void VirtualCANInterface::free()
{
    if (interface) {
        destroy_interface();
    }
    super::free();
}

bool VirtualCANInterface::start(IOService *provider)
{
    if (!super::start(provider)) {
        return false;
    }
    
    // Initialize interface lock if not already done
    if (!interface_lock) {
        interface_lock = IOLockAlloc();
        if (!interface_lock) {
            IOLog("vcan: Failed to allocate interface lock\n");
            return false;
        }
    }
    
    if (!create_interface()) {
        IOLog("vcan: Failed to create network interface\n");
        return false;
    }
    
    IOLog("vcan: Virtual CAN interface started (echo: %s)\n", 
          echo_mode ? "enabled" : "disabled");
    
    return true;
}

void VirtualCANInterface::stop(IOService *provider)
{
    destroy_interface();
    super::stop(provider);
}

bool VirtualCANInterface::create_interface()
{
    errno_t result;
    
    IOLockLock(interface_lock);
    
    // Find available interface slot
    UInt32 index;
    for (index = 0; index < VCAN_MAX_INTERFACES; index++) {
        if (interfaces[index] == NULL) {
            break;
        }
    }
    
    if (index >= VCAN_MAX_INTERFACES) {
        IOLockUnlock(interface_lock);
        IOLog("vcan: Maximum number of interfaces reached\n");
        return false;
    }
    
    interface_index = index;
    interfaces[index] = this;
    interface_count++;
    
    IOLockUnlock(interface_lock);
    
    // Create a simple interface using the older API
    struct ifnet_init_params init_params;
    bzero(&init_params, sizeof(init_params));
    
    // Set the basic parameters that are always available
    init_params.name = "vcan";
    init_params.unit = index;
    init_params.type = IFT_OTHER;
    init_params.output = vcan_output;
    init_params.ioctl = vcan_ioctl;
    init_params.softc = this;
    
    result = ifnet_allocate(&init_params, &interface);
    if (result != 0) {
        IOLog("vcan: ifnet_allocate failed with error %d\n", result);
        IOLockLock(interface_lock);
        interfaces[index] = NULL;
        interface_count--;
        IOLockUnlock(interface_lock);
        return false;
    }
    
    // Set interface properties
    ifnet_set_mtu(interface, (u_int32_t)CANFD_MTU);
    ifnet_set_flags(interface, IFF_BROADCAST | IFF_SIMPLEX | IFF_NOARP, 0xffff);
    ifnet_set_addrlen(interface, 0);
    ifnet_set_hdrlen(interface, 0);
    
    // Attach the interface
    result = ifnet_attach(interface, NULL);
    if (result != 0) {
        IOLog("vcan: ifnet_attach failed with error %d\n", result);
        ifnet_release(interface);
        interface = NULL;
        IOLockLock(interface_lock);
        interfaces[index] = NULL;
        interface_count--;
        IOLockUnlock(interface_lock);
        return false;
    }
    
    return true;
}

void VirtualCANInterface::destroy_interface()
{
    if (interface) {
        ifnet_detach(interface);
        ifnet_release(interface);
        interface = NULL;
        
        IOLockLock(interface_lock);
        interfaces[interface_index] = NULL;
        interface_count--;
        IOLockUnlock(interface_lock);
    }
}

errno_t VirtualCANInterface::vcan_output(ifnet_t interface, mbuf_t packet)
{
    VirtualCANInterface *self = (VirtualCANInterface *)ifnet_softc(interface);
    if (self) {
        return self->output_handler(packet);
    }
    return EINVAL;
}

errno_t VirtualCANInterface::output_handler(mbuf_t packet)
{
    struct ifnet_stat_increment_param stats;
    size_t packet_len;
    bool should_loop = false;
    
    if (!packet) {
        return EINVAL;
    }
    
    packet_len = mbuf_pkthdr_len(packet);
    
    // Update transmit statistics
    bzero(&stats, sizeof(stats));
    stats.packets_out = 1;
    stats.bytes_out = (u_int32_t)packet_len;
    ifnet_stat_increment(interface, &stats);
    
    // Check if this packet should be looped back
    if (mbuf_flags(packet) & MBUF_LOOP) {
        should_loop = true;
    }
    
    if (!echo_mode) {
        // No echo handling - just count loopback packets
        if (should_loop) {
            bzero(&stats, sizeof(stats));
            stats.packets_in = 1;
            stats.bytes_in = (u_int32_t)packet_len;
            ifnet_stat_increment(interface, &stats);
        }
        mbuf_freem(packet);
        return 0;
    }
    
    // Echo mode - handle loopback
    if (should_loop) {
        mbuf_t echo_packet;
        
        // Create echo packet
        if (mbuf_dup(packet, MBUF_DONTWAIT, &echo_packet) == 0) {
            input_handler(echo_packet);
        }
    }
    
    mbuf_freem(packet);
    return 0;
}

void VirtualCANInterface::input_handler(mbuf_t packet)
{
    struct ifnet_stat_increment_param stats;
    size_t packet_len;
    
    if (!packet) {
        return;
    }
    
    packet_len = mbuf_pkthdr_len(packet);
    
    // Update receive statistics
    bzero(&stats, sizeof(stats));
    stats.packets_in = 1;
    stats.bytes_in = (u_int32_t)packet_len;
    ifnet_stat_increment(interface, &stats);
    
    // Set packet type to broadcast
    mbuf_pkthdr_setrcvif(packet, interface);
    
    // Pass to network stack
    ifnet_input(interface, packet, NULL);
}

errno_t VirtualCANInterface::vcan_ioctl(ifnet_t interface, u_long command, void *data)
{
    VirtualCANInterface *self = (VirtualCANInterface *)ifnet_softc(interface);
    if (self) {
        return self->ioctl_handler(command, data);
    }
    return EINVAL;
}

errno_t VirtualCANInterface::ioctl_handler(u_long command, void *data)
{
    errno_t result = 0;
    
    switch (command) {
        case SIOCGIFMTU:
        case SIOCSIFMTU: {
            struct ifreq *ifr = (struct ifreq *)data;
            
            if (command == SIOCGIFMTU) {
                ifr->ifr_mtu = (int)ifnet_mtu(interface);
            } else {
                int new_mtu = ifr->ifr_mtu;
                
                // Validate MTU - must be for CAN, CAN-FD, or CAN-XL
                if (new_mtu != CAN_MTU && new_mtu != CANFD_MTU && 
                    (new_mtu < CANXL_MIN_MTU || new_mtu > CANXL_MAX_MTU)) {
                    result = EINVAL;
                    break;
                }
                
                // Don't allow MTU changes while interface is up
                if (ifnet_flags(interface) & IFF_UP) {
                    result = EBUSY;
                    break;
                }
                
                ifnet_set_mtu(interface, (u_int32_t)new_mtu);
            }
            break;
        }
        
        default:
            result = EOPNOTSUPP;
            break;
    }
    
    return result;
}

extern "C" {

kern_return_t vcan_start(kmod_info_t *ki, void *data)
{
    IOLog("vcan: Virtual CAN interface driver v%s loaded\n", VCAN_DRIVER_VERSION);
    return KERN_SUCCESS;
}

kern_return_t vcan_stop(kmod_info_t *ki, void *data)
{
    IOLog("vcan: Virtual CAN interface driver unloaded\n");
    return KERN_SUCCESS;
}

}

// Export symbols for kernel extension
extern "C" {
    kern_return_t _start(kmod_info_t *ki, void *data);
    kern_return_t _stop(kmod_info_t *ki, void *data);
    
    __attribute__((visibility("default"))) KMOD_EXPLICIT_DECL(com.opensource.vcan, VCAN_DRIVER_VERSION, _start, _stop)
    
    kern_return_t _start(kmod_info_t *ki, void *data)
    {
        return vcan_start(ki, data);
    }
    
    kern_return_t _stop(kmod_info_t *ki, void *data)
    {
        return vcan_stop(ki, data);
    }
}