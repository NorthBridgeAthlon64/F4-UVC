/* UVC Interface Implementation */
#include "usbd_uvc_if.h"
#include "usb_device.h"
#include "uvc_app.h"

// Private function prototypes
static int8_t UVC_Init_FS(void);
static int8_t UVC_DeInit_FS(void);
static int8_t UVC_Control_FS(uint8_t cmd, uint8_t *pbuf, uint16_t length);
static int8_t UVC_StartStreaming_FS(void);
static int8_t UVC_StopStreaming_FS(void);

// UVC Interface callbacks
USBD_UVC_ItfTypeDef USBD_UVC_fops_FS = {
    UVC_Init_FS,
    UVC_DeInit_FS,
    UVC_Control_FS,
    UVC_StartStreaming_FS,
    UVC_StopStreaming_FS
};

// External USB device handle
extern USBD_HandleTypeDef hUsbDeviceFS;

/**
  * @brief  Initialize UVC interface
  */
static int8_t UVC_Init_FS(void) {
    return 0;
}

/**
  * @brief  DeInitialize UVC interface
  */
static int8_t UVC_DeInit_FS(void) {
    return 0;
}

/**
  * @brief  Handle UVC control requests
  */
static int8_t UVC_Control_FS(uint8_t cmd, uint8_t *pbuf, uint16_t length) {
    return 0;
}

/**
  * @brief  Start video streaming
  */
static int8_t UVC_StartStreaming_FS(void) {
    // 通知应用层开始USB流传输
    UVC_App_SetStreaming(1);
    return 0;
}

/**
  * @brief  Stop video streaming
  */
static int8_t UVC_StopStreaming_FS(void) {
    // 通知应用层停止USB流传输
    UVC_App_SetStreaming(0);
    return 0;
}

/**
  * @brief  Transmit video frame over USB
  */
uint8_t UVC_Transmit_FS(uint8_t *frame, uint32_t size) {
    return USBD_UVC_TransmitFrame(&hUsbDeviceFS, frame, size);
}
