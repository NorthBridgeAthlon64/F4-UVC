# 简单调试测试

## 问题分析

你只看到了：
```
MID:32674
HID:97
```

这说明：
- ✅ 串口工作
- ✅ OV2640被检测到
- ❌ 之后的printf没有输出

## 可能的原因

### 1. printf缓冲区问题
C标准库的printf默认是行缓冲的，需要遇到`\n`才会输出。但在嵌入式系统中可能需要手动刷新。

### 2. 程序卡住
程序可能在某个函数中卡住了（死循环、等待、错误）。

### 3. 堆栈溢出
如果堆栈太小，可能导致程序崩溃。

## 立即测试方案

### 方案1：在main.c中添加LED闪烁测试

在`Core/Src/main.c`的初始化代码中添加LED测试：

```c
// 在OV2640_Init()之后立即添加：
printf("===OV2640 Init Complete===\r\n");
HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);  // LED0闪烁一次
HAL_Delay(100);

printf("Configuring JPEG mode...\r\n");
OV2640_JPEG_Mode();
HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);  // LED0再闪烁
HAL_Delay(100);

printf("Setting output size...\r\n");
OV2640_OutSize_Set(320, 240);
HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);  // LED0再闪烁
HAL_Delay(100);

printf("Initializing UVC App...\r\n");
UVC_App_Init();
HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);  // LED0再闪烁
HAL_Delay(100);

printf("Starting capture...\r\n");
UVC_App_StartCapture();
HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);  // LED0再闪烁
```

**观察LED0**：
- 如果LED0闪烁5次：说明所有初始化都完成了
- 如果LED0闪烁N次后停止：说明程序卡在第N+1步

### 方案2：使用fflush强制刷新

在每个printf后添加：

```c
printf("Test message\r\n");
fflush(stdout);  // 强制刷新输出缓冲区
```

### 方案3：直接使用HAL_UART_Transmit

绕过printf，直接发送：

```c
char msg[] = "OV2640 OK\r\n";
HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 1000);
```

## 推荐的调试代码

将main.c的USER CODE BEGIN 2部分改为：

```c
/* USER CODE BEGIN 2 */
char msg[100];

// 测试1
sprintf(msg, "=== System Start ===\r\n");
HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 1000);

TFTLCD_Init(LCD_DIR_0);
LCD_Clear(WHITE);
LCD_ShowString(30,40,210,24,24,"UVC Camera System");
LCD_ShowString(30,70,210,16,16,"Initializing...");

sprintf(msg, "LCD Init OK\r\n");
HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 1000);

// 初始化OV2640
while(OV2640_Init())
{
  LCD_ShowString(30,100,240,16,16,"OV2640 ERR");
  HAL_Delay(200);
  LCD_Fill(30,100,239,140,WHITE);
  HAL_Delay(200);
}
LCD_ShowString(30,100,200,16,16,"OV2640 OK");

sprintf(msg, "OV2640 Init OK\r\n");
HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 1000);
HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);

// 配置JPEG模式
OV2640_JPEG_Mode();
sprintf(msg, "JPEG Mode OK\r\n");
HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 1000);
HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);

OV2640_OutSize_Set(320, 240);
sprintf(msg, "Output Size OK\r\n");
HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 1000);
HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);

LCD_ShowString(30,120,200,16,16,"JPEG Mode: 320x240");

// 初始化UVC
UVC_App_Init();
sprintf(msg, "UVC Init OK\r\n");
HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 1000);
HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);

LCD_ShowString(30,140,200,16,16,"UVC Ready");
LCD_ShowString(30,160,200,16,16,"USB Streaming Only");
LCD_ShowString(30,180,200,16,16,"View on PC");

// 启动捕获
UVC_App_StartCapture();
sprintf(msg, "Capture Started\r\n");
HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 1000);
HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);

HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);

sprintf(msg, "=== Init Complete ===\r\n");
HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 1000);
/* USER CODE END 2 */
```

这样你就能看到每一步的执行情况，并且LED0会闪烁指示进度。
