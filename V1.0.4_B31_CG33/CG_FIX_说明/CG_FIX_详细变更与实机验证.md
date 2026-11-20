# V1.0.4_B31 CG_FIX：超高显示与标定反馈修复

**范围**：只修改 STM32L151 主控超高显示、标定提示和超高系数初始化；倾角板（QINGJIAOBAN）与 B3.1 EEPROM 驱动、CRC、A/B 双副本及地址规划均未修改。

## 静态核对的根因

1. `CG_Data_Init()` 只有实现，没有在原工程中被调用；`K_CG_Left/B_CG_Left/K_CG_Right/B_CG_Right` 初始为零，`Get_CG_Value()` 无法使用 EEPROM 的实际超高标定参数。
2. `Main_Page_Init()` 将 `CG_Value_History` 清成零，而 `Main_Page_Reflash()` 只在数值变化时画数字；计算值刚好为 `0.00` 时 OLED 已清屏却不会绘制零值。
3. `CG_Calibration_Status == 8` 原来立即显示“成功”，但存储键回调直到状态 9 才尝试 EEPROM 写入。保存失败会退回状态 8 再次显示“成功”，误导用户。

## 修改点

- **CGFIX-01**：声明并在 `Eep_Data_Get()` 后调用 `CG_Data_Init()`；每次 EEPROM 校验写入成功后再次调用，让新标定值立即参与计算。
- **CGFIX-02**：主测量页面首次刷新强制绘制轨距和超高值，包括 `0.00`。
- **CGFIX-03**：超高采样完成时显示“存储键保存”；只有 `Eep_Save_CG_Calibration()` 返回成功才显示“成功”；失败时显示 ASCII `EEP ERROR` 并蜂鸣三声，下一次按存储键可重试。失败返回页面时强制清屏，避免错误提示与旧文案重叠。成功后再按一次存储键返回标定菜单。原标定采样顺序与 AD 值未改。

## 不变内容

`EEP_B3_SERVICE_ENABLE_LEGACY_CAL_UPGRADE` 与 `EEP_B3_SERVICE_IMPORT_LEGACY_COUNT` 仍为 **0**；EEPROM 地址、出厂空白判定、双副本提交顺序、已有参数保护、记录管理逻辑完全不变。

## 真机验证（Keil MDK）

1. 打开 `STM32L151/MDK-ARM/STM32L151.uvprojx`，执行 **Rebuild all target files**，编译成功后烧录主控板；倾角板程序不需要重新烧录。
2. 进入 Watch，观察 `CG_Ad`、`Temp`、`CG_Value`、`CG_Value_t`、`CG_Param`、`CG_Param_Temp`、`CG_Calibration_Status`、`CG_Cal_Save_Failed`、`B3_Eeprom_Boot_OK`、`Debug_Eep_Storage`。
3. 仅进入主界面，水平处也必须显示 `+00.00`（或对应温度补偿后的数值），而不是空白。倾斜时 `CG_Ad` 与 `CG_Value` 应随之变化。
4. 按原步骤采集水平、水平掉头、180 mm、180 mm 掉头四个点；采完后显示“存储键保存”，**还要再按一次存储键**才写入 EEPROM。成功出现“超高标定成功”，失败出现 `EEP ERROR` 并响三声。
5. 保存成功后在 Watch 比对 `CG_Param` / `CG_Param_Temp`；关机后重新上电，`B3_Eeprom_Boot_OK == 1` 且 `Debug_Eep_Storage == 1`，新的 `CG_Param` 仍保持。用具备精度的标准器做超高检定；软件显示正常不代表精度合格。

## 验证边界

已执行源码结构检查、超高计算主机编译/单元测试及 B3.1 EEPROM 原有模拟回归测试（记录见测试报告）。**未在本环境运行 Keil ARMCC 编译、实机烧录、传感器精度标定或硬件断电测试**，因此这是实机待验收的修复候选版。

> 提醒：轨距、查护两套标定页面在原始工程中也存在“保存前先显示成功”的类似用户界面问题；本修复版仅针对本次反馈的超高功能，暂未修改那两套流程。
