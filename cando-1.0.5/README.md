cando 手册
===
# 一 内部常量

## CAN 工作模式标志位
`CANDO_MODE_NORMAL` 正常工作模式
`CANDO_MODE_LISTEN_ONLY` CAN 侦听模式
`CANDO_MODE_LOOP_BACK` CAN 回环模式
`CANDO_MODE_ONE_SHOT` CAN 发送失败后不自动重新发送模式

## CAN ID 标志位
`CANDO_ID_MASK`用于和 Frame.can_id 按位`与`运算，得到 can id
`CANDO_ID_EXTENDED` 用于和 Frame.can_id 按位`与`运算，判断是否为扩展帧
`CANDO_ID_RTR` 用于和 Frame.can_id 按位`与`运算，判断是否为远程帧
`CANDO_ID_ERR` 用于和 Frame.can_id 按位`与`运算，判断是否错误帧

## CAN 错误标志位
`CAN_ERR_BUSOFF` 离线错误
`CAN_ERR_RX_TX_WARNING` 发送或接收错误报警
`CAN_ERR_RX_TX_PASSIVE` 发送或接收被动错误
`CAN_ERR_OVERLOAD` 总线过载
`CAN_ERR_STUFF` 填充规则错误
`CAN_ERR_FORM` 格式错误
`CAN_ERR_ACK` 应答错误
`CAN_ERR_BIT_RECESSIVE` 位隐性错误
`CAN_ERR_BIT_DOMINANT` 位显性错误
`CAN_ERR_CRC` CRC校验错误

# 二 can 数据帧类
`Class Frame` 内部只有以下成员变量：
* echo_id :  判断是否为发送的 ECHO 帧，ECHO 帧时值为 0，否则为 0xFFFFFFFF
* can_id : 帧ID，用于判断帧类型，和`CANDO_ID_MASK`按位`与`运算得到can id
* can_dlc :  can数据长度，0~8
* channel :  用于内部通信，用户无需理会
* flags : 用于内部通信，用户无需理会
* reserved : 暂未使用，用户无需理会
* data :  can数据，类型为长度为8的列表
* timestamp_us : can 数据时间戳，单位为 us

# 三 内部函数
`list_scan()` 
    扫描当前连接到电脑的所有设备
    :return: 设备句柄的列表

`dev_start(dev, mode=0)` 
    启动设备，启动后Cando 或 Cando_pro 上的蓝灯亮起
    :param dev: 设备句柄
    :param mode: 启动模式标志，可以是 ***CAN 工作模式标志位***  的任意按位`或`运算组合，默认为正常工作模式
    :return: 无

`dev_stop(dev)`
    关闭设备，关闭后Cando 或 Cando_pro 上的蓝灯熄灭
    :param dev: 设备句柄
    :return: 无

`dev_set_timing(dev, prop_seg, phase_seg1, phase_seg2, sjw, brp)`
    设置 CAN 的波特率和采样点
    :param dev: 设备句柄
    :param prop_seg: propagation Segment (固定为 1)
    :param phase_seg1: phase segment 1 (1~15)
    :param phase_seg2: phase segment 2 (1~8)
    :param sjw: synchronization segment (1~4)
    :param brp: CAN时钟分频 (1~1024)，内部CAN时钟为 48MHz 
    :return: 无

`dev_get_serial_number_str(dev)`
    获取设备序列号字符串
    :param dev: 设备句柄
    :return: 设备序列号字符串

`dev_get_dev_info_str(dev)`
    获取设备固件、硬件版本信息字符串
    :param dev: 设备句柄
    :return: 设备固件、硬件版本信息字符串

`parse_err_frame(frame)`
    解析错误帧的错误信息
    :param frame: 错误帧
    :return: (错误代码，发送错误计数，接收错误计数)，错误代码参考 ***CAN 错误标志位***

`dev_frame_send(dev, frame)`
    发送帧数据
    :param dev: 设备句柄
    :param frame: 数据帧类，参考 ***can 数据帧类***
    :return: 无

`dev_frame_read(dev, frame, timeout_ms)`
    读取帧数据，读取到的数据帧将会赋值到传入的 frame 中
    :param dev: 设备句柄
    :param frame: 数据帧类，参考 ***can 数据帧类***
    :param timeout_ms: 读取超时时间，单位为 ms
    :return: 如果读取成功返回 True 否则 返回 False

# 四 其他资料

***/docs/tutorial.md***   使用教程

***/docs/faq.md***		   常见问题