#ifndef USB_UVC_ATTACH_H
#define USB_UVC_ATTACH_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Replace the USB stack class registered by CubeMX (CDC) with UVC.
 * @note  Call only from MX_USB_DEVICE_Init PostTreatment after CDC template init.
 */
void USB_AttachUVC_Stack(void);

#ifdef __cplusplus
}
#endif

#endif /* USB_UVC_ATTACH_H */
