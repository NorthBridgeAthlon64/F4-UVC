/* UVC (USB Video Class) Device Driver Implementation */
#include "usbd_uvc.h"
#include "usbd_ctlreq.h"
#include <string.h>

// UVC Class Callbacks
static uint8_t USBD_UVC_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_UVC_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_UVC_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t USBD_UVC_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_UVC_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_UVC_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t *USBD_UVC_GetFSCfgDesc(uint16_t *length);
static uint8_t *USBD_UVC_GetDeviceQualifierDescriptor(uint16_t *length);

// UVC Class Definition
USBD_ClassTypeDef USBD_UVC = {
    USBD_UVC_Init,
    USBD_UVC_DeInit,
    USBD_UVC_Setup,
    NULL,
    USBD_UVC_EP0_RxReady,
    USBD_UVC_DataIn,
    USBD_UVC_DataOut,
    NULL,
    NULL,
    NULL,
    NULL,
    USBD_UVC_GetFSCfgDesc,
    NULL,
    USBD_UVC_GetDeviceQualifierDescriptor,
};

// UVC Configuration Descriptor
#define UVC_CONFIG_DESC_SIZE 163

__ALIGN_BEGIN static uint8_t USBD_UVC_CfgFSDesc[UVC_CONFIG_DESC_SIZE] __ALIGN_END = {
    // Configuration Descriptor (9 bytes)
    0x09,       // bLength
    0x02,       // bDescriptorType (CONFIGURATION)
    LOBYTE(UVC_CONFIG_DESC_SIZE), 
    HIBYTE(UVC_CONFIG_DESC_SIZE), 
    0x02,       // bNumInterfaces (2: VC + VS)
    0x01,       // bConfigurationValue
    0x00,       // iConfiguration
    0x80,       // bmAttributes (Bus Powered)
    0xFA,       // bMaxPower (500mA)
    
    // Interface Association Descriptor (8 bytes)
    0x08,       // bLength
    0x0B,       // bDescriptorType (IAD)
    0x00,       // bFirstInterface
    0x02,       // bInterfaceCount
    0x0E,       // bFunctionClass (Video)
    0x03,       // bFunctionSubClass (Video Interface Collection)
    0x00,       // bFunctionProtocol
    0x00,       // iFunction - 修复：改回0避免字符串问题
    
    // Video Control Interface Descriptor (9 bytes)
    0x09,       // bLength
    0x04,       // bDescriptorType (INTERFACE)
    0x00,       // bInterfaceNumber
    0x00,       // bAlternateSetting
    0x00,       // bNumEndpoints
    0x0E,       // bInterfaceClass (Video)
    0x01,       // bInterfaceSubClass (Video Control)
    0x00,       // bInterfaceProtocol
    0x00,       // iInterface - 修复：改回0避免字符串问题
    
    // VC Interface Header Descriptor (13 bytes)
    0x0D,       // bLength
    0x24,       // bDescriptorType (CS_INTERFACE)
    0x01,       // bDescriptorSubtype (VC_HEADER)
    0x00, 0x01, // bcdUVC (1.1) - 修复：升级到UVC 1.1
    0x33, 0x00, // wTotalLength (40 bytes for VC descriptors)
    0x00, 0x6C, 0xDC, 0x02, // dwClockFrequency (48MHz) - 修复：标准时钟
    0x01,       // bInCollection
    0x01,       // baInterfaceNr(1)
    
    // Camera Terminal Descriptor (18 bytes)
    0x12,       // bLength
    0x24,       // bDescriptorType (CS_INTERFACE)
    0x02,       // bDescriptorSubtype (VC_INPUT_TERMINAL)
    0x01,       // bTerminalID
    0x01, 0x02, // wTerminalType (ITT_CAMERA)
    0x00,       // bAssocTerminal
    0x00,       // iTerminal
    0x00, 0x00, // wObjectiveFocalLengthMin
    0x00, 0x00, // wObjectiveFocalLengthMax
    0x00, 0x00, // wOcularFocalLength
    0x03,       // bControlSize
    0x00, 0x00, 0x00, // bmControls
    
    // Processing Unit Descriptor (11 bytes)
    0x0B,       // bLength
    0x24,       // bDescriptorType (CS_INTERFACE)
    0x05,       // bDescriptorSubtype (VC_PROCESSING_UNIT)
    0x02,       // bUnitID
    0x01,       // bSourceID
    0x00, 0x00, // wMaxMultiplier
    0x02,       // bControlSize
    0x01, 0x00, // bmControls - 修复：支持Brightness控制
    0x00,       // iProcessing
    
    // Output Terminal Descriptor (9 bytes)
    0x09,       // bLength
    0x24,       // bDescriptorType (CS_INTERFACE)
    0x03,       // bDescriptorSubtype (VC_OUTPUT_TERMINAL)
    0x03,       // bTerminalID
    0x01, 0x01, // wTerminalType (TT_STREAMING)
    0x00,       // bAssocTerminal
    0x02,       // bSourceID
    0x00,       // iTerminal
    
    // Video Streaming Interface Descriptor (Alt 0) (9 bytes)
    0x09,       // bLength
    0x04,       // bDescriptorType (INTERFACE)
    0x01,       // bInterfaceNumber
    0x00,       // bAlternateSetting
    0x00,       // bNumEndpoints (no streaming)
    0x0E,       // bInterfaceClass (Video)
    0x02,       // bInterfaceSubClass (Video Streaming)
    0x00,       // bInterfaceProtocol
    0x00,       // iInterface
    
    // VS Input Header Descriptor (14 bytes)
    0x0E,       // bLength
    0x24,       // bDescriptorType (CS_INTERFACE)
    0x01,       // bDescriptorSubtype (VS_INPUT_HEADER)
    0x01,       // bNumFormats
    0x3D, 0x00, // wTotalLength (71 bytes for VS descriptors)
    0x81,       // bEndpointAddress
    0x00,       // bmInfo
    0x03,       // bTerminalLink
    0x00,       // bStillCaptureMethod
    0x00,       // bTriggerSupport
    0x00,       // bTriggerUsage
    0x01,       // bControlSize
    0x00,       // bmaControls(0)
    
    // VS Format MJPEG Descriptor (11 bytes)
    0x0B,       // bLength
    0x24,       // bDescriptorType (CS_INTERFACE)
    0x06,       // bDescriptorSubtype (VS_FORMAT_MJPEG)
    0x01,       // bFormatIndex
    0x01,       // bNumFrameDescriptors
    0x01,       // bmFlags (Fixed size samples)
    0x01,       // bDefaultFrameIndex
    0x00,       // bAspectRatioX
    0x00,       // bAspectRatioY
    0x00,       // bmInterlaceFlags
    0x00,       // bCopyProtect
    
    // VS Frame MJPEG Descriptor (30 bytes) - 320x240 @ 15fps
    0x1E,       // bLength
    0x24,       // bDescriptorType (CS_INTERFACE)
    0x07,       // bDescriptorSubtype (VS_FRAME_MJPEG)
    0x01,       // bFrameIndex
    0x00,       // bmCapabilities
    0x40, 0x01, // wWidth (320)
    0xF0, 0x00, // wHeight (240)
    0x00, 0x00, 0x65, 0x04,  // dwMinBitRate (320*240*2*8*15 = 18432000)
    0x00, 0x00, 0xCA, 0x08,  // dwMaxBitRate (320*240*2*8*30 = 36864000)
    0x00, 0x00, 0x60, 0x09,  // dwMaxVideoFrameBufferSize (38400 bytes)
    0x15, 0x16, 0x05, 0x00,  // dwDefaultFrameInterval (666666 = 15fps)
    0x01,       // bFrameIntervalType (1 discrete)
    0x15, 0x16, 0x05, 0x00,  // dwFrameInterval(1) (666666 = 15fps)
    
    // Color Matching Descriptor (6 bytes)
    0x06,       // bLength
    0x24,       // bDescriptorType (CS_INTERFACE)
    0x0D,       // bDescriptorSubtype (VS_COLORFORMAT)
    0x01,       // bColorPrimaries (BT.709)
    0x01,       // bTransferCharacteristics (BT.709)
    0x04,       // bMatrixCoefficients (SMPTE 170M)
    
    // Video Streaming Interface Descriptor (Alt 1) (9 bytes)
    0x09,       // bLength
    0x04,       // bDescriptorType (INTERFACE)
    0x01,       // bInterfaceNumber
    0x01,       // bAlternateSetting
    0x01,       // bNumEndpoints
    0x0E,       // bInterfaceClass (Video)
    0x02,       // bInterfaceSubClass (Video Streaming)
    0x00,       // bInterfaceProtocol
    0x00,       // iInterface
    
    // Endpoint Descriptor (7 bytes) - Isochronous IN
    0x07,       // bLength
    0x05,       // bDescriptorType (ENDPOINT)
    UVC_IN_EP,  // bEndpointAddress (0x81 = IN)
    0x05,       // bmAttributes (Isochronous, Async)
    LOBYTE(UVC_PACKET_SIZE), 
    HIBYTE(UVC_PACKET_SIZE), // wMaxPacketSize (512)
    0x01        // bInterval (1ms)
};

// Device Qualifier Descriptor
__ALIGN_BEGIN static uint8_t USBD_UVC_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END = {
    USB_LEN_DEV_QUALIFIER_DESC,
    USB_DESC_TYPE_DEVICE_QUALIFIER,
    0x00, 0x02,
    0x00, 0x00, 0x00,
    0x40,
    0x01,
    0x00,
};

static USBD_UVC_ItfTypeDef *UVC_fops = NULL;

/**
  * @brief  Initialize UVC interface
  */
static uint8_t USBD_UVC_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
    USBD_UVC_HandleTypeDef *huvc;
    
    // Allocate UVC handle
    pdev->pClassData = USBD_malloc(sizeof(USBD_UVC_HandleTypeDef));
    if (pdev->pClassData == NULL) {
        return USBD_FAIL;
    }
    
    huvc = (USBD_UVC_HandleTypeDef *)pdev->pClassData;
    memset(huvc, 0, sizeof(USBD_UVC_HandleTypeDef));
    
    // Initialize default probe/commit controls
    huvc->probe.bmHint = 0x0001;
    huvc->probe.bFormatIndex = 0x01;
    huvc->probe.bFrameIndex = 0x01;
    huvc->probe.dwFrameInterval = 666666;  // 15fps
    huvc->probe.wKeyFrameRate = 0;
    huvc->probe.wPFrameRate = 0;
    huvc->probe.wCompQuality = 50;
    huvc->probe.wCompWindowSize = 0;
    huvc->probe.wDelay = 0;
    huvc->probe.dwMaxVideoFrameSize = UVC_MAX_FRAME_SIZE;
    huvc->probe.dwMaxPayloadTransferSize = UVC_PACKET_SIZE;
    
    memcpy(&huvc->commit, &huvc->probe, sizeof(UVC_ProbeCommitControl));
    
    // Open endpoint
    USBD_LL_OpenEP(pdev, UVC_IN_EP, USBD_EP_TYPE_ISOC, UVC_PACKET_SIZE);
    pdev->ep_in[UVC_IN_EP & 0xFU].is_used = 1U;
    
    // Initialize interface
    if (UVC_fops != NULL) {
        UVC_fops->Init();
    }
    
    return USBD_OK;
}

/**
  * @brief  DeInitialize UVC interface
  */
static uint8_t USBD_UVC_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
    // Close endpoint
    USBD_LL_CloseEP(pdev, UVC_IN_EP);
    pdev->ep_in[UVC_IN_EP & 0xFU].is_used = 0U;
    
    // DeInit interface
    if (UVC_fops != NULL) {
        UVC_fops->DeInit();
    }
    
    // Free class data
    if (pdev->pClassData != NULL) {
        USBD_free(pdev->pClassData);
        pdev->pClassData = NULL;
    }
    
    return USBD_OK;
}

/**
  * @brief  Handle UVC Setup requests
  */
static uint8_t USBD_UVC_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req) {
    USBD_UVC_HandleTypeDef *huvc = (USBD_UVC_HandleTypeDef *)pdev->pClassData;
    uint8_t *pbuf = NULL;
    uint16_t len = 0;
    uint8_t ret = USBD_OK;
    
    switch (req->bmRequest & USB_REQ_TYPE_MASK) {
        case USB_REQ_TYPE_CLASS:
            // 只处理Video Streaming接口的请求
            if ((req->wIndex & 0xFF) == UVC_VS_IF_NUM) {
                uint8_t cs = (req->wValue >> 8) & 0xFF;  // Control Selector
                
                switch (req->bRequest) {
                    case UVC_SET_CUR:
                        if (cs == VS_PROBE_CONTROL) {
                            USBD_CtlPrepareRx(pdev, (uint8_t *)&huvc->probe, req->wLength);
                        } else if (cs == VS_COMMIT_CONTROL) {
                            USBD_CtlPrepareRx(pdev, (uint8_t *)&huvc->commit, req->wLength);
                        }
                        break;
                        
                    case UVC_GET_CUR:
                        if (cs == VS_PROBE_CONTROL) {
                            pbuf = (uint8_t *)&huvc->probe;
                            len = sizeof(UVC_ProbeCommitControl);
                        } else if (cs == VS_COMMIT_CONTROL) {
                            pbuf = (uint8_t *)&huvc->commit;
                            len = sizeof(UVC_ProbeCommitControl);
                        }
                        if (pbuf != NULL) {
                            len = (req->wLength < len) ? req->wLength : len;
                            USBD_CtlSendData(pdev, pbuf, len);
                        }
                        break;
                        
                    case UVC_GET_MIN:
                    case UVC_GET_MAX:
                    case UVC_GET_DEF:
                        // 对于MIN/MAX/DEF，返回相同的probe值
                        if (cs == VS_PROBE_CONTROL || cs == VS_COMMIT_CONTROL) {
                            pbuf = (uint8_t *)&huvc->probe;
                            len = sizeof(UVC_ProbeCommitControl);
                            len = (req->wLength < len) ? req->wLength : len;
                            USBD_CtlSendData(pdev, pbuf, len);
                        }
                        break;
                        
                    case UVC_GET_INFO:
                        // 返回支持GET和SET
                        pbuf = (uint8_t[]){0x03};
                        USBD_CtlSendData(pdev, pbuf, 1);
                        break;
                        
                    case UVC_GET_RES:
                        // 返回分辨率信息（全0表示不支持）
                        pbuf = (uint8_t *)&huvc->probe;
                        len = sizeof(UVC_ProbeCommitControl);
                        memset(pbuf, 0, len);
                        len = (req->wLength < len) ? req->wLength : len;
                        USBD_CtlSendData(pdev, pbuf, len);
                        break;
                        
                    default:
                        USBD_CtlError(pdev, req);
                        ret = USBD_FAIL;
                        break;
                }
            }
            break;
            
        case USB_REQ_TYPE_STANDARD:
            switch (req->bRequest) {
                case USB_REQ_SET_INTERFACE:
                    if (pdev->dev_state == USBD_STATE_CONFIGURED) {
                        huvc->alt_setting = (uint8_t)(req->wValue);
                        
                        if ((req->wIndex & 0xFF) == UVC_VS_IF_NUM) {
                            if (huvc->alt_setting == 1) {
                                // 启动流传输
                                huvc->streaming = 1;
                                if (UVC_fops != NULL) {
                                    UVC_fops->StartStreaming();
                                }
                            } else {
                                // 停止流传输
                                huvc->streaming = 0;
                                if (UVC_fops != NULL) {
                                    UVC_fops->StopStreaming();
                                }
                            }
                        }
                    }
                    break;
                    
                case USB_REQ_GET_INTERFACE:
                    if (pdev->dev_state == USBD_STATE_CONFIGURED) {
                        USBD_CtlSendData(pdev, &huvc->alt_setting, 1);
                    }
                    break;
                    
                default:
                    USBD_CtlError(pdev, req);
                    ret = USBD_FAIL;
                    break;
            }
            break;
            
        default:
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
            break;
    }
    
    return ret;
}

/**
  * @brief  Handle data IN stage
  */
static uint8_t USBD_UVC_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum) {
    USBD_UVC_HandleTypeDef *huvc = (USBD_UVC_HandleTypeDef *)pdev->pClassData;
    
    if (huvc == NULL || !huvc->streaming) {
        return USBD_OK;
    }
    
    // Continue sending frame data if available
    if (huvc->frame_offset < huvc->frame_size) {
        uint32_t remaining = huvc->frame_size - huvc->frame_offset;
        uint32_t packet_size = (remaining > UVC_PACKET_SIZE - UVC_HEADER_LENGTH) ? 
                               (UVC_PACKET_SIZE - UVC_HEADER_LENGTH) : remaining;
        
        uint8_t packet[UVC_PACKET_SIZE];
        
        // Build UVC payload header (simplified 2-byte header)
        packet[0] = UVC_HEADER_LENGTH;
        packet[1] = (huvc->frame_id & 0x01);  // FID bit
        
        if (huvc->frame_offset + packet_size >= huvc->frame_size) {
            packet[1] |= UVC_HEADER_EOF;  // End of frame
        }
        
        // Copy frame data
        memcpy(&packet[UVC_HEADER_LENGTH], 
               &huvc->frame_buffer[huvc->frame_offset], 
               packet_size);
        
        huvc->frame_offset += packet_size;
        
        // Transmit packet
        USBD_LL_Transmit(pdev, UVC_IN_EP, packet, packet_size + UVC_HEADER_LENGTH);
        
        // Check if frame complete
        if (huvc->frame_offset >= huvc->frame_size) {
            huvc->frame_id ^= 1;  // Toggle frame ID
            huvc->frame_offset = 0;
            huvc->frame_size = 0;
        }
    }
    
    return USBD_OK;
}

/**
  * @brief  Handle data OUT stage
  */
static uint8_t USBD_UVC_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum) {
    return USBD_OK;
}

/**
  * @brief  Handle EP0 Rx Ready event
  */
static uint8_t USBD_UVC_EP0_RxReady(USBD_HandleTypeDef *pdev) {
    USBD_UVC_HandleTypeDef *huvc = (USBD_UVC_HandleTypeDef *)pdev->pClassData;
    
    if (huvc != NULL && UVC_fops != NULL) {
        UVC_fops->Control(0, (uint8_t *)&huvc->probe, sizeof(UVC_ProbeCommitControl));
    }
    
    return USBD_OK;
}

/**
  * @brief  Get configuration descriptor
  */
static uint8_t *USBD_UVC_GetFSCfgDesc(uint16_t *length) {
    *length = sizeof(USBD_UVC_CfgFSDesc);
    return USBD_UVC_CfgFSDesc;
}

/**
  * @brief  Get device qualifier descriptor
  */
static uint8_t *USBD_UVC_GetDeviceQualifierDescriptor(uint16_t *length) {
    *length = sizeof(USBD_UVC_DeviceQualifierDesc);
    return USBD_UVC_DeviceQualifierDesc;
}

/**
  * @brief  Register UVC interface callbacks
  */
uint8_t USBD_UVC_RegisterInterface(USBD_HandleTypeDef *pdev, USBD_UVC_ItfTypeDef *fops) {
    if (fops == NULL) {
        return USBD_FAIL;
    }
    
    UVC_fops = fops;
    return USBD_OK;
}

/**
  * @brief  Transmit video frame
  */
uint8_t USBD_UVC_TransmitFrame(USBD_HandleTypeDef *pdev, uint8_t *frame, uint32_t size) {
    USBD_UVC_HandleTypeDef *huvc = (USBD_UVC_HandleTypeDef *)pdev->pClassData;
    
    if (huvc == NULL || size == 0 || size > UVC_MAX_FRAME_SIZE) {
        return USBD_FAIL;
    }
    
    // 强制启用streaming，确保传输不会因为streaming标志而失败
    huvc->streaming = 1;
    
    // 等待之前的帧传输完成，但添加超时机制
    static uint32_t last_busy_time = 0;
    if (huvc->frame_size > 0) {
        uint32_t current_time = HAL_GetTick();
        if (current_time - last_busy_time > 100) {
            // 如果超过100ms还在忙，强制重置frame_size
            huvc->frame_size = 0;
            huvc->frame_offset = 0;
        } else {
            return USBD_BUSY;
        }
    }
    last_busy_time = HAL_GetTick();
    
    // Set new frame
    huvc->frame_buffer = frame;
    huvc->frame_size = size;
    huvc->frame_offset = 0;
    
    // Start transmission
    uint32_t packet_size = (size > UVC_PACKET_SIZE - UVC_HEADER_LENGTH) ? 
                           (UVC_PACKET_SIZE - UVC_HEADER_LENGTH) : size;
    
    static uint8_t packet[UVC_PACKET_SIZE]; // 使用静态变量避免栈溢出
    
    // Build UVC payload header (标准2-byte header)
    packet[0] = UVC_HEADER_LENGTH;         // 头部长度
    packet[1] = (huvc->frame_id & 0x01);   // FID bit
    
    if (packet_size >= size) {
        packet[1] |= UVC_HEADER_EOF;       // End of frame
    }
    
    // 确保数据拷贝安全
    if (packet_size > 0) {
        memcpy(&packet[UVC_HEADER_LENGTH], frame, packet_size);
    }
    
    huvc->frame_offset = packet_size;
    
    // Transmit first packet
    uint8_t result = USBD_LL_Transmit(pdev, UVC_IN_EP, packet, packet_size + UVC_HEADER_LENGTH);
    
    return result;
}
