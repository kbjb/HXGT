> **历史 B3 说明，仅供追溯。当前正式功能请优先阅读 `README_B31.md`。**

# V1.0.3_B3｜AT24C512 EEPROM第三批修复

基于用户上传的V1.0.2_B2完整ZIP开发，主控EEPROM驱动/校准/记录/上位机存储访问已修改；倾角板源码未改。

优先阅读：`B3_20261103_详细修改日志.md`、`B3_20261103_EEPROM备份与迁移操作指南.md`、`B3_20261103_回归测试报告.md`。

**注意：正常开机不自动写EEPROM；历史记录与标定不自动迁移；必须先完成64KiB外部备份。**

工程：`STM32L151/MDK-ARM/STM32L151.uvprojx`；`QINGJIAOBAN/PROJECT/GuiJuChi.uvprojx`。
B3新版尚未经Keil编译/真机验证，ZIP内历史AXF/HEX不是B3新固件；烧录前需Rebuild。
