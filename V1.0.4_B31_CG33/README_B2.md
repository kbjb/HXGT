# V1.0.2_B2 数显轨距尺

本包包含两套基于用户上传 `V1.0.1.zip` (B1版本) 的完整工程：

- `STM32L151/` 主控板 Keil 工程
- `QINGJIAOBAN/` 倾角板 Keil 工程

B2 修复内容：UART1/UART2 帧协议校验、ISR 与前台分离、单帧邮箱防覆盖、UART 异常恢复、记录响应边界、倾角板 ACK 定长解析及固定500ms发送。

详情见 `B2_20261103_详细修改日志.md` 和 `B2_20261103_回归测试报告.md`；电脑端可复测代码在 `B2_HOST_TESTS/`。

**请务必注意：项目原有 Objects/、AXF/HEX/MAP 等是 V1.0.1 的历史构建产物。B2 源代码修改后尚未使用 Keil ARM Compiler 5 重新编译，这些旧二进制不能当作 B2 固件烧录。**

Keil 工程入口：

```text
STM32L151/MDK-ARM/STM32L151.uvprojx
QINGJIAOBAN/PROJECT/GuiJuChi.uvprojx
```

本批次未改 EEPROM 地址布局、标定参数、轨距/超高公式。不要在确认 EEPROM 实物型号和备份标定数据前擦写/重新初始化。
