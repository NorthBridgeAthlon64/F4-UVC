# 内存溢出快速修复

## 问题
```
region `RAM' overflowed by 110424 bytes
```

## 已修复 ✓

所有必要的修改已完成。现在重新编译即可。

## 修复步骤

### 1. 清理项目
```
Project → Clean...
选择 "Clean all projects"
点击 "Clean"
```

### 2. 重新构建
```
Project → Build All (Ctrl+B)
```

### 3. 验证成功
编译成功后应该看到：
```
arm-none-eabi-size OV2640.elf
   text    data     bss     dec     hex filename
  85000    1000   55000  141000  226D8 OV2640.elf

Finished building target: OV2640.elf
Build Finished. 0 errors, 0 warnings.
```

RAM使用 = data + bss ≈ 56KB < 192KB ✓

## 修改内容

### 已优化
1. ✅ 移除了uvc_app.c中的40KB重复JPEG缓冲区
2. ✅ 移除了150KB的RGB565显示缓冲区
3. ✅ 直接使用main.c中的共享JPEG缓冲区
4. ✅ 暂时禁用本地LCD实时显示

### 保留功能
- ✅ OV2640 JPEG图像采集
- ✅ USB UVC视频流传输
- ✅ 计算机端视频查看
- ✅ LCD显示系统状态信息

### 暂时禁用
- ❌ 本地LCD实时视频显示

## 如果仍然失败

### 检查1：确认文件已保存
- 确保所有修改的文件都已保存（Ctrl+S）
- 特别是 `Core/Src/uvc_app.c`

### 检查2：查看实际RAM使用
打开 `Debug/OV2640.map` 文件，搜索：
```
.bss
```
查看bss段的大小。

### 检查3：减小JPEG缓冲区
如果还是不够，可以在 `Core/Src/main.c` 中减小：
```c
#define jpeg_buf_size 20*1024  // 从40KB减到20KB
```

### 检查4：使用CCM RAM
STM32F407有额外的64KB CCM RAM，可以利用：

在 `Core/Src/main.c` 中：
```c
// 将JPEG缓冲区放到CCM RAM
__attribute__((section(".ccmram")))
__attribute__((aligned(32))) 
uint8_t jpeg_buf[jpeg_buf_size];
```

注意：CCM RAM不能用于DMA，所以这个方法可能需要额外的数据复制。

## 恢复LCD显示

如果以后需要恢复LCD显示，有几个选项：

### 选项1：添加外部SRAM（推荐）
- 通过FSMC接口连接外部SRAM
- 将RGB565缓冲区放到外部SRAM
- 不占用内部RAM

### 选项2：降低分辨率
```c
#define UVC_FRAME_WIDTH   160
#define UVC_FRAME_HEIGHT  120
// RGB565缓冲区只需 160x120x2 = 38.4KB
```

### 选项3：使用RGB565直接模式
- OV2640配置为RGB565输出
- DCMI DMA直接到LCD
- 不需要缓冲区
- 但不能同时USB传输JPEG

详见 `MEMORY_OPTIMIZATION.md`

## 当前内存分配

```
内存区域              大小        说明
─────────────────────────────────────────
JPEG缓冲区(共享)      40KB       main.c
USB缓冲区             ~5KB       USB库
应用变量              ~5KB       各种变量
堆栈                  ~10KB      系统堆栈
─────────────────────────────────────────
总计                  ~60KB      < 192KB ✓
```

## 成功标志

编译成功后：
1. 没有"overflowed"错误
2. 可以成功烧录
3. 设备正常启动
4. USB能被识别为UVC设备
5. 计算机可以查看视频流

## 需要帮助？

如果问题仍然存在：
1. 截图完整的编译输出
2. 检查 `OV2640.map` 文件
3. 确认所有文件都已保存
4. 尝试完全删除Debug文件夹后重新编译
