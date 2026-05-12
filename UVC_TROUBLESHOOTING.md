# UVC设备故障排查指南

## 问题：OBS找不到UVC摄像头

设备能被Windows识别，但OBS等软件找不到，通常是以下原因：

## 排查步骤

### 步骤1：检查设备管理器

1. 打开设备管理器（Win+X → 设备管理器）
2. 查找以下分类：
   - **照相机** 或 **Cameras**
   - **图像设备** 或 **Imaging devices**
   - **通用串行总线设备** 或 **Universal Serial Bus devices**

**期望结果**：
- 应该在"照相机"分类下看到"STM32 UVC Camera"
- 没有黄色感叹号或问号

**如果看到黄色感叹号**：
- 右键设备 → 属性 → 查看错误代码
- 可能需要更新驱动或重新枚举

### 步骤2：使用Windows相机应用测试

1. 按 Win+S 搜索"相机"
2. 打开相机应用
3. 点击右上角切换摄像头按钮

**期望结果**：
- 能在列表中看到"STM32 UVC Camera"
- 能切换到该摄像头并看到图像

**如果看不到**：
- 说明Windows没有正确识别为视频设备
- 需要检查UVC描述符

### 步骤3：使用VLC测试

1. 打开VLC播放器
2. 媒体 → 打开捕获设备
3. 捕获模式：DirectShow
4. 视频设备名称：下拉查看

**期望结果**：
- 列表中有"STM32 UVC Camera"
- 点击播放能看到视频

### 步骤4：检查OBS设置

1. 打开OBS Studio
2. 来源 → 添加 → 视频捕获设备
3. 创建新的源
4. 设备：下拉列表

**常见问题**：
- OBS可能缓存了设备列表
- 需要重启OBS
- 某些版本的OBS对UVC支持有问题

**解决方法**：
```
1. 完全关闭OBS
2. 拔掉USB设备
3. 重新插入USB
4. 重新打开OBS
5. 添加视频捕获设备
```

## 常见问题及解决方案

### 问题1：设备显示为"USB复合设备"

**原因**：UVC描述符配置不正确

**解决方案**：
检查设备描述符的bDeviceClass：
```c
// 应该是：
0xEF,  // bDeviceClass - Miscellaneous
0x02,  // bDeviceSubClass - Common Class
0x01,  // bDeviceProtocol - Interface Association
```

### 问题2：设备在"通用串行总线设备"下

**原因**：Windows没有识别为视频设备

**解决方案**：
1. 检查接口描述符
2. 确认Video Control和Video Streaming接口正确配置
3. 检查IAD（Interface Association Descriptor）

### 问题3：OBS能看到但无法打开

**错误信息**：
```
Failed to open video device
无法启动视频捕获
```

**原因**：
- 设备不支持OBS请求的格式
- Probe/Commit控制有问题
- 端点配置错误

**解决方案**：
检查UVC控制请求处理

### 问题4：设备频繁断开重连

**现象**：
- 设备管理器中设备消失又出现
- USB连接不稳定

**原因**：
- USB枚举失败
- 描述符返回错误
- 端点配置问题

## 使用USBPcap诊断

### 安装USBPcap

1. 下载Wireshark（包含USBPcap）
2. 安装时勾选USBPcap组件

### 捕获USB流量

1. 打开Wireshark
2. 选择USBPcap接口
3. 开始捕获
4. 插入STM32设备
5. 查看枚举过程

### 关键检查点

**设备描述符请求**：
```
GET_DESCRIPTOR Device
应该返回18字节的设备描述符
```

**配置描述符请求**：
```
GET_DESCRIPTOR Configuration
应该返回完整的配置描述符（165字节）
```

**接口请求**：
```
SET_INTERFACE
应该正确处理Alt Setting切换
```

**UVC控制请求**：
```
GET_CUR VS_PROBE_CONTROL
GET_MAX VS_PROBE_CONTROL
SET_CUR VS_PROBE_CONTROL
SET_CUR VS_COMMIT_CONTROL
```

## 修复UVC描述符问题

如果发现描述符有问题，需要修改`usbd_uvc.c`中的配置描述符。

### 检查配置描述符长度

当前定义：
```c
#define UVC_CONFIG_DESC_SIZE 165
```

实际计算：
```
配置描述符：9字节
IAD：8字节
VC接口：9字节
VC头部：13字节
输入终端：18字节
处理单元：11字节
输出终端：9字节
VS接口(Alt0)：9字节
VS输入头部：14字节
VS格式MJPEG：11字节
VS帧MJPEG：30字节
颜色匹配：6字节
VS接口(Alt1)：9字节
端点：7字节
─────────────
总计：163字节
```

**如果不匹配，需要调整！**

### 验证端点配置

检查端点描述符：
```c
// 端点描述符（同步IN）
0x07,                           // bLength
0x05,                           // bDescriptorType (ENDPOINT)
UVC_IN_EP,                      // bEndpointAddress (0x81 = IN)
0x05,                           // bmAttributes (Isochronous)
LOBYTE(UVC_PACKET_SIZE),        // wMaxPacketSize (512)
HIBYTE(UVC_PACKET_SIZE),
0x01                            // bInterval (1ms)
```

## 测试工具

### 推荐工具

1. **USBView**（微软官方）
   - 查看USB设备树
   - 查看描述符详情
   - 下载：Windows SDK

2. **USB Device Tree Viewer**
   - 更友好的界面
   - 详细的描述符信息
   - 免费下载

3. **Wireshark + USBPcap**
   - 捕获USB通信
   - 分析协议
   - 调试问题

4. **VLC Media Player**
   - 测试视频流
   - 支持多种格式
   - 显示详细错误

### 使用USBView检查

1. 运行USBView
2. 找到STM32设备
3. 检查以下信息：

**设备描述符**：
```
bDeviceClass: 0xEF (Miscellaneous)
bDeviceSubClass: 0x02
bDeviceProtocol: 0x01
idVendor: 0x0483
idProduct: 0x5740
```

**接口描述符**：
```
Interface 0: Video Control
  bInterfaceClass: 0x0E (Video)
  bInterfaceSubClass: 0x01 (Video Control)

Interface 1 Alt 0: Video Streaming
  bInterfaceClass: 0x0E (Video)
  bInterfaceSubClass: 0x02 (Video Streaming)
  
Interface 1 Alt 1: Video Streaming (Active)
  bInterfaceClass: 0x0E (Video)
  bInterfaceSubClass: 0x02 (Video Streaming)
  Endpoint: 0x81 (IN, Isochronous)
```

## 快速修复清单

- [ ] 设备在设备管理器的"照相机"分类下
- [ ] 没有黄色感叹号
- [ ] Windows相机应用能看到设备
- [ ] VLC能列出设备
- [ ] 重启OBS后能看到设备
- [ ] USBView显示正确的描述符
- [ ] 接口类为0x0E (Video)
- [ ] 有两个接口（VC和VS）
- [ ] 端点配置正确

## 如果所有方法都失败

### 最后的调试步骤

1. **启用USB调试输出**
   在`usbd_uvc.c`中添加调试信息：
   ```c
   printf("UVC Setup: bmRequest=0x%02X, bRequest=0x%02X\n", 
          req->bmRequest, req->bRequest);
   ```

2. **检查Probe/Commit响应**
   确保返回正确的参数

3. **简化配置**
   先实现最基本的UVC功能，逐步添加

4. **参考其他UVC设备**
   使用USBView查看商业UVC摄像头的描述符

5. **更新Windows驱动**
   某些Windows版本的UVC驱动有bug

## 联系支持

如果问题仍未解决，请提供：
1. 设备管理器截图
2. USBView的设备信息
3. Wireshark捕获的USB枚举过程
4. OBS的错误日志
