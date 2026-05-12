/* UVC Interface Header */
#ifndef __USBD_UVC_IF_H__
#define __USBD_UVC_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_uvc.h"

// External declarations
extern USBD_UVC_ItfTypeDef USBD_UVC_fops_FS;

// Function prototypes
uint8_t UVC_Transmit_FS(uint8_t *frame, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* __USBD_UVC_IF_H__ */
