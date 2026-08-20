# 海上风机浮筒传感器采集固件

## 本次实现范围

- `USART1` 配置为数据输出接口（默认 9600, 8N1，PA9/PA10），输出解析后的 IWT603 数据。
- `USART3` 接收 IWT603 的 TTL UART 数据（默认 9600, 8N1），解析 WIT 私有协议的11字节数据帧：加速度 `0x51`、角速度 `0x52`、姿态角 `0x53`。
- FA66/Modbus 相关代码暂时保留但未接入应用流程，USART2 和 PB12 方向控制不再初始化。
- 采样结果统一保存在 `SensorApp_Data_t`，并通过 USART1 周期输出。

## 固定引脚分配

| 功能 | STM32F103ZET6 引脚 | 连接 |
|---|---|---|
| IWT603 数据输出 UART TX | PA9 (USART1_TX) | 接串口工具或主控 RX |
| IWT603 数据输出 UART RX | PA10 (USART1_RX) | 预留 |
| IWT603 UART RX | PB11 (USART3_RX) | IWT603 TX |
| IWT603 UART TX | PB10 (USART3_TX) | IWT603 RX，可选，仅配置/单次查询使用 |

USART1 和 USART3 均为 9600 8N1，所有串口设备必须共地。IWT603 使用 3.3V +

## 构建前置

工程路径：`MDK-ARM/sensor.uvprojx`。原构建日志提示缺少 ARM Compiler 5；需在 Keil MDK 中将目标编译器改为已安装的 Arm Compiler 6（同时确认项目包含 `stm32f1xx_hal_uart.c`），或安装 ARM Compiler 5。本工程已经加入 UART HAL 和应用源文件；构建后应生成 `sensor.hex`。

## 现场联调顺序

1. 将 IWT603 TX/RX 接入 PB11/PB10 对应的 USART3，确认波特率为 9600。
2. 将 USART1 的 PA9 接串口工具 RX，PA10 可接工具 TX，确认参数为 9600 8N1。
3. 观察 USART1 输出的 IWT603 文本数据，格式为 `IWT603,A=...;G=...;R=...;T=...;ms=...`。
