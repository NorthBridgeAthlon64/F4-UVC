/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dcmi.c
  * @brief   This file provides code for the configuration
  *          of the DCMI instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "dcmi.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

DCMI_HandleTypeDef hdcmi;
DMA_HandleTypeDef hdma_dcmi_main;

/* DCMI init function */
void MX_DCMI_Init(void)
{

  /* USER CODE BEGIN DCMI_Init 0 */

  /* USER CODE END DCMI_Init 0 */

  /* USER CODE BEGIN DCMI_Init 1 */

  /* USER CODE END DCMI_Init 1 */
  hdcmi.Instance = DCMI;
  hdcmi.Init.SynchroMode = DCMI_SYNCHRO_HARDWARE;
  hdcmi.Init.PCKPolarity = DCMI_PCKPOLARITY_RISING;
  hdcmi.Init.VSPolarity = DCMI_VSPOLARITY_LOW;
  hdcmi.Init.HSPolarity = DCMI_HSPOLARITY_LOW;
  hdcmi.Init.CaptureRate = DCMI_CR_ALL_FRAME;
  hdcmi.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;
  hdcmi.Init.JPEGMode = DCMI_JPEG_ENABLE;
  if (HAL_DCMI_Init(&hdcmi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DCMI_Init 2 */

  /* USER CODE END DCMI_Init 2 */

}

void HAL_DCMI_MspInit(DCMI_HandleTypeDef* dcmiHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(dcmiHandle->Instance==DCMI)
  {
  /* USER CODE BEGIN DCMI_MspInit 0 */

  /* USER CODE END DCMI_MspInit 0 */
    /* DCMI clock enable */
    __HAL_RCC_DCMI_CLK_ENABLE();

    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**DCMI GPIO Configuration
    PE5     ------> DCMI_D6
    PE6     ------> DCMI_D7
    PA4     ------> DCMI_HSYNC
    PA6     ------> DCMI_PIXCLK
    PC6     ------> DCMI_D0
    PC7     ------> DCMI_D1
    PC8     ------> DCMI_D2
    PC9     ------> DCMI_D3
    PC11     ------> DCMI_D4
    PB6     ------> DCMI_D5
    PB7     ------> DCMI_VSYNC
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9
                          |GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* DCMI DMA Init */
    /* DCMI Init */
    hdma_dcmi_main.Instance = DMA2_Stream1;
    hdma_dcmi_main.Init.Channel = DMA_CHANNEL_1;
    hdma_dcmi_main.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_dcmi_main.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_dcmi_main.Init.MemInc = DMA_MINC_ENABLE;
    hdma_dcmi_main.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_dcmi_main.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_dcmi_main.Init.Mode = DMA_CIRCULAR;
    hdma_dcmi_main.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma_dcmi_main.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_dcmi_main.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
    hdma_dcmi_main.Init.MemBurst = DMA_MBURST_SINGLE;
    hdma_dcmi_main.Init.PeriphBurst = DMA_PBURST_SINGLE;
    if (HAL_DMA_Init(&hdma_dcmi_main) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(dcmiHandle,DMA_Handle,hdma_dcmi_main);

    /* DCMI interrupt Init */
    HAL_NVIC_SetPriority(DCMI_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DCMI_IRQn);
  /* USER CODE BEGIN DCMI_MspInit 1 */

  /* USER CODE END DCMI_MspInit 1 */
  }
}

void HAL_DCMI_MspDeInit(DCMI_HandleTypeDef* dcmiHandle)
{

  if(dcmiHandle->Instance==DCMI)
  {
  /* USER CODE BEGIN DCMI_MspDeInit 0 */

  /* USER CODE END DCMI_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_DCMI_CLK_DISABLE();

    /**DCMI GPIO Configuration
    PE5     ------> DCMI_D6
    PE6     ------> DCMI_D7
    PA4     ------> DCMI_HSYNC
    PA6     ------> DCMI_PIXCLK
    PC6     ------> DCMI_D0
    PC7     ------> DCMI_D1
    PC8     ------> DCMI_D2
    PC9     ------> DCMI_D3
    PC11     ------> DCMI_D4
    PB6     ------> DCMI_D5
    PB7     ------> DCMI_VSYNC
    */
    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_5|GPIO_PIN_6);

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_4|GPIO_PIN_6);

    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9
                          |GPIO_PIN_11);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6|GPIO_PIN_7);

    /* DCMI DMA DeInit */
    HAL_DMA_DeInit(&hdma_dcmi_main);

    /* DCMI interrupt Deinit */
    HAL_NVIC_DisableIRQ(DCMI_IRQn);
  /* USER CODE BEGIN DCMI_MspDeInit 1 */

  /* USER CODE END DCMI_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
// JPEG捕获相关变量
extern volatile uint32_t jpeg_data_len;
extern volatile uint8_t jpeg_data_ok;
extern uint8_t *jpeg_buf;
extern DMA_HandleTypeDef hdma_dcmi_main;

void DCMI_DMA_Init(uint32_t mem0addr,uint32_t mem1addr,uint16_t memsize,uint32_t memblen,uint32_t meminc)
{
    __HAL_RCC_DMA2_CLK_ENABLE();                              //使能DMA2时钟
    
    // 配置DMA参数
    hdma_dcmi_main.Instance=DMA2_Stream1;                          //DMA2数据流1
    hdma_dcmi_main.Init.Channel=DMA_CHANNEL_1;                     //通道1
    hdma_dcmi_main.Init.Direction=DMA_PERIPH_TO_MEMORY;            //外设到存储器
    hdma_dcmi_main.Init.PeriphInc=DMA_PINC_DISABLE;                //外设非增量模式
    hdma_dcmi_main.Init.MemInc=meminc;                             //存储器增量模式
    hdma_dcmi_main.Init.PeriphDataAlignment=DMA_PDATAALIGN_WORD;   //外设数据长度:32位
    hdma_dcmi_main.Init.MemDataAlignment=memblen;                  //存储器数据长度:8/16/32位
    hdma_dcmi_main.Init.Mode=DMA_CIRCULAR;                         //使用循环模式
    hdma_dcmi_main.Init.Priority=DMA_PRIORITY_HIGH;                //高优先级
    hdma_dcmi_main.Init.FIFOMode=DMA_FIFOMODE_ENABLE;              //使能FIFO
    hdma_dcmi_main.Init.FIFOThreshold=DMA_FIFO_THRESHOLD_HALFFULL; //使用1/2的FIFO
    hdma_dcmi_main.Init.MemBurst=DMA_MBURST_SINGLE;                //存储器突发传输
    hdma_dcmi_main.Init.PeriphBurst=DMA_PBURST_SINGLE;             //外设突发单次传输

    HAL_DMA_DeInit(&hdma_dcmi_main);                               //先清除以前的设置
    if (HAL_DMA_Init(&hdma_dcmi_main) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(&hdcmi,DMA_Handle,hdma_dcmi_main);        		  //将DMA与DCMI联系起来
    
    /*
     * 在开启DMA之前先使用__HAL_UNLOCK()解锁一次DMA,因为HAL_DMA_Statrt()HAL_DMAEx_MultiBufferStart()
     * 这两个函数一开始要先使用__HAL_LOCK()锁定DMA,而函数__HAL_LOCK()会判断当前的DMA状态是否为锁定状态，如果是
     * 锁定状态的话就直接返回HAL_BUSY，这样会导致函数HAL_DMA_Statrt()和HAL_DMAEx_MultiBufferStart()后续的DMA配置
     * 程序直接被跳过！DMA也就不能正常工作，为了避免这种现象，所以在启动DMA之前先调用__HAL_UNLOC()先解锁一次DMA。
     */
    __HAL_UNLOCK(&hdma_dcmi_main);
    HAL_DMA_Start(&hdma_dcmi_main,(uint32_t)&DCMI->DR,mem0addr,memsize);

}

// 改进的双缓冲实现
uint8_t jpeg_buf1[40*1024];  // 缓冲区1
uint8_t jpeg_buf2[40*1024];  // 缓冲区2
uint8_t *capture_buf = jpeg_buf1;  // 用于捕获的缓冲区
uint8_t *process_buf = jpeg_buf2;  // 用于处理的缓冲区
volatile uint8_t buffer_ready = 0;  // 缓冲区就绪标志

// 定义jpeg_buf指向jpeg_buf1，供main.c使用（使用常量表达式初始化）
uint8_t *jpeg_buf = jpeg_buf1;

// DCMI JPEG模式DMA初始化
void DCMI_DMA_JPEG_Init(uint32_t mem_addr, uint32_t bufsize)
{
    __HAL_RCC_DMA2_CLK_ENABLE();
    __HAL_LINKDMA(&hdcmi, DMA_Handle, hdma_dcmi_main);
    
    hdma_dcmi_main.Instance = DMA2_Stream1;
    hdma_dcmi_main.Init.Channel = DMA_CHANNEL_1;
    hdma_dcmi_main.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_dcmi_main.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_dcmi_main.Init.MemInc = DMA_MINC_ENABLE;  // JPEG模式需要内存递增
    hdma_dcmi_main.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_dcmi_main.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_dcmi_main.Init.Mode = DMA_NORMAL;  // 使用正常模式，避免数据覆盖
    hdma_dcmi_main.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_dcmi_main.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_dcmi_main.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_dcmi_main.Init.MemBurst = DMA_MBURST_INC4;
    hdma_dcmi_main.Init.PeriphBurst = DMA_PBURST_SINGLE;
    
    HAL_DMA_DeInit(&hdma_dcmi_main);
    HAL_DMA_Init(&hdma_dcmi_main);
    
    __HAL_LINKDMA(&hdcmi, DMA_Handle, hdma_dcmi_main);
}

// DCMI启动JPEG捕获
void DCMI_Start_JPEG(void)
{
    // 无论当前状态如何，都重置状态
    jpeg_data_ok = 0;
    jpeg_data_len = 0;
    buffer_ready = 0;
    
    static uint32_t capture_count = 0;
    if (capture_count % 50 == 0) {
        printf("DCMI: Starting capture sequence\r\n");
    }
    capture_count++;
    
    // 重新初始化DMA，确保使用正确的缓冲区
    if (capture_count % 50 == 0) {
        printf("DCMI: Initializing DMA\r\n");
    }
    DCMI_DMA_JPEG_Init((uint32_t)capture_buf, 40*1024);
    
    // 确保DCMI已经初始化
    if (hdcmi.State != HAL_DCMI_STATE_READY) {
        if (capture_count % 50 == 0) {
            printf("DCMI: Reinitializing DCMI\r\n");
        }
        MX_DCMI_Init();
    }
    
    // 清除任何挂起的中断
    __HAL_DCMI_CLEAR_FLAG(&hdcmi, DCMI_FLAG_FRAMERI);
    __HAL_DCMI_CLEAR_FLAG(&hdcmi, DCMI_FLAG_VSYNCRI);
    __HAL_DCMI_CLEAR_FLAG(&hdcmi, DCMI_FLAG_LINERI);
    __HAL_DCMI_CLEAR_FLAG(&hdcmi, DCMI_FLAG_OVRRI);
    __HAL_DCMI_CLEAR_FLAG(&hdcmi, DCMI_FLAG_ERRRI);
    
    // 启用DCMI帧中断
    __HAL_DCMI_ENABLE_IT(&hdcmi, DCMI_IT_FRAME);
    
    // 启动DCMI JPEG捕获 - 使用SNAPSHOT模式确保数据完整性
    HAL_StatusTypeDef status = HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, 
                           (uint32_t)capture_buf, 
                           40*1024/4);  // 以字为单位
    
    if (status == HAL_OK) {
        if (capture_count % 50 == 0) {
            printf("DCMI: Capture started successfully\r\n");
        }
    } else {
        printf("DCMI: Failed to start capture: %d\r\n", status);
    }
}

//DCMI,启动传输
void DCMI_Start(void)
{
    LCD_SetCursor(0,0);
    LCD_WriteRAM_Prepare();                //开始写入GRAM
    __HAL_DMA_ENABLE(&hdma_dcmi_main); //使能DMA
    DCMI->CR|=DCMI_CR_CAPTURE;          //DCMI捕获使能
    printf("DCMI_Start OK\n");
}

//DCMI,关闭传输
void DCMI_Stop(void)
{
    DCMI->CR&=~(DCMI_CR_CAPTURE);       //关闭捕获
    while(DCMI->CR&0X01);               //等待传输完成
    __HAL_DMA_DISABLE(&hdma_dcmi_main);      //关闭DMA
}
/* USER CODE END 1 */
