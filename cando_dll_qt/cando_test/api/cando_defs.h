#pragma once

#include <stdint.h>
#include <windows.h>
#include <winbase.h>
#include <winusb.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>

#undef __CRT__NO_INLINE
#include <strsafe.h>
#define __CRT__NO_INLINE

#include "cando.h"

#define USB_DIR_OUT                     0               /* to device */
#define USB_DIR_IN                      0x80            /* to host */

#define USB_TYPE_MASK                   (0x03 << 5)
#define USB_TYPE_STANDARD               (0x00 << 5)
#define USB_TYPE_CLASS                  (0x01 << 5)
#define USB_TYPE_VENDOR                 (0x02 << 5)
#define USB_TYPE_RESERVED               (0x03 << 5)

#define USB_RECIP_MASK                  0x1f
#define USB_RECIP_DEVICE                0x00
#define USB_RECIP_INTERFACE             0x01
#define USB_RECIP_ENDPOINT              0x02
#define USB_RECIP_OTHER                 0x03

#define CANDO_MAX_DEVICES   32
#define CANDO_URB_COUNT     30

#pragma pack(push, 1)

typedef struct {
    uint8_t reserved1;
    uint8_t reserved2;
    uint8_t reserved3;
    uint8_t icount;
    uint32_t sw_version;
    uint32_t hw_version;
} cando_device_config_t;

typedef struct {
    uint32_t mode;
    uint32_t flags;
} cando_device_mode_t;

#pragma pack(pop)

typedef struct {
    OVERLAPPED ovl;
    uint8_t buf[128];
} cando_rx_urb;

typedef struct {
    wchar_t path[256];

    HANDLE deviceHandle;
    WINUSB_INTERFACE_HANDLE winUSBHandle;
    UCHAR interfaceNumber;
    UCHAR bulkInPipe;
    UCHAR bulkOutPipe;

    cando_device_config_t dconf;

    cando_rx_urb rxurbs[CANDO_URB_COUNT];
    HANDLE rxevents[CANDO_URB_COUNT];
} cando_device_t;

typedef struct {
    uint8_t num_devices;
    cando_device_t dev[CANDO_MAX_DEVICES];
} cando_list_t;
