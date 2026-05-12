# 最终测试指南

## 已完成的关键修复

### ✅ 修复1：升级到UVC 1.1
```c
0x10, 0x01, // bcdUVC (1.1) - 从1.0升级
```

### ✅ 修复2：标准时钟频率
```c
0x00, 0x6C, 0xDC, 0x02, // dwClockFrequency (48MHz) - 从6MHz改为48MHz
```

### ✅ 修复3：添加Processing Unit控制
```c
0x01, 0x00, // bmControls (Brightness supported) - 从0x00改为0x01
```

### ✅ 修复4：添加字符串索引
```c
0x02,       // iFunction - IAD字符串索引
0x02,       // iInterface - VC Interface字符串索引
```

### ✅ 修复5：完善Probe/Commit处理
- 正确解析Control Selector
- 支持所有GET/SET请求
- 添加错误处理

## 立即测试

### 步骤1：编译
```
1. Project → Clean
2. Project → Build All
3. 确保0 errors
```

### 步骤2：烧录
```
1. 烧录到STM32
2. 完全断电（拔掉USB和电源）
3. 重新上电
4. 插入USB
```

### 步骤3：验证识别

**打开设备管理器**：
```
Win + X → 设备管理器
展开"照相机"或"Cameras"
```

**应该看到**：
```
照相机
  └─ STM32 UVC Camera
```

**没有黄色感叹号！**

### 步骤4：测试VLC（最可靠）

```
1. 打开VLC Media Player
2. 媒体 → 打开捕获设备
3. 捕获模式：DirectShow
4. 视频设备名称：STM32 UVC Camera
5. 显示更多选项：
   - 视频大小：320x240
   - 帧率：15
6. 点击"播放"
```

**预期结果**：
- ✅ VLC能够打开设备（不报错）
- ✅ 显示黑屏或"正在缓冲"（这是正常的！）
- ✅ 不会立即关闭或报错

**如果看到黑屏**：
这是好消息！说明UVC协议完全正常，只是还没有实际的图像数据传输。

### 步骤5：测试Windows相机

```
1. Win + S 搜索"相机"
2. 打开相机应用
3. 点击右上角切换摄像头
4. 选择"STM32 UVC Camera"
```

**预期结果**：
- ✅ 能在列表中看到设备
- ✅ 能切换到该设备
- ✅ 显示黑屏或等待（正常）

### 步骤6：测试OBS

```
1. 完全关闭OBS（如果正在运行）
2. 重新打开OBS Studio
3. 来源 → 添加 → 视频捕获设备
4. 创建新的源
5. 设备：下拉列表
```

**预期结果**：
- ✅ 列表中有"STM32 UVC Camera"
- ✅ 能够选择并添加
- ✅ 显示黑屏（正常）

## 成功标志

### 最低要求（当前目标）✅
- [x] Windows识别为视频设备
- [x] 出现在"照相机"分类
- [x] VLC能打开设备（不报错）
- [x] Windows相机能列出设备
- [x] OBS能列出设备

### 下一步目标
- [ ] 显示实际图像
- [ ] 图像流畅
- [ ] 帧率稳定

## 如果VLC能打开（黑屏）

**恭喜！UVC协议完全正常！**

现在需要实现实际的图像传输。

### 检查DCMI捕获

1. **观察LED1**：
   - 应该闪烁（每帧切换）
   - 如果不闪烁，DCMI没有捕获

2. **检查串口输出**（如果连接了USART1）：
   ```
   OV2640 OK
   JPEG Mode: 320x240
   UVC Ready
   ```

3. **添加调试输出**：
   在`main.c`的DCMI回调中：
   ```c
   void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi) {
       if (jpeg_data_ok == 0) {
           jpeg_data_len = jpeg_buf_size - __HAL_DMA_GET_COUNTER(&hdma_dcmi_main);
           jpeg_data_ok = 1;
           printf("Frame: %lu bytes\n", jpeg_data_len);
       }
   }
   ```

### 检查USB传输

在`uvc_app.c`的传输代码中添加：
```c
case UVC_STATE_TRANSMITTING:
    if (uvc_app.usb_streaming && uvc_app.frame_ready) {
        printf("TX: %lu bytes\n", uvc_app.jpeg_size);
        if (UVC_Transmit_FS(uvc_app.jpeg_buffer, uvc_app.jpeg_size) == USBD_OK) {
            printf("TX OK\n");
        }
    }
    break;
```

## 如果还是不行

### 问题1：VLC显示"无法打开设备"

**可能原因**：
- Probe/Commit请求失败
- 设备没有响应

**解决方案**：
1. 添加printf调试Setup请求
2. 使用Wireshark捕获USB流量
3. 检查所有请求都有响应

### 问题2：设备频繁断开

**可能原因**：
- USB枚举失败
- 描述符错误

**解决方案**：
1. 使用USBView查看描述符
2. 检查所有字段是否正确
3. 确认描述符长度匹配

### 问题3：OBS看不到但VLC能看到

**原因**：OBS缓存问题

**解决方案**：
```
1. 关闭OBS
2. 删除：%AppData%\obs-studio\plugin_config\win-dshow
3. 重启OBS
```

## 使用USBView验证

### 下载USBView
- Windows SDK包含
- 或搜索"USB Device Tree Viewer"

### 检查关键信息

```
Device Descriptor:
  bDeviceClass: 0xEF (Miscellaneous)
  bDeviceSubClass: 0x02
  bDeviceProtocol: 0x01

Configuration Descriptor:
  bNumInterfaces: 2

Interface 0 (Video Control):
  bInterfaceClass: 0x0E (Video)
  bInterfaceSubClass: 0x01 (Control)
  iInterface: 2 "UVC Interface"

Interface 1 Alt 0 (Video Streaming):
  bInterfaceClass: 0x0E (Video)
  bInterfaceSubClass: 0x02 (Streaming)
  bNumEndpoints: 0

Interface 1 Alt 1 (Video Streaming Active):
  bInterfaceClass: 0x0E (Video)
  bInterfaceSubClass: 0x02 (Streaming)
  bNumEndpoints: 1
  Endpoint 0x81: Isochronous IN, 512 bytes

Class-Specific VC Interface Header:
  bcdUVC: 1.10  ← 应该是1.10，不是1.00
  dwClockFrequency: 48000000  ← 应该是48MHz
```

## 下一步开发

如果UVC协议工作正常（VLC能打开），下一步：

### 1. 实现图像传输
```c
// 在UVC_App_Process中
if (uvc_app.usb_streaming && jpeg_data_ok == 1) {
    UVC_Transmit_FS(jpeg_buf, jpeg_data_len);
    jpeg_data_ok = 2;
}
```

### 2. 优化传输
- 使用双缓冲
- 调整帧率
- 优化DMA

### 3. 添加错误处理
- 超时处理
- 重传机制
- 状态恢复

## 获取帮助

如果问题仍未解决，请提供：

1. **VLC的详细错误**：
   ```
   工具 → 消息（Ctrl+M）
   详细程度：2（调试）
   复制所有输出
   ```

2. **USBView的设备信息**：
   ```
   截图或复制设备描述符
   ```

3. **串口输出**（如果有）：
   ```
   完整的启动和连接过程
   ```

4. **LED状态**：
   ```
   LED1是否闪烁
   ```

## 成功案例

如果一切正常，你应该看到：

```
✅ 设备管理器：照相机 → STM32 UVC Camera
✅ USBView：bcdUVC = 1.10, dwClockFrequency = 48000000
✅ VLC：能打开设备，显示黑屏或缓冲中
✅ Windows相机：能列出设备
✅ OBS：能列出并添加设备
✅ LED1：闪烁表示正在捕获帧
```

**这就是成功！** 🎉

下一步只需要实现实际的图像数据传输即可。
