#ifndef VCAN_H
#define VCAN_H

#include <sys/types.h>
#include <sys/socket.h>

/* CAN frame structure - compatible with Linux SocketCAN */
struct can_frame {
    uint32_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
    uint8_t  can_dlc; /* frame payload length in byte (0 .. 8) */
    uint8_t  __pad;   /* padding */
    uint8_t  __res0;  /* reserved / padding */
    uint8_t  __res1;  /* reserved / padding */
    uint8_t  data[8] __attribute__((aligned(8)));
};

/* CAN FD frame structure - compatible with Linux SocketCAN */
struct canfd_frame {
    uint32_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
    uint8_t  len;     /* frame payload length in byte */
    uint8_t  flags;   /* additional flags for CAN FD */
    uint8_t  __res0;  /* reserved / padding */
    uint8_t  __res1;  /* reserved / padding */
    uint8_t  data[64] __attribute__((aligned(8)));
};

/* CAN XL frame structure - compatible with Linux SocketCAN */
struct canxl_frame {
    uint32_t prio;    /* 11 bit priority / 21 bit vcid */
    uint8_t  flags;   /* CAN XL flags */
    uint8_t  sdt;     /* SDU type */
    uint16_t len;     /* frame payload length in byte */
    uint32_t af;      /* acceptance field */
    uint8_t  data[];  /* CAN XL frame payload (CANXL_MIN_DLEN .. CANXL_MAX_DLEN) */
};

/* CAN ID flags */
#define CAN_EFF_FLAG 0x80000000U /* EFF/SFF is set in the MSB */
#define CAN_RTR_FLAG 0x40000000U /* remote transmission request */
#define CAN_ERR_FLAG 0x20000000U /* error message frame */

/* CAN frame sizes */
#define CAN_MTU     16  /* = sizeof(struct can_frame) */
#define CANFD_MTU   72  /* = sizeof(struct canfd_frame) */
#define CANXL_MIN_MTU 80 /* = sizeof(struct canxl_frame) with CANXL_MIN_DLEN */
#define CANXL_MAX_MTU 2048 /* maximum CAN XL frame size */

/* CAN data length codes */
#define CAN_MAX_DLC 8
#define CANFD_MAX_DLC 15
#define CANXL_MIN_DLEN 1
#define CANXL_MAX_DLEN 2048

/* Protocol family for CAN */
#define PF_CAN 35
#define AF_CAN PF_CAN

/* CAN protocol types */
#define CAN_RAW 1

#endif /* VCAN_H */