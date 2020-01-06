#include "cando.h"
#include <stdlib.h>

#include "cando_defs.h"
#include "cando_ctrl_req.h"

#define CANDO_MODE_HW_TIMESTAMP                (1<<4)

static bool cando_read_di(HDEVINFO hdi, SP_DEVICE_INTERFACE_DATA interfaceData, cando_device_t *dev);
static bool cando_close_rxurbs(cando_device_t *dev);
static bool cando_prepare_read(cando_device_t *dev, unsigned urb_num);
static bool cando_interal_open(cando_handle hdev);

bool __stdcall DLL cando_list_malloc(cando_list_handle *list)
{
    if (list==NULL) {
        return false;
    }

    cando_list_t *l = (cando_list_t *)calloc(1, sizeof(cando_list_t));
    *list = l;
    if (l==NULL) {
        return false;
    }

    return true;
}

bool __stdcall DLL cando_list_free(cando_list_handle list)
{
    free(list);
    return true;
}

bool __stdcall DLL cando_list_scan(cando_list_handle list)
{
    cando_list_t *l = (cando_list_t *)list;

    GUID guid;
    if (CLSIDFromString(L"{c15b4308-04d3-11e6-b3ea-6057189e6443}", &guid) != NOERROR) {
        return false;
    }

    HDEVINFO hdi = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hdi == INVALID_HANDLE_VALUE) {
        return false;
    }

    bool rv = false;
    for (uint8_t i=0; i<CANDO_MAX_DEVICES; i++) {
        SP_DEVICE_INTERFACE_DATA interfaceData;
        interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        if (SetupDiEnumDeviceInterfaces(hdi, NULL, &guid, i, &interfaceData)) {
            if (!cando_read_di(hdi, interfaceData, &l->dev[i])) {
                rv = false;
                break;
            }
        } else {
            DWORD err = GetLastError();
            if (err==ERROR_NO_MORE_ITEMS) {
                l->num_devices = i;
                rv = true;
            } else {
                rv = false;
            }
            break;
        }
    }

    SetupDiDestroyDeviceInfoList(hdi);

    return rv;
}

bool __stdcall DLL cando_list_num(cando_list_handle list, uint8_t *num)
{
    cando_list_t *l = (cando_list_t *)list;
    *num = l->num_devices;
    return true;
}

bool __stdcall DLL cando_malloc(cando_list_handle list, uint8_t index, cando_handle *hdev)
{
    cando_list_t *l = (cando_list_t *)list;
    if (l==NULL) {
        return false;
    }

    if (index >= CANDO_MAX_DEVICES) {
        return false;
    }

    cando_device_t *dev = calloc(1, sizeof(cando_device_t));
    *hdev = dev;
    if (dev==NULL) {
        return false;
    }

    memcpy(dev, &l->dev[index], sizeof(cando_device_t));

    return true;
}

bool __stdcall DLL cando_free(cando_handle hdev)
{
    free(hdev);
    return true;
}

bool __stdcall DLL cando_open(cando_handle hdev)
{
    cando_device_t *dev = (cando_device_t*)hdev;

    if (cando_interal_open(dev)) {
        for (unsigned i=0; i<CANDO_URB_COUNT; i++) {
            HANDLE ev = CreateEvent(NULL, true, false, NULL);
            dev->rxevents[i] = ev;
            dev->rxurbs[i].ovl.hEvent = ev;
            if (!cando_prepare_read(dev, i)) {
                cando_close_rxurbs(dev);
                return false; // keep last_error from prepare_read call
            }
        }
        return true;
    } else {
        return false; // keep last_error from open_device call
    }
}

bool __stdcall DLL cando_close(cando_handle hdev)
{
    cando_device_t *dev = (cando_device_t*)hdev;

    cando_close_rxurbs(dev);

    WinUsb_Free(dev->winUSBHandle);
    dev->winUSBHandle = NULL;
    CloseHandle(dev->deviceHandle);
    dev->deviceHandle = NULL;

    return true;
}

bool __stdcall DLL cando_get_dev_info(cando_handle hdev, uint32_t *sw_version, uint32_t *hw_version)
{
    cando_device_t *dev = (cando_device_t*)hdev;
    *sw_version = dev->dconf.sw_version;
    *hw_version = dev->dconf.hw_version;
    return true;
}

wchar_t __stdcall DLL *cando_get_path(cando_handle hdev)
{
    if (hdev==NULL) {
        return NULL;
    } else {
        cando_device_t *dev = (cando_device_t*)hdev;
        return dev->path;
    }
}

bool __stdcall DLL cando_set_timing(cando_handle hdev, cando_bittiming_t *timing)
{
    // TODO ensure device is open, check channel count..
    cando_device_t *dev = (cando_device_t*)hdev;
    return cando_ctrl_set_bittiming(dev, timing);
}

bool __stdcall DLL cando_start(cando_handle hdev, uint32_t mode)
{
    // TODO ensure device is open, check channel count..
    cando_device_t *dev = (cando_device_t*)hdev;
    mode |= (CANDO_MODE_HW_TIMESTAMP);
    return cando_ctrl_set_device_mode(dev, CANDO_DEVMODE_START, mode);
}

bool __stdcall DLL cando_stop(cando_handle hdev)
{
    // TODO ensure device is open, check channel count..
    cando_device_t *dev = (cando_device_t*)hdev;
    return cando_ctrl_set_device_mode(dev, CANDO_DEVMODE_RESET, 0);
}

bool __stdcall DLL cando_frame_send(cando_handle hdev, cando_frame_t *frame)
{
    // TODO ensure device is open, check channel count..
    cando_device_t *dev = (cando_device_t*)hdev;

    unsigned long bytes_sent = 0;

    bool rc = WinUsb_WritePipe(
        dev->winUSBHandle,
        dev->bulkOutPipe,
        (uint8_t*)frame,
        sizeof(*frame),
        &bytes_sent,
        0
    );

    return rc;
}

bool __stdcall DLL cando_frame_read(cando_handle hdev, cando_frame_t *frame, uint32_t timeout_ms)
{
    // TODO ensure device is open..
    cando_device_t *dev = (cando_device_t*)hdev;

    DWORD wait_result = WaitForMultipleObjects(CANDO_URB_COUNT, dev->rxevents, false, timeout_ms);
    if (wait_result == WAIT_TIMEOUT) {
        return false;
    }

    if (wait_result >= WAIT_OBJECT_0 + CANDO_URB_COUNT) {
        return false;
    }

    DWORD urb_num = wait_result - WAIT_OBJECT_0;
    DWORD bytes_transfered;

    if (!WinUsb_GetOverlappedResult(dev->winUSBHandle, &dev->rxurbs[urb_num].ovl, &bytes_transfered, false)) {
        cando_prepare_read(dev, urb_num);
        return false;
    }

    if (bytes_transfered < sizeof(*frame)-4) {
        cando_prepare_read(dev, urb_num);
        return false;
    }

    if (bytes_transfered < sizeof(*frame)) {
        frame->timestamp_us = 0;
    }

    memcpy(frame, dev->rxurbs[urb_num].buf, sizeof(*frame));

    return cando_prepare_read(dev, urb_num);
}

bool __stdcall DLL cando_parse_err_frame(cando_frame_t *frame, uint32_t *err_code, uint8_t *err_tx, uint8_t *err_rx)
{
    *err_code = 0;

    if(frame->can_id & 0x00000040U){
        *err_code |= CAN_ERR_BUSOFF;
    }

    if(frame->data[1] & 0x04){
        *err_code |= CAN_ERR_RX_TX_WARNING;
    }else if(frame->data[1] & 0x10){
        *err_code |= CAN_ERR_RX_TX_PASSIVE;
    }

    if(frame->flags & 0x00000001U){
        *err_code |= CAN_ERR_OVERLOAD;
    }

    if(frame->data[2] & 0x04){
        *err_code |= CAN_ERR_STUFF;
    }
    if(frame->data[2] & 0x02){
        *err_code |= CAN_ERR_FORM;
    }
    if(frame->can_id & 0x00000020U){
        *err_code |= CAN_ERR_ACK;
    }
    if(frame->data[2] & 0x10){
        *err_code |= CAN_ERR_BIT_RECESSIVE;
    }
    if(frame->data[2] & 0x08){
        *err_code |= CAN_ERR_BIT_DOMINANT;
    }
    if(frame->data[3] & 0x08){
        *err_code |= CAN_ERR_CRC;
    }

    *err_tx = frame->data[6];
    *err_rx = frame->data[7];

    return true;
}

static bool cando_read_di(HDEVINFO hdi, SP_DEVICE_INTERFACE_DATA interfaceData, cando_device_t *dev)
{
    /* get required length first (this call always fails with an error) */
    ULONG requiredLength=0;
    SetupDiGetDeviceInterfaceDetail(hdi, &interfaceData, NULL, 0, &requiredLength, NULL);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        return false;
    }

    PSP_DEVICE_INTERFACE_DETAIL_DATA detail_data =
        (PSP_DEVICE_INTERFACE_DETAIL_DATA) LocalAlloc(LMEM_FIXED, requiredLength);

    if (detail_data != NULL) {
        detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    } else {
        return false;
    }

    bool retval = true;
    ULONG length = requiredLength;
    if (!SetupDiGetDeviceInterfaceDetail(hdi, &interfaceData, detail_data, length, &requiredLength, NULL) ) {
        retval = false;
    } else if (FAILED(StringCchCopy(dev->path, sizeof(dev->path), detail_data->DevicePath))) {
        retval = false;
    }

    LocalFree(detail_data);

    if (!retval) {
        return false;
    }

    return true;
}

static bool cando_close_rxurbs(cando_device_t *dev)
{
    for (unsigned i=0; i<CANDO_URB_COUNT; i++) {
        if (dev->rxevents[i] != NULL) {
            CloseHandle(dev->rxevents[i]);
        }
    }
    return true;
}

static bool cando_prepare_read(cando_device_t *dev, unsigned urb_num)
{
    bool rc = WinUsb_ReadPipe(
        dev->winUSBHandle,
        dev->bulkInPipe,
        dev->rxurbs[urb_num].buf,
        sizeof(dev->rxurbs[urb_num].buf),
        NULL,
        &dev->rxurbs[urb_num].ovl
    );

    if (rc || (GetLastError() != ERROR_IO_PENDING)) {
        return false;
    } else {
        return true;
    }
}

static bool cando_interal_open(cando_handle hdev)
{
    cando_device_t *dev = (cando_device_t*)hdev;

    memset(dev->rxevents, 0, sizeof(dev->rxevents));
    memset(dev->rxurbs, 0, sizeof(dev->rxurbs));

    dev->deviceHandle = CreateFile(
        dev->path,
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (dev->deviceHandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    if (!WinUsb_Initialize(dev->deviceHandle, &dev->winUSBHandle)) {
        goto close_handle;
    }

    USB_INTERFACE_DESCRIPTOR ifaceDescriptor;
    if (!WinUsb_QueryInterfaceSettings(dev->winUSBHandle, 0, &ifaceDescriptor)) {
        goto winusb_free;
    }

    dev->interfaceNumber = ifaceDescriptor.bInterfaceNumber;
    unsigned pipes_found = 0;

    for (uint8_t i=0; i<ifaceDescriptor.bNumEndpoints; i++) {
        WINUSB_PIPE_INFORMATION pipeInfo;
        if (!WinUsb_QueryPipe(dev->winUSBHandle, 0, i, &pipeInfo)) {
            goto winusb_free;
        }

        if (pipeInfo.PipeType == UsbdPipeTypeBulk && USB_ENDPOINT_DIRECTION_IN(pipeInfo.PipeId)) {
            dev->bulkInPipe = pipeInfo.PipeId;
            pipes_found++;
        } else if (pipeInfo.PipeType == UsbdPipeTypeBulk && USB_ENDPOINT_DIRECTION_OUT(pipeInfo.PipeId)) {
            dev->bulkOutPipe = pipeInfo.PipeId;
            pipes_found++;
        } else {
            goto winusb_free;
        }
    }

    if (pipes_found != 2) {
        goto winusb_free;
    }

    char use_raw_io = 1;
    if (!WinUsb_SetPipePolicy(dev->winUSBHandle, dev->bulkInPipe, RAW_IO, sizeof(use_raw_io), &use_raw_io)) {
        goto winusb_free;
    }

    if (!cando_ctrl_get_config(dev, &dev->dconf)) {
        goto winusb_free;
    }

    return true;

winusb_free:
    WinUsb_Free(dev->winUSBHandle);

close_handle:
    CloseHandle(dev->deviceHandle);
    return false;
}
