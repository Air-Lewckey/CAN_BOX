# STM32F407 CAN通信系统

## 项目简介

本项目是基于STM32F407ZGT6微控制器的CAN通信系统，集成了多种CAN通信功能模块。系统主要使用STM32F407内置的CAN1控制器，实现了完整的CAN总线通信功能，包括双节点通信、触发式发送、消息接收处理和状态监控等功能。

### 主要特性

- **多功能CAN通信架构**：基于STM32F407内置CAN1控制器
- **双节点通信模块**：支持与WCMCU-230模块的双向通信
- **触发式发送功能**：通过串口命令触发CAN消息发送
- **CAN2静默监听**：CAN2工作在静默模式，纯监听总线消息
- **自动ACK应答机制**：接收到CAN消息后自动发送应用层ACK确认
- **多任务设计**：基于FreeRTOS的多任务并发处理
- **完整的CAN协议栈**：从底层驱动到应用层的完整实现
- **智能诊断功能**：自动检测和修复常见CAN通信问题
- **丰富的消息类型**：支持心跳、数据、状态、控制、ACK等多种消息
- **实时调试输出**：通过USART2串口提供详细的调试信息

## 系统架构

### 硬件架构

#### 核心硬件组件

1. **STM32F407ZGT6微控制器**
   - ARM Cortex-M4内核，168MHz主频
   - 内置双CAN控制器（CAN1/CAN2）
   - 丰富的外设接口

2. **CAN收发器模块**
   - SN65HVD230或WCMCU-230模块
   - 提供CAN总线物理层接口
   - 支持标准CAN 2.0B协议

3. **调试接口**
   - USART2用于串口调试和命令输入
   - ST-Link调试器接口

#### 引脚连接

##### STM32F407 CAN1引脚（主要通信）
- **CAN1_TX**: PA12
- **CAN1_RX**: PA11

##### STM32F407 CAN2引脚（静默监听）
- **CAN2_TX**: PB13（未使用）
- **CAN2_RX**: PB12

##### 串口调试接口
- **USART2_TX**: PA2
- **USART2_RX**: PA3

##### 预留SPI接口（用于MCP2515扩展）
- **SPI1_SCK**: PA5
- **SPI1_MISO**: PA6
- **SPI1_MOSI**: PA7
- **CS**: PA4
- **INT**: PA3

```
STM32F407开发板
├── 内置CAN控制器 (CAN1)
│   └── 连接到WCMCU-230模块
├── 内置CAN控制器 (CAN2)
│   └── 静默监听模式
├── USART2
│   └── 调试串口输出
└── GPIO
    ├── LED指示灯
    └── 预留SPI接口
```

### 软件架构

#### 模块组织

```
CAN通信系统
├── CAN双节点通信模块 (can_dual_node.c)
│   ├── 与WCMCU-230模块通信
│   ├── 心跳、数据、状态消息处理
│   └── 节点状态监控
├── CAN触发发送模块 (can_trigger_send.c)
│   ├── 串口命令触发
│   ├── 三种消息类型发送
│   └── UART中断处理
├── CAN2静默监听模块 (can2_demo.c)
│   ├── 静默模式监听
│   ├── 消息统计
│   └── 总线诊断
├── 扩展功能模块
│   ├── CAN总线诊断 (can_bus_diagnosis.c)
│   ├── 环回测试 (can_loop_test.c)
│   └── 桥接测试 (can1_can2_bridge_test.c)
└── 系统服务模块
    ├── FreeRTOS任务管理
    ├── 消息队列
    └── 串口调试输出
```

项目采用分层设计，主要包含以下模块：

#### 1. 驱动层 (Driver Layer)
- **can.c/h**: STM32内置CAN控制器驱动
- **usart.c/h**: 串口通信驱动
- **gpio.c/h**: GPIO控制驱动

#### 2. 应用层 (Application Layer)
- **can_dual_node.c/h**: 双CAN节点通信管理
- **can_trigger_send.c/h**: 触发式CAN消息发送
- **can2_demo.c/h**: CAN2静默监听功能
- **main.c**: 主程序和任务调度

#### 3. 系统层 (System Layer)
- **FreeRTOS**: 实时操作系统
- **HAL库**: STM32硬件抽象层
- **中断处理**: 系统中断和回调函数

## 核心功能模块

### 1. CAN双节点通信模块 (can_dual_node.c)

#### 主要功能
- 与WCMCU-230模块的双向CAN通信
- 支持心跳、数据请求/响应、状态和控制消息
- 节点状态监控和超时检测
- 通信统计和错误处理
- 消息校验和完整性检查

#### 核心函数详解

##### 初始化函数
```c
HAL_StatusTypeDef CAN_DualNode_Init(void)
```
**功能**: 初始化双CAN节点通信
**返回值**: HAL_OK表示成功
**实现逻辑**:
1. 配置CAN过滤器
2. 启动CAN控制器
3. 激活接收中断
4. 激活发送完成中断
5. 激活错误中断
6. 初始化统计信息

##### 消息发送函数
```c
HAL_StatusTypeDef CAN_SendToWCMCU(uint32_t id, uint8_t* data, uint8_t len)
HAL_StatusTypeDef CAN_SendHeartbeat(void)
HAL_StatusTypeDef CAN_SendDataRequest(uint8_t req_type, uint8_t req_param)
HAL_StatusTypeDef CAN_SendStatusMessage(void)
```
**功能**: 发送不同类型的CAN消息
**消息格式**:
- **心跳消息**: 魔数(2字节) + 计数器(2字节)
- **数据请求**: 请求类型(1字节) + 请求参数(1字节)
- **状态消息**: 魔数(2字节) + 状态(1字节) + 计数器(2字节) + 时间戳(1字节)

##### 消息处理函数
```c
void CAN_ProcessReceivedMessage(CAN_RxHeaderTypeDef* header, uint8_t* data)
CAN_MessageType_t CAN_GetMessageType(uint32_t id)
void CAN_ProcessHeartbeat(uint8_t* data, uint8_t len)
void CAN_ProcessDataRequest(uint8_t* data, uint8_t len)
```
**功能**: 处理接收到的不同类型消息
**实现逻辑**:
1. 根据消息ID确定消息类型
2. 验证消息格式和魔数
3. 解析消息内容
4. 执行相应的处理逻辑
5. 更新节点状态和统计信息

### 2. CAN触发发送模块 (can_trigger_send.c)

#### 主要功能
- 通过串口命令触发CAN消息发送
- 支持三种不同ID的消息类型
- UART中断接收处理
- 替代周期性发送方式

#### 核心函数详解

##### 初始化函数
```c
HAL_StatusTypeDef CAN_TriggerSend_Init(void)
```
**功能**: 初始化触发发送功能
**实现逻辑**:
1. 配置UART接收中断
2. 初始化CAN控制器
3. 设置消息模板
4. 启动接收监听

##### 消息发送函数
```c
HAL_StatusTypeDef CAN_TriggerSend_SendMessage1(void)  // ID: 0x100
HAL_StatusTypeDef CAN_TriggerSend_SendMessage2(void)  // ID: 0x200
HAL_StatusTypeDef CAN_TriggerSend_SendMessage3(void)  // ID: 0x300
```
**功能**: 发送预定义的三种消息类型
**触发方式**: 通过串口发送字符'1'、'2'、'3'触发对应消息

##### 中断回调函数
```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
```
**功能**: UART接收完成中断回调
**实现逻辑**:
1. 检查接收到的字符
2. 根据字符选择消息类型
3. 调用对应的发送函数
4. 重新启动接收

### 3. CAN2静默监听模块 (can2_demo.c)

#### 主要功能
- CAN2工作在静默模式，纯监听总线消息
- 消息统计和分析
- 总线流量监控
- 错误检测和报告

#### 核心函数详解

##### 初始化函数
```c
HAL_StatusTypeDef CAN2_Demo_Init(void)
```
**功能**: 初始化CAN2静默监听
**实现逻辑**:
1. 配置CAN2为静默模式
2. 设置接收过滤器
3. 启动接收中断
4. 初始化统计计数器

##### 消息监听函数
```c
void CAN2_ProcessReceivedMessage(CAN_RxHeaderTypeDef* header, uint8_t* data)
void CAN2_UpdateStatistics(uint32_t id, uint8_t dlc)
```
**功能**: 处理监听到的CAN消息
**统计信息**:
- 总消息数量
- 不同ID的消息计数
- 数据长度分布
- 错误帧统计

### 4. 双节点通信模块 (can_dual_node.c)

#### 主要功能
- STM32内置CAN控制器与WCMCU-230模块通信
- 双节点状态监控
- 消息协议定义
- 通信统计和诊断

#### 核心函数详解

##### 双节点初始化函数
```c
HAL_StatusTypeDef CAN_DualNode_Init(void)
```
**功能**: 初始化双CAN节点通信
**实现逻辑**:
1. 配置CAN过滤器
2. 启动CAN控制器
3. 激活接收中断
4. 激活发送完成中断
5. 激活错误中断
6. 初始化统计信息

##### 消息发送函数
```c
HAL_StatusTypeDef CAN_SendToWCMCU(uint32_t id, uint8_t* data, uint8_t len)
HAL_StatusTypeDef CAN_SendHeartbeat(void)
HAL_StatusTypeDef CAN_SendDataRequest(uint8_t req_type, uint8_t req_param)
HAL_StatusTypeDef CAN_SendDataResponse(uint8_t* data, uint8_t len)
HAL_StatusTypeDef CAN_SendStatusMessage(void)
HAL_StatusTypeDef CAN_SendControlCommand(uint16_t cmd, uint16_t param)
```
**功能**: 发送不同类型的CAN消息
**消息格式**:
- **心跳消息**: 魔数(2字节) + 计数器(2字节)
- **数据请求**: 请求类型(1字节) + 请求参数(1字节)
- **状态消息**: 魔数(2字节) + 状态(1字节) + 计数器(2字节) + 时间戳(1字节)
- **控制指令**: 魔数(2字节) + 命令(2字节)

##### 消息处理函数
```c
void CAN_ProcessReceivedMessage(CAN_RxHeaderTypeDef* header, uint8_t* data)
CAN_MessageType_t CAN_GetMessageType(uint32_t id)
void CAN_ProcessHeartbeat(uint8_t* data, uint8_t len)
void CAN_ProcessDataRequest(uint8_t* data, uint8_t len)
void CAN_ProcessDataResponse(uint8_t* data, uint8_t len)
void CAN_ProcessStatusMessage(uint8_t* data, uint8_t len)
void CAN_ProcessControlCommand(uint8_t* data, uint8_t len)
```
**功能**: 处理接收到的不同类型消息
**实现逻辑**:
1. 根据消息ID确定消息类型
2. 验证消息格式和魔数
3. 解析消息内容
4. 执行相应的处理逻辑
5. 更新节点状态和统计信息

## 数据结构定义

### 1. CAN消息结构体
```c
typedef struct {
    uint32_t id;        // CAN ID
    uint8_t ide;        // 标识符扩展位 (0=标准帧, 1=扩展帧)
    uint8_t rtr;        // 远程传输请求 (0=数据帧, 1=远程帧)
    uint8_t dlc;        // 数据长度代码 (0-8)
    uint8_t data[8];    // 数据字节
} MCP2515_CANMessage_t;
```

### 2. 应用层消息结构体
```c
typedef struct {
    MCP2515_CANMessage_t message;  // CAN消息
    uint32_t timestamp;            // 时间戳
    uint8_t priority;              // 优先级
} CAN_QueueMessage_t;
```

### 3. 统计信息结构体
```c
typedef struct {
    uint8_t initialized;     // 初始化状态
    uint32_t tx_count;       // 发送计数
    uint32_t rx_count;       // 接收计数
    uint32_t error_count;    // 错误计数
    uint32_t last_tx_time;   // 最后发送时间
    uint32_t last_rx_time;   // 最后接收时间
} CAN_App_Stats_t;
```

## 消息协议定义

### 双节点通信消息ID分配

| 消息类型 | 消息ID | 方向 | 描述 | 数据长度 |
|---------|--------|------|------|----------|
| 心跳消息 | 0x101 | STM32→WCMCU | 节点存活检测 | 4字节 |
| 心跳响应 | 0x201 | WCMCU→STM32 | 心跳确认 | 4字节 |
| 数据请求 | 0x102 | STM32→WCMCU | 请求数据 | 2字节 |
| 数据响应 | 0x202 | WCMCU→STM32 | 数据传输 | 1-8字节 |
| 状态消息 | 0x103 | STM32→WCMCU | 状态信息 | 6字节 |
| 状态响应 | 0x203 | WCMCU→STM32 | 状态确认 | 可变 |
| 控制指令 | 0x104 | STM32→WCMCU | 控制命令 | 4字节 |
| 错误消息 | 0x7FF | 双向 | 错误报告 | 2字节 |

### 触发发送消息ID分配

| 触发字符 | 消息ID | 描述 | 数据内容 |
|----------|--------|------|----------|
| '1' | 0x100 | 测试消息1 | 计数器+时间戳 |
| '2' | 0x200 | 测试消息2 | 传感器数据模拟 |
| '3' | 0x300 | 测试消息3 | 状态信息 |

```c
#define CAN_HEARTBEAT_ID        0x100  // 心跳消息
#define CAN_DATA_ID             0x200  // 数据消息
#define CAN_APP_STATUS_ID       0x300  // 应用状态消息
#define CAN_SENSOR_ID           0x400  // 传感器数据
#define CAN_CONTROL_ID          0x500  // 控制指令
#define CAN_ERROR_ID            0x7FF  // 错误消息

// 双节点通信ID
#define CAN_DATA_REQUEST_ID     0x601  // 数据请求
#define CAN_DATA_RESPONSE_ID    0x602  // 数据响应
#define CAN_STATUS_ID           0x603  // 状态消息
#define CAN_ACK_ID              0x700  // ACK应答消息
```

### 消息格式详细定义

#### 双节点通信消息格式

##### 心跳消息 (0x101)
| 字节 | 描述 | 值 |
|------|------|----|----|
| 0-1  | 魔数 | 0xAA55 |
| 2-3  | 发送计数器 | 16位计数值 |

##### 数据请求消息 (0x102)
| 字节 | 描述 | 值 |
|------|------|----|----|
| 0    | 请求类型 | 1=传感器, 2=状态, 3=配置 |
| 1    | 请求参数 | 具体参数ID |

##### 状态消息 (0x103)
| 字节 | 描述 | 值 |
|------|------|----|----|
| 0-1  | 魔数 | 0xBBCC |
| 2    | 状态标志 | bit0=运行, bit1=错误, bit2=警告 |
| 3-4  | 状态计数器 | 16位计数值 |
| 5    | 时间戳 | 秒的低8位 |

##### 控制指令消息 (0x104)
| 字节 | 描述 | 值 |
|------|------|----|----|----|
| 0-1  | 控制命令 | 1=启动, 2=停止, 3=复位, 4=配置 |
| 2-3  | 命令参数 | 具体参数值 |

##### ACK应答消息 (0x700)
| 字节 | 描述 | 值 |
|------|------|----|----|----|
| 0-1  | 魔数 | 0xACDC |
| 2    | ACK代码 | 1=心跳, 2=数据请求, 3=数据响应, 4=状态, 5=控制, 6=错误 |
| 3    | 原始消息ID低字节 | 被确认消息的ID低8位 |

#### 触发发送消息格式

##### 测试消息1 (0x100) - 触发字符'1'
| 字节 | 描述 | 值 |
|------|------|----|----|
| 0-3  | 发送计数器 | 32位计数值 |
| 4-7  | 时间戳 | 32位毫秒时间戳 |

##### 测试消息2 (0x200) - 触发字符'2'
| 字节 | 描述 | 值 |
|------|------|----|----|
| 0-1  | 传感器ID | 16位传感器标识 |
| 2-3  | 传感器数值 | 16位数据值 |
| 4    | 传感器状态 | 状态标志 |
| 5-7  | 保留字节 | 0x00 |

##### 测试消息3 (0x300) - 触发字符'3'
| 字节 | 描述 | 值 |
|------|------|----|----|
| 0    | 系统状态 | 系统运行状态 |
| 1    | 错误代码 | 错误类型代码 |
| 2-3  | 运行时间 | 分钟为单位 |
| 4-7  | 保留字节 | 0x00 |

#### 数据消息 (0x200)
| 字节 | 描述 | 值 |
|------|------|----|
| 0-1  | 魔数 | 0x1234 |
| 2-3  | 数据计数器 | 16位计数值 |
| 4-7  | 测试数据 | 随机数据 |

#### 状态消息 (0x300)
| 字节 | 描述 | 值 |
|------|------|----|
| 0-1  | 魔数 | 0x5354 |
| 2    | 系统状态 | 状态码 |
| 3    | 错误标志 | 0=正常, 1=错误 |
| 4-5  | 发送计数 | 16位计数值 |
| 6-7  | 接收计数 | 16位计数值 |

## 任务调度

### FreeRTOS任务配置

| 任务名称 | 优先级 | 堆栈大小 | 功能描述 |
|----------|--------|----------|----------|
| defaultTask | osPriorityNormal | 128 words | 系统默认任务，LED闪烁 |
| CANSendTask | osPriorityNormal | 512 words | CAN消息发送任务 |
| CANReceiveTask | osPriorityNormal | 512 words | CAN消息接收任务 |

### 消息队列配置

| 队列名称 | 大小 | 元素类型 | 功能描述 |
|----------|------|----------|----------|
| myQueue01 | 16 | uint16_t | CAN消息队列 |

### 当前启用的功能模块

- ✅ **CAN1双节点通信**: 与WCMCU-230模块通信
- ✅ **CAN触发发送**: 串口命令触发消息发送
- ✅ **CAN2静默监听**: 监听总线消息
- ❌ **CAN2发送功能**: 已禁用
- ❌ **MCP2515模块**: 预留接口，未启用
- ❌ **CAN1-CAN2桥接**: 已禁用

## 编译和使用

### 开发环境要求
- **STM32CubeIDE**: 1.8.0或更高版本
- **STM32CubeMX**: 6.0或更高版本（用于配置修改）
- **STM32F4xx HAL库**: 集成在CubeIDE中
- **FreeRTOS**: V10.3.1或更高版本
- **ARM GCC工具链**: 集成在STM32CubeIDE中
- **调试器**: ST-Link V2/V3
- **操作系统**: Windows 10/11, Linux, macOS
- **硬件平台**: 正点原子STM32F407开发板
- **CAN模块**: WCMCU-230或兼容的CAN收发器模块

### 硬件连接

#### STM32F407与MCP2515连接
| STM32F407 | MCP2515 | 功能 |
|-----------|---------|------|
| PA5 (SPI1_SCK) | SCK | SPI时钟 |
| PA6 (SPI1_MISO) | SO | SPI数据输出 |
| PA7 (SPI1_MOSI) | SI | SPI数据输入 |
| PA4 | CS | 片选信号 |
| PA3 | INT | 中断信号 |
| 3.3V | VCC | 电源 |
| GND | GND | 地线 |

#### 串口连接
| STM32F407 | USB转串口 | 功能 |
|-----------|-----------|------|
| PA2 (USART2_TX) | RX | 串口发送 |
| PA3 (USART2_RX) | TX | 串口接收 |
| GND | GND | 地线 |

## 🔄 CAN循环测试功能

### 测试原理
本项目实现了双CAN节点循环通信测试，测试流程如下：

```
[STM32 CAN1] --发送--> [MCP2515] --转发--> [STM32 CAN1] --接收完成--
     ↑                                                      |
     |                    1秒周期                            |
     +--------------------下一轮发送<---------------------+
```

### 硬件连接（循环测试）
**重要**: 使用杜邦线将两路CAN直接连接

```
STM32F407 CAN1 ←→ MCP2515 CAN
├─ CAN1_H (PD1) ←→ MCP2515 CAN_H
└─ CAN1_L (PD0) ←→ MCP2515 CAN_L

MCP2515 SPI连接：
├─ CS   ←→ PA4
├─ SCK  ←→ PA5  
├─ MISO ←→ PA6
├─ MOSI ←→ PA7
└─ INT  ←→ PA3
```

### 测试消息格式

#### 循环测试消息 (ID: 0x123)
| 字节 | 描述 | 值 |
|------|------|----||
| 0-1  | 起始标识 | 0xAA55 |
| 2-3  | 循环计数器 | 16位计数值 |
| 4-7  | 时间戳 | 32位时间戳 |

### 测试日志输出

#### 成功循环示例
```
[LOOP #1] STM32 CAN1 -> Message sent to MCP2515 (Time: 5000 ms)
[RELAY] MCP2515 received message from STM32 CAN1 (Time: 5001 ms)
[DATA] MCP2515 received: AA 55 00 01 00 00 13 88
[RELAY] MCP2515 -> Message relayed to STM32 CAN1
[LOOP #1] STM32 CAN1 <- Message received from MCP2515 (Loop time: 15 ms)
[SUCCESS] Loop #1 completed successfully
[DATA] Received: AA 55 00 01 00 00 13 88
```

#### 统计信息示例
```
=== CAN Loop Test Statistics ===
Total Loops: 10
Successful Loops: 9
Failed Loops: 1
Timeout Count: 1
Success Rate: 90.0%
Current Time: 15000 ms
===============================
```

### 编译步骤

#### 快速编译（推荐）
```bash
# 双击运行编译脚本
build_project.bat
```

#### 手动编译
1. **导入项目**
   - 打开STM32CubeIDE
   - 选择 `File -> Import -> Existing Projects into Workspace`
   - 浏览并选择项目文件夹
   - 点击 `Finish` 完成导入

2. **项目配置检查**
   - **目标芯片**: STM32F407ZGTx
   - **调试器**: ST-Link GDB Server
   - **系统时钟**: 168MHz
   - **编译器**: ARM GCC

3. **编译项目**
   ```bash
   # 方法1: 使用IDE界面
   Project -> Build Project (Ctrl+B)
   
   # 方法2: 使用命令行（在项目根目录）
   make clean
   make all
   ```

4. **下载和调试**
   - 连接ST-Link调试器
   - 点击 `Run -> Debug As -> STM32 MCU C/C++ Application`
   - 或使用快捷键 `F11` 进入调试模式

5. **快速编译脚本**
   项目提供了便捷的批处理脚本：
   ```bash
   # Windows环境
   build_project.bat      # 编译项目
   quick_start.bat        # 快速启动
   syntax_check.bat       # 语法检查
   ```

### 调试配置

#### 串口设置
- 波特率: 115200
- 数据位: 8
- 停止位: 1
- 校验位: 无
- 流控: 无

#### 调试输出示例
```
=== CAN Communication System Starting ===
Initializing CAN application...
MCP2515 initialization successful
CAN application initialized successfully
Starting CAN dual node communication...
CAN dual node communication initialized

=== System Ready ===
Heartbeat: System running, TX count: 1
Sent Message: ID=0x100, Standard, Data, DLC=6, Data=AA 55 00 00 00 01
Received Message: ID=0x200, Standard, Data, DLC=4, Data=12 34 00 01
Test data received, count: 1
```

## 功能测试

### 1. 系统启动测试
观察串口输出，确认以下信息：
- CAN应用初始化成功
- MCP2515硬件检测通过
- 双节点通信启动成功

### 2. 心跳消息测试
每秒应该看到心跳消息发送：
```
Heartbeat: System running, TX count: X
Sent Message: ID=0x100, Standard, Data, DLC=6, Data=AA 55 XX XX XX XX
```

### 3. 回环测试
在MCP2515回环模式下，发送的消息应该能够接收到：
```
Loopback test message sent successfully
Loopback test successful!
```

### 4. 双节点通信测试
连接两个节点，观察消息交互：
```
Sent Message: ID=0x601, Standard, Data, DLC=2, Data=01 02
Received Message: ID=0x602, Standard, Data, DLC=8, Data=...
```

## 常见问题和解决方案

### 1. MCP2515初始化失败
**现象**: "MCP2515 initialization failed"
**原因**: 
- SPI连接问题
- 电源供电不足
- 晶振频率不匹配
**解决方案**:
- 检查SPI连线
- 确认3.3V供电稳定
- 验证8MHz晶振

### 2. CAN消息发送失败
**现象**: "Message send failed"
**原因**:
- CAN总线未连接
- 波特率不匹配
- 总线负载过高
**解决方案**:
- 检查CAN_H和CAN_L连接
- 确认波特率设置
- 添加终端电阻(120Ω)

### 3. 串口无输出
**现象**: 串口调试助手无数据
**原因**:
- 串口连线错误
- 波特率设置错误
- printf重定向失败
**解决方案**:
- 检查TX/RX连线
- 确认115200波特率
- 检查_write函数实现

### 4. 任务调度异常
**现象**: 系统卡死或重启
**原因**:
- 堆栈溢出
- 优先级配置错误
- 中断处理时间过长
**解决方案**:
- 增加任务堆栈大小
- 调整任务优先级
- 优化中断处理函数

## 扩展开发

### 添加自定义消息类型
1. 在`can_app.h`中定义新的消息ID
2. 在`CAN_ProcessReceivedMessage_App`中添加处理逻辑
3. 创建对应的发送函数
4. 更新消息协议文档

### 增加新的CAN节点
1. 修改过滤器配置
2. 扩展消息处理函数
3. 更新统计信息结构
4. 添加节点状态监控

### 优化性能
1. 使用DMA进行SPI传输
2. 实现中断驱动的消息接收
3. 优化消息队列大小
4. 添加消息优先级处理

## 技术支持

### 参考文档
- [STM32F407_MCP2515_CAN通信系统软件说明书.md](STM32F407_MCP2515_CAN通信系统软件说明书.md)
- [STM32F407_MCP2515_CAN通信系统测试指南.md](STM32F407_MCP2515_CAN通信系统测试指南.md)
- STM32F4xx参考手册
- MCP2515数据手册
- FreeRTOS用户手册

### 版本信息
- **当前版本**: V3.0.0
- **发布日期**: 2024-12-20
- **兼容性**: STM32F407ZGTx + WCMCU-230/SN65HVD230
- **依赖**: STM32 HAL库 + FreeRTOS V10.3.1
- **开发环境**: STM32CubeIDE 1.8.0+
- **作者**: 正点原子技术专家
- **许可**: MIT License

### 更新日志

#### V3.0.0 (2024-12-20) - 当前版本
- ✅ **重构项目架构**: 基于STM32内置CAN控制器
- ✅ **双节点通信**: 完整的与WCMCU-230模块通信协议
- ✅ **触发发送功能**: 串口命令触发CAN消息发送
- ✅ **CAN2静默监听**: 总线消息监控和统计
- ✅ **优化消息协议**: 标准化消息格式和ID分配
- ✅ **完善错误处理**: 增强的错误检测和恢复机制
- ✅ **文档更新**: 详细的功能说明和使用指南
- ✅ **代码优化**: 清理冗余代码，提高可维护性

#### V2.1.0 (2024-12-19)
- ✅ 完善双CAN节点通信协议
- ✅ 优化消息处理性能
- ✅ 增强错误处理机制
- ✅ 完善调试输出信息
- ✅ 更新文档和注释

#### V2.0.0 (2024-12-18)
- ✅ 重构CAN通信架构
- ✅ 实现双节点通信功能
- ✅ 集成MCP2515驱动
- ✅ 添加FreeRTOS任务管理
- ✅ 完善消息协议定义

#### V1.0.0 (2024-12-15)
- ✅ 基础CAN通信功能
- ✅ MCP2515驱动实现
- ✅ 基本消息收发
- ✅ 串口调试输出

### 项目特色

#### 🚀 技术亮点
- **多功能集成**: 双节点通信、触发发送、静默监听三大核心功能
- **实时性能**: 基于FreeRTOS的多任务并发处理
- **可扩展性**: 预留MCP2515 SPI接口，支持功能扩展
- **调试友好**: 完整的串口调试信息和统计数据
- **文档完善**: 详细的技术文档和使用说明

#### 📋 应用场景
- **CAN总线学习**: 理解CAN协议和STM32 CAN控制器
- **双节点通信**: 实现设备间的可靠数据交换
- **总线监控**: 分析和诊断CAN总线通信
- **原型开发**: 快速搭建CAN通信系统原型
- **教学演示**: CAN通信技术的教学和演示

---

**注意**: 本项目仅供学习和研究使用，在实际产品中使用前请进行充分的测试和验证。
