# 最终调试指南

## 已完成的修改

### 1. 使用HAL_UART_Transmit替代printf
- 绕过printf缓冲区问题
- 每个关键步骤都有输出
- 使用\r\n确保立即显示

### 2. 添加LED指示
- LED0在每个初始化步骤后闪烁
- 可以通过LED闪烁次数判断程序执行到哪一步

### 3. 周期性状态报告
- 主循环每2秒输出一次状态
- 显示jpeg_data_ok, jpeg_data_len, uvc_app.state

## 预期串口输出

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
Initializing UVC App...\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
  
  UVC_App_Init();
  HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
  
  sprintf(debug_msg, "UVC App: Initialized\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
UVC App: Initialized
Starting video capture...
Capture: Started

=== System Ready ===
jpeg_data_ok=0, len=0
Entering main loop...

Main loop running...
[2] ok=0, len=0, state=1
[4] ok=0, len=0, state=1
[6] ok=1, len=15234, state=2
```

## LED指示含义

- LED0闪烁5次：所有初始化完成
- LED1常亮：系统就绪
- LED1闪烁：帧捕获（如果DCMI工作）

## 故障定位

### 如果只看到MID/HID
说明程序在OV2640_Init()之后卡住了

**检查**：
- LCD初始化是否卡住
- OV2640_JPEG_Mode()是否有问题

### 如果看到"LCD: OK"但没有后续
说明OV2640初始化有问题（但这不太可能，因为你已经看到MID/HID了）

### 如果看到"JPEG: Configured"但没有后续
说明UVC_App_Init()有问题

### 如果看到"UVC App: Initialized"但没有后续
说明UVC_App_StartCapture()卡住了

### 如果看到"Main loop running"
说明初始化全部完成，检查：
- state值（应该是1=CAPTURING）
- ok值（应该从0变为1当帧捕获时）
- len值（应该是5000-30000之间）

## 下一步

1. **重新编译并烧录**
2. **观察串口输出**
3. **观察LED0闪烁次数**
4. **将完整的串口输出发给我**

这样我就能准确定位问题所在！
