# 数显轨距尺 `B1-20261103` · 测试报告

## 测试对象和环境

- 基于最新上传的 `STM32L151.zip` 和 `QINGJIAOBAN.zip` 的第一批修复版。
- 平台：Linux 沙盒中的 GCC，采用 `-std=c11 -O1 -g -Wall -Wextra -fno-omit-frame-pointer -fsanitize=address,undefined`。
- 模拟方式：UART 管理器和命令协议测试编译实际修改后的完整 `.c` 源文件（仅为适配主机使用替身 MCU 头文件）；倾角板中断和 ACK 处理、主控格式化代码从修改后的源码直接抽取并执行；OLED 格式化进行静态检查和参数域测试。
- **不等同于** Keil 编译、整机运行测试或实际硬件调试。

## 测试分组与期望结果

| ID | 测试方法 | 期望结果 | 实际 |
|---|---|---|---|
| T01-A | `CmdParam` 全局初始化后，没有任何 UART2 帧就调用命令处理 | 不发送，不访问空指针 | PASS |
| T01-B | UART2 握手 `5A 01 00 00 5B 0D 0A` | 正确应答并恢复空闲 | PASS |
| T01-C | UART2 读取密码请求 | 用原有格式返回 4 字节密码 | PASS |
| T01-D | UART2 长度 0–6、超接收容量、坏帧头、错误长度 | 不越界，异常帧不执行 EEPROM 写入 | PASS |
| T01-E | UART1 传感器长度 0–256、有效帧、错误校验 | 仅有效 13 字节帧更新原始量 | PASS |
| T02-A | 初始化 UART 管理器 | 仅 UART1、UART2 的 HAL 初始化调用发生 | PASS |
| T02-B | 手动调用未配置的 UART3 收发 | 不访问不存在的硬件句柄 | PASS |
| T02-C | 连续向 UART1 注入 410 字节 | RX 指针保持在缓冲区边界内 | PASS |
| T03-A | 倾角板 ACK / NAK / 错误校验 / 错误帧尾 | 接收/拒绝逻辑符合预期 | PASS |
| T03-B | ACK 尚未消费时再次收到数据 | 已完成的 ACK 不被覆盖 | PASS |
| T03-C | 倾角板混合噪声输入 | RX 写指针未越界，哨兵区未修改 | PASS |
| T04-A | 两种主控文本上传页面及负值 | 输出格式可用 | PASS |
| T04-B | 超大浮点数导致输出长度超过 96 字节 | 丢弃该次文本帧，不发送截断数据 | PASS |
| T05-A | `-32768` 至 `65535` 的数字格式化 | 字符串结束符正常，数组哨兵未改写 | PASS |

## 原始测试运行记录

```text
compile test_agreement
compiler warnings: /mnt/data/review_batch1/tests/build/agreement_impl.c: In function ‘UART2_PC_Clear_Eep’:
/mnt/data/review_batch1/tests/build/agreement_impl.c:151:45: warning: unused parameter ‘AgreementParam’ [-Wunused-parameter]
  151 | uint8_t UART2_PC_Clear_Eep(AGREEMEN_PARAM * AgreementParam)
      |                            ~~~~~~~~~~~~~~~~~^~~~~~~~~~~~~~
/mnt/data/review_batch1/tests/build/agreement_impl.c: In function ‘UART2_PC_Get_All_Eep’:
/mnt/data/review_batch1/tests/build/agreement_impl.c:181:25: warning: unused variable ‘addr’ [-Wunused-variable]
  181 |     uint16_t number = 0,addr = 0,size;
      |                         ^~~~
/mnt/data/review_batch1/tests/build/agreement_impl.c: In function ‘Agreement_Ack_For_Fun’:
/mnt/data/review_batch1/tests/build/agreement_impl.c:323:14: warning: unused variable ‘Sum’ [-Wunused-variable]
  323 |     uint32_t Sum = 0;
      |              ^~~
/mnt/data/review_batch1/tests/build/agreement_impl.c: In function ‘Agreement_Ack_For_Uart2’:
/mnt/data/review_batch1/tests/build/agreement_impl.c:274:26: warning: this statement may fall through [-Wimplicit-fallthrough=]
  274 |             CmdParam.cmd = UART2_PC_CLEAR_EEP_DATA_ACK;
/mnt/data/review_batch1/tests/build/agreement_impl.c:275:9: note: here
  275 |         case UART2_PC_GET_EEP_NUM_DATA:
      |         ^~~~

PASS test_agreement: startup, 13-byte sensor, ACK, short frames, malformed header/length, unsupported commands, capacity
compile test_uart
PASS test_uart: UART1/2 only, UART3 no-op, 410-byte stream within bounds, send guards
compile test_tilt
PASS test_tilt: valid/error ACK, bad checksum, short frame, bad tail, pending frame guard, 50000-byte fuzz (50992 RXNE reads)
compile test_main
PASS test_main: both pages, typical and negative values, float extremes rejected
compile test_app
PASS test_app: signed 16-bit and unsigned 16-bit numeric formatting, 98K values
PASS: all five host test binaries
```

## 测试限制

1. 不能据此证明 Keil MDK-ARM 的项目整体能顺利构建；本机没有 ARM Compiler 5/Keil 工具链。
2. 不能据此证明 STM32 的真实 UART 中断、DMA、时钟、优先级或串口硬件异常行为全部正确。
3. 未操作 EEPROM，未重新计算 `CG_Data_Init()`，未检查测量精度。
4. UART2 校验和验证、全部记录读取、接收回调与主循环共享缓冲区仍是下一批任务。
5. 这些自动化测试可以重复运行。测试文件包内 `tests/runner.py` 会自动从同包工程源码提取对应实现，在安装 GCC 和 ASan 的 Linux 环境下生成可执行程序。
