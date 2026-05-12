/* UVC Application Header */
#ifndef __UVC_APP_H__
#define __UVC_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

// Application configuration
#define UVC_FRAME_WIDTH         320
#define UVC_FRAME_HEIGHT        240
#define UVC_JPEG_BUFFER_SIZE    (20 * 1024)
#define UVC_RGB565_BUFFER_SIZE  (UVC_FRAME_WIDTH * UVC_FRAME_HEIGHT * 2)

// Application states
typedef enum {
    UVC_STATE_IDLE = 0,
    UVC_STATE_CAPTURING,
    UVC_STATE_JPEG_READY,
    UVC_STATE_DECODING,
    UVC_STATE_DISPLAYING,
    UVC_STATE_TRANSMITTING
} UVC_StateTypeDef;

// Application structure
typedef struct {
    UVC_StateTypeDef state;
    uint8_t *jpeg_buffer;
    uint32_t jpeg_size;
    uint16_t *rgb565_buffer;
    uint8_t frame_ready;
    uint8_t usb_streaming;
} UVC_AppTypeDef;

// External declaration of application instance
extern UVC_AppTypeDef uvc_app;

// Function prototypes
void UVC_App_Init(void);
void UVC_App_Process(void);
void UVC_App_StartCapture(void);
void UVC_App_StopCapture(void);
uint8_t UVC_App_IsStreaming(void);
void UVC_App_SetStreaming(uint8_t enable);

// JPEG to RGB565 conversion
int JPEG_DecodeToRGB565(uint8_t *jpeg_data, uint32_t jpeg_size, 
                        uint16_t *rgb565_out, uint16_t *width, uint16_t *height);

#ifdef __cplusplus
}
#endif

#endif /* __UVC_APP_H__ */
