#include "cando_ctrl_req.h"

enum {
    CANDO_BREQ_HOST_FORMAT = 0,
    CANDO_BREQ_BITTIMING,
    CANDO_BREQ_MODE,
    CANDO_BREQ_BERR,
    CANDO_BREQ_BT_CONST,
    CANDO_BREQ_DEVICE_CONFIG,
    CANDO_BREQ_TIMESTAMP,
    CANDO_BREQ_IDENTIFY,
    CANDO_BREQ_GET_USER_ID,
    CANDO_BREQ_SET_USER_ID,
};

static bool usb_control_msg(WINUSB_INTERFACE_HANDLE hnd, uint8_t request, uint8_t requesttype, uint16_t value, uint16_t index, void *data, uint16_t size)
{
    WINUSB_SETUP_PACKET packet;
    memset(&packet, 0, sizeof(packet));

    packet.Request = request;
    packet.RequestType = requesttype;
    packet.Value = value;
    packet.Index = index;
    packet.Length = size;

    unsigned long bytes_sent = 0;
    return WinUsb_ControlTransfer(hnd, packet, (uint8_t*)data, size, &bytes_sent, 0);
}

bool cando_ctrl_set_bittiming(cando_device_t *dev, uint16_t ch, cando_bittiming_t *timing)
{
    bool rc = usb_control_msg(
        dev->winUSBHandle,
        CANDO_BREQ_BITTIMING,
        USB_DIR_OUT|USB_TYPE_VENDOR|USB_RECIP_INTERFACE,
        ch,  // channel 0
        0,
        timing,
        sizeof(*timing)
    );

    return rc;
}

bool cando_ctrl_set_device_mode(cando_device_t *dev, uint16_t ch, uint32_t mode, uint32_t flags)
{
    cando_device_mode_t dm;
    dm.mode = mode;
    dm.flags = flags;

    bool rc = usb_control_msg(
        dev->winUSBHandle,
        CANDO_BREQ_MODE,
        USB_DIR_OUT|USB_TYPE_VENDOR|USB_RECIP_INTERFACE,
        ch, // channel 0
        0,
        &dm,
        sizeof(dm)
    );

    return rc;
}

bool cando_ctrl_get_config(cando_device_t *dev, cando_device_config_t *dconf)
{
    bool rc = usb_control_msg(
        dev->winUSBHandle,
        CANDO_BREQ_DEVICE_CONFIG,
        USB_DIR_IN|USB_TYPE_VENDOR|USB_RECIP_INTERFACE,
        0,
        0,
        dconf,
        sizeof(*dconf)
    );

    return rc;
}
