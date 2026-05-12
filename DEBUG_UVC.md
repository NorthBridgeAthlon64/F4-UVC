# UVC调试指南

## 当前状态

✅ Windows已识别设备为UVC摄像头
✅ 加载了usbvideo.inf驱动
❌ 应用程序无法打开设备

## 问题分析

设备事件显示：
```
驱动程序名称： usbvideo.inf
类 GUID： {ca3e7ab9-b4c3-4ae6-8251-579ef933890f}
匹配设备 ID： USB\Class_0e
```

这说明Windows正确识别了Video类设备（Class 0x0E），但应用程序打不开。

## 可能的原因

### 1. Probe/Commit控制请求未正确响应
应用程序在打开设备时会发送Probe和Commit请求，如果设备不响应或响应错误，应用程序会失败。

### 2. 没有实际的视频数据传输
即使控制请求正确，如果没有数据传输，应用程序也会超时。

### 3. 端点配置问题
同步端点配置可能有问题。

## 已实施的修复

### 修复1：完善Probe/Commit处理
- ✅ 正确解析Control Selector
- ✅ 支持GET_CUR, GET_MIN, GET_MAX, GET_DEF
- ✅ 支持GET_INFO和GET_RES
- ✅ 正确处理SET_INTERFACE

### 修复2：启动视频流
- ✅ 在SET_INTERFACE(1)时启动流传输
- ✅ 通知应用层开始传输数据

## 测试步骤

### 1. 重新编译和烧录
```
1. Project → Clean
2. Project → Build All
3. 烧录到设备
4. 重新插入USB
```

### 2. 添加调试输出

在`usbd_uvc.c`的Setup函数中添加：

```c
// 在文件开头添加
#include <stdio.h>

// 在USBD_UVC_Setup函数开始处添加
printf("UVC Setup: bmRequest=0x%02X, bRequest=0x%02X, wValue=0x%04X, wIndex=0x%04X\n",
       req->bmRequest, req->bRequest, req->wValue, req->wIndex);
```

### 3. 监控串口输出

连接串口（USART1, 115200bps），观察：
- 设备枚举时的请求
- Probe/Commit控制请求
- SET_INTERFACE请求

### 4. 使用VLC测试

VLC通常会显示更详细的错误信息：

```
1. 打开VLC
2. 工具 → 消息（Ctrl+M）
3. 详细程度：调试
4. 媒体 → 打开捕获设备
5. 选择STM32 UVC Camera
6. 点击播放
7. 查看消息窗口的错误
```

## 预期的控制请求序列

正常的UVC设备打开流程：

```
1. GET_CUR VS_PROBE_CONTROL
   → 设备返回当前probe参数

2. SET_CUR VS_PROBE_CONTROL
   ← 主机设置probe参数
   → 设备接受并保存

3. GET_CUR VS_PROBE_CONTROL
   → 设备返回协商后的参数

4. GET_MAX VS_PROBE_CONTROL
   → 设备返回最大值

5. GET_MIN VS_PROBE_CONTROL
   → 设备返回最小值

6. SET_CUR VS_COMMIT_CONTROL
   ← 主机提交最终参数
   → 设备接受

7. SET_INTERFACE(1)
   ← 主机切换到Alt Setting 1
   → 设备开始流传输
```

## 检查Probe/Commit参数

当前的默认参数：

```c
huvc->probe.bmHint = 0x0001;
huvc->probe.bFormatIndex = 0x01;        // MJPEG
huvc->probe.bFrameIndex = 0x01;         // 320x240
huvc->probe.dwFrameInterval = 666666;   // 15fps
huvc->probe.wKeyFrameRate = 0;
huvc->probe.wPFrameRate = 0;
huvc->probe.wCompQuality = 50;
huvc->probe.wCompWindowSize = 0;
huvc->probe.wDelay = 0;
huvc->probe.dwMaxVideoFrameSize = 40960;     // 40KB
huvc->probe.dwMaxPayloadTransferSize = 512;  // 512 bytes
```

这些参数必须与描述符中的定义匹配！

## 常见错误及解决

### 错误1：应用程序显示"设备正忙"

**原因**：另一个应用程序已经打开了设备

**解决**：
```
1. 关闭所有可能使用摄像头的应用
2. 任务管理器 → 结束所有相机相关进程
3. 重新插入USB
```

### 错误2：应用程序显示"无法启动视频捕获"

**原因**：Probe/Commit失败或没有数据传输

**解决**：
```
1. 检查串口输出，确认收到Probe/Commit请求
2. 确认SET_INTERFACE(1)被调用
3. 确认streaming标志被设置为1
```

### 错误3：应用程序打开后立即关闭

**原因**：没有收到视频数据，超时

**解决**：
```
1. 确认DCMI正在捕获数据
2. 确认UVC_App_Process()在主循环中被调用
3. 确认USB传输函数被调用
```

## 使用Wireshark诊断

### 捕获USB流量

1. 安装Wireshark + USBPcap
2. 选择USBPcap接口
3. 开始捕获
4. 打开VLC并尝试连接设备
5. 停止捕获

### 分析关键请求

**过滤器**：
```
usb.bmRequestType == 0xa1  // GET请求
usb.bmRequestType == 0x21  // SET请求
```

**查找**：
```
GET_CUR VS_PROBE_CONTROL (bRequest=0x81, wValue=0x0100)
SET_CUR VS_PROBE_CONTROL (bRequest=0x01, wValue=0x0100)
SET_CUR VS_COMMIT_CONTROL (bRequest=0x01, wValue=0x0200)
SET_INTERFACE (bRequest=0x0B, wValue=0x0001)
```

**检查响应**：
- 每个GET请求应该有对应的响应
- 响应数据长度应该是26字节（Probe/Commit结构）
- SET请求应该返回成功（无错误）

## 数据传输检查

### 确认DCMI捕获

在`main.c`的DCMI回调中添加：

```c
void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi) {
    if (jpeg_data_ok == 0) {
        jpeg_data_len = jpeg_buf_size - __HAL_DMA_GET_COUNTER(&hdma_dcmi_main);
        jpeg_data_ok = 1;
        
        printf("DCMI Frame: %lu bytes\n", jpeg_data_len);
        HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    }
}
```

### 确认USB传输

在`uvc_app.c`的传输代码中添加：

```c
case UVC_STATE_TRANSMITTING:
    if (uvc_app.usb_streaming && uvc_app.frame_ready) {
        printf("USB TX: %lu bytes\n", uvc_app.jpeg_size);
        if (UVC_Transmit_FS(uvc_app.jpeg_buffer, uvc_app.jpeg_size) == USBD_OK) {
            printf("USB TX OK\n");
            uvc_app.frame_ready = 0;
        } else {
            printf("USB TX FAIL\n");
        }
    }
    break;
```

## 成功的标志

如果一切正常，你应该看到：

### 串口输出
```
OV2640 OK
JPEG Mode: 320x240
UVC Ready
UVC Setup: bmRequest=0xA1, bRequest=0x81, wValue=0x0100, wIndex=0x0001
UVC Setup: bmRequest=0x21, bRequest=0x01, wValue=0x0100, wIndex=0x0001
UVC Setup: bmRequest=0xA1, bRequest=0x81, wValue=0x0100, wIndex=0x0001
UVC Setup: bmRequest=0x21, bRequest=0x01, wValue=0x0200, wIndex=0x0001
UVC Setup: bmRequest=0x01, bRequest=0x0B, wValue=0x0001, wIndex=0x0001
Streaming started
DCMI Frame: 15234 bytes
USB TX: 15234 bytes
USB TX OK
```

### VLC显示
```
打开设备成功
开始接收数据
显示视频（可能是黑屏或图像）
```

### LED指示
```
LED1应该闪烁（每帧切换）
```

## 下一步

如果设备能被打开但图像是黑屏：

1. **检查OV2640配置**
   - 确认JPEG模式正确
   - 检查曝光和增益设置

2. **检查JPEG数据**
   - 验证JPEG数据的有效性
   - 检查JPEG头部（应该以0xFFD8开始）

3. **优化传输**
   - 调整帧率
   - 优化USB传输

## 获取帮助

如果问题仍未解决，请提供：

1. **串口输出**（完整的启动和连接过程）
2. **VLC错误信息**（工具→消息，详细程度：调试）
3. **Wireshark捕获**（USB枚举和控制请求）
4. **LED状态**（是否闪烁）
