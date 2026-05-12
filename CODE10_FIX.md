# 代码10错误修复

## 问题

```
该设备无法启动。 (代码 10)
{操作失败}请求的操作不成功。
```

## 原因

代码10错误通常是因为：
1. **字符串描述符索引无效**：配置描述符引用了不存在的字符串
2. **描述符格式错误**：某些字段不符合规范
3. **设备响应超时**：设备没有正确响应枚举请求

## 已修复

### 修复：移除字符串索引引用

将IAD和VC Interface的字符串索引改回0：

```c
// IAD
0x00,       // iFunction (不使用字符串)

// VC Interface  
0x00,       // iInterface (不使用字符串)
```

这样可以避免字符串描述符的问题。

## 立即测试

### 1. 重新编译
```
Project → Clean
Project → Build All
```

### 2. 完全重置设备
```
1. 拔掉USB
2. 断开STM32电源
3. 等待5秒
4. 重新上电
5. 烧录新固件
6. 拔掉USB
7. 重新插入USB
```

### 3. 清除Windows驱动缓存

**方法1：设备管理器**
```
1. 打开设备管理器
2. 查看 → 显示隐藏的设备
3. 找到STM32设备（可能在"其他设备"或"通用串行总线设备"下）
4. 右键 → 卸载设备
5. 勾选"删除此设备的驱动程序软件"
6. 点击"卸载"
7. 拔掉USB
8. 重新插入USB
```

**方法2：使用USBDeview**
```
1. 下载USBDeview工具
2. 运行（管理员权限）
3. 找到STM32设备
4. 右键 → Uninstall Selected Devices
5. 重新插入USB
```

### 4. 验证

**设备管理器应该显示**：
```
照相机
  └─ STM32 UVC Camera
```

**没有黄色感叹号或问号！**

## 如果还是代码10

### 检查1：验证描述符

使用USBView或USB Device Tree Viewer查看设备：

**期望看到**：
```
Device Descriptor:
  bDeviceClass: 0xEF
  bDeviceSubClass: 0x02
  bDeviceProtocol: 0x01
  idVendor: 0x0483
  idProduct: 0x5740

Configuration Descriptor:
  bNumInterfaces: 2
  
Interface 0:
  bInterfaceClass: 0x0E (Video)
  bInterfaceSubClass: 0x01 (Control)
  
Interface 1:
  bInterfaceClass: 0x0E (Video)
  bInterfaceSubClass: 0x02 (Streaming)
```

**如果看不到这些信息**：
- 设备枚举失败
- 描述符有严重错误

### 检查2：使用Wireshark捕获

```
1. 安装Wireshark + USBPcap
2. 选择USBPcap接口
3. 开始捕获
4. 插入STM32设备
5. 查看枚举过程
```

**查找错误**：
- GET_DESCRIPTOR请求是否成功
- 设备是否返回了正确的描述符
- 是否有STALL或NAK

### 检查3：回退到最简配置

如果问题持续，可以尝试最简单的配置：

**临时修改usbd_desc.c**：

```c
// 将设备类改为0x00（在设备级别不指定类）
__ALIGN_BEGIN uint8_t USBD_FS_DeviceDesc[USB_LEN_DEV_DESC] __ALIGN_END =
{
  0x12,                       /*bLength */
  USB_DESC_TYPE_DEVICE,       /*bDescriptorType*/
  0x00,                       /*bcdUSB */
  0x02,
  0x00,                       /*bDeviceClass - 改为0x00*/
  0x00,                       /*bDeviceSubClass - 改为0x00*/
  0x00,                       /*bDeviceProtocol - 改为0x00*/
  // ... 其余不变
};
```

这样类信息只在接口级别指定。

## 常见原因和解决方案

### 原因1：字符串描述符问题

**症状**：代码10，USBView显示枚举失败

**解决**：
- 将所有iFunction、iInterface改为0
- 或正确实现字符串描述符

### 原因2：描述符长度错误

**症状**：代码10，设备频繁断开

**解决**：
- 验证配置描述符总长度
- 确保所有子描述符长度正确

### 原因3：端点配置错误

**症状**：代码10，枚举后立即失败

**解决**：
- 检查端点地址（0x81）
- 检查端点类型（同步）
- 检查最大包大小（512）

### 原因4：电源问题

**症状**：间歇性代码10

**解决**：
- 使用有源USB Hub
- 连接到主板后面的USB口
- 检查bMaxPower设置（当前500mA）

## 调试步骤

### 1. 添加串口调试

在`usbd_uvc.c`的Init函数中添加：

```c
static uint8_t USBD_UVC_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
    printf("UVC Init called\n");
    
    // ... 原有代码
    
    printf("UVC Init complete\n");
    return USBD_OK;
}
```

### 2. 检查Setup请求

在Setup函数中添加：

```c
static uint8_t USBD_UVC_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req) {
    printf("Setup: bmReq=0x%02X bReq=0x%02X\n", req->bmRequest, req->bRequest);
    
    // ... 原有代码
}
```

### 3. 观察LED

如果设备正常初始化：
- LED应该有反应
- 串口应该有输出

如果完全没反应：
- 可能是固件没烧录成功
- 或者设备根本没启动

## 快速验证清单

- [ ] 重新编译（Clean + Build）
- [ ] 完全断电重启
- [ ] 卸载Windows驱动
- [ ] 重新插入USB
- [ ] 设备管理器无代码10
- [ ] USBView能看到设备
- [ ] 设备在"照相机"分类下

## 成功标志

修复后应该看到：

```
设备管理器：
  照相机
    └─ STM32 UVC Camera (正常，无感叹号)

USBView：
  Device Descriptor: 成功读取
  Configuration Descriptor: 成功读取
  Interface Descriptors: 正确显示

串口输出：
  UVC Init called
  UVC Init complete
```

## 如果仍然失败

### 最后的手段：使用CDC测试

临时切换回CDC类验证硬件和USB基础功能：

```c
// 在usb_device.c中
// 临时改回CDC
if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC) != USBD_OK)
```

如果CDC能工作，说明硬件正常，问题在UVC实现。

如果CDC也不行，说明USB硬件或基础配置有问题。

## 获取帮助

如果问题仍未解决，请提供：

1. **完整的错误信息**（设备管理器截图）
2. **USBView的输出**（如果能看到设备）
3. **Wireshark捕获**（USB枚举过程）
4. **串口输出**（如果有）
5. **编译输出**（确认无错误）

## 参考

- Windows错误代码：https://docs.microsoft.com/en-us/windows-hardware/drivers/install/device-manager-error-messages
- USB规范：https://www.usb.org/documents
- UVC规范：https://www.usb.org/document-library/video-class-v11-document-set
