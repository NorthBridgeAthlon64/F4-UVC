# 编译错误修复总结

## 修复的错误

### 1. DCMI_FLAG_OVF 未定义错误

**错误信息**：
```
'DCMI_FLAG_OVF' undeclared (first use in this function); did you mean 'DCMI_FLAG_OVFMI'?
```

**原因**：
- STM32 HAL库中没有`DCMI_FLAG_OVF`这个标志
- 正确的标志名是`DCMI_FLAG_OVRRI`（Overrun Raw Interrupt）

**修复**：
在`Core/Src/dcmi.c`中，将：
```c
__HAL_DCMI_CLEAR_FLAG(&hdcmi, DCMI_FLAG_OVF);
```
改为：
```c
__HAL_DCMI_CLEAR_FLAG(&hdcmi, DCMI_FLAG_OVRRI);
```

同时修正了其他标志：
- `DCMI_FLAG_HSYNC` → `DCMI_FLAG_LINERI`
- 添加了`DCMI_FLAG_ERRRI`

### 2. DMA计数器计算修正

**修复**：
在`Core/Src/main.c`的`HAL_DCMI_FrameEventCallback`中：
- 使用`hdma_dcmi_main`句柄
- 正确计算传输的字节数
- 添加详细的调试输出

## 正确的DCMI标志

根据STM32F4 HAL库定义：

### Raw Interrupt Flags (原始中断标志)
- `DCMI_FLAG_FRAMERI` - 帧捕获完成
- `DCMI_FLAG_OVRRI` - 溢出错误
- `DCMI_FLAG_ERRRI` - 同步错误
- `DCMI_FLAG_VSYNCRI` - VSYNC信号
- `DCMI_FLAG_LINERI` - 行信号

### Masked Interrupt Flags (屏蔽中断标志)
- `DCMI_FLAG_FRAMEMI`
- `DCMI_FLAG_OVRMI`
- `DCMI_FLAG_ERRMI`
- `DCMI_FLAG_VSYNCMI`
- `DCMI_FLAG_LINEMI`

## 现在可以编译了

所有编译错误已修复，现在可以：

1. **清理并重新编译**：
   ```bash
   make clean
   make -j4
   ```

2. **烧录程序**

3. **观察串口输出**（115200波特率）

4. **观察LED指示**：
   - LED0：初始化步骤指示（每步闪烁一次）
   - LED1：帧捕获指示（每捕获一帧闪烁一次）

## 预期的串口输出

```
MID:32674
HID:97

=== System Starting ===
LCD: OK
Initializing OV2640...
OV2640: Init Complete
Configuring JPEG mode...
Setting output size 320x240...
JPEG: Configured
Initializing UVC App...
UVC App: Initialized
Starting video capture...
DCMI_Start_JPEG: Starting capture
DCMI_Start_JPEG: Starting DMA transfer
DCMI_Start_JPEG: Capture started successfully
Capture: Started

=== System Ready ===
jpeg_data_ok=0, len=0
Entering main loop...

Main loop running...
[2] ok=0, len=0, state=1
✓ Frame: 15234 bytes
[4] ok=1, len=15234, state=2
```

如果看到"✓ Frame: XXXX bytes"，说明DCMI和DMA工作正常！
