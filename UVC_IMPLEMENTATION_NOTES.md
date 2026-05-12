# UVC实现说明

## 已实现的功能

### 1. USB UVC设备类驱动
- ✅ UVC描述符配置（`usbd_uvc.c/h`）
- ✅ Video Control接口
- ✅ Video Streaming接口
- ✅ MJPEG格式支持
- ✅ Probe/Commit控制
- ✅ 同步传输端点

### 2. OV2640 JPEG模式配置
- ✅ JPEG模式初始化
- ✅ 320x240分辨率设置
- ✅ DCMI JPEG捕获配置

### 3. DCMI+DMA配置
- ✅ JPEG模式DMA初始化
- ✅ 快照模式捕获
- ✅ 帧完成中断处理

### 4. 应用层状态机
- ✅ 图像捕获状态
- ✅ JPEG数据就绪状态
- ✅ 解码状态
- ✅ 显示状态
- ✅ USB传输状态

### 5. 本地显示功能
- ✅ RGB565格式转换
- ✅ LCD显示接口
- ⚠️ JPEG解码器（简化实现，需完整解码库）

## 需要注意的事项

### JPEG解码器
当前`JPEG_DecodeToRGB565`函数是一个占位实现。要实现完整的JPEG解码，有以下选项：

#### 选项1：使用NanoJPEG库
```c
// 在uvc_app.c中集成NanoJPEG
#include "NanoJpeg.h"

int JPEG_DecodeToRGB565(uint8_t *jpeg_data, uint32_t jpeg_size, 
                        uint16_t *rgb565_out, uint16_t *width, uint16_t *height) {
    nj_ctx *ctx = nj_create();
    if (nj_decode(ctx, jpeg_data, jpeg_size) != NJ_OK) {
        nj_destroy(ctx);
        return -1;
    }
    
    // 获取解码后的RGB数据并转换为RGB565
    const unsigned char *rgb = nj_get_image(ctx, NULL);
    *width = nj_get_width(ctx);
    *height = nj_get_height(ctx);
    
    for (int i = 0; i < (*width) * (*height); i++) {
        uint8_t r = rgb[i*3];
        uint8_t g = rgb[i*3+1];
        uint8_t b = rgb[i*3+2];
        rgb565_out[i] = RGB888_to_RGB565(r, g, b);
    }
    
    nj_destroy(ctx);
    return 0;
}
```

#### 选项2：使用STM32硬件JPEG解码器（仅限STM32F7/H7）
如果使用STM32F7或H7系列，可以使用硬件JPEG解码器。

#### 选项3：直接显示JPEG（不解码）
如果LCD控制器支持JPEG显示，可以跳过解码步骤。

### USB传输优化

当前实现使用同步传输。如果遇到带宽问题，可以考虑：

1. **降低分辨率**：160x120可以显著减少数据量
2. **调整JPEG质量**：在OV2640中配置压缩率
3. **使用批量传输**：修改UVC描述符使用批量端点（非标准但某些主机支持）

### 内存优化

如果RAM不足，可以：

1. 使用单缓冲区（JPEG和RGB565共用）
2. 减小缓冲区大小
3. 使用外部SRAM

## 编译配置

### 必需的编译选项

在项目设置中确保：

1. **C/C++ Build → Settings → MCU GCC Compiler → Optimization**
   - 优化级别：-O2或-O3（提高性能）

2. **C/C++ Build → Settings → MCU GCC Linker → Miscellaneous**
   - 确保堆栈大小足够：
     - Heap Size: 0x2000 (8KB)
     - Stack Size: 0x1000 (4KB)

### 链接器脚本修改

如果遇到内存不足，在`.ld`文件中调整：

```ld
_Min_Heap_Size = 0x2000;  /* 8KB */
_Min_Stack_Size = 0x1000; /* 4KB */
```

## 调试技巧

### 1. 使用串口调试
```c
printf("JPEG size: %lu bytes\n", jpeg_data_len);
printf("Frame captured: %d\n", jpeg_data_ok);
```

### 2. LED指示
- LED1闪烁：正常捕获帧
- LED0常亮：系统运行
- LED1常灭：捕获停止

### 3. USB分析
使用USB分析工具（如Wireshark + USBPcap）查看USB流量。

## 性能调优

### 提高帧率
1. 增加USB端点大小（最大512字节）
2. 优化DCMI DMA配置
3. 减少图像处理时间

### 降低延迟
1. 使用双缓冲
2. 减少状态机切换时间
3. 优化中断处理

## 已知限制

1. **USB Full Speed带宽**：最大12Mbps，限制了分辨率和帧率
2. **JPEG解码性能**：软件解码较慢，建议使用硬件加速
3. **内存限制**：STM32F407有192KB RAM，大分辨率需要外部RAM

## 下一步开发

### 短期目标
- [ ] 集成完整的JPEG解码库
- [ ] 优化USB传输性能
- [ ] 添加错误处理和恢复机制

### 长期目标
- [ ] 支持多种分辨率切换
- [ ] 添加图像增强功能
- [ ] 实现H.264编码（需要更强大的MCU）
- [ ] 支持音频流（UVC+UAC）

## 测试清单

- [ ] USB设备枚举成功
- [ ] Windows识别为UVC设备
- [ ] VLC能够打开视频流
- [ ] 帧率达到预期（10-15fps）
- [ ] LCD显示正常
- [ ] 按键控制响应
- [ ] 长时间运行稳定

## 常见问题解答

**Q: 为什么帧率只有5fps？**
A: 检查JPEG大小，如果超过20KB，USB带宽可能不足。降低分辨率或提高压缩率。

**Q: USB设备无法识别？**
A: 检查USB描述符配置，确保VID/PID正确，使用USBView工具查看枚举过程。

**Q: LCD显示花屏？**
A: JPEG解码可能有问题，检查解码器实现或使用测试图案验证LCD功能。

**Q: 编译错误：undefined reference to 'nj_decode'**
A: 需要实现或链接JPEG解码库，当前是占位实现。

## 联系和支持

如有问题，请检查：
1. STM32 HAL库文档
2. USB.org的UVC规范
3. OV2640数据手册
