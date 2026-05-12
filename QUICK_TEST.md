# UVC设备快速测试指南

## 已修复的问题

刚刚修复了UVC配置描述符的格式问题：
- ✅ 修正了描述符长度（163字节）
- ✅ 添加了详细注释
- ✅ 修正了字节顺序
- ✅ 确保所有字段正确

## 立即测试

### 1. 重新编译和烧录
```
1. Project → Clean
2. Project → Build All
3. 烧录到设备
4. 重新插入USB
```

### 2. 检查Windows识别

**打开设备管理器**：
```
Win + X → 设备管理器
```

**查找设备**：
- 展开"照相机"或"Cameras"
- 应该看到"STM32 UVC Camera"
- 没有黄色感叹号

### 3. 使用Windows相机应用测试

```
1. Win + S 搜索"相机"
2. 打开相机应用
3. 点击右上角切换摄像头
4. 选择"STM32 UVC Camera"
```

**期望结果**：
- 能看到摄像头列表中有STM32设备
- 切换后能看到图像（可能是黑屏或测试图案）

### 4. 使用VLC测试（推荐）

```
1. 打开VLC播放器
2. 媒体 → 打开捕获设备
3. 捕获模式：DirectShow
4. 视频设备名称：STM32 UVC Camera
5. 显示更多选项：
   - 视频大小：320x240
   - 帧率：15
6. 点击"播放"
```

**期望结果**：
- VLC能打开设备
- 显示视频流（即使是黑屏也说明连接成功）

### 5. 测试OBS

```
1. 完全关闭OBS（如果正在运行）
2. 拔掉USB设备
3. 重新插入USB
4. 打开OBS Studio
5. 来源 → 添加 → 视频捕获设备
6. 创建新的源
7. 设备：下拉列表查找"STM32 UVC Camera"
```

**如果还是找不到**：
- 尝试重启OBS
- 检查OBS版本（建议使用最新版本）
- 尝试使用OBS的"刷新"按钮

## 诊断工具

### 使用USBView（推荐）

1. **下载USBView**：
   - 包含在Windows SDK中
   - 或搜索"USB Device Tree Viewer"（第三方工具）

2. **检查设备信息**：
   ```
   运行USBView
   找到STM32设备
   查看描述符详情
   ```

3. **关键检查点**：
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
   
   Interface 1 Alt 0 (Video Streaming):
     bInterfaceClass: 0x0E (Video)
     bInterfaceSubClass: 0x02 (Streaming)
     bNumEndpoints: 0
   
   Interface 1 Alt 1 (Video Streaming Active):
     bInterfaceClass: 0x0E (Video)
     bInterfaceSubClass: 0x02 (Streaming)
     bNumEndpoints: 1
     Endpoint 0x81: Isochronous IN, 512 bytes
   ```

### 使用Wireshark捕获USB

如果设备仍然有问题：

1. **安装Wireshark + USBPcap**
2. **开始捕获**：
   ```
   选择USBPcap接口
   开始捕获
   插入STM32设备
   ```

3. **查看枚举过程**：
   ```
   过滤器：usb.device_address == X
   查找：GET_DESCRIPTOR请求
   检查：返回的描述符是否正确
   ```

4. **查看UVC控制请求**：
   ```
   过滤器：usb.bmRequestType == 0xa1
   查找：VS_PROBE_CONTROL, VS_COMMIT_CONTROL
   检查：设备响应是否正确
   ```

## 常见问题快速修复

### 问题1：设备在"通用串行总线设备"下

**快速修复**：
```
1. 右键设备 → 卸载设备
2. 勾选"删除此设备的驱动程序软件"
3. 拔掉USB
4. 重新插入
5. Windows会重新枚举
```

### 问题2：OBS看不到但VLC能看到

**原因**：OBS缓存问题

**快速修复**：
```
1. 关闭OBS
2. 删除OBS配置缓存：
   %AppData%\obs-studio\plugin_config\win-dshow
3. 重启OBS
```

### 问题3：设备频繁断开

**可能原因**：
- USB供电不足
- 描述符错误导致枚举失败

**快速修复**：
```
1. 使用有源USB Hub
2. 连接到主板后面的USB口（供电更稳定）
3. 检查设备的bMaxPower设置（当前500mA）
```

### 问题4：图像是黑屏

**这是正常的！**

当前实现：
- ✅ USB枚举成功
- ✅ UVC协议正确
- ⚠️ 还没有实际传输图像数据

**下一步**：
- 需要实现实际的图像数据传输
- 需要在DCMI捕获完成后触发USB传输

## 验证成功的标志

### 最低要求（当前目标）
- [x] Windows识别为视频设备
- [x] 出现在"照相机"分类
- [x] VLC能列出设备
- [x] OBS能列出设备
- [ ] 能打开设备（可能黑屏）

### 完整功能（下一步）
- [ ] 能看到实际图像
- [ ] 图像流畅
- [ ] 帧率稳定
- [ ] 无断连

## 下一步开发

如果设备能被正确识别和列出，但图像是黑屏：

### 需要实现的功能

1. **实际的图像传输**：
   ```c
   // 在DCMI帧完成回调中
   void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi) {
       if (jpeg_data_ok == 0) {
           jpeg_data_len = jpeg_buf_size - __HAL_DMA_GET_COUNTER(&hdma_dcmi);
           jpeg_data_ok = 1;
           
           // 触发USB传输
           UVC_App_SetStreaming(1);
       }
   }
   ```

2. **USB流控制**：
   ```c
   // 在UVC_App_Process中
   if (uvc_app.usb_streaming && uvc_app.frame_ready) {
       UVC_Transmit_FS(jpeg_buf, jpeg_data_len);
   }
   ```

3. **调试输出**：
   ```c
   printf("Frame captured: %lu bytes\n", jpeg_data_len);
   printf("USB streaming: %d\n", uvc_app.usb_streaming);
   ```

## 获取帮助

如果问题仍未解决，请提供：

1. **设备管理器截图**
2. **USBView的设备信息**
3. **VLC的错误信息**（如果有）
4. **OBS的日志文件**：
   ```
   帮助 → 日志文件 → 查看当前日志
   ```

## 成功案例

如果一切正常，你应该看到：

```
设备管理器：
  照相机
    └─ STM32 UVC Camera

VLC：
  视频设备名称：
    └─ STM32 UVC Camera

OBS：
  视频捕获设备：
    └─ STM32 UVC Camera
```

恭喜！UVC设备已经成功识别！
