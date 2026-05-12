/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dcmi.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"
#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ov2640.h"
#include "ILI93xx.h"
#include "uvc_app.h"
#include "tjpgd.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* TJpgDec相关结构和变量 */
typedef struct {
    uint8_t *jpeg_data;
    uint32_t jpeg_size;
    uint32_t current_pos;
    uint16_t display_width;
    uint16_t display_height;
    uint16_t x_offset;
    uint16_t y_offset;
} JPEG_Decode_Context;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t Show_LCDWord[64];//用于存储准备在LCD上显示的字符串
char USB_Buffer[100];
_lcd_dev tftlcd_data;
// 外部声明DMA句柄，避免重复定义
extern DMA_HandleTypeDef hdma_dcmi_main;

// 外部声明UVC应用状态
extern UVC_AppTypeDef uvc_app;

#pragma region ov2640_State_PV
uint8_t ov2640_mode=0;
volatile uint32_t jpeg_data_len=0; 			//buf中的JPEG有效数据长度
volatile uint8_t jpeg_data_ok=0;				//JPEG数据采集完成标志
														//0,数据没有采集完;
														//1,数据采集完了,但是还没处理;
														//2,数据已经处理完成了,可以开始下一帧接收

// 使用dcmi.c中的缓冲区，避免重复定义
extern uint8_t jpeg_buf1[40*1024];
extern uint8_t jpeg_buf2[40*1024];
extern uint8_t *capture_buf;
extern uint8_t *process_buf;

// 定义jpeg_buf指向capture_buf，保持兼容性
extern uint8_t *jpeg_buf;

// 功能控制标志位
volatile uint8_t jpeg_decode_enabled = 1;  // JPEG解码开关 (1:开启, 0:关闭)
volatile uint8_t uvc_transfer_enabled = 1; // UVC传输开关 (1:开启, 0:关闭)
#pragma region end
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
//重写fputc函数，目的是让printf函数能顺利运行
int fputc(int c, FILE *stream)
{
 /*
    huart1是工具生成代码定义的UART1结构体，
    如果以后要使用其他串口打印，只需要把这个结构体改成其他UART结构体。
*/
    HAL_UART_Transmit(&huart1, (unsigned char *)&c, 1, 1000);
    return 1;
}

//重写__io_putchar函数，目的是让syscalls.c中的_write函数能正常工作
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (unsigned char *)&ch, 1, 1000);
    return ch;
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// 外部声明改进的双缓冲变量
extern uint8_t jpeg_buf1[40*1024];
extern uint8_t jpeg_buf2[40*1024];
extern uint8_t *capture_buf;
extern uint8_t *process_buf;
extern volatile uint8_t buffer_ready;

// DCMI JPEG捕获完成回调
void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi_handle)
{
    // 停止DCMI捕获，避免数据覆盖
    HAL_DCMI_Stop(&hdcmi);
    
    // 获取实际传输的数据长度
    uint32_t total_words = (40*1024) / 4;  // 总字数 (10240)
    uint32_t remaining = __HAL_DMA_GET_COUNTER(&hdma_dcmi_main);  // 剩余字数
    uint32_t transferred_words = total_words - remaining;  // 已传输的字数
    jpeg_data_len = transferred_words * 4;  // 转换为字节数
    
    // 验证JPEG数据有效性
    if (jpeg_data_len > 100 && jpeg_data_len < (40*1024)) {
        // 检查JPEG头部标记 (0xFFD8)
        if (capture_buf[0] == 0xFF && capture_buf[1] == 0xD8) {
            // 数据有效，标记为就绪
            jpeg_data_ok = 1;
            buffer_ready = 1;
            
            // 复制数据到jpeg_buf，保持兼容性
            memcpy(jpeg_buf, capture_buf, jpeg_data_len);
            
        } else {
            // 即使JPEG头无效，也标记为数据就绪，尝试传输
            jpeg_data_ok = 1;
            buffer_ready = 1;
        }
    } else {
        // 尝试强制处理，即使长度为0
        if (remaining < total_words) {
            jpeg_data_ok = 1;
            buffer_ready = 1;
        }
    }
    
    // 切换缓冲区
    uint8_t *temp = capture_buf;
    capture_buf = process_buf;
    process_buf = temp;
    
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
}

// DCMI VSYNC回调（可选）
void HAL_DCMI_VsyncEventCallback(DCMI_HandleTypeDef *hdcmi)
{
    // VSYNC事件处理
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	switch(GPIO_Pin)
	{
		case KEY0_Pin:
			// 切换JPEG解码开关
			jpeg_decode_enabled = !jpeg_decode_enabled;
			
			// 更新LED状态
			if (jpeg_decode_enabled) {
				LED0_GPIO_Port->BSRR = (uint32_t)LED0_Pin << 16U; // LED0亮
			} else {
				LED0_GPIO_Port->BSRR = (uint32_t)LED0_Pin; // LED0灭
			}
			
			// 在屏幕上显示状态
			LCD_ShowString(30, 110, 240, 16, 16, "                    ");
			if (jpeg_decode_enabled) {
				LCD_ShowString(30, 110, 240, 16, 16, "JPEG: Enabled");
			} else {
				LCD_ShowString(30, 110, 240, 16, 16, "JPEG: Disabled");
			}
			
			break;

		case KEY1_Pin:
			// 切换UVC传输开关
			uvc_transfer_enabled = !uvc_transfer_enabled;
			
			// 更新LED状态
			if (uvc_transfer_enabled) {
				LED1_GPIO_Port->BSRR = (uint32_t)LED1_Pin; // LED1亮
			} else {
				LED1_GPIO_Port->BSRR = (uint32_t)LED1_Pin << 16U; // LED1灭
			}
			
			// 在屏幕上显示状态
			LCD_ShowString(30, 130, 240, 16, 16, "                    ");
			if (uvc_transfer_enabled) {
				LCD_ShowString(30, 130, 240, 16, 16, "UVC: Enabled");
			} else {
				LCD_ShowString(30, 130, 240, 16, 16, "UVC: Disabled");
			}
			
			break;

		default:
			break;
	}
	
	// 更新组合状态显示
	LCD_ShowString(30, 150, 240, 16, 16, "                    ");
	if (jpeg_decode_enabled && uvc_transfer_enabled) {
		LCD_ShowString(30, 150, 240, 16, 16, "Mode: Both Enabled");
	} else if (jpeg_decode_enabled && !uvc_transfer_enabled) {
		LCD_ShowString(30, 150, 240, 16, 16, "Mode: JPEG Only");
	} else if (!jpeg_decode_enabled && uvc_transfer_enabled) {
		LCD_ShowString(30, 150, 240, 16, 16, "Mode: UVC Only");
	} else {
		LCD_ShowString(30, 150, 240, 16, 16, "Mode: Both Disabled");
	}
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DCMI_Init();
  MX_TIM7_Init();
  MX_FSMC_Init();
  MX_USART1_UART_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  // 使用HAL_UART直接发送，绕过printf缓冲区
  char debug_msg[100];
  
  sprintf(debug_msg, "\r\n=== System Starting ===\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
  
  TFTLCD_Init(LCD_DIR_270);	//初始化LCD
  LCD_Clear(WHITE);	//初始化后清屏并显示白屏
  LCD_ShowString(30,40,210,24,24,"UVC Camera System");
  LCD_ShowString(30,70,210,16,16,"Initializing...");
  
  sprintf(debug_msg, "LCD: OK\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);

  // 初始化OV2640
  sprintf(debug_msg, "Initializing OV2640...\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
  
  while(OV2640_Init())
  {
    LCD_ShowString(30,100,240,16,16,"OV2640 ERR");
    HAL_Delay(200);
    LCD_Fill(30,100,239,140,WHITE);
    HAL_Delay(200);
  }
  LCD_ShowString(30,100,200,16,16,"OV2640 OK");
  
  sprintf(debug_msg, "OV2640: Init Complete\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
  HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);  // LED0闪烁表示OV2640 OK

  // 配置OV2640为JPEG模式，320x240分辨率
  sprintf(debug_msg, "Configuring JPEG mode...\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
  
  // 详细配置OV2640
  printf("OV2640: Setting JPEG mode\r\n");
  OV2640_JPEG_Mode();
  
  
  // 设置最高JPEG画质
  printf("OV2640: Setting highest quality JPEG\r\n");
  
  HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
  

  printf("OV2640: Setting output size 1280x720 (720p)\r\n");
  OV2640_OutSize_Set(320, 240);
  OV2640_Set_JPEG_Quality(10);  // 10级最高画质，减少方块感
  OV2640_Auto_Exposure(2);
  OV2640_Brightness(2);
  OV2640_Light_Mode(2);
  OV2640_Contrast(0);

  HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
  
  // 确保输出格式正确
  printf("OV2640: Verifying configuration\r\n");
  
  LCD_ShowString(30,120,200,16,16,"JPEG Mode: 320x240");
  
  sprintf(debug_msg, "JPEG: Configured\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
  
  // 等待OV2640稳定
  printf("OV2640: Waiting for stability\r\n");
  HAL_Delay(500);
  
  // 初始化UVC应用
  sprintf(debug_msg, "Initializing UVC App...\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
  
  UVC_App_Init();
  HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
  
  sprintf(debug_msg, "UVC App: Initialized\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
  LCD_ShowString(30,140,200,16,16,"UVC Ready");
  LCD_ShowString(30,160,200,16,16,"USB Streaming Only");
  LCD_ShowString(30,180,200,16,16,"View on PC");
  
  // 启动视频采集
  sprintf(debug_msg, "Starting video capture...\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
  
  UVC_App_StartCapture();
  HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
  
  sprintf(debug_msg, "Capture: Started\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
  
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
  
  // 等待并检查状态
  HAL_Delay(500);
  
  sprintf(debug_msg, "\r\n=== System Ready ===\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
  
  sprintf(debug_msg, "jpeg_data_ok=%d, len=%lu\r\n", jpeg_data_ok, jpeg_data_len);
  HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
  
  sprintf(debug_msg, "Entering main loop...\r\n\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t last_status_print = 0;
  uint8_t loop_started = 0;
  
  // 用于屏幕显示的变量
  static uint32_t last_display_update = 0;
  static uint8_t display_enabled = 0;
  
  while (1)
  {
    // 首次进入循环时打印
    if (!loop_started) {
      sprintf(debug_msg, "Main loop running...\r\n");
      HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
      loop_started = 1;
      
      // 启用屏幕显示
      display_enabled = 1;
      
      // 初始化LED状态
      if (jpeg_decode_enabled) {
        LED0_GPIO_Port->BSRR = (uint32_t)LED0_Pin << 16U; // LED0亮
      } else {
        LED0_GPIO_Port->BSRR = (uint32_t)LED0_Pin; // LED0灭
      }
      
      if (uvc_transfer_enabled) {
        LED1_GPIO_Port->BSRR = (uint32_t)LED1_Pin; // LED1亮
      } else {
        LED1_GPIO_Port->BSRR = (uint32_t)LED1_Pin << 16U; // LED1灭
      }
      
      // 显示初始状态
      LCD_ShowString(30, 110, 240, 16, 16, "                    ");
      if (jpeg_decode_enabled) {
        LCD_ShowString(30, 110, 240, 16, 16, "JPEG: Enabled");
      } else {
        LCD_ShowString(30, 110, 240, 16, 16, "JPEG: Disabled");
      }
      
      LCD_ShowString(30, 130, 240, 16, 16, "                    ");
      if (uvc_transfer_enabled) {
        LCD_ShowString(30, 130, 240, 16, 16, "UVC: Enabled");
      } else {
        LCD_ShowString(30, 130, 240, 16, 16, "UVC: Disabled");
      }
      
      LCD_ShowString(30, 150, 240, 16, 16, "                    ");
      if (jpeg_decode_enabled && uvc_transfer_enabled) {
        LCD_ShowString(30, 150, 240, 16, 16, "Mode: Both Enabled");
      } else if (jpeg_decode_enabled && !uvc_transfer_enabled) {
        LCD_ShowString(30, 150, 240, 16, 16, "Mode: JPEG Only");
      } else if (!jpeg_decode_enabled && uvc_transfer_enabled) {
        LCD_ShowString(30, 150, 240, 16, 16, "Mode: UVC Only");
      } else {
        LCD_ShowString(30, 150, 240, 16, 16, "Mode: Both Disabled");
      }
    }
    
    // 处理UVC应用状态机（优先处理，确保UVC不卡）
    if (uvc_transfer_enabled) {
        UVC_App_Process();
    }
    
    // 每2秒打印一次状态
    if (HAL_GetTick() - last_status_print >= 2000) {
      last_status_print = HAL_GetTick();
      
      sprintf(debug_msg, "[%lu] ok=%d, len=%lu, state=%d\r\n", 
              HAL_GetTick()/1000, jpeg_data_ok, jpeg_data_len, uvc_app.state);
      HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
    }
    
    // 屏幕显示摄像头图像（低优先级，允许卡顿）
    if (display_enabled && jpeg_decode_enabled && jpeg_data_ok == 2 && jpeg_data_len > 100) {
      // 每2帧解码一次，平衡刷新速度和性能
      static uint8_t decode_counter = 0;
      if (decode_counter % 2 == 0) {
        decode_counter = 0;
        
        // 尝试解码JPEG并显示
        // 使用TJpgDec库解码JPEG，直接输出到LCD显存
        uint16_t display_width = 0;
        uint16_t display_height = 0;
        
        // 检查JPEG头
        if (jpeg_data_len >= 2 && jpeg_buf[0] == 0xFF && jpeg_buf[1] == 0xD8) {
            // 使用TJpgDec解码JPEG（直接输出到LCD显存）
            int decode_result = JPEG_Decode_Using_TJpgDec(jpeg_buf, jpeg_data_len, 
                                                       &display_width, &display_height);
            
            if (decode_result == 0) {

            } else {
              // 解码失败
              printf("Display: JPEG decode failed\r\n");
            }
        } else {
          printf("Display: Invalid JPEG data\r\n");
        }
      }
      decode_counter++;
    }
    
    // 简单的延时，避免CPU占用过高
    HAL_Delay(1);
    
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* TJpgDec输入函数 */
static uint16_t jpeg_input_func(JDEC *jdec, uint8_t *buf, uint16_t len) {
    // 使用实际的jpeg_buf数据
    extern uint8_t *jpeg_buf;
    extern volatile uint32_t jpeg_data_len;
    
    // 使用设备指针存储当前位置
    JPEG_Decode_Context *ctx = (JPEG_Decode_Context *)jdec->device;
    
    // 检查参数有效性
    if (jdec == NULL || ctx == NULL || jpeg_buf == NULL || jpeg_data_len == 0) {
        return 0;
    }
    
    if (buf == NULL) {
        // NULL缓冲区表示跳过len字节
        ctx->current_pos += len;
        if (ctx->current_pos > jpeg_data_len) {
            ctx->current_pos = jpeg_data_len;
        }
        return len;
    }
    
    if (len == 0) {
        // 重置位置（开始新的解码）
        ctx->current_pos = 0;
        return 0;
    }
    
    // 计算可读取的字节数
    uint32_t remaining = jpeg_data_len - ctx->current_pos;
    if (remaining == 0) {
        return 0;
    }
    
    // 计算实际读取的字节数
    uint16_t actual_len = len;
    if (actual_len > remaining) {
        actual_len = remaining;
    }
    
    // 检查内存访问是否安全
    if (ctx->current_pos + actual_len > jpeg_data_len) {
        actual_len = jpeg_data_len - ctx->current_pos;
    }
    
    // 复制数据
    memcpy(buf, jpeg_buf + ctx->current_pos, actual_len);
    
    // 更新位置
    ctx->current_pos += actual_len;
    
    return actual_len;
}

/* TJpgDec输出函数 - 直接输出到LCD显存 */
static uint16_t jpeg_output_func(JDEC *jdec, void *bitmap, JRECT *rect) {
    JPEG_Decode_Context *ctx = (JPEG_Decode_Context *)jdec->device;
    uint8_t *src = (uint8_t *)bitmap;
    int x, y;
    
    // 设置LCD窗口到当前解码区域
    LCD_Set_Window(ctx->x_offset + rect->left, ctx->y_offset + rect->top, 
                  rect->right - rect->left + 1, rect->bottom - rect->top + 1);
    LCD_WriteRAM_Prepare();
    
    // 直接将RGB565数据写入LCD显存
    for (y = rect->top; y <= rect->bottom; y++) {
        for (x = rect->left; x <= rect->right; x++) {
            // TJpgDec输出为RGB565格式
            LCD_WriteRAM(*((uint16_t *)src));
            src += 2;
        }
    }
    
    return 1; // 继续解码
}

/* 使用TJpgDec解码JPEG */
// 使用静态缓冲区避免栈溢出，增加大小以解决内存不足问题
static uint8_t jpeg_work_buf[8192]; // 静态工作缓冲区，分配在全局内存中（足够满足320x240解码需求）

int JPEG_Decode_Using_TJpgDec(uint8_t *jpeg_data, uint32_t jpeg_size, 
                             uint16_t *width, uint16_t *height) {
    JDEC jdec;
    JPEG_Decode_Context ctx;
    JRESULT result;
    
    // 检查输入数据
    if (jpeg_data == NULL || jpeg_size < 10) {
        printf("JPEG: Error: Invalid input data\r\n");
        return -1;
    }
    
    // 检查JPEG头
    if (jpeg_data[0] != 0xFF || jpeg_data[1] != 0xD8) {
        printf("JPEG: Error: Invalid JPEG header (found 0x%02X%02X)\r\n", jpeg_data[0], jpeg_data[1]);
        return -1;
    }
    
    // 检查数据大小，避免过大
    if (jpeg_size > 30*1024) {
        printf("JPEG: Error: Data too large\r\n");
        return -1;
    }
    
    // 初始化上下文
    ctx.jpeg_data = jpeg_data;
    ctx.jpeg_size = jpeg_size;
    ctx.current_pos = 0;
    ctx.x_offset = 0;
    ctx.y_offset = 0;
    
    // 准备解码
    //printf("JPEG: Preparing decode, size=%lu bytes\r\n", jpeg_size);
    result = jd_prepare(&jdec, jpeg_input_func, jpeg_work_buf, sizeof(jpeg_work_buf), &ctx);
    if (result != JDR_OK) {
        //printf("JPEG: Prepare failed: %d\r\n", result);
        switch(result) {
            case JDR_INP: printf("JPEG: Error: Invalid input stream\r\n"); break;
            case JDR_MEM1: printf("JPEG: Error: Insufficient memory\r\n"); break;
            case JDR_MEM2: printf("JPEG: Error: Insufficient input buffer\r\n"); break;
            case JDR_PAR: printf("JPEG: Error: Parameter error\r\n"); break;
            case JDR_FMT1: printf("JPEG: Error: Data format error\r\n"); break;
            case JDR_FMT2: printf("JPEG: Error: Unsupported format\r\n"); break;
            case JDR_FMT3: printf("JPEG: Error: Not supported JPEG standard\r\n"); break;
        }
        return -1;
    }
    
    // 更新图像尺寸
    *width = jdec.width;
    *height = jdec.height;
    //printf("JPEG: Image size: %dx%d\r\n", *width, *height);
    
    // 检查图像尺寸，避免过大
    if (*width > 1280 || *height > 720) {
        printf("JPEG: Error: Image too large\r\n");
        return -1;
    }
    
    // 开始解码
    //printf("JPEG: Starting decode\r\n");
    result = jd_decomp(&jdec, jpeg_output_func, 0);
    if (result != JDR_OK) {
        printf("JPEG: Decode failed: %d\r\n", result);
        return -1;
    }
    
    //printf("JPEG: Decode successful\r\n");
    return 0;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
