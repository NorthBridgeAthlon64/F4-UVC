# 内存优化说明

## 问题

STM32F407ZGT6的RAM配置：
- 总RAM：192KB (128KB + 64KB)
- 可用RAM：约180KB（扣除系统开销）

原始内存分配：
- JPEG缓冲区（main.c）：40KB
- JPEG缓冲区（uvc_app.c）：40KB（重复！）
- RGB565缓冲区：150KB (320x240x2)
- USB缓冲区和其他：约10KB
- **总计：约240KB - 超出60KB！**

## 优化方案

### 已实施的优化

1. **移除重复的JPEG缓冲区**
   - 删除了uvc_app.c中的40KB JPEG缓冲区
   - 直接使用main.c中的jpeg_buf
   - 节省：40KB

2. **移除RGB565缓冲区**
   - 删除了150KB的RGB565缓冲区
   - 暂时禁用本地LCD显示功能
   - 节省：150KB

3. **优化后的内存使用**
   - JPEG缓冲区：40KB（共享）
   - USB相关：约10KB
   - 堆栈和其他：约10KB
   - **总计：约60KB - 完全可用！**

## 功能影响

### 保留的功能
✅ OV2640 JPEG图像采集（320x240）
✅ USB UVC视频流传输
✅ 计算机端视频查看
✅ 按键控制

### 暂时禁用的功能
❌ 本地LCD实时显示（需要额外RAM）

## 恢复LCD显示的方案

如果需要本地LCD显示，有以下选择：

### 方案1：使用外部SRAM（推荐）
STM32F407支持FSMC接口连接外部SRAM：

```c
// 配置外部SRAM作为RGB565缓冲区
#define EXT_SRAM_ADDR  0x68000000
uint16_t *rgb565_buffer = (uint16_t *)EXT_SRAM_ADDR;
```

优点：
- 不占用内部RAM
- 可以支持更大的缓冲区
- 性能足够

缺点：
- 需要外部硬件
- 增加成本

### 方案2：逐行解码显示
不分配完整的RGB565缓冲区，而是逐行解码并显示：

```c
// 伪代码
for (int y = 0; y < height; y++) {
    // 解码一行
    decode_jpeg_line(jpeg_data, line_buffer, y);
    // 立即显示这一行
    LCD_DrawLine(0, y, width, line_buffer);
}
```

优点：
- 只需要一行的缓冲区（约1.5KB）
- 不需要外部硬件

缺点：
- 实现复杂
- 显示速度较慢
- 需要支持逐行解码的JPEG库

### 方案3：降低分辨率
使用更小的分辨率，如160x120：

```c
#define UVC_FRAME_WIDTH   160
#define UVC_FRAME_HEIGHT  120
// RGB565缓冲区：160x120x2 = 38.4KB
```

优点：
- 简单直接
- 可以同时显示和传输

缺点：
- 图像质量降低
- 分辨率较小

### 方案4：使用DMA直接传输到LCD
配置DCMI+DMA直接将RGB565数据传输到LCD（不使用JPEG）：

```c
// 配置OV2640为RGB565模式
OV2640_RGB565_Mode();
// DCMI DMA直接到LCD
DCMI_DMA_Init((uint32_t)&TFT_LCD->LCD_RAM, 0, 1, 
              DMA_MDATAALIGN_HALFWORD, DMA_MINC_DISABLE);
```

优点：
- 零RAM开销
- 实时显示
- 最快的方案

缺点：
- 不能同时USB传输（需要JPEG）
- 需要在JPEG和RGB565模式间切换

## 当前推荐配置

### 仅USB传输（当前实现）
```c
内存使用：约60KB
功能：USB UVC视频流
优点：内存充足，稳定可靠
```

### USB传输 + 外部SRAM显示
```c
内存使用：约60KB（内部）+ 150KB（外部）
功能：USB传输 + LCD显示
优点：功能完整
```

### USB传输 + 低分辨率显示
```c
内存使用：约100KB
分辨率：160x120
功能：USB传输 + LCD显示
优点：无需外部硬件
```

## 内存使用监控

### 查看内存使用
编译后查看.map文件：
```
OV2640.map
搜索 "Memory Configuration"
```

或查看编译输出：
```
arm-none-eabi-size OV2640.elf
   text    data     bss     dec     hex filename
  xxxxx    xxxx   xxxxx  xxxxxx  xxxxxx OV2640.elf
```

### 内存区域说明
- text：代码段（Flash）
- data：初始化数据（Flash + RAM）
- bss：未初始化数据（RAM）
- dec：总计（十进制）

### RAM使用计算
```
RAM使用 = data + bss
```

应该小于192KB（STM32F407的RAM大小）

## 进一步优化建议

### 1. 减小JPEG缓冲区
如果图像质量可以接受，可以减小缓冲区：
```c
#define jpeg_buf_size (20*1024)  // 从40KB减到20KB
```

### 2. 使用动态内存分配
对于不常用的大缓冲区，使用malloc/free：
```c
uint8_t *temp_buffer = malloc(10*1024);
// 使用
free(temp_buffer);
```

### 3. 优化USB缓冲区
减小USB传输缓冲区大小（如果可能）。

### 4. 使用CCM RAM
STM32F407有64KB的CCM RAM（Core Coupled Memory）：
```c
__attribute__((section(".ccmram")))
uint8_t ccm_buffer[64*1024];
```

注意：CCM RAM不能用于DMA。

## 链接器脚本修改

如果需要使用外部SRAM，修改链接器脚本：

```ld
MEMORY
{
  RAM (xrw)      : ORIGIN = 0x20000000, LENGTH = 128K
  CCMRAM (rw)    : ORIGIN = 0x10000000, LENGTH = 64K
  FLASH (rx)     : ORIGIN = 0x8000000, LENGTH = 1024K
  EXTSRAM (rw)   : ORIGIN = 0x68000000, LENGTH = 1024K  /* 外部SRAM */
}

/* 将大缓冲区放到外部SRAM */
.ext_ram :
{
  *(.ext_ram)
  *(.ext_ram*)
} >EXTSRAM
```

使用方法：
```c
__attribute__((section(".ext_ram")))
uint16_t rgb565_buffer[320*240];
```

## 验证优化效果

编译后应该看到：
```
arm-none-eabi-size OV2640.elf
   text    data     bss     dec     hex filename
  85000    1000   55000  141000  226D8 OV2640.elf
```

RAM使用 = data + bss = 1000 + 55000 = 56KB < 192KB ✓

## 总结

当前优化方案：
- ✅ 移除重复缓冲区
- ✅ 禁用本地显示
- ✅ 保留核心USB功能
- ✅ RAM使用约60KB
- ✅ 编译成功

如需恢复LCD显示，建议使用外部SRAM或降低分辨率。
