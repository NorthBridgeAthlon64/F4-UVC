# UVC摄像头系统 - 快速入门指南

## 5分钟快速开始

### 步骤1：硬件连接
1. 将OV2640摄像头模块连接到STM32F407开发板的DCMI接口
2. 连接LCD显示屏到FSMC接口
3. 使用USB线连接开发板的USB_FS接口到计算机

### 步骤2：编译和烧录
```bash
# 在STM32CubeIDE中
1. 打开项目
2. Project → Build All (Ctrl+B)
3. Run → Debug (F11) 或 Run (Ctrl+F11)
```

### 步骤3：验证运行
1. 观察LCD显示屏：
   ```
   UVC Camera System
   Initializing...
   OV2640 OK
   JPEG Mode: 320x240
   UVC Ready
   Capturing...
   ```

2. 检查LED：
   - LED1应该开始闪烁（表示正在捕获帧）

### 步骤4：在Windows上查看视频

#### 最简单的方法：使用Windows相机应用
1. 按 `Win + S` 搜索"相机"
2. 打开相机应用
3. 如果有多个摄像头，点击右上角切换到"STM32 UVC Camera"
4. 应该能看到实时视频流

#### 使用VLC（推荐用于调试）
1. 下载并安装VLC播放器
2. 打开VLC
3. 媒体 → 打开捕获设备
4. 捕获模式：DirectShow
5. 视频设备名称：STM32 UVC Camera
6. 显示更多选项 → 配置
   - 视频大小：320x240
   - 帧率：15
7. 点击"播放"

## 核心文件说明

### 新增的UVC相关文件
```
USB_DEVICE/App/
├── usbd_uvc.h          # UVC类驱动头文件
├── usbd_uvc.c          # UVC类驱动实现
├── usbd_uvc_if.h       # UVC接口头文件
└── usbd_uvc_if.c       # UVC接口实现

Core/Inc/
└── uvc_app.h           # UVC应用层头文件

Core/Src/
└── uvc_app.c           # UVC应用层实现（状态机）
```

### 修改的文件
```
USB_DEVICE/App/
├── usb_device.c        # 改用UVC类而非CDC类
└── usbd_desc.c         # 更新设备描述符

Core/Src/
├── main.c              # 集成UVC应用
└── dcmi.c              # 添加JPEG捕获支持

Core/Inc/
└── dcmi.h              # 添加JPEG函数声明
```

## 系统工作流程

```
[开机] → [初始化LCD] → [初始化OV2640] → [配置JPEG模式]
   ↓
[初始化USB UVC] → [启动DCMI捕获]
   ↓
[循环]
   ├─ [DCMI捕获JPEG帧]
   └─ [通过USB传输到PC]
```

**注意**：由于RAM限制，当前版本暂时禁用了本地LCD实时显示功能。
LCD仅显示初始化信息。视频流只通过USB传输到计算机。

如需恢复LCD显示，请参考 `MEMORY_OPTIMIZATION.md` 文档。

## 按键功能

- **KEY0**：停止视频捕获
- **KEY1**：启动视频捕获

## LED指示

- **LED0**：系统状态（启动后常亮）
- **LED1**：帧捕获指示（每捕获一帧切换一次）

## 常见问题快速解决

### 问题：编译错误
```
解决方案：
1. 确保所有新文件都已添加到项目
2. 检查包含路径是否正确
3. 清理项目后重新编译：Project → Clean
```

### 问题：USB设备无法识别
```
解决方案：
1. 检查USB线缆（需要数据线，不是充电线）
2. 尝试不同的USB端口
3. 在设备管理器中查看是否有未知设备
4. 重新插拔USB
```

### 问题：LCD显示"OV2640 ERR"
```
解决方案：
1. 检查OV2640连接
2. 确认SCCB（I2C）引脚连接正确
3. 检查摄像头供电
```

### 问题：Windows看不到摄像头
```
解决方案：
1. 打开设备管理器
2. 查看"照相机"或"图像设备"分类
3. 如果显示黄色感叹号，尝试更新驱动
4. 如果完全看不到，检查USB枚举是否成功
```

### 问题：视频流卡顿或帧率低
```
解决方案：
1. 降低分辨率（修改uvc_app.h中的宽高）
2. 改善光照条件（JPEG压缩率与光照有关）
3. 检查USB连接质量
```

## 性能参数

| 参数 | 值 |
|------|-----|
| 分辨率 | 320x240 |
| 帧率 | 10-15 fps |
| 格式 | MJPEG |
| USB速度 | Full Speed (12 Mbps) |
| 延迟 | 100-200ms |

## 调试技巧

### 1. 使用串口输出
连接串口（USART1）查看调试信息：
- 波特率：115200
- 数据位：8
- 停止位：1
- 校验：无

### 2. 使用USB分析工具
- Windows：USBPcap + Wireshark
- 查看USB描述符和数据传输

### 3. 检查状态机
在`uvc_app.c`中添加printf查看状态：
```c
printf("State: %d\n", uvc_app.state);
```

## 下一步

1. **优化图像质量**
   - 调整OV2640参数（曝光、白平衡等）
   - 参考`ov2640.c`中的配置函数

2. **提高性能**
   - 实现完整的JPEG解码器
   - 优化DMA配置
   - 使用双缓冲

3. **添加功能**
   - 图像增强
   - 运动检测
   - 人脸识别

## 技术支持

遇到问题？检查以下资源：
1. `README_UVC.md` - 完整文档
2. `UVC_IMPLEMENTATION_NOTES.md` - 实现细节
3. STM32 HAL库文档
4. USB.org UVC规范

## 版本信息

- 版本：1.0
- 日期：2026-02-23
- MCU：STM32F407ZGT6
- 摄像头：OV2640
- USB：Full Speed

---

**祝你使用愉快！如有问题，请参考详细文档或联系技术支持。**
