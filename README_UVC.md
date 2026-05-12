# STM32 UVC摄像头系统

## 项目概述

本项目实现了一个完整的UVC（USB Video Class）类设备功能，具有以下特性：

- OV2640图像传感器配置为JPEG模式
- 320×240分辨率图像采集
- 通过USB_FS接口传输JPEG图像数据到计算机
- 本地JPEG解码为RGB565格式
- 实时显示在LCD屏幕上

## 系统架构

### 硬件组件
- STM32F407ZGT6微控制器
- OV2640摄像头模块
- ILI93xx LCD显示屏
- USB Full Speed接口

### 软件模块

1. **UVC设备驱动** (`USB_DEVICE/App/usbd_uvc.c/h`)
   - 实现USB Video Class标准协议
   - 支持MJPEG格式视频流
   - 配置为320x240 @ 15fps

2. **UVC接口层** (`USB_DEVICE/App/usbd_uvc_if.c/h`)
   - USB设备与应用层的接口
   - 提供帧传输API

3. **UVC应用层** (`Core/Src/uvc_app.c`, `Core/Inc/uvc_app.h`)
   - 状态机管理
   - JPEG捕获、解码、显示、传输流程控制

4. **DCMI驱动** (`Core/Src/dcmi.c`, `Core/Inc/dcmi.h`)
   - 配置DCMI接口用于JPEG捕获
   - DMA传输管理

5. **OV2640驱动** (`Core/Src/ov2640.c`, `Core/Inc/ov2640.h`)
   - 摄像头初始化和配置
   - JPEG模式设置
   - 分辨率配置

## 工作流程

```
┌─────────────┐
│  OV2640     │ JPEG模式
│  摄像头     │ 320x240
└──────┬──────┘
       │ DCMI接口
       ▼
┌─────────────┐
│   DCMI+DMA  │ 捕获JPEG数据
│   控制器    │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  JPEG缓冲区 │ 40KB（共享）
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  USB传输    │
│  (UVC协议)  │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  计算机     │
│  (主机)     │
└─────────────┘
```

**内存优化说明**：
- 当前版本优化了内存使用，移除了重复缓冲区
- 本地LCD显示功能暂时禁用以节省RAM
- 所有视频流通过USB传输到计算机查看
- 详见 `MEMORY_OPTIMIZATION.md`

## 使用说明

### 编译和烧录

1. 使用STM32CubeIDE打开项目
2. 编译项目（Build Project）
3. 烧录到STM32F407开发板

### 运行

1. 上电后系统自动初始化：
   - LCD显示初始化信息
   - OV2640配置为JPEG模式
   - USB UVC设备枚举

2. 连接USB到计算机：
   - Windows会识别为"STM32 UVC Camera"
   - 可使用任何支持UVC的软件查看视频流
   - 推荐软件：VLC、OBS、Windows Camera应用

3. 按键控制：
   - KEY0：停止视频捕获
   - KEY1：启动视频捕获

4. LED指示：
   - LED0：系统状态
   - LED1：帧捕获指示（每帧切换）

### 在Windows上查看视频流

#### 方法1：使用Windows Camera应用
1. 打开"相机"应用
2. 选择"STM32 UVC Camera"
3. 查看实时视频流

#### 方法2：使用VLC播放器
1. 打开VLC
2. 媒体 → 打开捕获设备
3. 捕获模式：DirectShow
4. 视频设备名称：选择"STM32 UVC Camera"
5. 点击播放

#### 方法3：使用OBS Studio
1. 添加视频捕获设备源
2. 选择"STM32 UVC Camera"
3. 配置分辨率为320x240

## 配置参数

### 视频参数（`Core/Inc/uvc_app.h`）
```c
#define UVC_FRAME_WIDTH         320
#define UVC_FRAME_HEIGHT        240
#define UVC_JPEG_BUFFER_SIZE    (40 * 1024)
```

### USB参数（`USB_DEVICE/App/usbd_uvc.h`）
```c
#define UVC_WIDTH               320
#define UVC_HEIGHT              240
#define UVC_FPS                 15
#define UVC_MAX_FRAME_SIZE      (40 * 1024)
#define UVC_PACKET_SIZE         512
```

## 性能指标

- 视频分辨率：320x240
- 帧率：约15fps（取决于JPEG压缩率和USB带宽）
- JPEG质量：中等（由OV2640硬件编码器决定）
- USB带宽：Full Speed (12 Mbps)
- 延迟：约100-200ms（捕获+传输+显示）

## 故障排除

### 问题1：USB设备无法识别
- 检查USB连接
- 确认USB驱动正确安装
- 查看设备管理器中的USB设备

### 问题2：无视频输出
- 检查OV2640连接
- 确认摄像头初始化成功（LCD显示"OV2640 OK"）
- 按KEY1启动捕获

### 问题3：图像质量差
- 调整OV2640曝光和白平衡设置
- 检查光照条件
- 可在`ov2640.c`中调整图像参数

### 问题4：帧率低
- JPEG压缩率影响帧大小
- USB Full Speed带宽限制
- 可降低分辨率提高帧率

## 扩展功能

### 支持更高分辨率
修改`uvc_app.h`中的分辨率定义，并相应调整缓冲区大小：
```c
#define UVC_FRAME_WIDTH         640
#define UVC_FRAME_HEIGHT        480
#define UVC_JPEG_BUFFER_SIZE    (80 * 1024)
```

### 添加图像处理
在`uvc_app.c`的`UVC_STATE_DECODING`状态中添加图像处理算法。

### 支持多种格式
可扩展UVC描述符支持YUV422等未压缩格式。

## 技术细节

### DCMI配置
- JPEG模式启用
- DMA正常模式（非循环）
- 快照模式捕获单帧

### USB描述符
- 设备类：0xEF（Miscellaneous）
- 接口类：0x0E（Video）
- 子类：0x01（Video Control）、0x02（Video Streaming）
- 格式：MJPEG

### 内存使用
- JPEG缓冲区：40KB
- RGB565缓冲区：150KB (320x240x2)
- USB传输缓冲区：512字节

## 参考资料

- [USB Video Class 1.1 Specification](https://www.usb.org/document-library/video-class-v11-document-set)
- [OV2640 Datasheet](https://www.ovt.com/sensors/OV2640)
- [STM32F4 Reference Manual](https://www.st.com/resource/en/reference_manual/dm00031020.pdf)

## 许可证

本项目基于STM32 HAL库开发，遵循BSD 3-Clause许可证。

## 作者

STM32 UVC Camera System
版本：1.0
日期：2026-02-23
