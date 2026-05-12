#include "usb_uvc_attach.h"
#include "main.h"
#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_uvc.h"
#include "usbd_uvc_if.h"

void USB_AttachUVC_Stack(void)
{
  (void)USBD_DeInit(&hUsbDeviceFS);

  if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK)
  {
    Error_Handler();
  }
  if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_UVC) != USBD_OK)
  {
    Error_Handler();
  }
  if (USBD_UVC_RegisterInterface(&hUsbDeviceFS, &USBD_UVC_fops_FS) != USBD_OK)
  {
    Error_Handler();
  }
  if (USBD_Start(&hUsbDeviceFS) != USBD_OK)
  {
    Error_Handler();
  }
}
