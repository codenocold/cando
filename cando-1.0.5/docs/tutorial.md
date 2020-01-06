使用 cando 编程
===
# 一 简介

cando 是基于 Python3 编写的模块，通过几个简单的函数便可以完成和 usb 转 can 模块（Cando 或者 Cando_pro）的通信，进行高效的 CAN 工具开发。

cando 后台 usb 通信基于 libusb 进行的，所以使用前请首先安装 libusb 驱动。

**Windows** 推荐使用 Zadig 工具进行安装。

1. 下载 [Zadig](https://zadig.akeo.ie/)
2. 将 Cando 或 Cando_pro 连接电脑
3. 双击运行 zadig-x.x.exe
4. 点击菜单栏中的 `Options` -> `List All Devices` 然后点击菜单栏下方的下拉列表，选择 Cando 或 Cando_pro
5. 选择下方的驱动为 libusb-win32 ，然后点击 `Replace Driver` ，等待安装完成即可

**Linux** Ubuntu18.04 默认已安装 libusb 无需安装，其他发行版本请根据情况自行安装。

好的，下面开始编写代码。

# 二 列出连接的设备

```py
import sys
from cando import *

# 获取设备列表
dev_lists = list_scan()

# 打印扫描到的Cando或Cando_pro的设备信息
if len(dev_lists):
    for dev in dev_lists:
        # 获取设备序列号
        serial_number = dev_get_serial_number_str(dev)
        # 获取设备版本信息
        dev_info = dev_get_dev_info_str(dev)
        # 打印设备信息
        print("Serial Number: " + serial_number + ', Dev Info: ' + dev_info)
else:
    print("Device not found!")
    sys.exit(0)

```

运行得到输出结果：(电脑连接两个Cando或Cando_pro)

```py
Serial Number: 004F00295734571020343132, Dev Info: fw: 3.2 hw: 1.2
Serial Number: 0028002B5734571020343132, Dev Info: fw: 3.2 hw: 1.2

Process finished with exit code 0
```

# 三 发送数据

```py
import sys
import time
from cando import *

# 获取设备列表
dev_lists = list_scan()

# 判断是否发现设备
if len(dev_lists) == 0:
    print("Device not found!")
    sys.exit(0)

# 设置波特率：500K 采样点：87.5%
dev_set_timing(dev_lists[0], 1, 12, 2, 1, 6)

# 启动设备
dev_start(dev_lists[0], 0)

# 设置发送的数据帧
send_frame = Frame()
send_frame.can_id = 0x12
# send_frame.can_id |= CANDO_ID_EXTENDED
# send_frame.can_id |= CANDO_ID_RTR
send_frame.can_dlc = 8
send_frame.data = [0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08]

# 循环发送 500 条数据帧
for i in range(500):
    # 发送数据帧
    send_frame.data[0] = i
    dev_frame_send(dev_lists[0], send_frame)
    time.sleep(0.001)  # 睡眠 1 ms

    # 读取数据帧，因为发送成功后 Cando 将返回 ECHO 帧，如果不进行读取，会阻塞通道，造成无法继续发送
    rec_frame = Frame()
    dev_frame_read(dev_lists[0], rec_frame, 100)

# 停止设备
dev_stop(dev_lists[0])

```

# 四 接收数据

```py
import sys
from cando import *

# 获取设备列表
dev_lists = list_scan()

# 判断是否发现设备
if len(dev_lists) == 0:
    print("Device not found!")
    sys.exit(0)

# 设置波特率：500K 采样点：87.5%
dev_set_timing(dev_lists[0], 1, 12, 2, 1, 6)

# 启动设备
dev_start(dev_lists[0], 0)

# 创建接收数据帧
rec_frame = Frame()

# 阻塞读取数据
print("Reading...")
while True:
    if dev_frame_read(dev_lists[0], rec_frame, 10):
        break

if rec_frame.can_id & CANDO_ID_ERR:    # 错误帧处理
    error_code, err_tx, err_rx = parse_err_frame(rec_frame)
    print("Error: ")
    if error_code & CAN_ERR_BUSOFF:
        print("    CAN_ERR_BUSOFF")
    if error_code & CAN_ERR_RX_TX_WARNING:
        print("    CAN_ERR_RX_TX_WARNING")
    if error_code & CAN_ERR_RX_TX_PASSIVE:
        print("    CAN_ERR_RX_TX_PASSIVE")
    if error_code & CAN_ERR_OVERLOAD:
        print("    CAN_ERR_OVERLOAD")
    if error_code & CAN_ERR_STUFF:
        print("    CAN_ERR_STUFF")
    if error_code & CAN_ERR_FORM:
        print("    CAN_ERR_FORM")
    if error_code & CAN_ERR_ACK:
        print("    CAN_ERR_ACK")
    if error_code & CAN_ERR_BIT_RECESSIVE:
        print("    CAN_ERR_BIT_RECESSIVE")
    if error_code & CAN_ERR_BIT_DOMINANT:
        print("    CAN_ERR_BIT_DOMINANT")
    if error_code & CAN_ERR_CRC:
        print("    CAN_ERR_CRC")
    print("    err_tx: " + str(err_tx))
    print("    err_rx: " + str(err_rx))
else:    # 数据帧处理
    print("Rec Frame: ")
    print("    is_extend    : " + ("True" if rec_frame.can_id & CANDO_ID_EXTENDED else "False"))
    print("    is_rtr       : " + ("True" if rec_frame.can_id & CANDO_ID_RTR else "False"))
    print("    can_id       : " + str(rec_frame.can_id & CANDO_ID_MASK))
    print("    can_dlc      : " + str(rec_frame.can_dlc))
    print("    data         : " + str(rec_frame.data))
    print("    timestamp_us : " + str(rec_frame.timestamp_us))

# 停止设备
dev_stop(dev_lists[0])

```

# 参考

zadig: https://zadig.akeo.ie/
libusb: http://www.libusb.org
libusb-win32: http://www.libusb.org/wiki/libusb-win32
Python: http://www.python.org