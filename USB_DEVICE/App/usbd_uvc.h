/* UVC (USB Video Class) Device Driver Header */
#ifndef __USBD_UVC_H
#define __USBD_UVC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_ioreq.h"

// UVC Class Codes
#define USB_DEVICE_CLASS_VIDEO              0x0E
#define VIDEO_SUBCLASS_CONTROL              0x01
#define VIDEO_SUBCLASS_STREAMING            0x02

// UVC Interface Protocol Codes
#define VIDEO_PROTOCOL_UNDEFINED            0x00

// Video Control Interface Descriptor Subtypes
#define VC_DESCRIPTOR_UNDEFINED             0x00
#define VC_HEADER                           0x01
#define VC_INPUT_TERMINAL                   0x02
#define VC_OUTPUT_TERMINAL                  0x03
#define VC_SELECTOR_UNIT                    0x04
#define VC_PROCESSING_UNIT                  0x05

// Video Streaming Interface Descriptor Subtypes
#define VS_UNDEFINED                        0x00
#define VS_INPUT_HEADER                     0x01
#define VS_FORMAT_MJPEG                     0x06
#define VS_FRAME_MJPEG                      0x07
#define VS_COLORFORMAT                      0x0D

// UVC Request Codes
#define UVC_SET_CUR                         0x01
#define UVC_GET_CUR                         0x81
#define UVC_GET_MIN                         0x82
#define UVC_GET_MAX                         0x83
#define UVC_GET_RES                         0x84
#define UVC_GET_INFO                        0x86
#define UVC_GET_DEF                         0x87

// VideoStreaming Interface Control Selectors
#define VS_PROBE_CONTROL                    0x01
#define VS_COMMIT_CONTROL                   0x02

// UVC Payload Header
#define UVC_HEADER_LENGTH                   2  // 简化的头部长度
#define UVC_HEADER_EOH                      0x80
#define UVC_HEADER_ERR                      0x40
#define UVC_HEADER_STI                      0x20
#define UVC_HEADER_RES                      0x10
#define UVC_HEADER_SCR                      0x08
#define UVC_HEADER_PTS                      0x04
#define UVC_HEADER_EOF                      0x02
#define UVC_HEADER_FID                      0x01

// Video Probe and Commit Controls
typedef struct {
    uint16_t bmHint;
    uint8_t  bFormatIndex;
    uint8_t  bFrameIndex;
    uint32_t dwFrameInterval;
    uint16_t wKeyFrameRate;
    uint16_t wPFrameRate;
    uint16_t wCompQuality;
    uint16_t wCompWindowSize;
    uint16_t wDelay;
    uint32_t dwMaxVideoFrameSize;
    uint32_t dwMaxPayloadTransferSize;
} __attribute__((packed)) UVC_ProbeCommitControl;

// UVC Configuration
#define UVC_WIDTH                           320
#define UVC_HEIGHT                          240
#define UVC_FPS                             15
#define UVC_MAX_FRAME_SIZE                  (40 * 1024)  // 40KB max JPEG frame
#define UVC_PACKET_SIZE                     512          // USB FS max packet

// UVC Endpoints
#define UVC_IN_EP                           0x81
#define UVC_CMD_EP                          0x82

// UVC Interface Numbers
#define UVC_VC_IF_NUM                       0x00
#define UVC_VS_IF_NUM                       0x01

// UVC Class Structure
typedef struct {
    uint8_t  interface;
    uint8_t  alt_setting;
    uint8_t  *frame_buffer;
    uint32_t frame_size;
    uint32_t frame_offset;
    uint8_t  frame_id;
    uint8_t  streaming;
    UVC_ProbeCommitControl probe;
    UVC_ProbeCommitControl commit;
} USBD_UVC_HandleTypeDef;

// UVC Interface Callback
typedef struct {
    int8_t (*Init)(void);
    int8_t (*DeInit)(void);
    int8_t (*Control)(uint8_t cmd, uint8_t *pbuf, uint16_t length);
    int8_t (*StartStreaming)(void);
    int8_t (*StopStreaming)(void);
} USBD_UVC_ItfTypeDef;

// External declarations
extern USBD_ClassTypeDef USBD_UVC;
#define USBD_UVC_CLASS &USBD_UVC

// Function prototypes
uint8_t USBD_UVC_RegisterInterface(USBD_HandleTypeDef *pdev, USBD_UVC_ItfTypeDef *fops);
uint8_t USBD_UVC_TransmitFrame(USBD_HandleTypeDef *pdev, uint8_t *frame, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* __USBD_UVC_H */
