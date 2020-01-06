# Copyright (C) 2019-2020 Codenocold
#
# The following terms apply to all files associated
# with the software unless explicitly disclaimed in individual files.
#
# The authors hereby grant permission to use, copy, modify, distribute,
# and license this software and its documentation for any purpose, provided
# that existing copyright notices are retained in all copies and that this
# notice is included verbatim in any distributions. No written agreement,
# license, or royalty fee is required for any of the authorized uses.
# Modifications to this software may be copyrighted by their authors
# and need not follow the licensing terms described here, provided that
# the new terms are clearly indicated on the first page of each file where
# they apply.
#
# IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
# FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
# ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
# DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
# IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
# NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
# MODIFICATIONS.

__author__ = 'Codenocold'

import usb.core
import usb.util
from struct import *

# Dev mode
CANDO_MODE_NORMAL = 0
CANDO_MODE_LISTEN_ONLY = (1 << 0)
CANDO_MODE_LOOP_BACK = (1 << 1)
CANDO_MODE_ONE_SHOT = (1 << 3)

# CAN ID mask
CANDO_ID_MASK = 0x1FFFFFFF
CANDO_ID_EXTENDED = 0x80000000
CANDO_ID_RTR = 0x40000000
CANDO_ID_ERR = 0x20000000

# CAN bus error
CAN_ERR_BUSOFF = 0x00000001
CAN_ERR_RX_TX_WARNING = 0x00000002
CAN_ERR_RX_TX_PASSIVE = 0x00000004
CAN_ERR_OVERLOAD = 0x00000008
CAN_ERR_STUFF = 0x00000010
CAN_ERR_FORM = 0x00000020
CAN_ERR_ACK = 0x00000040
CAN_ERR_BIT_RECESSIVE = 0x00000080
CAN_ERR_BIT_DOMINANT = 0x00000100
CAN_ERR_CRC = 0x00000200

# control request
__CANDO_BREQ_MODE = 2
__CANDO_BREQ_BITTIMING = 1
__CANDO_BREQ_DEVICE_CONFIG = 5


class Frame:
    def __init__(self):
        self.echo_id = 0  # 0: echo; 0xFFFFFFFF: not echo
        self.can_id = 0
        self.can_dlc = 0
        self.channel = 0
        self.flags = 0
        self.reserved = 0
        self.data = [0x00] * 8
        self.timestamp_us = 0

    def __sizeof__(self):
        return 24


def list_scan():
    r"""
    Retrieve the list of cando devices handle
    :return: list of cando devices handle
    """
    return list(usb.core.find(find_all=True, idVendor=0x1D50, idProduct=0x606F))


def dev_start(dev, mode=0):
    r"""
    Start cando device
    :param dev: cando device handle
    :param mode: flags @ CANDO_MODE_*
    :return: none
    """
    mode_ = 1
    mode |= (1 << 4)
    data = pack('II', mode_, mode)
    dev.ctrl_transfer(0x41, __CANDO_BREQ_MODE, 0, 0, data)


def dev_stop(dev):
    r"""
    Stop cando device
    :param dev: cando device handle
    :return: none
    """
    mode_ = 0
    mode = 0
    data = pack('II', mode_, mode)
    dev.ctrl_transfer(0x41, __CANDO_BREQ_MODE, 0, 0, data)


def dev_set_timing(dev, prop_seg, phase_seg1, phase_seg2, sjw, brp):
    r"""
    Set CAN bit timing
    :param dev: cando device handle
    :param prop_seg: propagation Segment (const 1)
    :param phase_seg1: phase segment 1 (1~15)
    :param phase_seg2: phase segment 2 (1~8)
    :param sjw: synchronization segment (1~4)
    :param brp: prescaler for quantum #base_clk = 48MHz (1~1024)
    :return: none
    """
    data = pack('5I', prop_seg, phase_seg1, phase_seg2, sjw, brp)
    dev.ctrl_transfer(0x41, __CANDO_BREQ_BITTIMING, 0, 0, data)


def dev_get_serial_number_str(dev):
    r"""
    Get cando device serial number in string format
    :param dev: cando device handle
    :return: cando device serial number string
    """
    return dev.serial_number


def dev_get_dev_info_str(dev):
    r"""
    Get cando device info
    :param dev: cando device handle
    :return: cando device info string
    """
    data = dev.ctrl_transfer(0xC1, __CANDO_BREQ_DEVICE_CONFIG, 0, 0, 12)
    tup = unpack('4B2I', data)
    info = "fw: " + str(tup[4] / 10) + " hw: " + str(tup[5] / 10)
    return info


def parse_err_frame(frame):
    r"""
    Parse can bus error status
    :param frame: error frame
    :return: <tuple> error_code @ CAN_ERR_*, err_tx, err_rx
    """
    error_code = 0

    if frame.can_id & 0x00000040:
        error_code |= CAN_ERR_BUSOFF

    if frame.data[1] & 0x04:
        error_code |= CAN_ERR_RX_TX_WARNING
    elif frame.data[1] & 0x10:
        error_code |= CAN_ERR_RX_TX_PASSIVE

    if frame.flags & 0x00000001:
        error_code |= CAN_ERR_OVERLOAD

    if frame.data[2] & 0x04:
        error_code |= CAN_ERR_STUFF

    if frame.data[2] & 0x02:
        error_code |= CAN_ERR_FORM

    if frame.can_id & 0x00000020:
        error_code |= CAN_ERR_ACK

    if frame.data[2] & 0x10:
        error_code |= CAN_ERR_BIT_RECESSIVE

    if frame.data[2] & 0x08:
        error_code |= CAN_ERR_BIT_DOMINANT

    if frame.data[3] & 0x08:
        error_code |= CAN_ERR_CRC

    return error_code, int(frame.data[6]), int(frame.data[7])


def dev_frame_send(dev, frame):
    r"""
    Send frame
    :param dev: cando device handle
    :param frame: cando frame @ class Frame
    :return: none
    """
    data = pack("2I12BI", frame.echo_id, frame.can_id, frame.can_dlc, frame.channel,
                frame.flags, frame.reserved, *frame.data, frame.timestamp_us)
    dev.write(0x02, data)


def dev_frame_read(dev, frame, timeout_ms):
    r"""
    Read frame
    :param dev: cando device handle
    :param frame: cando frame @ class Frame
    :param timeout_ms: read time out in ms
    :return: return True if success else False
    """
    try:
        data = dev.read(0x81, frame.__sizeof__(), timeout_ms)
    except usb.core.USBError:
        return False
    frame.echo_id, frame.can_id, frame.can_dlc, frame.channel, frame.flags, frame.reserved, *frame.data, \
        frame.timestamp_us = unpack("2I12BI", data)
    return True
