# CubeIDE 工程配置说明（CAN_BOX）

以下为本工程在 STM32CubeIDE/CubeMX 配置所涉及的全部主要选项与参数，按模块分节列出，并标注来源文件与关键行号，便于复核与维护。

项目与芯片信息
- 工程名称：CAN_BOX（来源：CAN_BOX.ioc，ProjectManager.ProjectName）
- 目标工具链：STM32CubeIDE（来源：CAN_BOX.ioc，ProjectManager.TargetToolchain）
- 编译器与优化：GCC，优化等级 6（来源：CAN_BOX.ioc，ProjectManager.CompilerLinker/GCC、ProjectManager.CompilerOptimize）
- 固件包版本：STM32Cube FW_F4 V1.28.2（来源：CAN_BOX.ioc，ProjectManager.FirmwarePackage）
- CubeMX 版本：6.15.0（来源：CAN_BOX.ioc，MxCube.Version）
- MCU：STM32F407ZGTx，封装 LQFP144（来源：CAN_BOX.ioc，Mcu.UserName/Mcu.Name/Mcu.Package）
- 生成代码主路径：Core/Src（来源：CAN_BOX.ioc，ProjectManager.MainLocation）
- 保留用户代码：true（来源：CAN_BOX.ioc，ProjectManager.KeepUserCode）
- 栈大小/堆大小（用于生成 main 模板）：Stack 0x400、Heap 0x200（来源：CAN_BOX.ioc，ProjectManager.StackSize/HeapSize）

RCC/时钟树配置
- 外部晶振：HSE 8 MHz（来源：CAN_BOX.ioc，RCC.HSE_VALUE；main.c SystemClock_Config 第 220–290 行）
- PLL 源：HSE（来源：CAN_BOX.ioc，RCC.PLLSourceVirtual；main.c SystemClock_Config）
- PLL 参数：PLLM=8、PLLN=336、PLLP=2、PLLQ=4（来源：CAN_BOX.ioc 与 main.c SystemClock_Config）
- 系统时钟：SYSCLK 168 MHz、AHB 168 MHz、APB1=42 MHz、APB2=84 MHz（来源：CAN_BOX.ioc RCC.*Freq_Value 与 main.c）
- 时基来源：TIM1_UP_TIM10（来源：CAN_BOX.ioc，NVIC.TimeBase=TIM1_UP_TIM10_IRQn、NVIC.TimeBaseIP=TIM1）

NVIC/中断配置（优先级组：NVIC_PRIORITYGROUP_4）
- 系统类中断：PendSV=15、SysTick=15（来源：CAN_BOX.ioc NVIC.PendSV_IRQn/NVIC.SysTick_IRQn；stm32f4xx_hal_msp.c HAL_MspInit）
- CAN1：TX=6、RX0=5、SCE=5（来源：CAN_BOX.ioc；stm32f4xx_hal_msp.c HAL_CAN_MspInit）
- CAN2：TX=5、RX0=5、RX1=5、SCE=5（来源：CAN_BOX.ioc；stm32f4xx_hal_msp.c HAL_CAN_MspInit）
- SPI1：IRQ=7（来源：CAN_BOX.ioc；stm32f4xx_hal_msp.c HAL_SPI_MspInit 第 260–310 行）
- USART2：IOC 配置为 8；实际 MSP 中设置为 5 以满足 FreeRTOS 中断安全要求（来源：CAN_BOX.ioc NVIC.USART2_IRQn；stm32f4xx_hal_msp.c 第 320–370 行）

GPIO 与引脚复用映射
- HSE：PH0-OSC_IN、PH1-OSC_OUT（来源：CAN_BOX.ioc）
- SWD：PA13=JTMS-SWDIO、PA14=JTCK-SWCLK（来源：CAN_BOX.ioc）
- CAN1：PA11=CAN1_RX（上拉）、PA12=CAN1_TX（无上拉）（来源：CAN_BOX.ioc 与 stm32f4xx_hal_msp.c HAL_CAN_MspInit）
- CAN2：PB12=CAN2_RX（上拉）、PB13=CAN2_TX（无上拉）（来源：CAN_BOX.ioc 与 stm32f4xx_hal_msp.c HAL_CAN_MspInit）
- USART2：PA2=TX、PA3=RX（来源：CAN_BOX.ioc 与 stm32f4xx_hal_msp.c HAL_UART_MspInit）
- SPI1：PB3=SCK、PB4=MISO、PB5=MOSI（来源：CAN_BOX.ioc 与 stm32f4xx_hal_msp.c HAL_SPI_MspInit）

CAN1 配置（代码当前生效值）
- 句柄：hcan1（来源：Core/Src/can.c 第 19–46 行）
- 模式：Normal（CAN_MODE_NORMAL）（来源：can.c）
- 预分频：Prescaler=6（目标 500 kbps）（来源：can.c）
- 位时序：SJW=1TQ、BS1=10TQ、BS2=3TQ（来源：can.c）
- 其他：TimeTriggeredMode/AutoBusOff/AutoWakeUp/AutoRetransmission/ReceiveFifoLocked/TransmitFifoPriority 均 DISABLE（来源：can.c）
- 过滤器分配：从 0–2 号过滤器配置 PEPS 相关 ID，SlaveStartFilterBank=14（来源：Core/Src/can_testbox_peps_filter.c 第 1–104 行）
- 注：IOC 中 CAN1.BS1=13TQ、BS2=2TQ（与当前 can.c 不一致，待下一次由 CubeMX 重新生成时统一）（来源：CAN_BOX.ioc）

CAN2 配置（工程中存在但应用层已不使用）
- 句柄：hcan2（来源：Core/Src/can.c 第 19–94 行）
- 模式：Silent（CAN_MODE_SILENT，仅监听）（来源：can.c）
- 预分频：Prescaler=6（目标 500 kbps）（来源：can.c）
- 位时序：SJW=1TQ、BS1=10TQ、BS2=3TQ（来源：can.c）
- NVIC/引脚均已按 IOC/MSP 配置启用，但 main.c 未调用 MX_CAN2_Init（来源：stm32f4xx_hal_msp.c、Core/Src/main.c 第 1–120 行）
- 注：IOC 中 CAN2.BS1=11TQ、BS2=2TQ（与当前 can.c 不一致，待下一次由 CubeMX 重新生成时统一）（来源：CAN_BOX.ioc）

USART2 配置
- 句柄：huart2（来源：Core/Src/usart.c 第 1–58 行）
- 波特率：115200、8N1、无校验、双向收发、无硬件流控、Oversampling=16（来源：usart.c）
- NVIC：MSP 设置优先级 5（来源：stm32f4xx_hal_msp.c 第 320–370 行）

SPI1 配置
- 主机模式，双线全双工，DataSize=8bit，CPOL=Low，CPHA=1Edge，NSS=Soft，Prescaler=32，FirstBit=MSB（来源：Core/Src/main.c 第 260–310 行）
- 计算速率：约 2.625 Mbit/s（来源：CAN_BOX.ioc SPI1.CalculateBaudRate）
- NVIC：SPI1_IRQn 优先级 7（来源：stm32f4xx_hal_msp.c 第 260–310 行）

FreeRTOS 配置
- Kernel：CMSIS-RTOS V2（来源：CAN_BOX.ioc VP_FREERTOS_VS_CMSIS_V2）
- Tick 频率：1000Hz（来源：Core/Inc/FreeRTOSConfig.h）
- 最大优先级数：56（来源：FreeRTOSConfig.h）
- 堆大小：15360（Heap_4）（来源：FreeRTOSConfig.h）
- 最小栈：128（来源：FreeRTOSConfig.h）
- 启用：Trace Facility、Mutex、递归互斥、计数信号量、软件定时器（优先级 2，栈 256）、Newlib Reentrant=1（来源：FreeRTOSConfig.h）
- 中断相关：configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY=5，PRIO_BITS=4，KERNEL_INTERRUPT_PRIORITY=15（来源：FreeRTOSConfig.h）
- IOC 任务定义（CubeMX 侧）：
  - defaultTask：优先级 24、栈 128、入口 StartDefaultTask（来源：CAN_BOX.ioc FREERTOS.Tasks01）
  - CANSendTask：优先级 24、栈 512、入口 StartCANSendTask
  - CANReceiveTask：优先级 24、栈 512、入口 StartCANReceiveTask
- 实际代码任务（应用侧）：
  - defaultTask：栈 128*4、优先级 Normal（来源：Core/Src/main.c 第 1–120、360–390 行）
  - CANTestBoxTask：栈 1024*4、优先级 Normal（来源：Core/Src/main.c 第 1–120、395–436 行）

中断服务与回调位置
- IRQHandler 原型：stm32f4xx_it.h（包含 CAN1/CAN2/SPI1/USART2 等）（来源：Core/Inc/stm32f4xx_it.h）
- IRQ 调用：stm32f4xx_it.c 调用 HAL_CAN_IRQHandler(&hcan1/&hcan2) 等（来源：Core/Src/stm32f4xx_it.c）
- CAN1 接收/错误回调：统一在 can_dual_node.c 中实现，并调用 CAN TestBox API 的处理函数（来源：Core/Src/can_dual_node.c 第 999–1112 行）

函数生成顺序（CubeMX）
- SystemClock_Config、MX_GPIO_Init、MX_SPI1_Init、MX_USART2_UART_Init、MX_CAN1_Init（来源：CAN_BOX.ioc ProjectManager.functionlistsort）

差异说明与维护建议
- IOC 与当前临时代码在 CAN 位时序（BS1/BS2）与 USART2 中断优先级上存在差异，后续以 STM32CubeMX 重新生成代码为准，并确保 FreeRTOS 最大可用中断优先级不高于 5（数值越小优先级越高）。
- 工程内保留了 CAN2 的 MSP/IRQ/句柄定义与 IOC 配置，但应用入口未启用 MX_CAN2_Init；如需完全禁用 CAN2，可在 IOC 中取消 CAN2 外设并重新生成。

来源文件索引（便于定位）
- CAN_BOX.ioc（项目/外设/NVIC/时钟/引脚/RTOS 选项）
- Core/Src/main.c（SystemClock、SPI1 初始化、任务定义与入口）
- Core/Src/can.c（CAN1/CAN2 初始化参数）
- Core/Src/usart.c（USART2 初始化参数）
- Core/Src/stm32f4xx_hal_msp.c（GPIO 复用、时钟使能、NVIC 优先级）
- Core/Inc/FreeRTOSConfig.h（RTOS 核心配置）
- Core/Src/can_dual_node.c（CAN 回调与业务处理）
- Core/Inc/stm32f4xx_it.h、Core/Src/stm32f4xx_it.c（中断向量与 HAL IRQ 调用）

（本说明文件由工程内现有 IOC 与源码自动提取整理，若后续通过 CubeMX 修改并重新生成代码，请以生成后的参数为准）