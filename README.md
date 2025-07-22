# STM32F407 + MCP2515 CAN通信系统

## 项目简介

本项目是基于STM32F407微控制器和MCP2515 CAN控制器的双CAN节点通信系统。系统采用FreeRTOS实时操作系统，实现了完整的CAN通信功能，包括消息发送、接收、处理和状态监控。

### 主要特性

- **双CAN节点架构**：支持STM32F407内置CAN控制器和MCP2515外部CAN控制器
- **多任务设计**：基于FreeRTOS的多任务并发处理
- **完整的CAN协议栈**：从底层驱动到应用层的完整实现
- **智能诊断功能**：自动检测和修复常见CAN通信问题
- **丰富的消息类型**：支持心跳、数据、状态、控制等多种消息
- **实时调试输出**：通过USART2串口提供详细的调试信息

## 系统架构

### 硬件架构

```
STM32F407开发板
├── 内置CAN控制器 (CAN1)
│   └── 连接到WCMCU-230模块
├── SPI1接口
│   └── 连接到MCP2515 CAN控制器
├── USART2
│   └── 调试串口输出
└── GPIO
    ├── MCP2515_CS (片选)
    ├── MCP2515_INT (中断)
    └── LED指示灯
```

### 软件架构

项目采用分层设计，主要包含以下模块：

#### 1. 驱动层 (Driver Layer)
- **mcp2515.c/h**: MCP2515 CAN控制器底层驱动
- **can.c/h**: STM32内置CAN控制器驱动
- **usart.c/h**: 串口通信驱动
- **spi.c/h**: SPI通信驱动

#### 2. 应用层 (Application Layer)
- **can_app.c/h**: MCP2515 CAN应用层封装
- **can_dual_node.c/h**: 双CAN节点通信管理
- **main.c**: 主程序和任务调度

#### 3. 系统层 (System Layer)
- **FreeRTOS**: 实时操作系统
- **HAL库**: STM32硬件抽象层
- **中断处理**: 系统中断和回调函数

## 核心功能模块

### 1. MCP2515驱动模块 (mcp2515.c)

#### 主要功能
- SPI底层通信
- 寄存器读写操作
- CAN控制器初始化和配置
- CAN消息发送和接收
- 中断处理和状态查询
- 错误处理和调试功能

#### 核心函数详解

##### 初始化函数
```c
MCP2515_Result_t MCP2515_Init(MCP2515_Baudrate_t baudrate)
```
**功能**: 初始化MCP2515控制器
**参数**: 
- `baudrate`: CAN总线波特率 (125K/250K/500K/1M)
**返回值**: MCP2515_OK表示成功
**实现逻辑**:
1. 复位MCP2515芯片
2. 设置配置模式
3. 配置波特率参数
4. 设置接收过滤器
5. 切换到正常工作模式

##### 消息发送函数
```c
MCP2515_Result_t MCP2515_SendMessage(MCP2515_CANMessage_t *message)
```
**功能**: 发送CAN消息
**参数**: 
- `message`: 指向CAN消息结构体的指针
**返回值**: MCP2515_OK表示发送成功
**实现逻辑**:
1. 检查发送缓冲区状态
2. 选择可用的发送缓冲区
3. 加载消息数据到缓冲区
4. 启动发送请求
5. 等待发送完成

##### 消息接收函数
```c
MCP2515_Result_t MCP2515_ReceiveMessage(MCP2515_CANMessage_t *message)
```
**功能**: 接收CAN消息
**参数**: 
- `message`: 用于存储接收消息的结构体指针
**返回值**: MCP2515_OK表示接收成功
**实现逻辑**:
1. 检查接收缓冲区状态
2. 读取接收缓冲区数据
3. 解析消息头和数据
4. 清除接收标志

##### 过滤器设置函数
```c
MCP2515_Result_t MCP2515_SetFilter(uint8_t filter_num, uint32_t filter_id, uint8_t extended)
MCP2515_Result_t MCP2515_SetMask(uint8_t mask_num, uint32_t mask_value, uint8_t extended)
```
**功能**: 设置接收过滤器和掩码
**参数**: 
- `filter_num/mask_num`: 过滤器/掩码编号 (0-5)
- `filter_id/mask_value`: 过滤器ID/掩码值
- `extended`: 是否为扩展帧
**实现逻辑**:
1. 切换到配置模式
2. 写入过滤器/掩码寄存器
3. 恢复工作模式

### 2. CAN应用层模块 (can_app.c)

#### 主要功能
- 基于MCP2515的CAN通信应用层封装
- 多任务CAN消息处理
- 消息队列管理
- 统计信息收集
- 应用层消息协议

#### 核心函数详解

##### 应用初始化函数
```c
uint8_t CAN_App_Init(void)
```
**功能**: 初始化CAN应用层
**返回值**: CAN_APP_OK表示成功
**实现逻辑**:
1. 初始化MCP2515硬件
2. 配置接收过滤器
3. 设置默认参数
4. 初始化统计计数器

##### 发送任务主函数
```c
void CAN_SendTask_Main(void *argument)
```
**功能**: CAN发送任务的主循环
**实现逻辑**:
1. 周期性发送心跳消息 (1秒间隔)
2. 周期性发送测试数据 (2秒间隔)
3. 周期性发送状态消息 (5秒间隔)
4. 周期性发送传感器数据 (3秒间隔)
5. 周期性发送控制指令 (10秒间隔)
6. 处理来自队列的发送请求

##### 接收任务主函数
```c
void CAN_ReceiveTask_Main(void *argument)
```
**功能**: CAN接收任务的主循环
**实现逻辑**:
1. 检查MCP2515接收状态
2. 接收并解析CAN消息
3. 根据消息类型进行处理
4. 更新统计信息
5. 输出调试信息

##### 消息处理函数
```c
static void CAN_ProcessReceivedMessage_App(MCP2515_CANMessage_t *message)
```
**功能**: 处理接收到的CAN消息
**支持的消息类型**:
- **心跳消息** (ID: 0x100): 包含发送计数器信息
- **数据消息** (ID: 0x200): 包含测试数据和计数器
- **状态消息** (ID: 0x300): 包含系统状态和错误标志
- **传感器消息** (ID: 0x400): 包含传感器数值和类型
- **控制消息** (ID: 0x500): 包含控制指令和参数

##### 应用接口函数
```c
uint8_t CAN_App_SendMessage(uint32_t id, uint8_t *data, uint8_t length, uint8_t extended)
uint8_t CAN_App_SendRemoteFrame(uint32_t id, uint8_t dlc, uint8_t extended)
uint8_t CAN_App_SetFilter(uint32_t filter_id, uint32_t mask, uint8_t extended)
```
**功能**: 提供给用户的应用层接口
**特点**: 
- 参数检查和错误处理
- 队列方式的异步发送
- 统计信息自动更新
- 调试信息输出

### 3. 双节点通信模块 (can_dual_node.c)

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

### 消息ID分配
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
```

### 消息格式

#### 心跳消息 (0x100)
| 字节 | 描述 | 值 |
|------|------|----|
| 0-1  | 魔数 | 0xAA55 |
| 2-5  | 发送计数器 | 32位计数值 |
| 6-7  | 保留 | 0x00 |

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
| defaultTask | osPriorityNormal | 128 words | 系统心跳监测 |
| CANSendTask | osPriorityNormal | 128 words | CAN消息发送 |
| CANReceiveTask | osPriorityAboveNormal | 128 words | CAN消息接收 |

### 消息队列配置

| 队列名称 | 大小 | 元素类型 | 功能描述 |
|----------|------|----------|----------|
| myQueue01 | 16 | CAN_QueueMessage_t | CAN发送消息队列 |

## 编译和使用

### 开发环境要求
- STM32CubeIDE 1.8.0或更高版本
- STM32F4xx HAL库
- FreeRTOS
- 正点原子STM32F407开发板
- MCP2515 CAN控制器模块

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
1. 打开STM32CubeIDE
2. 导入项目工程
3. 检查硬件配置
4. 编译项目
5. 下载到开发板

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
- 版本: V1.0
- 日期: 2024-12-19
- 作者: 正点原子技术专家
- 许可: MIT License

### 更新日志
- V1.0 (2024-12-19): 初始版本发布
  - 完成MCP2515驱动开发
  - 实现CAN应用层封装
  - 添加双节点通信功能
  - 集成FreeRTOS多任务系统
  - 完善调试和测试功能

---

**注意**: 本项目仅供学习和研究使用，在实际产品中使用前请进行充分的测试和验证。
