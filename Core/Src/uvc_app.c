/* UVC Application Implementation */
#include "uvc_app.h"
#include "ov2640.h"
#include "dcmi.h"
#include "ILI93xx.h"
#include "usbd_uvc_if.h"
#include "usbd_def.h"
#include <string.h>
#include <stdlib.h>

// 外部声明USB设备句柄
extern USBD_HandleTypeDef hUsbDeviceFS;

// DCMI frame complete flag and buffers from main.c
extern volatile uint8_t jpeg_data_ok;
extern volatile uint32_t jpeg_data_len;
extern uint8_t *jpeg_buf;

// JPEG buffer size (must match main.c definition)
#define JPEG_BUF_SIZE (40*1024)

// NanoJPEG decoder (simplified for embedded use)
// This is a minimal JPEG decoder for baseline JPEG
typedef struct {
    uint8_t *data;
    uint32_t size;
    uint32_t pos;
    uint16_t width;
    uint16_t height;
    uint8_t *rgb_data;
} JPEG_Decoder;

// Application instance
UVC_AppTypeDef uvc_app;

// 不再分配独立的JPEG缓冲区，直接使用main.c中的jpeg_buf
// RGB565缓冲区也移除，直接在LCD上显示或使用外部SRAM

/**
  * @brief  Initialize UVC application
  */
void UVC_App_Init(void) {
    printf("UVC_App_Init: START\r\n");
    
    memset(&uvc_app, 0, sizeof(UVC_AppTypeDef));
    printf("  Cleared uvc_app structure\r\n");
    
    // 直接使用main.c中的jpeg_buf，不分配新缓冲区
    uvc_app.jpeg_buffer = jpeg_buf;
    uvc_app.rgb565_buffer = NULL;  // 不使用RGB565缓冲区
    uvc_app.state = UVC_STATE_IDLE;
    uvc_app.frame_ready = 0;
    uvc_app.usb_streaming = 0;
    
    printf("UVC_App_Init: COMPLETE\r\n");
}

/**
  * @brief  Start video capture
  */
void UVC_App_StartCapture(void) {
    // Configure OV2640 for JPEG mode at 320x240
    printf("UVC: Configuring OV2640 for 320x240 JPEG mode\r\n");
    OV2640_JPEG_Mode();
    OV2640_OutSize_Set(320, 240);  // 明确设置为320x240
    
    // 验证OV2640配置
    printf("UVC: OV2640 configured\r\n");
    
    // Initialize DCMI for JPEG capture
    DCMI_DMA_JPEG_Init((uint32_t)jpeg_buf, JPEG_BUF_SIZE);
    
    // Start DCMI capture
    jpeg_data_ok = 0;
    jpeg_data_len = 0;
    
    // Start JPEG capture
    DCMI_Start_JPEG();
    
    uvc_app.state = UVC_STATE_CAPTURING;
    printf("UVC: Capture started\r\n");
}

/**
  * @brief  Stop video capture
  */
void UVC_App_StopCapture(void) {
    HAL_DCMI_Stop(&hdcmi);
    uvc_app.state = UVC_STATE_IDLE;
}

/**
  * @brief  Check if USB streaming is active
  */
uint8_t UVC_App_IsStreaming(void) {
    return uvc_app.usb_streaming;
}

/**
  * @brief  Set USB streaming state
  */
void UVC_App_SetStreaming(uint8_t enable) {
    uvc_app.usb_streaming = enable;
    
    if (enable) {
        UVC_App_StartCapture();
    } else {
        UVC_App_StopCapture();
    }
}

/**
  * @brief  Convert RGB888 to RGB565
  */
static inline uint16_t RGB888_to_RGB565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/**
  * @brief  Simple JPEG to RGB565 decoder
  * @note   This is a simplified decoder for baseline JPEG from OV2640
  */
int JPEG_DecodeToRGB565(uint8_t *jpeg_data, uint32_t jpeg_size, 
                        uint16_t *rgb565_out, uint16_t *width, uint16_t *height) {
    // For OV2640 JPEG output, we can use a simplified approach
    // In a real implementation, you would use a full JPEG decoder
    // For now, we'll create a placeholder that fills with test pattern
    
    *width = UVC_FRAME_WIDTH;
    *height = UVC_FRAME_HEIGHT;
    
    // TODO: Implement full JPEG decoder
    // For demonstration, fill with a gradient pattern
    for (int y = 0; y < *height; y++) {
        for (int x = 0; x < *width; x++) {
            uint8_t r = (x * 255) / *width;
            uint8_t g = (y * 255) / *height;
            uint8_t b = 128;
            rgb565_out[y * (*width) + x] = RGB888_to_RGB565(r, g, b);
        }
    }
    
    return 0;  // Success
}

/**
  * @brief  Display RGB565 frame on LCD
  */
static void Display_RGB565_Frame(uint16_t *rgb565_data, uint16_t width, uint16_t height) {
    // Set LCD window
    uint16_t x_offset = (lcddev.width - width) / 2;
    uint16_t y_offset = (lcddev.height - height) / 2;
    
    LCD_Set_Window(x_offset, y_offset, width, height);
    LCD_WriteRAM_Prepare();
    
    // Write pixel data
    for (uint32_t i = 0; i < width * height; i++) {
        LCD_WriteRAM(rgb565_data[i]);
    }
}

/**
  * @brief  Process UVC application state machine
  */
void UVC_App_Process(void) {
    // 减少调试信息，提高性能
    static uint32_t check_count = 0;
    if (check_count % 100 == 0) {
        check_count = 0;
        printf("UVC: State=%d, ok=%d, len=%lu, streaming=%d\r\n", 
               uvc_app.state, jpeg_data_ok, jpeg_data_len, uvc_app.usb_streaming);
    }
    check_count++;
    
    // 确保USB streaming状态正确
    if (!uvc_app.usb_streaming) {
        // 对于UVC设备，默认开启streaming
        uvc_app.usb_streaming = 1;
    }
    
    // 确保状态机状态有效
    if (uvc_app.state > UVC_STATE_TRANSMITTING) {
        printf("UVC: Resetting invalid state: %d\r\n", uvc_app.state);
        uvc_app.state = UVC_STATE_CAPTURING;
    }
    
    static uint32_t last_capture = 0;
    static uint32_t last_transmit = 0;
    static uint8_t frame_skip_count = 0;
    static uint32_t frame_count = 0;
    
    switch (uvc_app.state) {
        case UVC_STATE_IDLE:
            // Waiting for start command
            break;
            
        case UVC_STATE_CAPTURING:
            // 检查是否超时没有数据
            if (HAL_GetTick() - last_capture > 3000) {
                last_capture = HAL_GetTick();
                printf("UVC: No frame for 3s, restarting\r\n");
                DCMI_Start_JPEG();
            }
            
            // Check if JPEG frame is ready
            if (jpeg_data_ok == 1) {
                // 直接使用jpeg_buf，不需要复制
                if (jpeg_data_len > 100 && jpeg_data_len < JPEG_BUF_SIZE) {
                    // 检查JPEG头部，确保数据有效
                    if (jpeg_buf[0] == 0xFF && jpeg_buf[1] == 0xD8) {
                        // 确保使用最新的缓冲区数据
                        uvc_app.jpeg_size = jpeg_data_len;
                        uvc_app.frame_ready = 1;
                        
                        jpeg_data_ok = 2;  // Mark as processed
                        uvc_app.state = UVC_STATE_TRANSMITTING;
                        
                        // 每20帧输出一次调试信息
                        frame_count++;
                        if (frame_count % 20 == 0) {
                            printf("UVC: Frame captured: %lu bytes\r\n", jpeg_data_len);
                        }
                    } else {
                        // 无效的JPEG数据，跳过
                        if (check_count % 100 == 0) {
                            printf("UVC: Invalid JPEG data, skipping\r\n");
                        }
                        jpeg_data_ok = 0;
                        HAL_Delay(5);
                        DCMI_Start_JPEG();
                    }
                } else {
                    // Invalid size, restart capture
                    if (check_count % 100 == 0) {
                        printf("UVC: Invalid size: %lu bytes\r\n", jpeg_data_len);
                    }
                    jpeg_data_ok = 0;
                    HAL_Delay(5);
                    DCMI_Start_JPEG();
                }
            } else if (jpeg_data_ok == 0 && jpeg_data_len > 0) {
                // 数据长度有值但状态未更新，强制处理
                jpeg_data_ok = 1;
            }
            break;
            
        case UVC_STATE_TRANSMITTING:
            // Transmit JPEG over USB if streaming is enabled
            if (uvc_app.usb_streaming && uvc_app.frame_ready) {
                // 检查USB设备状态
                if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) {
                    printf("UVC: USB not configured\r\n");
                } else {
                    // 传输JPEG数据
                    printf("UVC: Transmitting JPEG: %lu bytes\r\n", uvc_app.jpeg_size);
                    uint8_t result = UVC_Transmit_FS(jpeg_buf, uvc_app.jpeg_size);
                    if (result == USBD_OK) {
                        printf("UVC: Transmit OK\r\n");
                    } else {
                        printf("UVC: Transmit failed: %d\r\n", result);
                    }
                }
                
                // 重置状态，准备下一帧
                uvc_app.frame_ready = 0;
            }
            
            // Restart capture for next frame
            if (jpeg_data_ok == 2 && !uvc_app.frame_ready) {
                // 快速重启，减少延迟
                jpeg_data_ok = 0;
                DCMI_Start_JPEG();
                last_capture = HAL_GetTick();
                uvc_app.state = UVC_STATE_CAPTURING;
            }
            break;
    }
}
